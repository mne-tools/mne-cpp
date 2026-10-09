//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     standard_montage.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Standard 10-20 / 10-10 / 10-05 EEG electrode positions of MNE-Python's standard_1005 montage.
 *
 * @ref UTILSLIB::StandardMontage gives every consumer in mne-cpp (the coregistration wizard,
 * the layout maker, forward modelling) a default cap when no digitised positions exist. The
 * positions are those of mne.channels.make_standard_montage("standard_1005"), in metres in
 * the MNI (fsaverage MRI) frame; the 10-20 and 10-10 systems are its electrodes of those names.
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
 *
 * @snippet ex_utils/main.cpp standard_montage_usage
 */
struct UTILSSHARED_EXPORT ElectrodePosition
{
    QString name;        /**< Electrode name (e.g. "Cz", "Fp1"). */
    Eigen::Vector3d pos; /**< Position in metres, MNI (fsaverage MRI) coordinates. */
};

//=============================================================================================================
/**
 * @brief Standard EEG montage with named electrode positions.
 *
 * @snippet ex_utils/main.cpp standard_montage_usage
 */
class UTILSSHARED_EXPORT StandardMontage
{
public:
    //=========================================================================================================
    /**
     * @brief Supported standard montage systems.
     */
    enum class System
    {
        Standard_1020, /**< 10-20 system (21 electrodes, including A1/A2). */
        Standard_1010, /**< 10-10 system (73 positions, including the 10-20 ones and Nz). */
        Standard_1005  /**< 10-05 system (343 electrodes, mne's standard_1005 montage). */
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
     * Searches every position of the standard_1005 table, including the fiducials Nz, LPA and RPA.
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
     *
     * @param[in] system  The montage system.
     *
     * @return Number of electrode positions defined for the montage.
     */
    static int electrodeCount(System system);
};

} // namespace UTILSLIB

#endif // STANDARD_MONTAGE_H
