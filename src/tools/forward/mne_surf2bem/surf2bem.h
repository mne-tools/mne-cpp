//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surf2bem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Surf2Bem class declaration.
 *
 *           Ported from the original MNE C tool mne_surf2bem by Matti Hamalainen
 *           (SVN $Id: mne_surf2bem.c 3351 2012-03-05 12:03:50Z msh $).
 *
 *           Converts FreeSurfer binary surfaces or ASCII triangle files into
 *           FIFF-format BEM surface files. Supports:
 *             - Multiple surfaces (head, skull, brain) in one BEM file
 *             - Vertex shifting along normals
 *             - FsSurface ordering for correct BEM nesting
 *             - Topology checks (solid angle, containment, thickness)
 *             - FsSurface coordinate size validation
 *
 *           Cross-referenced with MNE-Python's mne.write_bem_surfaces().
 */

#ifndef SURF2BEM_H
#define SURF2BEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem_surface.h>

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
// FORWARD DECLARATIONS
//=============================================================================================================

class MNESurf2BemSettings;
struct SurfaceInput;

//=============================================================================================================
/**
 * Converts FreeSurfer surfaces into BEM FIFF files.
 *
 * This class implements the full workflow of the original mne_surf2bem C tool:
 *   1. Read surfaces (FreeSurfer binary or ASCII triangle format)
 *   2. Assign BEM surface IDs and conductivities
 *   3. Optionally shift vertices along normals
 *   4. Order surfaces for proper BEM nesting (head > skull > brain)
 *   5. Perform topology checks if requested
 *   6. Write BEM FIFF output file
 *
 * @brief Converts FreeSurfer surfaces to BEM FIFF files.
 */
class Surf2Bem
{
public:
    //=========================================================================================================
    /**
     * Constructs the Surf2Bem processor.
     *
     * @param[in] settings  The parsed command-line settings.
     */
    Surf2Bem(const MNESurf2BemSettings& settings);

    //=========================================================================================================
    /**
     * Runs the surface-to-BEM conversion process.
     *
     * @return 0 on success, 1 on failure.
     */
    int run();

private:
    //=========================================================================================================
    /**
     * Reads a FreeSurfer binary surface file and creates an MNEBemSurface.
     *
     * @param[in] input  FsSurface input specification.
     *
     * @return true on success, false on failure. Result stored in the surfs vector.
     */
    bool readFreeSurferSurface(const SurfaceInput& input, MNELIB::MNEBemSurface& bemSurf);

    //=========================================================================================================
    /**
     * Reads an ASCII triangle file and creates an MNEBemSurface.
     *
     * ASCII triangle file format:
     *   Line 1: nvert
     *   Lines 2..nvert+1: x y z (vertex coordinates)
     *   Line nvert+2: ntri
     *   Lines nvert+3..end: v1 v2 v3 (triangle vertex indices, 1-based)
     *
     * @param[in] input  FsSurface input specification.
     *
     * @return true on success, false on failure. Result stored in the surfs vector.
     */
    bool readAsciiTriSurface(const SurfaceInput& input, MNELIB::MNEBemSurface& bemSurf);

    //=========================================================================================================
    /**
     * Shifts surface vertices outward along their normals.
     *
     * @param[in,out] surf   The surface to modify.
     * @param[in]     shift  Shift distance in meters.
     *
     * @return true on success.
     */
    bool shiftVertices(MNELIB::MNEBemSurface& surf, float shift);

    //=========================================================================================================
    /**
     * Orders surfaces for proper BEM nesting: head (4), skull (3), brain (1).
     * Only reorders if all three surface types are present.
     *
     * @param[in,out] surfs  Vector of BEM surfaces to reorder.
     */
    void orderSurfaces(QVector<MNELIB::MNEBemSurface>& surfs);

    //=========================================================================================================

    const MNESurf2BemSettings& m_settings;  /**< Command-line settings. */
};

} // namespace MNESURF2BEM

#endif // SURF2BEM_H
