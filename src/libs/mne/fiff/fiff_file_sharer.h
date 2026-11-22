//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     fiff_file_sharer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2021
 * @brief    Hands finished FIFF recordings from one application to another through a shared directory.
 *
 * The producer (e.g. MNE Scan's file writer) calls @ref mne::FiffFileSharer::copy_realtime_file, which copies
 * a recording into the shared directory as @c realtime_file<N>_raw.fif and closes its raw block so it is a complete
 * FIFF file. The consumer (e.g. MNE Analyze) calls @ref mne::FiffFileSharer::new_files whenever it is told the
 * directory changed, for instance by a file system watcher.
 */

#ifndef FIFF_FILE_SHARER_H
#define FIFF_FILE_SHARER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include <mne/base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Copies FIFF recordings into a shared directory and lists the ones a consumer has not seen yet.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_file_sharer_usage
 */
class FIFFSHARED_EXPORT FiffFileSharer
{
public:
    //=========================================================================================================
    /**
     * @param[in] directory   The shared directory; a relative path is taken from the current directory now.
     */
    explicit FiffFileSharer(const std::filesystem::path& directory = "realtime_shared_files");

    /** @return The shared directory, absolute. */
    const std::filesystem::path& directory() const;

    //=========================================================================================================
    /**
     * Producer: copies an unfinished FIFF recording into the shared directory and closes its raw block. The copy
     * appears under its name only once it is complete.
     *
     * @param[in] source   The recording, still being written.
     *
     * @return The shared file, or an Io error if the directory cannot be made or the source cannot be copied.
     */
    [[nodiscard]] mne::Expected<std::filesystem::path> copy_realtime_file(const std::filesystem::path& source);

    //=========================================================================================================
    /**
     * Consumer: the files shared since the last call, in the order they were shared.
     *
     * @return Paths of complete FIFF files.
     */
    [[nodiscard]] std::vector<std::filesystem::path> new_files();

    //=========================================================================================================
    /**
     * Consumer: creates the shared directory if needed, removes the files of earlier sessions and starts counting
     * from the first file again.
     *
     * @return Whether the directory exists and is empty.
     */
    bool clear();

private:
    /** @return The path of shared file @p index. */
    std::filesystem::path fileAt(int index) const;

    std::filesystem::path m_directory;
    int m_next = 0; /**< Index of the next file to share (producer) or to report (consumer). */
};
} // namespace mne

#endif // FIFF_FILE_SHARER_H
