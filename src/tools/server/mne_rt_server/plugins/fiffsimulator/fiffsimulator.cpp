//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffSimulator class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"

#include <mne/base/json.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/acq/acq_device_registry.h>
#include <mne/acq/acq_eigen.h>
#include <mne/acq/fiff/acq_fiff_replay.h>
#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_types.h>

#include <rtserver_connector/command.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORRTSERVERPLUGIN;
using namespace mne;
using namespace RTSERVER;

namespace
{

/** Description of the connector and its commands, the former plugin meta data. */
constexpr const char* kMetaData = R"({
    "encoding": "UTF-8",
    "device": "FiffSimulator",
    "description": "Fiff file simulator",
    "commands": {
        "bufsize": {
            "description": "Sets the buffer size of the FiffStreamClient raw data buffer.",
            "parameters": {
                "samples": {
                    "description": "samples",
                    "type": "uint"
                }
            }
        },
        "getbufsize": {
            "description": "Returns the current buffer size of the FiffStreamClient raw data buffer.",
            "parameters": {}
        },
        "accel": {
            "description": "Sets the acceleration factor to simulate different sampling rates.",
            "parameters": {
                "factor": {
                    "description": "acceleration factor",
                    "type": "float"
                }
            }
        },
        "getaccel": {
            "description": "Returns the acceleration factor.",
            "parameters": {}
        },
        "simfile": {
            "description": "The fiff file which should be used as simulation file.",
            "parameters": {
                "file": {
                    "description": "file",
                    "type": "QString"
                }
            }
        }
    }
})";

/** @return The directory of the executable, as QCoreApplication::applicationDirPath() names it. */
std::string applicationDirPath()
{
    return utf8_from_path(executable_dir());
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_sResourceDataPath(std::format("{}/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif", applicationDirPath()))
, m_uiBufferSampleSize(200) //(4)
, m_AccelerationFactor(1.0)
, m_TrueSamplingRate(0.0)
, m_bIsRunning(false)
{
    this->init();
}

//=============================================================================================================

FiffSimulator::~FiffSimulator()
{
    stop();
}

//=============================================================================================================

void FiffSimulator::comBufsize(const Command& p_command)
{
    //ToDO JSON
    const std::uint32_t t_uiBuffSize = p_command.pValues()[0].toUInt();

    if (t_uiBuffSize > 0) {
        const bool t_bWasRunning = m_bIsRunning;

        if (m_bIsRunning) {
            this->stop();
        }

        m_uiBufferSampleSize = t_uiBuffSize;

        if (t_bWasRunning) {
            this->start();
        }

        m_commandManager[Commands::BUFSIZE].reply(std::format("\tSet {} buffer sample size to {} samples\r\n\n", getName(), t_uiBuffSize));
    } else {
        m_commandManager[Commands::BUFSIZE].reply("Buffer size not set\r\n");
    }
}

//=============================================================================================================

void FiffSimulator::comGetBufsize(const Command& p_command)
{
    if (p_command.isJson()) {
        JsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::BUFSIZE, static_cast<double>(m_uiBufferSampleSize));
        m_commandManager[Commands::GETBUFSIZE].reply(JsonDocument(t_qJsonObjectRoot).to_json());
    } else {
        m_commandManager[Commands::GETBUFSIZE].reply(std::format("\t{}\r\n\n", m_uiBufferSampleSize));
    }
}

//=============================================================================================================

void FiffSimulator::comAccel(const Command& p_command)
{
    //ToDO JSON
    const float t_uiAccel = p_command.pValues()[0].toFloat();

    if (t_uiAccel > 0) {
        const bool t_bWasRunning = m_bIsRunning;

        if (m_bIsRunning) {
            this->stop();
        }

        m_AccelerationFactor = t_uiAccel;
        m_RawInfo.info.sfreq = m_AccelerationFactor * m_TrueSamplingRate;

        if (t_bWasRunning) {
            this->start();
        }

        m_commandManager[Commands::ACCEL].reply(std::format("\tSet acceleration factor to {:.3f}\r\n\n", t_uiAccel));
    } else {
        m_commandManager[Commands::ACCEL].reply("Acceleration facor not set\r\n");
    }
}

//=============================================================================================================

void FiffSimulator::comGetAccel(const Command& p_command)
{
    if (p_command.isJson()) {
        JsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::ACCEL, static_cast<double>(m_AccelerationFactor));
        m_commandManager[Commands::GETACCEL].reply(JsonDocument(t_qJsonObjectRoot).to_json());
    } else {
        m_commandManager[Commands::GETACCEL].reply(std::format("\t{:.3f}\r\n\n", m_AccelerationFactor));
    }
}

//=============================================================================================================

void FiffSimulator::comSimfile(const Command& p_command)
{
    //
    // simulation file
    //
    const std::string t_sFile = p_command.pValues()[0].toString();
    const std::string t_sResourceDataPathOld = m_sResourceDataPath;

    std::error_code t_error;
    if (!t_sFile.empty() && std::filesystem::exists(path_from_utf8(t_sFile), t_error)) {
        m_sResourceDataPath = t_sFile;
        m_RawInfo = FiffRawData();

        if (this->readRawInfo()) {
            this->stop();

            m_commandManager[Commands::SIMFILE].reply("New simulation file set succefully.\r\n");
        } else {
            Log::debug("Didn't set new file");
            m_sResourceDataPath = t_sResourceDataPathOld;

            m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
        }
    } else {
        Log::debug("File does not exist on server!");
        m_sResourceDataPath = t_sResourceDataPathOld;
        m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
    }
}

