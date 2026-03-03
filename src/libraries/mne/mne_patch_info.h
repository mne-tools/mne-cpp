//=============================================================================================================
/**
 * @file     mne_patch_info.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Patch Information (MnePatchInfo) class declaration.
 *
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

class MneSourceSpaceOld;

//=============================================================================================================
/**
 * Implements an MNE Patch Information (Replaces *mnePatchInfo,mnePatchInfoRec; struct of MNE-C mne_types.h).
 *
 * @brief Patch information for a single source space point including vertex members and area.
 */
class MNESHARED_EXPORT MnePatchInfo
{
public:
    typedef QSharedPointer<MnePatchInfo> SPtr;              /**< Shared pointer type for MnePatchInfo. */
    typedef QSharedPointer<const MnePatchInfo> ConstSPtr;   /**< Const shared pointer type for MnePatchInfo. */

    //=========================================================================================================
    /**
     * Constructs the MNE Patch Information
     * Refactored: mne_new_patch (mne_source_space.c)
     */
    MnePatchInfo();

    //=========================================================================================================
    /**
     * Destroys the MNE Patch Information
     * Refactored: mne_free_patch (mne_source_space.c)
     */
    ~MnePatchInfo();

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
    void calculate_area(MneSourceSpaceOld* s);

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
    void calculate_normal_stats(MneSourceSpaceOld* s);

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
