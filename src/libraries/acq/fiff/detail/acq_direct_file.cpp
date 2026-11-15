//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_direct_file.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqDirectFile: O_DIRECT, FILE_FLAG_NO_BUFFERING or F_NOCACHE writes with a buffered fallback.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_direct_file.h"

#include <cerrno>
#include <cstring>
#include <memory>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL
//=============================================================================================================

namespace
{

#if defined(_WIN32)
AcqError systemError(const std::string& what)
{
    const DWORD code = GetLastError();
    char text[256] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, text,
                   static_cast<DWORD>(sizeof(text)), nullptr);
    return AcqError(AcqErrorCode::DeviceIo, what + ": " + text);
}

std::wstring widen(const std::string& utf8)
{
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(n > 0 ? static_cast<std::size_t>(n) : 0U, L'\0');
    if (n > 0) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), n);
        wide.resize(static_cast<std::size_t>(n - 1));
    }
    return wide;
}
#else
AcqError systemError(const std::string& what)
{
    return AcqError(AcqErrorCode::DeviceIo, what + ": " + std::strerror(errno));
}
#endif

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqAlignedBuffer::AcqAlignedBuffer(std::size_t bytes)
: m_capacity((bytes + kAlignment - 1) / kAlignment * kAlignment)
, m_storage(std::make_unique<std::byte[]>(m_capacity + kAlignment)) // value-initialised: zeros
{
    void* start = m_storage.get();
    std::size_t space = m_capacity + kAlignment;
    m_data = static_cast<std::byte*>(std::align(kAlignment, m_capacity, start, space));
}

//=============================================================================================================

AcqDirectFile::~AcqDirectFile()
{
    close();
}

//=============================================================================================================

#if defined(_WIN32)

bool AcqDirectFile::isOpen() const noexcept
{
    return m_handle != nullptr;
}

AcqError AcqDirectFile::open(const std::string& path, bool direct, bool create)
{
    close();
    m_path = path;
    const std::wstring wide = widen(path);
    const DWORD disposition = create ? CREATE_ALWAYS : OPEN_EXISTING;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_DELETE;
    HANDLE handle = INVALID_HANDLE_VALUE;
    if (direct) {
        handle = CreateFileW(wide.c_str(), GENERIC_WRITE, share, nullptr, disposition,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, nullptr);
    }
    m_direct = handle != INVALID_HANDLE_VALUE;
    if (!m_direct) {
        handle = CreateFileW(wide.c_str(), GENERIC_WRITE, share, nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    if (handle == INVALID_HANDLE_VALUE) {
        return systemError("cannot open " + path);
    }
    m_handle = handle;
    return {};
}

AcqError AcqDirectFile::writeAt(std::uint64_t offset, const std::byte* data, std::size_t bytes)
{
    while (bytes > 0) {
        OVERLAPPED at = {};
        at.Offset = static_cast<DWORD>(offset & 0xFFFFFFFFULL);
        at.OffsetHigh = static_cast<DWORD>(offset >> 32U);
        const DWORD chunk = static_cast<DWORD>(bytes > (1U << 30U) ? (1U << 30U) : bytes);
        DWORD written = 0;
        if (!WriteFile(static_cast<HANDLE>(m_handle), data, chunk, &written, &at) || written == 0) {
            return systemError("cannot write " + m_path);
        }
        data += written;
        bytes -= written;
        offset += written;
    }
    return {};
}

AcqError AcqDirectFile::barrier()
{
    if (!FlushFileBuffers(static_cast<HANDLE>(m_handle))) {
        return systemError("cannot flush " + m_path);
    }
    return {};
}

AcqError AcqDirectFile::truncate(std::uint64_t size)
{
    FILE_END_OF_FILE_INFO end = {};
    end.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
    if (!SetFileInformationByHandle(static_cast<HANDLE>(m_handle), FileEndOfFileInfo, &end, sizeof(end))) {
        return systemError("cannot truncate " + m_path);
    }
    return {};
}

void AcqDirectFile::close() noexcept
{
    if (m_handle != nullptr) {
        CloseHandle(static_cast<HANDLE>(m_handle));
        m_handle = nullptr;
    }
}

#else

bool AcqDirectFile::isOpen() const noexcept
{
    return m_fd >= 0;
}

AcqError AcqDirectFile::open(const std::string& path, [[maybe_unused]] bool direct, bool create)
{
    close();
    m_path = path;
    const int flags = O_WRONLY | O_CLOEXEC | (create ? O_CREAT | O_TRUNC : 0);
    m_direct = false;
#if defined(O_DIRECT) && !defined(__EMSCRIPTEN__)
    if (direct) {
        // tmpfs and some network file systems refuse O_DIRECT with EINVAL; those fall back to buffered writes.
        m_fd = ::open(path.c_str(), flags | O_DIRECT, 0644);
        m_direct = m_fd >= 0;
    }
#endif
    if (m_fd < 0) {
        m_fd = ::open(path.c_str(), flags, 0644);
    }
    if (m_fd < 0) {
        return systemError("cannot open " + path);
    }
#if defined(__APPLE__)
    m_direct = direct && fcntl(m_fd, F_NOCACHE, 1) == 0;
#endif
    return {};
}

AcqError AcqDirectFile::writeAt(std::uint64_t offset, const std::byte* data, std::size_t bytes)
{
    while (bytes > 0) {
        const ssize_t written = ::pwrite(m_fd, data, bytes, static_cast<off_t>(offset));
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return systemError("cannot write " + m_path);
        }
        data += written;
        bytes -= static_cast<std::size_t>(written);
        offset += static_cast<std::uint64_t>(written);
    }
    return {};
}

AcqError AcqDirectFile::barrier()
{
#if defined(__APPLE__)
    // fsync alone does not order writes on the drive; the barrier does, without a full cache flush.
    if (fcntl(m_fd, F_BARRIERFSYNC) == 0) {
        return {};
    }
    if (::fsync(m_fd) != 0) {
        return systemError("cannot flush " + m_path);
    }
#elif defined(__linux__)
    if (::fdatasync(m_fd) != 0) {
        return systemError("cannot flush " + m_path);
    }
#else
    if (::fsync(m_fd) != 0) {
        return systemError("cannot flush " + m_path);
    }
#endif
    return {};
}

AcqError AcqDirectFile::truncate(std::uint64_t size)
{
    if (::ftruncate(m_fd, static_cast<off_t>(size)) != 0) {
        return systemError("cannot truncate " + m_path);
    }
    return {};
}

void AcqDirectFile::close() noexcept
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

#endif
