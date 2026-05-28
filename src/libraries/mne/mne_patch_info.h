//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_patch_info.h
 * @since March 2026
 * @brief Patch information (cluster of cortex vertices around each decimated source) used by orientation priors.
 *
 * @ref MNELIB::MNEPatchInfo stores, for every decimated source vertex,
 * the list of dense-cortex vertices that map to it (the @c patch) and a
 * summary triangle normal used as the patch-averaged surface normal.
 * Required for the loose-orientation MNE prior and for patch-based
 * current-density rendering.
 */

#ifndef MNEPATCHINFO_H
#define MNEPATCHINFO_H

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

class MNESourceSpace;

//=============================================================================================================
/**
 * Implements an MNE Patch Information (Replaces *mnePatchInfo,mnePatchInfoRec; struct of MNE-C mne_types.h).
 *
 * @brief Patch information for a single source space point including vertex members and area.
 */
class MNESHARED_EXPORT MNEPatchInfo
{
public:
    typedef QSharedPointer<MNEPatchInfo> SPtr;              /**< Shared pointer type for MNEPatchInfo. */
    typedef QSharedPointer<const MNEPatchInfo> ConstSPtr;   /**< Const shared pointer type for MNEPatchInfo. */

    //=========================================================================================================
    /**
     * Constructs the MNE Patch Information
     * Refactored: mne_new_patch (mne_source_space.c)
     */
    MNEPatchInfo();

    //=========================================================================================================
    /**
     * Destroys the MNE Patch Information
     * Refactored: mne_free_patch (mne_source_space.c)
     */
    ~MNEPatchInfo();

    //=========================================================================================================
    /**
     * Calculates the total surface area of this patch.
     *
     * Iterates over all member vertices of the patch. For each vertex, sums
     * one-third of the area of every neighboring triangle. The factor of 1/3
     * accounts for each triangle being shared among its three vertices. The
     * result is stored in the member variable area.
     *
     * Refactored from: calculate_patch_area (mne_patches.c).
     *
     * @param[in] s   Pointer to the source space containing the triangle mesh
     *                (neighbor triangles and triangle areas).
     */
    void calculate_area(MNESourceSpace* s);

    //=========================================================================================================
    /**
     * Calculates the average surface normal and its deviation for this patch.
     *
     * Computes the mean normal direction (ave_nn) by summing the normals of all
     * member vertices and normalizing the result to unit length. Then computes
     * the average angular deviation (dev_nn) of each member vertex normal from
     * the mean normal, in radians, using the arccosine of the dot product.
     *
     * @param[in] s   Pointer to the source space containing vertex normals (nn)
     *                for each vertex in the mesh.
     */
    void calculate_normal_stats(MNESourceSpace* s);

public:
    int   vert;                    /**< Source vertex index this patch applies to. */
    Eigen::VectorXi memb_vert;     /**< Vertex indices that constitute the patch. */
    float area;                    /**< Total surface area of the patch (m^2). */
    float ave_nn[3];               /**< Average outward surface normal of the patch. */
    float dev_nn;                  /**< Average angular deviation of member normals from ave_nn (radians). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEPATCHINFO_H
