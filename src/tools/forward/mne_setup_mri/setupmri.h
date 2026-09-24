//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     setupmri.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    SetupMri class declaration.
 *           Ported from the original MNE C tools mne_setup_mri (shell script) and
 *           mne_make_cor_set (C program) by Matti Hamalainen.
 *           Cross-referenced with MNE-Python (which never ported this tool, as
 *           modern MNE-Python reads .mgz files directly via nibabel).
 */

#ifndef SETUPMRI_H
#define SETUPMRI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri/mri_vol_data.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNESETUPMRI
//=============================================================================================================

namespace MNESETUPMRI {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNESetupMriSettings;

//=============================================================================================================
/**
 * Sets up FreeSurfer MRI data for use with MNE software.
 *
 * This class implements the functionality of the original MNE C tools:
 * - mne_setup_mri: Creates the Neuromag directory structure
 * - mne_make_cor_set: Converts COR slices or .mgz files to COR.fif
 *
 * The actual I/O operations (MGH/MGZ reading, COR reading, COR.fif writing)
 * are delegated to the MRILIB library classes:
 *   - MRILIB::MriMghIO  — reads .mgz/.mgh files
 *   - MRILIB::MriCorIO  — reads COR-NNN slice files
 *   - MRILIB::MriCorFifIO — writes COR.fif FIFF files
 *
 * @brief Sets up MRI data for MNE use.
 */
class SetupMri
{
public:
    //=========================================================================================================
    /**
     * Constructs the SetupMri processor.
     *
     * @param[in] settings  The parsed command-line settings.
     */
    SetupMri(const MNESetupMriSettings& settings);

    //=========================================================================================================
    /**
     * Runs the MRI setup process.
     *
     * @return 0 on success, 1 on failure.
     */
    int run();

private:
    //=========================================================================================================
    /**
     * Process a single MRI set (e.g., "T1" or "brain").
     *
     * @param[in] mriName  Name of the MRI set.
     *
     * @return true on success.
     */
    bool processMriSet(const QString& mriName);

    //=========================================================================================================

    const MNESetupMriSettings& m_settings;      /**< Command-line settings. */
};

} // namespace MNESETUPMRI

#endif // SETUPMRI_H
