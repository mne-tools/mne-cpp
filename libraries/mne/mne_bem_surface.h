//=============================================================================================================
/**
 * @file     mne_bem_surface.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     MNEBemSurface class declaration.
 *
 */

#ifndef MNE_BEM_SURFACE_H
#define MNE_BEM_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

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
 * BEM surface geometry information
 *
 * @brief BEM surface provides geometry information
 */
class MNESHARED_EXPORT MNEBemSurface
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

public:
    FIFFLIB::fiff_int_t id;            /**< Id information. */
    FIFFLIB::fiff_int_t np;            /**< Number of vertices of the whole/original surface used to create the source locations. */
    FIFFLIB::fiff_int_t ntri;          /**< Number of available triangles. */
    FIFFLIB::fiff_int_t coord_frame;   /**< Coil coordinate system definition. */
    FIFFLIB::fiff_float_t sigma;       /**< Conductivity of a compartment. */
    Eigen::MatrixX3f rr;               /**< Source locations of available dipoles. */
    Eigen::MatrixX3f nn;               /**< Source normals of available dipoles. */
    Eigen::MatrixX3i tris;             /**< Triangles. */
    Eigen::MatrixX3d tri_cent;         /**< Triangle centers. */
    Eigen::MatrixX3d tri_nn;           /**< Triangle normals. */
    Eigen::VectorXd tri_area;          /**< Triangle areas. */
    QVector<QVector<int> > neighbor_tri;           /**< Vector of neighboring triangles for each vertex. */
    QVector<QVector<int> > neighbor_vert;          /**< Vector of neighboring vertices for each vertex. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // MNE_BEMSURFACE_H
