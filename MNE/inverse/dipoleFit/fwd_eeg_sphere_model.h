//=============================================================================================================
/**
* @file     fwd_eeg_sphere_model.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    FwdEegSphereModel class declaration.
*
*/

#ifndef FWDEEGSPHEREMODEL_H
#define FWDEEGSPHEREMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "fwd_eeg_sphere_layer.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{


//=============================================================================================================
/**
* Implements FwdEegSphereModel (Replaces *fwdEegSphereModel,fwdEegSphereModelRec struct of MNE-C fwd_types.h).
*
* @brief Electric Current Dipole description
*/
class INVERSESHARED_EXPORT FwdEegSphereModel
{
public:
    typedef QSharedPointer<FwdEegSphereModel> SPtr;              /**< Shared pointer type for FwdEegSphereModel. */
    typedef QSharedPointer<const FwdEegSphereModel> ConstSPtr;   /**< Const shared pointer type for FwdEegSphereModel. */

    //=========================================================================================================
    /**
    * Constructs the Forward EEG Sphere Model
    */
    FwdEegSphereModel();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FwdEegSphereModel      Forward EEG Sphere Model which should be copied
    */
    FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel);

    //=========================================================================================================
    /**
    * Destroys the Electric Current Dipole description
    */
    ~FwdEegSphereModel();

    //=========================================================================================================
    /**
    * Returns the number of layers
    *
    * @return the number of layers
    */
    int nlayer() { return layers.size(); }

//    // fwd_multi_spherepot.c
//    /*
//    * Get the model depended weighting factor for n
//    */
//    double fwd_eeg_get_multi_sphere_model_coeff(int n);


public:
    QString name;                       /**< Textual identifier */
    QList<FwdEegSphereLayer> layers;    /**< A list of layers */
    Eigen::Vector3f  r0;                /**< The origin */

    Eigen::VectorXd fn;                 /**< Coefficients saved to speed up the computations */
    int    nterms;                      /**< How many? */

    Eigen::VectorXf mu;                 /**< The Berg-Scherg equivalence parameters */
    Eigen::VectorXf lambda;
    int    nfit;                        /**< How many? */
    int    scale_pos;                   /**< Scale the positions to the surface of the sphere? */




// ### OLD STRUCT ###
//    typedef struct {
//      char  *name;            /* Textual identifier */
//      int   nlayer;			/* Number of layers */
//      fwdEegSphereLayer layers;	/* An array of layers */
//      float  r0[3];			/* The origin */

//      double *fn;		        /* Coefficients saved to speed up the computations */
//      int    nterms;		/* How many? */

//      float  *mu;			/* The Berg-Scherg equivalence parameters */
//      float  *lambda;
//      int    nfit;			/* How many? */
//      int    scale_pos;		/* Scale the positions to the surface of the sphere? */
//    } *fwdEegSphereModel,fwdEegSphereModelRec;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE INVERSELIB

#endif // FWDEEGSPHEREMODEL_H
