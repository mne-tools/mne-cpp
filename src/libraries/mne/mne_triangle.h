//=============================================================================================================
/**
 * @file     mne_triangle.h
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
 * @brief    MneTriangle class declaration.
 *
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
class MNESHARED_EXPORT MneTriangle
{
public:
    typedef QSharedPointer<MneTriangle> SPtr;              /**< Shared pointer type for MneTriangle. */
    typedef QSharedPointer<const MneTriangle> ConstSPtr;   /**< Const shared pointer type for MneTriangle. */

    //=========================================================================================================
    /**
     * Constructs an empty MneTriangle with zeroed geometry.
     */
    MneTriangle();

    //=========================================================================================================
    /**
     * Default destructor (non-owning pointers are not freed).
     */
    ~MneTriangle() = default;

    //=========================================================================================================
    /**
     * Compute derived geometry (edge vectors, normal, area, centroid,
     * auxiliary unit vectors) from the current vertex positions.
     */
    void compute_data();

public:
    int              *vert = nullptr;  /**< Triangle vertex indices (non-owning; points into parent surface itris). */
    const float      *r1 = nullptr;    /**< Position of vertex 0 (non-owning; points into parent surface rr). */
    const float      *r2 = nullptr;    /**< Position of vertex 1 (non-owning; points into parent surface rr). */
    const float      *r3 = nullptr;    /**< Position of vertex 2 (non-owning; points into parent surface rr). */
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
