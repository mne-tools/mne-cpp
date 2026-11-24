//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamthread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffStreamThread Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"
#include "mne_rt_commands.h"

#include <mne/base/log.h>
#include <mne/fiff/fiff_byte_swap.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_tag.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstring>
#include <optional>
#include <span>
#include <system_error>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace mne;

namespace
{

/**
 * Longest wait for a client to take a block. One that does not (it stopped reading) is dropped, so neither the
 * memory of the pending blocks nor the shutdown of the server depends on it.
 */
constexpr std::chrono::milliseconds kWriteTimeout(2000);

/** Appends the tags @p write puts on a FIFF stream to @p block. */
template<typename Write>
void appendTags(std::vector<std::byte>& block, Write&& write)
{
    auto bytes = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *bytes;
    FiffStream stream(std::move(bytes));
    write(stream);
    const auto data = written.data();
    block.insert(block.end(), data.begin(), data.begin() + static_cast<std::ptrdiff_t>(written.size()));
}

/** @return The big-endian 32-bit integer at @p offset of @p data. */
std::int32_t bigInt32(const std::vector<std::byte>& data, std::size_t offset)
{
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        value = (value << 8) | std::to_integer<std::uint32_t>(data[offset + i]);
    }
    return static_cast<std::int32_t>(value);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamThread::FiffStreamThread(std::int32_t id, mne::TcpSocket socket)
: m_iDataClientId(id)
, m_socket(std::move(socket))
, m_bIsSendingRawBuffer(false)
, m_bIsRunning(false)
, m_bIsFinished(false)
{
}

//=============================================================================================================

FiffStreamThread::~FiffStreamThread()
{
    stop();
}

//=============================================================================================================

void FiffStreamThread::start()
{
    m_bIsRunning = true;
    m_thread = std::thread([this] { run(); });
}

//=============================================================================================================

void FiffStreamThread::stop()
{
    m_bIsRunning = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

//=============================================================================================================

bool FiffStreamThread::isFinished() const
{
    return m_bIsFinished;
}

//=============================================================================================================

std::string FiffStreamThread::getAlias()
{
    const std::scoped_lock lock(m_qMutex);
    return m_sDataClientAlias;
}

//=============================================================================================================

void FiffStreamThread::startMeas(std::int32_t ID)
{
    if (ID == m_iDataClientId) {
        Log::debug("Activate raw buffer sending.");

        const std::scoped_lock lock(m_qMutex);
        appendTags(m_qSendBlock, [](FiffStream& out) { out.start_block(FIFFB_RAW_DATA); });
        m_bIsSendingRawBuffer = true;
    }
}

//=============================================================================================================

void FiffStreamThread::stopMeas(std::int32_t ID)
{
    Log::debug("void FiffStreamThread::stopMeas(qint32 ID)");
    if (ID == m_iDataClientId || ID == -1) {
        Log::debug("stop raw buffer sending.");

        const std::scoped_lock lock(m_qMutex);
        appendTags(m_qSendBlock, [](FiffStream& out) { out.end_block(FIFFB_RAW_DATA); });
        m_bIsSendingRawBuffer = false;
    }
}

//=============================================================================================================

void FiffStreamThread::parseCommand(const FiffTag& p_pTag)
{
    if (p_pTag.size() >= 4) {
        std::int32_t t_iCmd = 0;
        std::memcpy(&t_iCmd, p_pTag.data(), sizeof(t_iCmd));
        t_iCmd = mne::swap_int(t_iCmd);

        if (t_iCmd == MNE_RT_SET_CLIENT_ALIAS) {
            //
            // Set Client Alias
            //
            std::string t_sAlias(reinterpret_cast<const char*>(p_pTag.data()) + 4, static_cast<std::size_t>(p_pTag.size() - 4));
            {
                const std::scoped_lock lock(m_qMutex);
                m_sDataClientAlias = t_sAlias;
            }
            Log::info("FiffStreamClient (ID {}): new alias = '{}'\r\n", m_iDataClientId, t_sAlias);
        } else if (t_iCmd == MNE_RT_GET_CLIENT_ID) {
            //
            // Send Client ID
            //
            Log::info("FiffStreamClient (ID {}): send client ID {}\r\n", m_iDataClientId, m_iDataClientId);
            writeClientId();
        } else {
            Log::info("FiffStreamClient (ID {}): unknown command\r\n", m_iDataClientId);
        }
    } else {
        Log::info("FiffStreamClient (ID {}): unknown command\r\n", m_iDataClientId);
    }
}

//=============================================================================================================

void FiffStreamThread::sendRawBuffer(const std::shared_ptr<const Eigen::MatrixXf>& m_pMatRawData)
{
    const std::scoped_lock locker(m_qMutex);
    if (m_bIsSendingRawBuffer) {
        appendTags(m_qSendBlock, [&m_pMatRawData](FiffStream& out) {
            out.write_float(FIFF_DATA_BUFFER, m_pMatRawData->data(), m_pMatRawData->rows() * m_pMatRawData->cols());
        });
    }
}

//=============================================================================================================

void FiffStreamThread::sendMeasurementInfo(std::int32_t ID, const FiffInfo& p_fiffInfo)
{
    if (ID == m_iDataClientId) {
        const std::scoped_lock locker(m_qMutex);
        appendTags(m_qSendBlock, [&p_fiffInfo](FiffStream& out) { p_fiffInfo.writeToStream(out); });
    }
}

//=============================================================================================================

void FiffStreamThread::writeClientId()
{
    const std::scoped_lock locker(m_qMutex);
    appendTags(m_qSendBlock, [this](FiffStream& out) { out.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId); });
}

//=============================================================================================================

void FiffStreamThread::run()
{
    Log::info("FiffStreamClient (assigned ID {}) accepted\n", m_iDataClientId);

    constexpr std::size_t kHeaderBytes = 4 * sizeof(std::int32_t);
    std::vector<std::byte> t_received;
    std::array<std::byte, 4096> t_chunk{};

    while (m_socket.is_open() && m_bIsRunning) {
        //
        // Write available data
        //
        std::vector<std::byte> t_qBlock;
        {
            const std::scoped_lock lock(m_qMutex);
            t_qBlock.swap(m_qSendBlock);
        }
        // The producers only append to m_qSendBlock, so writing here in this thread never stalls them.
        if (!t_qBlock.empty()) {
            if (const std::error_code t_error = m_socket.write(t_qBlock, kWriteTimeout)) {
                Log::info("FiffStreamClient (ID {}) dropped: {}\n", m_iDataClientId, t_error.message());
                break;
            }
        }

        // Read: wait 10 ms for data; parse every complete tag (kind, type, size, next, data)
        const auto t_read = m_socket.read_some(t_chunk, std::chrono::milliseconds(10));
        if (t_read) {
            t_received.insert(t_received.end(), t_chunk.begin(), t_chunk.begin() + static_cast<std::ptrdiff_t>(*t_read));
        } else if (t_read.error() != std::errc::timed_out) {
            break;
        }
        while (t_received.size() >= kHeaderBytes) {
            const std::int32_t size = bigInt32(t_received, 2 * sizeof(std::int32_t));
            if (size < 0) {
                t_received.clear();
                break;
            }
            const std::size_t t_tagBytes = kHeaderBytes + static_cast<std::size_t>(size);
            if (t_received.size() < t_tagBytes) {
                break;
            }
            FiffStream in(std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(t_received.begin(), t_received.begin() + static_cast<std::ptrdiff_t>(t_tagBytes))));
            t_received.erase(t_received.begin(), t_received.begin() + static_cast<std::ptrdiff_t>(t_tagBytes));
            if (const std::optional<FiffTag> tag = in.read_tag(); tag && tag->kind == FIFF_MNE_RT_COMMAND) {
                parseCommand(*tag);
            }
        }
    }

    m_socket.close();
    m_bIsFinished = true;
}
