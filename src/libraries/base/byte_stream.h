//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     byte_stream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Seekable byte streams over memory and files, with big-endian value I/O, in standard C++.
 */

#ifndef BASE_BYTE_STREAM_H
#define BASE_BYTE_STREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "endian.h"
#include "error.h"
#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A seekable sequence of bytes; positions are absolute byte offsets.
 *
 * @snippet ex_base/main.cpp byte_stream_usage
 */
class BASESHARED_EXPORT ByteStream
{
public:
    virtual ~ByteStream() = default;

    //=========================================================================================================
    /**
     * Reads up to @p bytes bytes at the current position and advances past them.
     *
     * @param[out] data    Destination of at least @p bytes bytes.
     * @param[in]  bytes   Number of bytes wanted.
     *
     * @return The number of bytes read; fewer than @p bytes at the end of the stream.
     */
    virtual std::size_t read(void* data, std::size_t bytes) = 0;

    //=========================================================================================================
    /**
     * Writes @p bytes bytes at the current position, overwriting or extending the stream.
     *
     * @param[in] data    Source of @p bytes bytes.
     * @param[in] bytes   Number of bytes.
     *
     * @return The number of bytes written; fewer than @p bytes if the stream cannot be written.
     */
    virtual std::size_t write(const void* data, std::size_t bytes) = 0;

    //=========================================================================================================
    /**
     * @param[in] position   New position, at most size().
     *
     * @return Whether the position changed; it stays put on failure.
     */
    virtual bool seek(std::uint64_t position) = 0;

    /** @return The current position. */
    virtual std::uint64_t pos() const = 0;

    /** @return The number of bytes in the stream. */
    virtual std::uint64_t size() const = 0;

    /** Releases the underlying resource; later reads, writes and seeks fail. */
    virtual void close()
    {
    }

    /** @return Whether the stream has not been closed. */
    virtual bool is_open() const
    {
        return true;
    }

    /** Opens a closed stream again at position 0, keeping its contents. @return Whether the stream is open. */
    virtual bool reopen()
    {
        return is_open();
    }

    /** @return Whether the position is at or past the end. */
    bool at_end() const
    {
        return pos() >= size();
    }

    //=========================================================================================================
    /**
     * Reads @p count big-endian values and converts them to host byte order.
     *
     * @param[out] values   Destination of @p count values.
     * @param[in]  count    Number of values.
     *
     * @return Whether all values were read.
     */
    template<Swappable T>
    bool read_big_endian(T* values, std::size_t count)
    {
        if (read(values, count * sizeof(T)) != count * sizeof(T)) {
            return false;
        }
        if constexpr (std::endian::native != std::endian::big) {
            byte_swap_in_place(values, count);
        }
        return true;
    }

    /**
     * @param[out] value   Receives one value converted from big-endian byte order.
     *
     * @return Whether the value was read.
     */
    template<Swappable T>
    bool read_big_endian(T& value)
    {
        return read_big_endian(&value, 1);
    }

    //=========================================================================================================
    /**
     * Writes @p count values in big-endian byte order.
     *
     * @param[in] values   Source of @p count values in host byte order.
     * @param[in] count    Number of values.
     *
     * @return Whether all values were written.
     */
    template<Swappable T>
    bool write_big_endian(const T* values, std::size_t count)
    {
        if constexpr (std::endian::native == std::endian::big) {
            return write(values, count * sizeof(T)) == count * sizeof(T);
        } else {
            constexpr std::size_t kChunk = 4096;
            T swapped[kChunk];
            for (std::size_t done = 0; done < count;) {
                const std::size_t n = count - done < kChunk ? count - done : kChunk;
                for (std::size_t i = 0; i < n; ++i) {
                    swapped[i] = byte_swap(values[done + i]);
                }
                if (write(swapped, n * sizeof(T)) != n * sizeof(T)) {
                    return false;
                }
                done += n;
            }
            return true;
        }
    }

    /**
     * @param[in] value   One value in host byte order, written in big-endian byte order.
     *
     * @return Whether the value was written.
     */
    template<Swappable T>
    bool write_big_endian(T value)
    {
        return write_big_endian(&value, 1);
    }
};

