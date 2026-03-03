//=============================================================================================================
/**
 * @file     mne_morph_map.h
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
 * @brief    MneMorphMap class declaration.
 *
 */

#ifndef MNEMORPHMAP_H
#define MNEMORPHMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "fiff/fiff_sparse_matrix.h"

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
#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *morphMap,morphMapRec struct (analyze_types.c).
 *
 * @brief Vertex-to-vertex mapping between two FreeSurfer surface meshes for morphing source estimates.
 */
class MNESHARED_EXPORT MneMorphMap
{
public:
    typedef QSharedPointer<MneMorphMap> SPtr;              /**< Shared pointer type for MneMorphMap. */
    typedef QSharedPointer<const MneMorphMap> ConstSPtr;   /**< Const shared pointer type for MneMorphMap. */

    //=========================================================================================================
    /**
     * Constructs the MneMorphMap.
     */
    MneMorphMap() = default;

    //=========================================================================================================
    /**
     * Destroys the MneMorphMap.
     */
    ~MneMorphMap() = default;

public:
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> map;  /**< Sparse interpolation matrix: multiply source surface data
                                                          by this to obtain values on the target ('this') surface. */
    Eigen::VectorXi best;                            /**< Index of the closest source surface vertex for each target vertex. */
    int from_kind = -1;                              /**< Surface kind identifier (e.g., hemisphere) of the source surface (-1 = unknown). */
    QString from_subj;                               /**< Subject name of the source surface. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMORPHMAP_H
