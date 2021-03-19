//=============================================================================================================
/**
 * @file     mne_surface.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the MNESurface class.
 *
 */

#ifndef MNE_SURFACE_H
#define MNE_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QList>
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
 * BEM Surface
 * TODO: Get rid of this? Not used anymore. Maybe by the Leipzig proct code?
 *
 * @brief BEM Surface
 */
class MNESHARED_EXPORT MNESurface
{
public:
    typedef QSharedPointer<MNESurface>SPtr; /**< Shared pointer type for MNESurface. */
    typedef QSharedPointer<const MNESurface>ConstSPtr; /**< Const shared pointer type for MNESurface. */

    typedef Eigen::Matrix3Xf PointsT; /**< Type abbreviation for points. */
    typedef Eigen::Matrix3Xf NormalsT; /**< Type abbreviation for normals. */
    typedef Eigen::Matrix3Xi TrianglesT; /**< Type abbreviation for triangles. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNESurface();

    /**
     * Default destructor
     */
    ~MNESurface()
    {
    }

    //=========================================================================================================
    /**
     * Reads a bem surface from a fif IO device
     *
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] surfaces      List to fill with found surfaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read(QIODevice& p_IODevice, QList<MNESurface::SPtr>& surfaces);

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_read_bem_surfaces function
     *
     * Reads a BEM surface from a fif stream
     *
     * @param[in] p_pStream     The open fiff file.
     * @param[in] add_geom      Add geometry information to the source spaces.
     * @param[in] p_Tree        Search for the surfaces here.
     *
     * @param[in, out] p_Surfaces    The read bem surfaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read(FIFFLIB::FiffStream::SPtr& p_pStream, bool add_geom, const FIFFLIB::FiffDirNode::SPtr& p_Tree, QList<MNESurface::SPtr>& surfaces);

    //=========================================================================================================

    FIFFLIB::fiff_int_t id; /**< Surface number. */
    FIFFLIB::fiff_float_t sigma; /**< Conductivity of a compartment. */
    FIFFLIB::fiff_int_t np; /**< Number of nodes. */
    FIFFLIB::fiff_int_t ntri; /**< Number of triangles. */
    FIFFLIB::fiff_int_t coord_frame; /**< The coordinate frame of the mode. */
    PointsT rr; /**< Points for each node. */
    NormalsT nn; /**< Normals for each node. */
    TrianglesT tris; /**< Triangulation information. */

private:
    /**
     * Reads and creates a surface from a given block.
     *
     * @param[in] fiffStream    The open fiff file.
     * @param[in] dir           Search for the surface here.
     * @param[in] def_coord_frame Default coordinate frame.
     *
     * @param[in, out] surf         The read bem surface.
     *
     * @return true if succeeded and surf was filled, false otherwise.
     */
    static bool read(FIFFLIB::FiffStream::SPtr& fiffStream,
            const FIFFLIB::FiffDirNode::SPtr& dir,
            const FIFFLIB::fiff_int_t def_coord_frame, MNESurface::SPtr& surf);
};
} // NAMESPACE

#endif // MNE_SURFACE_H
