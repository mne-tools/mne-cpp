//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   sheinke <simon.j.w.heinke@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_bem_surface.h
 * @since June 2015
 * @brief Single closed BEM surface (triangulation, normals, conductivity).
 *
 * @ref MNELIB::MNEBemSurface stores one of the head-model boundary
 * surfaces of an @ref MNEBem: vertices, oriented triangles, vertex
 * normals, surface id (inner skull / outer skull / scalp), coordinate
 * frame and conductivity. FIFF tags: @c FIFFB_BEM_SURF,
 * @c FIFF_BEM_SURF_ID, @c FIFF_BEM_SURF_NTRI, @c FIFF_BEM_SURF_TRIANGLES,
 * @c FIFF_BEM_SURF_NODES, @c FIFF_BEM_SURF_NORMALS.
 */

#ifndef MNE_BEM_SURFACE_H
#define MNE_BEM_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_surface.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * BEM surface geometry information.
 *
 * Inherits core geometry (vertices, normals, triangles, neighbors)
 * from MNESurface/MNESurfaceOrVolume. Adds BEM-specific computed
 * triangle metadata (centers, normals, areas) and I/O.
 *
 * @brief BEM surface provides geometry information
 */
class MNESHARED_EXPORT MNEBemSurface : public MNESurface
{
public:
    typedef QSharedPointer<MNEBemSurface> SPtr;             /**< Shared pointer type for MNEBemSurface. */
    typedef QSharedPointer<const MNEBemSurface> ConstSPtr;  /**< Const shared pointer type for MNEBemSurface. */

    //=========================================================================================================
    /**
     * Constructors the bem surface.
     */
    MNEBemSurface();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNEBemSurface    BEM surface which should be copied.
     */
    MNEBemSurface(const MNEBemSurface& p_MNEBemSurface);

    //=========================================================================================================
    /**
     * Destroys the BEM surface.
     */
    ~MNEBemSurface();

    //=========================================================================================================
    /**
     * Initializes the BEM surface.
     */
    void clear();

    //=========================================================================================================
    /**
     * Definition of the   mne_add_triangle_data function in mne_add_geometry_info.c
     *
     * Completes triangulation info
     *
     * @return true if succeeded, false otherwise.
     */
    bool addTriangleData();

    //=========================================================================================================
    /**
     * Add vertex normals and neighbourhood information
     *
     * @return true if succeeded, false otherwise.
     */
    bool add_geometry_info();

    //=========================================================================================================
    /**
     * Definition of the addVertexNormals function in mne_add_geometry_info.c
     *
     * Completes triangulation info
     *
     * @return true if succeeded, false otherwise.
     */
    bool addVertexNormals();

    //=========================================================================================================
    /**
     * Writes the bem surface to a FIFF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Map bem id integers to human-readable names
     *
     * @param[in] frame  The bem id integer.
     *
     * @return Human readable form of the bem id.
     */
    static QString id_name(int id);

    //=========================================================================================================
    /**
     * Decimate the outer skin surface to the target vertex counts
     * using iterative edge collapse simplification.
     *
     * @param[in] outerSkin       The outer skin BEM surface to decimate.
     * @param[in] targetVertices  List of target vertex counts (default: {2562, 10242, 40962}).
     *
     * @return A list of decimated BEM surfaces, one per target resolution.
     *
     * @since 2.2.0
     */
    static QList<MNEBemSurface> makeScalpSurfaces(
        const MNEBemSurface& outerSkin,
        const QList<int>& targetVertices = {2562, 10242, 40962});

public:
    Eigen::MatrixX3d tri_cent;         /**< Triangle centers. */
    Eigen::MatrixX3d tri_nn;           /**< Triangle normals. */
    Eigen::VectorXd tri_area;          /**< Triangle areas. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // MNE_BEMSURFACE_H
