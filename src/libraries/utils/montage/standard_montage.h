//=============================================================================================================
/**
 * @file     standard_montage.h
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
 * @brief    Standard EEG montage definitions (10-20, 10-10, 10-05).
 *
 * Provides built-in electrode position tables for the standard EEG
 * placement systems. Positions are in MNI head coordinates (metres).
 */

#ifndef STANDARD_MONTAGE_H
#define STANDARD_MONTAGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Represents an electrode position in a montage.
 */
struct UTILSSHARED_EXPORT ElectrodePosition
{
    QString name;              /**< Electrode name (e.g. "Cz", "Fp1"). */
    Eigen::Vector3d pos;       /**< Position in metres (MNI head coords). */
};

//=============================================================================================================
/**
 * @brief Standard EEG montage with named electrode positions.
 */
class UTILSSHARED_EXPORT StandardMontage
{
public:
    //=========================================================================================================
    /**
     * @brief Supported standard montage systems.
     */
    enum class System {
        Standard_1020,    /**< 10-20 system (21 electrodes). */
        Standard_1010,    /**< 10-10 system (81 electrodes). */
        Standard_1005     /**< 10-05 system (345 electrodes). */
    };

    //=========================================================================================================
    /**
     * @brief Get a standard montage by system name.
     *
     * @param[in] system  The montage system.
     *
     * @return List of electrode positions.
     */
    static QList<ElectrodePosition> getMontage(System system);

    //=========================================================================================================
    /**
     * @brief Get electrode names for a standard montage.
     *
     * @param[in] system  The montage system.
     *
     * @return List of electrode names.
     */
    static QStringList getElectrodeNames(System system);

    //=========================================================================================================
    /**
     * @brief Look up a single electrode position by name.
     *
     * Searches through all built-in montages.
     *
     * @param[in] name  Electrode name (case-insensitive).
     * @param[out] pos  Position if found.
     *
     * @return true if found.
     */
    static bool findElectrode(const QString& name, Eigen::Vector3d& pos);

    //=========================================================================================================
    /**
     * @brief Get the number of electrodes in a montage.
     */
    static int electrodeCount(System system);

private:
    static QList<ElectrodePosition> buildStandard1020();
    static QList<ElectrodePosition> buildStandard1010();
};

} // namespace UTILSLIB

#endif // STANDARD_MONTAGE_H
