//=============================================================================================================
/**
 * @file     mne_surface.h
 * @author   Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//*************************************************************************************************************
//=============================================================================================================
// GENERAL INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_dir_tree.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_types.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNE
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * BEM Surface
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
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket
     * @param[out] surfaces      List to fill with found surfaces
     *
     * @return true if succeeded, false otherwise
     */
    static bool read(QIODevice& p_IODevice, QList<MNESurface::SPtr>& surfaces);

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Implementation of the mne_read_bem_surfaces function
     *
     * Reads a BEM surface from a fif stream
     *
     * @param [in] p_pStream     The open fiff file
     * @param [in] add_geom      Add geometry information to the source spaces
     * @param [in] p_Tree        Search for the surfaces here
     *
     * @param [out] p_Surfaces    The read bem surfaces
     *
     * @return true if succeeded, false otherwise
     */
    static bool read(FIFFLIB::FiffStream::SPtr& p_pStream, bool add_geom,
            FIFFLIB::FiffDirTree& p_Tree, QList<MNESurface::SPtr>& surfaces);

    //=========================================================================================================

    FIFFLIB::fiff_int_t id; /**< Surface number */
    FIFFLIB::fiff_float_t sigma; /**< Conductivity of a compartment */
    FIFFLIB::fiff_int_t np; /**< Number of nodes */
    FIFFLIB::fiff_int_t ntri; /**< Number of triangles */
    FIFFLIB::fiff_int_t coord_frame; /**< The coordinate frame of the mode */
    PointsT rr; /**< Points for each node */
    NormalsT nn; /**< Normals for each node */
    TrianglesT tris; /**< Triangulation information */

private:
    struct BlockType
    {
        enum Enum
        {
            FIFFB_BEM = 310,
            FIFFB_BEM_SURF = 311
        };
    };

    struct Tag
    {
        enum Enum
        {
            FIFF_BEM_COORD_FRAME = 3112,
            FIFF_BEM_SURF_ID = 3101,
            FIFF_BEM_SURF_NAME = 3102,
            FIFF_BEM_SURF_NNODE = 3103,
            FIFF_BEM_SURF_NODES = 3105,
            FIFF_BEM_SURF_NORMALS = 3107,
            FIFF_BEM_SIGMA = 3113
        };
    };

    /**
     * Reads and creates a surface from a given block.
     *
     * @param [in] fiffStream    The open fiff file
     * @param [in] dir           Search for the surface here
     * @param [in] def_coord_frame Default coordinate frame
     *
     * @param [out] surf         The read bem surface
     *
     * @return true if succeeded and surf was filled, false otherwise
     */
    static bool read(FIFFLIB::FiffStream* fiffStream,
            const FIFFLIB::FiffDirTree& dir,
            const FIFFLIB::fiff_int_t def_coord_frame, MNESurface::SPtr& surf);
};

} // NAMESPACE

#endif // MNE_SURFACE_H
