//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Text-based and FIFF-based export/import for @ref INVLIB::InvSourceEstimate that complement the binary STC / W format.
 *
 * @ref INVLIB::InvSourceEstimateIO adds CSV and tab-separated matrix
 * export (and matching CSV import) on top of the built-in STC / W binary
 * I/O of @ref InvSourceEstimate. CSV output is intended for downstream
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
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Extended I/O methods for InvSourceEstimate.
 */
class INVSHARED_EXPORT InvSourceEstimateIO
{
public:
    //=========================================================================================================
    /**
     * @brief Export source estimate to CSV format.
     *
     * Writes a header line with vertex indices, then one row per time point.
     * First column is time (seconds).
     *
     * @param[in] stc      Source estimate to export.
     * @param[in] sPath    Output file path.
     * @param[in] cDelim   Delimiter character (default comma).
     *
     * @return true if successful.
     */
    static bool writeCsv(const InvSourceEstimate& stc,
                          const QString& sPath,
                          char cDelim = ',');

    //=========================================================================================================
    /**
     * @brief Read source estimate from CSV format.
     *
     * @param[in] sPath    Input file path.
     * @param[in] cDelim   Delimiter character (default comma).
     *
     * @return Source estimate (empty if failed).
     */
    static InvSourceEstimate readCsv(const QString& sPath,
                                      char cDelim = ',');

    //=========================================================================================================
    /**
     * @brief Export source estimate as a tab-separated matrix for easy import.
     *
     * No header; pure numeric matrix (n_vertices × n_times).
     *
     * @param[in] stc      Source estimate to export.
     * @param[in] sPath    Output file path.
     *
     * @return true if successful.
     */
    static bool writeMatrix(const InvSourceEstimate& stc,
                             const QString& sPath);
};

} // namespace INVLIB

#endif // INV_SOURCE_ESTIMATE_IO_H
