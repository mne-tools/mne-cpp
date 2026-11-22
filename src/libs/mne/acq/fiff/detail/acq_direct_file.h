//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_direct_file.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqDirectFile: a write-only file for whole aligned blocks that bypasses the page cache.
 *
 * O_DIRECT on Linux, FILE_FLAG_NO_BUFFERING on Windows, F_NOCACHE on macOS; where the file system refuses
 * (tmpfs, network shares) or the platform has none (WebAssembly) the file is opened buffered instead and
 * isDirect() says so. Callers write whole multiples of kBlockBytes at aligned offsets from buffers aligned
 * to kBlockBytes, which every mode accepts.
 */

#ifndef ACQ_DIRECT_FILE_H
#define ACQ_DIRECT_FILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

//=============================================================================================================
/**
 * Heap buffer aligned for direct I/O.
 */
class AcqAlignedBuffer
{
public:
    static constexpr std::size_t kAlignment = 4096; /**< Alignment of start and size. */

    /** @param[in] bytes   Capacity, rounded up to kAlignment. */
    explicit AcqAlignedBuffer(std::size_t bytes);
    AcqAlignedBuffer(const AcqAlignedBuffer&) = delete;
    AcqAlignedBuffer& operator=(const AcqAlignedBuffer&) = delete;

    /** @return The buffer start. */
    [[nodiscard]] std::byte* data() noexcept
    {
        return m_data;
    }

    /** @return The capacity in bytes. */
    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

private:
    std::size_t m_capacity = 0;
    std::unique_ptr<std::byte[]> m_storage;
    std::byte* m_data = nullptr;
};

//=============================================================================================================
/**
 * A file written in aligned blocks, page cache bypassed where possible.
 */
class AcqDirectFile
{
public:
    static constexpr std::size_t kBlockBytes = AcqAlignedBuffer::kAlignment; /**< Write granularity. */

    AcqDirectFile() noexcept = default;
    ~AcqDirectFile();
    AcqDirectFile(const AcqDirectFile&) = delete;
    AcqDirectFile& operator=(const AcqDirectFile&) = delete;

    /**
     * Creates or truncates @p path.
     *
     * @param[in] path     File to write.
     * @param[in] direct   Try to bypass the page cache.
     * @param[in] create   Create or truncate; false opens an existing file for rewriting.
     *
     * @return DeviceIo with the system's message.
     */
    AcqError open(const std::string& path, bool direct, bool create = true);

    /** @return true when the page cache is bypassed. */
    [[nodiscard]] bool isDirect() const noexcept
    {
        return m_direct;
    }

    /** @return true when a file is open. */
    [[nodiscard]] bool isOpen() const noexcept;

    /**
     * Writes @p bytes from @p data at @p offset; all three are multiples of kBlockBytes in direct mode.
     *
     * @return DeviceIo with the system's message.
     */
    AcqError writeAt(std::uint64_t offset, const std::byte* data, std::size_t bytes);

    /**
     * Makes everything written so far durable and orders it before later writes (fdatasync, F_BARRIERFSYNC on
     * macOS, FlushFileBuffers on Windows).
     *
     * @return DeviceIo with the system's message.
     */
    AcqError barrier();

    /**
     * Cuts the file at @p size.
     *
     * @return DeviceIo with the system's message.
     */
    AcqError truncate(std::uint64_t size);

    /** Closes the file. Idempotent. */
    void close() noexcept;

private:
    std::string m_path;
    bool m_direct = false;
#if defined(_WIN32)
    void* m_handle = nullptr;
#else
    int m_fd = -1;
#endif
};

} // namespace mne::detail

#endif // ACQ_DIRECT_FILE_H
