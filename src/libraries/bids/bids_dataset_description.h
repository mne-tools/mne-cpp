//=============================================================================================================
/**
 * @file     bids_dataset_description.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
 * @brief    BidsDatasetDescription struct — dataset_description.json I/O.
 *
 */

#ifndef BIDS_DATASET_DESCRIPTION_H
#define BIDS_DATASET_DESCRIPTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Dataset-level metadata from dataset_description.json.
 *
 * Every BIDS dataset must contain a dataset_description.json at its root.
 * This struct captures the required and recommended fields.
 */
struct BIDSSHARED_EXPORT BidsDatasetDescription
{
    QString name;           /**< Human-readable dataset name (REQUIRED). */
    QString bidsVersion;    /**< BIDS specification version, e.g. "1.9.0" (REQUIRED). */
    QString datasetType;    /**< "raw" or "derivative" (RECOMMENDED). */
    QString license;        /**< License identifier (RECOMMENDED). */

    /**
     * @brief Read a dataset_description.json file.
     * @param[in] sFilePath  Path to dataset_description.json.
     * @return Populated description, or default if file cannot be read.
     */
    static BidsDatasetDescription read(const QString& sFilePath);

    /**
     * @brief Write a dataset_description.json file.
     * @param[in] sFilePath  Output path.
     * @param[in] desc       Dataset description to write.
     * @return true on success.
     */
    static bool write(const QString& sFilePath,
                      const BidsDatasetDescription& desc);
};

} // namespace BIDSLIB

#endif // BIDS_DATASET_DESCRIPTION_H
