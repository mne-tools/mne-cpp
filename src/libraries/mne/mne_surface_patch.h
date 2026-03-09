//=============================================================================================================
/**
 * @file     mne_surface_patch.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MNESurfacePatch class declaration.
 *
 */

#ifndef MNESURFACEPATCH_H
#define MNESURFACEPATCH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNESourceSpace;

//=============================================================================================================
/**
 * @brief Cortical surface patch linking a set of vertices to a single source space point.
 *
 * Each patch represents a contiguous region of a FreeSurfer cortical surface.
 * It stores the mapping between patch-local and full-surface vertex/triangle
 * indices and marks which vertices lie on the patch border.
 */
class MNESHARED_EXPORT MNESurfacePatch
{
public:
    typedef QSharedPointer<MNESurfacePatch> SPtr;              /**< Shared pointer type for MNESurfacePatch. */
    typedef QSharedPointer<const MNESurfacePatch> ConstSPtr;   /**< Const shared pointer type for MNESurfacePatch. */

    //=========================================================================================================
    /**
     * Constructs a patch with @p np vertices.
     *
     * @param[in] np  Number of vertices in the patch (0 for an empty patch).
     */
    MNESurfacePatch(int np);

    //=========================================================================================================
    /**
     * Destroys the MNESurfacePatch.
     */
    ~MNESurfacePatch();

public:
    std::unique_ptr<MNESourceSpace> s;		    /**< Patch represented as a source space surface. */
    Eigen::VectorXi  vert;	    /**< Vertex numbers in the complete surface (size np). */
    Eigen::VectorXi  surf_vert;	    /**< Map from complete-surface vertex index to patch vertex index (-1 if absent). */
    Eigen::VectorXi  tri;	    /**< Map from patch triangle index to complete-surface triangle index. */
    Eigen::VectorXi  surf_tri;	    /**< Map from complete-surface triangle index to patch triangle index (-1 if absent). */
    Eigen::VectorXi  border;	    /**< Per-vertex flag: non-zero if the vertex lies on the patch border (size np). */
    int              flat = 0;	    /**< Non-zero if the patch has been flattened. */
};

} // NAMESPACE MNELIB

#endif // MNESURFACEPATCH_H
