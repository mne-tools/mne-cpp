//=============================================================================================================
/**
* @file     mne_hemisphere.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     MNEHemisphere class declaration.
*
*/

#ifndef MNE_HEMISPHERE_H
#define MNE_HEMISPHERE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_cluster_info.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Hemisphere source space geometry information
*
* @brief Hemisphere provides geometry information
*/
class MNESHARED_EXPORT MNEHemisphere
{
public:
    typedef QSharedPointer<MNEHemisphere> SPtr;             /**< Shared pointer type for MNEHemisphere. */
    typedef QSharedPointer<const MNEHemisphere> ConstSPtr;  /**< Const shared pointer type for MNEHemisphere. */

    //=========================================================================================================
    /**
    * Constructors the hemisphere source space.
    */
    MNEHemisphere();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MNEHemisphere    Hemisphere source space which should be copied
    */
    MNEHemisphere(const MNEHemisphere& p_MNEHemisphere);

    //=========================================================================================================
    /**
    * Destroys the hemisphere source space.
    */
    ~MNEHemisphere();

    //=========================================================================================================
    /**
    * Initializes the hemisphere source space.
    */
    void clear();

    //=========================================================================================================
    /**
    * Qt 3d geometry information. Data are generated within first call.
    *
    * @param[in] p_fScaling  Scale factor of the returned geometry tri model.
    *
    * @return the geometry model
    */
    MatrixXf& getTriCoords(float p_fScaling = 1.0f);

    //=========================================================================================================
    /**
    * is hemisphere clustered?
    *
    * @return true if hemisphere is clustered, false otherwise.
    */
    inline bool isClustered() const;

    //=========================================================================================================
    /**
    * mne_transform_source_space_to
    *
    * ### MNE toolbox root function ###
    *
    * Implementation of the mne_transform_source_space_to for a single hemisphere function
    * Transform source space data to the desired coordinate system.
    *
    * @param[in] dest       The id of the destination coordinate system (FIFFV_COORD_...)
    * @param[in] p_Trans    The coordinate transformation structure to use
    *
    * @return true if succeeded, false otherwise
    */
    bool transform_hemisphere_to(fiff_int_t dest, const FiffCoordTrans &p_Trans);

    //=========================================================================================================
    /**
    * mne_python _write_one_source_space
    *
    * ### MNE toolbox root function ###
    *
    * Write the hemisphere to a FIF stream
    *
    * @param[in] p_pStream  The stream to write to.
    */
    void writeToStream(FiffStream* p_pStream);

    //ToDo write(IODevice &)

    //=========================================================================================================
    /**
    * Qt 3d geometry information. Data are generated within first call.
    */
    //QGeometryData* getGeometryData(float p_fScaling = 1.0f);

public:
    fiff_int_t type;            /**< Type of the source space: 1 = "surf" or 2 = "vol". ToDo not used jet. */
    fiff_int_t id;              /**< Id information */
    fiff_int_t np;              /**< Number of vertices of the whole/original surface used to create the source locations. */
    fiff_int_t ntri;            /**< Number of available triangles */
    fiff_int_t coord_frame;     /**< Coil coordinate system definition */
    MatrixX3f rr;               /**< Source locations of available dipoles. */
    MatrixX3f nn;               /**< Source normals of available dipoles. */
    MatrixX3i tris;             /**< Triangles */
    fiff_int_t nuse;            /**< Number of used dipoles. */
    VectorXi inuse;             /**< Used source points indicated by 1, 0 otherwise */
    VectorXi vertno;            /**< Zero based (different to MATLAB) indices of the used vertices/If label based clustered gain matrix vertno contains label IDs*/
    qint32 nuse_tri;            /**< Number of used triangles. */
    MatrixX3i use_tris;         /**< Triangle information of the used triangles. */
    VectorXi nearest;           /**< All indeces mapped to the indeces of the used vertices (using option -cps during mne_setup_source_space) */
    VectorXd nearest_dist;      /**< Distance to the nearest vertices (using option -cps during mne_setup_source_space). */
    QList<VectorXi> pinfo;      /**< Patch information (using option -cps during mne_setup_source_space) */
    VectorXi patch_inds;        /**< List of neighboring vertices in the high resolution triangulation. */
    float dist_limit;           /**< ToDo... (using option -cps during mne_setup_source_space) */
    SparseMatrix<double> dist;  /**< ToDo... (using option -cps during mne_setup_source_space) */
    MatrixX3d tri_cent;         /**< Triangle centers */
    MatrixX3d tri_nn;           /**< Triangle normals */
    VectorXd tri_area;          /**< Triangle areas */
    MatrixX3d use_tri_cent;     /**< Triangle centers of used triangles */
    MatrixX3d use_tri_nn;       /**< Triangle normals of used triangles */
    VectorXd use_tri_area;      /**< Triangle areas of used triangles */

    QMap<int, QVector<int>> neighbor_tri;       /**< Map of neighboring triangles for each vertex */
    QMap<int, QVector<int>> neighbor_vert;      /**< Map of neighboring vertices for each vertex */

    MNEClusterInfo cluster_info; /**< Holds the cluster information. */
private:
    // Newly added
    MatrixXf m_TriCoords; /**< Holds the rr tri Matrix transformed to geometry data. */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEHemisphere::isClustered() const
{
    return !cluster_info.isEmpty();
}

} // NAMESPACE

#endif // MNE_HEMISPHERE_H
