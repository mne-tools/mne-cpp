//=============================================================================================================
/**
 * @file     setupforwardmodel.h
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
 * @brief    SetupForwardModel class declaration.
 *
 *           Ported from the original MNE shell script mne_setup_forward_model
 *           by Matti Hamalainen (SVN $Id: mne_setup_forward_model 3282).
 *
 *           Sets up the BEM for forward modeling by:
 *             1. Creating a BEM geometry FIFF file from triangulated surfaces
 *             2. Exporting ASCII .pnt and FreeSurfer .surf files
 *             3. Computing the BEM solution matrix (optional)
 *
 *           Cross-referenced with MNE-Python's mne.make_bem_model() and
 *           mne.make_bem_solution().
 *
 */

#ifndef SETUPFORWARDMODEL_H
#define SETUPFORWARDMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNESETUPFORWARDMODEL
//=============================================================================================================

namespace MNESETUPFORWARDMODEL {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneSetupForwardModelSettings;

//=============================================================================================================
/**
 * Sets up the BEM for forward modeling.
 *
 * This class implements the full workflow of the original mne_setup_forward_model
 * shell script:
 *   1. Locate BEM surface files (inner_skull, outer_skull, outer_skin)
 *   2. Read surfaces and create a BEM geometry FIFF file (mne_surf2bem equivalent)
 *   3. Export .pnt (ASCII vertex) and .surf (FreeSurfer binary) files
 *      (mne_list_bem equivalent)
 *   4. Compute the BEM solution matrix (mne_prepare_bem_model equivalent)
 *
 * @brief Sets up BEM for forward modeling.
 */
class SetupForwardModel
{
public:
    //=========================================================================================================
    /**
     * Constructs the SetupForwardModel processor.
     *
     * @param[in] settings  The parsed command-line settings.
     */
    SetupForwardModel(const MneSetupForwardModelSettings& settings);

    //=========================================================================================================
    /**
     * Runs the BEM setup process.
     *
     * @return 0 on success, 1 on failure.
     */
    int run();

private:
    //=========================================================================================================
    /**
     * Locates a BEM surface file in the bem directory.
     *
     * Tries two naming conventions:
     *   1. <bem_dir>/<name>.<ext>
     *   2. <bem_dir>/<subject>-<name>.<ext>
     *
     * @param[in] bemDir   The BEM directory path.
     * @param[in] name     Surface name (e.g., "inner_skull").
     * @param[in] ext      File extension ("tri" or "surf").
     * @param[out] path    The found file path.
     *
     * @return true if the file was found.
     */
    bool locateSurface(const QString& bemDir, const QString& name,
                       const QString& ext, QString& path) const;

    //=========================================================================================================
    /**
     * Reads an ASCII triangle file and populates an MNEBemSurface.
     *
     * @param[in] fileName  Path to the .tri file.
     * @param[in] id        BEM surface ID to assign.
     * @param[in] sigma     Conductivity [S/m].
     * @param[in] shift     Vertex shift along normals [mm].
     * @param[out] surf     The populated BEM surface.
     *
     * @return true on success.
     */
    bool readAsciiTriFile(const QString& fileName, int id, float sigma,
                          float shift, MNELIB::MNEBemSurface& surf) const;

    //=========================================================================================================
    /**
     * Reads a FreeSurfer binary surface file and populates an MNEBemSurface.
     *
     * @param[in] fileName  Path to the .surf file.
     * @param[in] id        BEM surface ID to assign.
     * @param[in] sigma     Conductivity [S/m].
     * @param[in] shift     Vertex shift along normals [mm].
     * @param[out] surf     The populated BEM surface.
     *
     * @return true on success.
     */
    bool readFreeSurferSurf(const QString& fileName, int id, float sigma,
                            float shift, MNELIB::MNEBemSurface& surf) const;

    //=========================================================================================================
    /**
     * Shifts surface vertices outward along their normals.
     *
     * @param[in,out] surf   The surface to modify.
     * @param[in]     shift  Shift distance in meters.
     */
    void shiftVertices(MNELIB::MNEBemSurface& surf, float shift) const;

    //=========================================================================================================
    /**
     * Exports a BEM surface as an ASCII .pnt file (mne_list_bem equivalent).
     *
     * Format: one vertex per line as "x y z" in millimeters.
     *
     * @param[in] surf      The BEM surface to export.
     * @param[in] fileName  Output .pnt file path.
     *
     * @return true on success.
     */
    bool exportPntFile(const MNELIB::MNEBemSurface& surf,
                       const QString& fileName) const;

    //=========================================================================================================
    /**
     * Exports a BEM surface as a FreeSurfer binary .surf file
     * (mne_list_bem --surf equivalent).
     *
     * @param[in] surf      The BEM surface to export.
     * @param[in] fileName  Output .surf file path.
     *
     * @return true on success.
     */
    bool exportSurfFile(const MNELIB::MNEBemSurface& surf,
                        const QString& fileName) const;

    //=========================================================================================================
    /**
     * Computes and saves the BEM solution matrix
     * (mne_prepare_bem_model equivalent).
     *
     * Uses the FwdBemModel to load the BEM geometry, compute the linear
     * collocation solution, and writes the result as a -sol.fif file.
     *
     * @param[in] bemFile   Path to the BEM geometry FIFF file.
     * @param[in] solFile   Path for the output BEM solution file.
     *
     * @return true on success.
     */
    bool prepareBemSolution(const QString& bemFile,
                            const QString& solFile) const;

    //=========================================================================================================

    const MneSetupForwardModelSettings& m_settings;  /**< Command-line settings. */
};

} // namespace MNESETUPFORWARDMODEL

#endif // SETUPFORWARDMODEL_H
