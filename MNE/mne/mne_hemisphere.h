//=============================================================================================================
/**
* @file     mne_hemisphere.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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

#include "../fiff/fiff_types.h"
#include "../fiff/fiff.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../3rdParty/Eigen/Core"


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
    fiff_int_t id;              /**< The ID of ... */
    fiff_int_t np;              /**< The... */
    fiff_int_t ntri;            /**< The... */
    fiff_int_t coord_frame;     /**< The... */
    MatrixX3f rr;               /**< The... */
    MatrixX3f nn;               /**< The... */
    MatrixX3i tris;             /**< The... */
    fiff_int_t nuse;
    VectorXi inuse;             /**< The... */
    VectorXi vertno;            /**< The... */
    qint32 nuse_tri;            /**< The... */
    MatrixX3i use_tris;         /**< The... */
    VectorXi nearest;           /**< The... */
    VectorXf nearest_dist;      /**< The... */
    QList<VectorXi> pinfo;      /**< The... */
    float dist_limit;           /**< The... */
    MatrixXf* dist;             /**< The... */
    MatrixX3f tri_cent;         /**< The... */
    MatrixX3f tri_nn;           /**< The... */
    VectorXf tri_area;          /**< The... */
    MatrixX3f use_tri_cent;     /**< The... */
    MatrixX3f use_tri_nn;       /**< The... */
    VectorXf use_tri_area;      /**< The... */
//    dist;
//    dist_limit;

private:
    // Newly added
    MatrixXf* m_pTriCoords; /**< Holds the rr tri Matrix transformed to geometry data. */

    //QGeometryData* m_pGeometryData;


};


} // NAMESPACE

#endif // MNE_HEMISPHERE_H
