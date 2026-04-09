//=============================================================================================================
/**
 * @file     bids_coordinate_system.h
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
 * @brief    BidsCoordinateSystem struct — iEEG coordinate system from *_coordsystem.json.
 *
 */

#ifndef BIDS_COORDINATE_SYSTEM_H
#define BIDS_COORDINATE_SYSTEM_H

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
 * @brief Coordinate system metadata from *_coordsystem.json.
 *
 * Describes the spatial reference frame used for electrode positions.
 */
struct BIDSSHARED_EXPORT BidsCoordinateSystem
{
    QString system;                     /**< e.g. "ACPC", "MNI305", "Other" (REQUIRED for iEEG). */
    QString units;                      /**< "m", "mm", or "cm" (REQUIRED for iEEG). */
    QString description;                /**< Description of the coordinate system (RECOMMENDED). */
    QString processingDescription;      /**< How coordinates were obtained (RECOMMENDED). */
    QString associatedImagePath;        /**< Relative path to associated T1w image (OPTIONAL). */

    /**
     * @brief Read a BIDS *_coordsystem.json file.
     * @param[in] sFilePath  Path to the coordsystem.json file.
     * @return Populated coordinate system, or default if file cannot be read.
     */
    static BidsCoordinateSystem readJson(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_coordsystem.json file.
     * @param[in] sFilePath  Output path.
     * @param[in] cs         Coordinate system metadata.
     * @return true on success.
     */
    static bool writeJson(const QString& sFilePath,
                          const BidsCoordinateSystem& cs);
};

} // namespace BIDSLIB

#endif // BIDS_COORDINATE_SYSTEM_H