//=============================================================================================================

void FiffSimulator::connectCommandManager()
{
    m_commandManager[Commands::BUFSIZE].connectExecuted([this](const Command& command) { comBufsize(command); });
    m_commandManager[Commands::GETBUFSIZE].connectExecuted([this](const Command& command) { comGetBufsize(command); });
    m_commandManager[Commands::ACCEL].connectExecuted([this](const Command& command) { comAccel(command); });
    m_commandManager[Commands::GETACCEL].connectExecuted([this](const Command& command) { comGetAccel(command); });
    m_commandManager[Commands::SIMFILE].connectExecuted([this](const Command& command) { comSimfile(command); });
}

//=============================================================================================================

ConnectorID FiffSimulator::getConnectorID() const
{
    return FIFF_SIMULATOR_CONNECTOR;
}

//=============================================================================================================

const char* FiffSimulator::getName() const
{
    return "Fiff File Simulator";
}

//=============================================================================================================

const char* FiffSimulator::getMetaData() const
{
    return kMetaData;
}

//=============================================================================================================

void FiffSimulator::init()
{
    //
    // Read cfg file
    //
    std::ifstream t_qFile(path_from_utf8(std::format("{}/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg", applicationDirPath())));
    if (!t_qFile) {
        return;
    }
    const std::string key = "simFile = ";
    std::string line;
    while (std::getline(t_qFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (to_lower(line).find(to_lower(key)) == std::string::npos) {
            continue;
        }
        // As before: the key is matched case-insensitively but located case-sensitively
        const std::size_t found = line.find(key);
        const std::size_t idx = found == std::string::npos ? key.size() - 1 : found + key.size();
        const std::string sFileName = idx < line.size() ? line.substr(idx) : std::string();

        if (std::ifstream(path_from_utf8(sFileName), std::ios::binary)) {
            m_sResourceDataPath = sFileName;
            Log::info("[FiffSimulator::init] Load simulation file {}", sFileName);
        } else {
            Log::info("[FiffSimulator::init] Trying to open simulation file {} read from FiffSimulation.cfg failed. Opening sample_audvis_raw.fif instead.", sFileName);
        }
    }
}

//=============================================================================================================

bool FiffSimulator::start()
{
    // A start while running restarts the replay
    if (m_thread.joinable()) {
        stop();
    }
    this->init();
    if (m_RawInfo.empty() && !readRawInfo()) {
        return false;
    }
    registerFiffScheme();
    const AcqUri uri{"fiff",
                     m_RawInfo.info.filename, // the file whose info clients got
                     {{"loop", "1"},
                      {"speed", std::to_string(m_AccelerationFactor)},
                      {"block", std::to_string(m_uiBufferSampleSize)},
                      {"events", "0"}}};
    auto device = AcqDevice::open(uri.toString());
    if (!device) {
        Log::warning("[FiffSimulator::start] {}", device.error().text());
        return false;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader || !device->start().ok()) {
        Log::warning("[FiffSimulator::start] Cannot start the replay of {}", m_RawInfo.info.filename);
        return false;
    }
    m_device = std::move(*device);
    m_reader = std::move(*reader);
    m_bIsRunning = true;
    m_thread = std::thread([this] { run(); });
    return true;
}

//=============================================================================================================

bool FiffSimulator::stop()
{
    m_bIsRunning = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_reader.reset();
    m_device.reset(); // closes the replay
    return true;
}

//=============================================================================================================

void FiffSimulator::info(std::int32_t ID)
{
    if (m_RawInfo.empty()) {
        readRawInfo();
    }

    if (!m_RawInfo.empty()) {
        remitMeasInfo(ID, m_RawInfo.info);
    }
}

//=============================================================================================================

bool FiffSimulator::readRawInfo()
{
    if (m_RawInfo.empty()) {
        const std::scoped_lock locker(mutex);

        auto raw = FiffRawData::read(path_from_utf8(m_sResourceDataPath));
        if (!raw) {
            Log::info("Error: Not able to read raw info: {}", raw.error().what());
            return false;
        }
        m_RawInfo = std::move(*raw);

        m_TrueSamplingRate = m_RawInfo.info.sfreq;
        m_RawInfo.info.sfreq *= m_AccelerationFactor;
    }

    return true;
}

//=============================================================================================================

void FiffSimulator::run()
{
    // The replay paces itself at sfreq * acceleration; buffers go out calibrated, as read_raw_segment returns them.
    Eigen::MatrixXd data;
    while (m_bIsRunning) {
        const auto block = mne::read(*m_reader, m_uiBufferSampleSize, data, std::chrono::milliseconds(100));
        if (!block) {
            if (block.error().code() != AcqErrorCode::Timeout) {
                Log::warning("[FiffSimulator::run] {}", block.error().text());
                return;
            }
            continue;
        }
        remitRawBuffer(std::make_shared<const Eigen::MatrixXf>(data.cast<float>()));
    }
}
