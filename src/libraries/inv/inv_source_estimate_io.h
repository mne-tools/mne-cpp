//=============================================================================================================
/**
 * @file     inv_source_estimate_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Extended I/O for source estimates (CSV, FIFF).
 *
 * Supplements the built-in STC/W binary format with text-based CSV export
 * and FIFF-format export for interoperability with MNE-Python.
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
