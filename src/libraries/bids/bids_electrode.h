//=============================================================================================================
/**
 * @file     bids_electrode.h
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
 * @brief    BidsElectrode struct — electrode position from *_electrodes.tsv.
 *
 */

#ifndef BIDS_ELECTRODE_H
#define BIDS_ELECTRODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Electrode position record corresponding to one row in *_electrodes.tsv.
 */
struct BIDSSHARED_EXPORT BidsElectrode
{
    QString name;       /**< Electrode name (REQUIRED). */
    QString x;          /**< X coordinate or "n/a" (REQUIRED). */
    QString y;          /**< Y coordinate or "n/a" (REQUIRED). */
    QString z;          /**< Z coordinate or "n/a" (REQUIRED). */
    QString size;       /**< Electrode size in mm or "n/a" (RECOMMENDED for iEEG). */
    QString type;       /**< Electrode type: "depth", "strip", "grid" (OPTIONAL). */
    QString material;   /**< Electrode material (OPTIONAL). */
    QString impedance;  /**< Impedance value or "n/a" (OPTIONAL). */

    /**
     * @brief Read a BIDS *_electrodes.tsv file.
     * @param[in] sFilePath  Path to the electrodes.tsv file.
     * @return List of electrode records.
     */
    static QList<BidsElectrode> readTsv(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_electrodes.tsv file.
     * @param[in] sFilePath    Output path.
     * @param[in] electrodes   List of electrode records.
     * @return true on success.
     */
    static bool writeTsv(const QString& sFilePath,
                         const QList<BidsElectrode>& electrodes);
};

} // namespace BIDSLIB

#endif // BIDS_ELECTRODE_H
