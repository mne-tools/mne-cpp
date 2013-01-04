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
* @brief    ToDo Documentation...
*
*/

#ifndef MNE_HEMISPHERE_H
#define MNE_HEMISPHERE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//#include <QGeometryData> //ToDo: This has to be excluded otherwise, always qt3d is needed


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
* DECLARE CLASS SourceSpace
*
* @brief The SourceSpace class provides
*/
class MNESHARED_EXPORT MNEHemisphere {
public:
    //=========================================================================================================
    /**
    * ctor
    */
    MNEHemisphere();

    //=========================================================================================================
    /**
    * Copy ctor
    */
    MNEHemisphere(MNEHemisphere* p_pMNEHemisphere);

    //=========================================================================================================
    /**
    * dtor
    */
    ~MNEHemisphere();

    //=========================================================================================================
    /**
    * Implementation of the mne_transform_source_space_to for a single hemisphere function
    */
    bool transform_hemisphere_to(fiff_int_t dest, FiffCoordTrans* trans);

    //=========================================================================================================
    /**
    * Qt 3d geometry information. Data are generated within first call.
    */
    MatrixXf* getTriCoords(float p_fScaling = 1.0f);

    //=========================================================================================================
    /**
    * Qt 3d geometry information. Data are generated within first call.
    */
    //QGeometryData* getGeometryData(float p_fScaling = 1.0f);

public:
    // MNE Suite
    fiff_int_t id;              /**< ID of ... */
    fiff_int_t np;              /**< ToDo... */
    fiff_int_t ntri;            /**< ToDo... */
    fiff_int_t coord_frame;     /**< ToDo... */
    MatrixX3d rr;               /**< ToDo... */
    MatrixX3d nn;               /**< ToDo... */
    MatrixX3i tris;             /**< ToDo... */
    fiff_int_t nuse;
    VectorXi inuse;             /**< ToDo... */
    VectorXi vertno;            /**< ToDo... */
    qint32 nuse_tri;            /**< ToDo... */
    MatrixX3i use_tris;         /**< ToDo... */
    VectorXi nearest;           /**< ToDo... */
    VectorXd nearest_dist;      /**< ToDo... */
    QList<VectorXi> pinfo;      /**< ToDo... */
    float dist_limit;           /**< ToDo... */
    MatrixXd dist;             /**< ToDo... */
    MatrixX3d tri_cent;         /**< ToDo... */
    MatrixX3d tri_nn;           /**< ToDo... */
    VectorXd tri_area;          /**< ToDo... */
    MatrixX3d use_tri_cent;     /**< ToDo... */
    MatrixX3d use_tri_nn;       /**< ToDo... */
    VectorXd use_tri_area;      /**< ToDo... */
//    dist;
//    dist_limit;


    QList<VectorXi> cluster_vertnos;    /**< Only used within clustered forward solutions */
    QList<VectorXd> cluster_distances;  /**< Distances to clusters centroid. */


private:
    // Newly added
    MatrixXf* m_pTriCoords; /**< Holds the rr tri Matrix transformed to geometry data. */

    //QGeometryData* m_pGeometryData;


};


} // NAMESPACE

#endif // MNE_HEMISPHERE_H
