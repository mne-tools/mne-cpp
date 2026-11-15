//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Text-based and FIFF-based export/import for @ref mne::InvSourceEstimate that complement the binary STC / W format.
 *
 * @ref mne::InvSourceEstimateIO adds CSV and tab-separated matrix
 * export (and matching CSV import) on top of the built-in STC / W binary
 * I/O of @ref mne::InvSourceEstimate. CSV output is intended for downstream
 * spreadsheet, Python or R analysis where consumers cannot read the
 * FreeSurfer-style STC blob, while the bare-matrix variant feeds plotting
 * tools that only need the numeric grid. The file-row layout is one time
 * sample per row with the leading column carrying the time in seconds,
 * matching the convention used by mne-python's @c stc_to_csv helpers.
 */

#ifndef INV_SOURCE_ESTIMATE_IO_H
#define INV_SOURCE_ESTIMATE_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include "inv_source_estimate.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Extended I/O methods for InvSourceEstimate.
 *
 * @snippet ex_inv_api/main.cpp inv_source_estimate_io_usage
 */
class INVSHARED_EXPORT InvSourceEstimateIO
{
public:
    //=========================================================================================================
    /**
     * @brief Export source estimate to CSV format.
     *
     * Writes a header line with vertex indices, then one row per time point.
     * First column is time (seconds). Numbers are written in the shortest form
     * that reads back to the same value.
     *
     * @param[in] stc      Source estimate to export.
     * @param[in] path     Output file path.
     * @param[in] delim    Delimiter character (default comma).
     *
     * @return true if successful.
     */
    [[nodiscard]] static bool write_csv(const InvSourceEstimate& stc,
                                        const std::filesystem::path& path,
                                        char delim = ',');

    //=========================================================================================================
    /**
     * @brief Read source estimate from CSV format.
     *
     * @param[in] path     Input file path.
     * @param[in] delim    Delimiter character (default comma).
     *
     * @return Source estimate; empty if the file cannot be read, a field is
     *         not a number or a row does not hold one value per vertex.
     */
    static InvSourceEstimate read_csv(const std::filesystem::path& path,
                                      char delim = ',');

    //=========================================================================================================
    /**
     * @brief Export source estimate as a tab-separated matrix for easy import.
     *
     * No header; pure numeric matrix (n_vertices × n_times).
     *
     * @param[in] stc      Source estimate to export.
     * @param[in] path     Output file path.
     *
     * @return true if successful.
     */
    [[nodiscard]] static bool write_matrix(const InvSourceEstimate& stc,
                                           const std::filesystem::path& path);
};

} // namespace mne

#endif // INV_SOURCE_ESTIMATE_IO_H
