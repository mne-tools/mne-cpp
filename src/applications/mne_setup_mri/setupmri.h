//=============================================================================================================
/**
 * @file     setupmri.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    SetupMri class declaration.
 *           Ported from the original MNE C tools mne_setup_mri (shell script) and
 *           mne_make_cor_set (C program) by Matti Hamalainen.
 *           Cross-referenced with MNE-Python (which never ported this tool, as
 *           modern MNE-Python reads .mgz files directly via nibabel).
 *
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

class MneSetupMriSettings;

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
    SetupMri(const MneSetupMriSettings& settings);

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

    const MneSetupMriSettings& m_settings;      /**< Command-line settings. */
};

} // namespace MNESETUPMRI

#endif // SETUPMRI_H
