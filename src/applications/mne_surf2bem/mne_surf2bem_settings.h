//=============================================================================================================
/**
 * @file     mne_surf2bem_settings.h
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
 * @brief    MneSurf2BemSettings class declaration.
 *
 */

#ifndef MNESURF2BEMSETTINGS_H
#define MNESURF2BEMSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE MNESURF2BEM
//=============================================================================================================

namespace MNESURF2BEM {

//=============================================================================================================
/**
 * Holds per-surface input parameters for mne_surf2bem.
 *
 * Each surface can have its own id, conductivity, shift, swap, etc.
 * Modeled after the original C code that stored parallel arrays.
 *
 * @brief Per-surface input parameters.
 */
struct SurfaceInput
{
    QString fileName;       /**< Input surface file name. */
    bool    isAsciiTri;     /**< True if ASCII triangle file (.tri), false for FreeSurfer binary. */
    int     id;             /**< BEM surface id (1=brain, 3=skull, 4=head), -1 if not set. */
    float   sigma;          /**< Compartment conductivity [S/m], -1 if not set. */
    int     ico;            /**< Icosahedron subdivision level for downsampling (0-6), -1 if not used. */
    bool    swap;           /**< Swap vertex winding order (ASCII tri files only). */
    bool    mm;             /**< Coordinates in millimeters (true) or meters (false). Default: true. */
    float   shift;          /**< Vertex shift along normals [meters]. 0 = no shift. */

    SurfaceInput()
    : isAsciiTri(false)
    , id(-1)
    , sigma(-1.0f)
    , ico(-1)
    , swap(false)
    , mm(true)
    , shift(0.0f)
    {}
};

//=============================================================================================================
/**
 * Parses and stores command-line settings for mne_surf2bem.
 *
 * Supports the same options as the original MNE C tool:
 *   --surf, --tri, --fif, --id, --swap, --meters, --ico, --sigma,
 *   --shift, --force, --check, --checkmore, --coordf
 *
 * The original tool used positional association: options like --id, --swap,
 * --shift apply to the most recently specified surface. This parser
 * replicates that behavior.
 *
 * @brief Command-line settings for mne_surf2bem.
 */
class MneSurf2BemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MneSurf2BemSettings(int *argc, char **argv);

    //=========================================================================================================
    /**
     * Returns the list of surface input specifications.
     *
     * @return Vector of SurfaceInput structs.
     */
    const QVector<SurfaceInput>& surfaces() const;

    //=========================================================================================================
    /**
     * Returns the output FIF file path.
     *
     * @return Output file path, empty if no output requested.
     */
    QString outputFile() const;

    //=========================================================================================================
    /**
     * Returns the coordinate frame for ASCII triangle files.
     *
     * @return FIFF coordinate frame constant.
     */
    int coordFrame() const;

    //=========================================================================================================
    /**
     * Returns whether topology checks should be performed.
     *
     * @return True if --check was specified.
     */
    bool check() const;

    //=========================================================================================================
    /**
     * Returns whether extended thickness checks should be performed.
     *
     * @return True if --checkmore was specified.
     */
    bool checkMore() const;

    //=========================================================================================================
    /**
     * Returns whether to force-load surfaces with topological defects.
     *
     * @return True if --force was specified.
     */
    bool force() const;

private:
    QVector<SurfaceInput> m_surfaces;   /**< Input surface specifications. */
    QString     m_sOutputFile;          /**< Output FIF file path. */
    int         m_iCoordFrame;          /**< Coordinate frame for ASCII files. */
    bool        m_bCheck;               /**< Perform topology checks. */
    bool        m_bCheckMore;           /**< Perform thickness checks. */
    bool        m_bForce;               /**< Force-load despite defects. */
};

} // namespace MNESURF2BEM

#endif // MNESURF2BEMSETTINGS_H
