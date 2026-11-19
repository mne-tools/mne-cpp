//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_evoked_file.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Internal reader shared by FiffEvoked::read and FiffEvokedSet::read; not installed.
 */

#ifndef FIFF_EVOKED_FILE_H
#define FIFF_EVOKED_FILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dir_node.h"
#include <memory>

#include <vector>
#include "fiff_evoked.h"
#include "fiff_info.h"
#include "fiff_stream.h"

#include <base/error.h>

#include <base/string_utils.h>

#include <filesystem>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * An evoked-response file opened once, so that several of its data sets are read in one pass.
 */
class FiffEvokedFile
{
public:
    /**
     * @param[in] path   The file.
     * @return The opened file, or why it holds no evoked data.
     */
    static mne::Expected<FiffEvokedFile> open(const std::filesystem::path& path);

    /** @return The number of evoked data sets. */
    std::size_t size() const
    {
        return m_sets.size();
    }

    /**
     * @param[in] condition   Index of the data set, or its comment together with @p kind.
     * @param[in] kind        FIFFV_ASPECT_AVERAGE or FIFFV_ASPECT_STD_ERR.
     * @return The index of the selected data set, or why there is none.
     */
    mne::Expected<int> find(const FiffEvoked::Condition& condition, fiff_int_t kind) const;

    /**
     * @param[in] set        Index of the data set.
     * @param[in] baseline   Interval to subtract the mean of, or none for no baseline correction.
     * @param[in] proj       Apply the SSP projectors of the file.
     * @return The data set, calibrated, projected and baseline corrected as requested.
     */
    mne::Expected<FiffEvoked> read(int set, const FiffEvoked::Baseline& baseline, bool proj);

    FiffInfo info; /**< The measurement info shared by all data sets. */

private:
    FiffEvokedFile(FiffStream stream, std::string file)
    : m_stream(std::move(stream))
    , m_file(std::move(file))
    {
    }

    FiffStream m_stream;
    std::string m_file;
    std::vector<std::shared_ptr<FiffDirNode>> m_sets;
    mne::StringList m_comments;
    std::vector<fiff_int_t> m_kinds;
};

} // namespace mne

#endif // FIFF_EVOKED_FILE_H