//=============================================================================================================
/**
 * @brief A byte stream over a growable buffer in memory.
 *
 * @snippet ex_base/main.cpp memory_byte_stream_usage
 */
class BASESHARED_EXPORT MemoryByteStream : public ByteStream
{
public:
    /** @param[in] data   Initial contents; the position starts at 0. */
    explicit MemoryByteStream(std::vector<std::byte> data = {});

    std::size_t read(void* data, std::size_t bytes) override;
    std::size_t write(const void* data, std::size_t bytes) override;
    bool seek(std::uint64_t position) override;
    std::uint64_t pos() const override;
    std::uint64_t size() const override;

    /** @return The contents. */
    const std::vector<std::byte>& data() const
    {
        return m_data;
    }

private:
    std::vector<std::byte> m_data;
    std::size_t m_pos = 0;
};

//=============================================================================================================
/**
 * @brief A buffered byte stream over a file, with 64-bit positions.
 *
 * @snippet ex_base/main.cpp byte_stream_usage
 */
class BASESHARED_EXPORT FileByteStream : public ByteStream
{
public:
    /** How the file is opened. */
    enum class Mode
    {
        Read,     ///< Existing file, read only.
        Write,    ///< New or truncated file, read and write.
        ReadWrite ///< Existing file, read and write without truncating.
    };

    //=========================================================================================================
    /**
     * @param[in] path   The file.
     * @param[in] mode   How to open it.
     *
     * @return The stream, positioned at 0, or nullptr if the file cannot be opened.
     */
    static std::unique_ptr<FileByteStream> open(const std::filesystem::path& path, Mode mode);

    ~FileByteStream() override;
    FileByteStream(const FileByteStream&) = delete;
    FileByteStream& operator=(const FileByteStream&) = delete;

    std::size_t read(void* data, std::size_t bytes) override;
    std::size_t write(const void* data, std::size_t bytes) override;
    bool seek(std::uint64_t position) override;
    std::uint64_t pos() const override;
    std::uint64_t size() const override;
    void close() override;
    bool is_open() const override;
    bool reopen() override;
    /** @return Whether everything written has reached the operating system. */
    bool flush();

    //=========================================================================================================
    /**
     * Cuts the file to, or extends it with zeros to, @p size bytes; the position moves back to the new end if it was past it.
     *
     * @param[in] size   New length in bytes.
     *
     * @return Whether the file has the new length; never for a read-only stream.
     */
    bool resize(std::uint64_t size);

private:
    FileByteStream(std::FILE* file, Mode mode, std::filesystem::path path);

    /** C streams need a seek between a write and a following read, and vice versa. */
    void switch_direction(bool writing);

    std::FILE* m_file = nullptr;
    Mode m_mode = Mode::Read;
    std::filesystem::path m_path;
    std::uint64_t m_pos = 0;
    std::uint64_t m_size = 0;
    bool m_lastWrite = false;
};

//=============================================================================================================
/** How the bytes of a file are stored. */
enum class Compression
{
    None, ///< As they are.
    Gzip  ///< Gzip-compressed, e.g. .mgz or .nii.gz.
};

//=============================================================================================================
/**
 * Reads a whole file, decompressing it if asked to.
 *
 * @param[in] path          The file.
 * @param[in] compression   How the file is stored.
 *
 * @return The (decompressed) bytes; an Io error if the file cannot be read, a Format error if the gzip data is
 *         damaged or cut short.
 */
BASESHARED_EXPORT Expected<std::vector<std::byte>> read_file_bytes(const std::filesystem::path& path, Compression compression = Compression::None);

//=============================================================================================================
/**
 * Reads a whole file as text, byte for byte (no line-ending or encoding conversion).
 *
 * @param[in] path   The file.
 *
 * @return The content; an Io error if the file cannot be read.
 */
BASESHARED_EXPORT Expected<std::string> read_text_file(const std::filesystem::path& path);

//=============================================================================================================
/**
 * Replaces a file with the given text, byte for byte, so "\n" stays LF on every platform.
 *
 * @param[in] path   The file.
 * @param[in] text   The content.
 *
 * @return Nothing; an Io error if the file cannot be written.
 */
BASESHARED_EXPORT Expected<void> write_text_file(const std::filesystem::path& path, std::string_view text);

} // namespace mne

#endif // BASE_BYTE_STREAM_H
