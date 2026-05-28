//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_triangle.h
 * @since 2026
 * @date  March 2026
 * @brief Triangle descriptor with cached centroid, area and normal vectors.
 *
 * @ref MNELIB::MNETriangle is the per-face record produced by
 * @ref MNESourceSpace and @ref MNEBemSurface readers so downstream code
 * (forward solver, projection-to-surface, rendering) does not have to
 * recompute these quantities. Carries the three vertex indices, the
 * centroid, the un-normalised and normalised triangle normals and the
 * area.
 */

#ifndef MNETRIANGLE_H
#define MNETRIANGLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

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
 * @brief Per-triangle geometric data for a cortical or BEM surface.
 *
 * Stores vertex indices, edge vectors, normal, area, centroid and
 * auxiliary unit vectors.  Vertex pointers (`r1`/`r2`/`r3`) and the
 * index pointer (`vert`) alias data owned by the parent surface and
 * must not be freed.
 */
class MNESHARED_EXPORT MNETriangle
{
public:
    typedef QSharedPointer<MNETriangle> SPtr;              /**< Shared pointer type for MNETriangle. */
    typedef QSharedPointer<const MNETriangle> ConstSPtr;   /**< Const shared pointer type for MNETriangle. */

    //=========================================================================================================
    /**
     * Constructs an empty MNETriangle with zeroed geometry.
     */
    MNETriangle();

    //=========================================================================================================
    /**
     * Default destructor (non-owning pointers are not freed).
     */
    ~MNETriangle() = default;

    //=========================================================================================================
    /**
     * Compute derived geometry (edge vectors, normal, area, centroid,
     * auxiliary unit vectors) from the current vertex positions.
     */
    void compute_data();

public:
    int              *vert = nullptr;  /**< Triangle vertex indices (non-owning; points into parent surface itris). */
    Eigen::Vector3f  r1 = Eigen::Vector3f::Zero();   /**< Position of vertex 0. */
    Eigen::Vector3f  r2 = Eigen::Vector3f::Zero();   /**< Position of vertex 1. */
    Eigen::Vector3f  r3 = Eigen::Vector3f::Zero();   /**< Position of vertex 2. */
    Eigen::Vector3f  r12 = Eigen::Vector3f::Zero();   /**< Edge vector from vertex 0 to vertex 1 (r2 - r1). */
    Eigen::Vector3f  r13 = Eigen::Vector3f::Zero();   /**< Edge vector from vertex 0 to vertex 2 (r3 - r1). */
    Eigen::Vector3f  nn  = Eigen::Vector3f::Zero();   /**< Unit normal vector. */
    float            area = 0.0f;      /**< Triangle area. */
    Eigen::Vector3f  cent = Eigen::Vector3f::Zero();  /**< Centroid position. */
    Eigen::Vector3f  ex  = Eigen::Vector3f::Zero();   /**< In-plane unit vector (ey x nn; used by BEM). */
    Eigen::Vector3f  ey  = Eigen::Vector3f::Zero();   /**< In-plane unit vector (normalized r13; used by BEM). */
};

} // NAMESPACE MNELIB

#endif // MNETRIANGLE_H
