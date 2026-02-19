//=============================================================================================================
/**
 * @file     watershedbem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    WatershedBem class declaration.
 *
 *           Ported from the original MNE shell script mne_watershed_bem
 *           by Matti Hamalainen (SVN $Id: mne_watershed_bem 3391).
 *
 *           The original shell script orchestrated three tools:
 *             1. mri_watershed (FreeSurfer) — BEM surface segmentation
 *             2. mne_convert_surface — coordinate system update from MGH volume
 *             3. mne_surf2bem — conversion of FreeSurfer surfaces to FIFF BEM
 *
 *           This C++ implementation:
 *             - Calls mri_watershed via QProcess
 *             - Reads surfaces using FSLIB::Surface
 *             - Applies voxel-to-RAS transforms from MRI volumes (MRILIB)
 *             - Writes BEM FIFF files using MNELIB::MNEBem/MNEBemSurface
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_watershed_bem().
 *
 */

#ifndef WATERSHEDBEM_H
#define WATERSHEDBEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNEWATERSHEDBEM
//=============================================================================================================

namespace MNEWATERSHEDBEM {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneWatershedBemSettings;

//=============================================================================================================
/**
 * Creates BEM surfaces using FreeSurfer's mri_watershed algorithm.
 *
 * This class implements the full workflow of the original mne_watershed_bem
 * shell script:
 *
 *   1. Validates the FreeSurfer environment and subject directory structure
 *   2. Runs mri_watershed to segment brain, inner skull, outer skull, and
 *      outer skin surfaces from the MRI volume
 *   3. Converts the watershed surfaces to use surface RAS coordinates
 *      (equivalent to the original mne_convert_surface --mghmri step)
 *   4. Creates a FIFF BEM head surface file from the outer skin surface
 *      (equivalent to the original mne_surf2bem step)
 *
 * Requires:
 *   - FreeSurfer installed and FREESURFER_HOME set
 *   - SUBJECTS_DIR set with the subject's MRI data
 *   - mri_watershed available in the FreeSurfer bin directory
 *
 * @brief Creates BEM surfaces via FreeSurfer's watershed algorithm.
 */
class WatershedBem
{
public:
    //=========================================================================================================
    /**
     * Constructs the WatershedBem processor.
     *
     * @param[in] settings  The parsed command-line settings.
     */
    WatershedBem(const MneWatershedBemSettings& settings);

    //=========================================================================================================
    /**
     * Runs the watershed BEM surface creation process.
     *
     * @return 0 on success, 1 on failure.
     */
    int run();

private:
    //=========================================================================================================
    /**
     * Runs mri_watershed to segment surfaces from the MRI volume.
     *
     * @param[in] mriInput   Path to the MRI input (directory or .mgz file).
     * @param[in] wsDir      Path to the watershed output directory.
     *
     * @return true on success.
     */
    bool runMriWatershed(const QString& mriInput, const QString& wsDir);

    //=========================================================================================================
    /**
     * Converts watershed surfaces to use surface RAS coordinates from the
     * MGH/MGZ volume. This replaces the original mne_convert_surface step.
     *
     * When mri_watershed outputs surfaces using .mgz input, the surface
     * vertex coordinates may need the volume geometry from the MGH file.
     * This method reads the MGH volume to extract the vox2ras transform
     * and applies it to update the surface coordinate metadata.
     *
     * @param[in] wsDir      Path to the watershed output directory.
     * @param[in] mgzFile    Path to the MGH/MGZ volume file.
     *
     * @return true on success.
     */
    bool convertSurfaces(const QString& wsDir, const QString& mgzFile);

    //=========================================================================================================
    /**
     * Creates a FIFF BEM head surface file from the outer skin surface.
     * This replaces the original mne_surf2bem step.
     *
     * Reads the FreeSurfer outer_skin_surface, creates an MNEBemSurface
     * with id FIFFV_BEM_SURF_ID_HEAD (4), and writes it as a FIFF BEM file.
     *
     * @param[in] surfFile   Path to the FreeSurfer outer skin surface file.
     * @param[in] fifFile    Path to the output FIFF BEM file.
     *
     * @return true on success.
     */
    bool createBemFif(const QString& surfFile, const QString& fifFile);

    //=========================================================================================================

    const MneWatershedBemSettings& m_settings;  /**< Command-line settings. */
};

} // namespace MNEWATERSHEDBEM

#endif // WATERSHEDBEM_H
