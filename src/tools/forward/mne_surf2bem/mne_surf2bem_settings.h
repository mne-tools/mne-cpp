//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_surf2bem_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNESurf2BemSettings class declaration.
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
class MNESurf2BemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MNESurf2BemSettings(int *argc, char **argv);

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

    //=========================================================================================================
    /**
     * Returns whether the process should exit immediately (e.g., --help or --version).
     *
     * @return True if the constructor encountered --help, --version, or an error.
     */
    bool shouldExit() const;

    //=========================================================================================================
    /**
     * Returns the exit code to use when shouldExit() is true.
     *
     * @return 0 for --help/--version, 1 for errors.
     */
    int exitCode() const;

private:
    QVector<SurfaceInput> m_surfaces;   /**< Input surface specifications. */
    QString     m_sOutputFile;          /**< Output FIF file path. */
    int         m_iCoordFrame;          /**< Coordinate frame for ASCII files. */
    bool        m_bCheck;               /**< Perform topology checks. */
    bool        m_bCheckMore;           /**< Perform thickness checks. */
    bool        m_bForce;               /**< Force-load despite defects. */
    bool        m_bShouldExit = false;  /**< True when the app should exit immediately. */
    int         m_iExitCode = 0;        /**< Exit code when m_bShouldExit is true. */
};

} // namespace MNESURF2BEM

#endif // MNESURF2BEMSETTINGS_H
