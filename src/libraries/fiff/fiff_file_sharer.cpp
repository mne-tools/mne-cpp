//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_file_sharer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref FiffFileSharer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_file_sharer.h"
#include "fiff_stream.h"

#include <base/string_utils.h>

#include <format>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using mne::ErrorCode;
using mne::fail;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffFileSharer::FiffFileSharer(const std::filesystem::path& directory)
: m_directory(std::filesystem::absolute(directory))
{
}

//=============================================================================================================

const std::filesystem::path& FiffFileSharer::directory() const
{
    return m_directory;
}

//=============================================================================================================

std::filesystem::path FiffFileSharer::fileAt(int index) const
{
    return m_directory / std::format("realtime_file{}_raw.fif", index);
}

//=============================================================================================================

mne::Expected<std::filesystem::path> FiffFileSharer::copy_realtime_file(const std::filesystem::path& source)
{
    std::error_code ec;
    std::filesystem::create_directories(m_directory, ec);
    std::filesystem::path target = fileAt(m_next);
    // Complete the copy under another name and rename it, so a consumer never sees a half-written file
    std::filesystem::path part = target;
    part += ".part";
    if (ec || !std::filesystem::copy_file(source, part, std::filesystem::copy_options::overwrite_existing, ec)) {
        return fail(ErrorCode::Io, "cannot copy {} to {}: {}", mne::utf8FromPath(source), mne::utf8FromPath(part), ec.message());
    }
    // The copy is cut mid-recording: close its raw data blocks so it reads as a complete file
    if (auto bytes = mne::FileByteStream::open(part, mne::FileByteStream::Mode::ReadWrite)) {
        bytes->seek(bytes->size());
        FiffStream(std::move(bytes)).finish_writing_raw();
    }
    std::filesystem::rename(part, target, ec);
    if (ec) {
        std::filesystem::remove(part, ec);
        return fail(ErrorCode::Io, "cannot share {}", mne::utf8FromPath(target));
    }
    ++m_next;
    return target;
}

//=============================================================================================================

std::vector<std::filesystem::path> FiffFileSharer::new_files()
{
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    for (std::filesystem::path next = fileAt(m_next); std::filesystem::exists(next, ec); next = fileAt(++m_next)) {
        files.push_back(std::move(next));
    }
    return files;
}

//=============================================================================================================

bool FiffFileSharer::clear()
{
    std::error_code ec;
    std::filesystem::create_directories(m_directory, ec);
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_directory, ec)) {
        if (entry.is_regular_file(ec)) {
            std::filesystem::remove(entry.path(), ec);
        }
    }
    m_next = 0;
    return std::filesystem::is_directory(m_directory, ec) && std::filesystem::is_empty(m_directory, ec);
}
