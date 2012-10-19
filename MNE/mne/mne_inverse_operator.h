//=============================================================================================================
/**
* @file     mne_epoch_data.h
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
* @brief    Contains the MNEInverseOperator class declaration.
*
*/

#ifndef MNE_INVERSE_OPERATOR_H
#define MNE_INVERSE_OPERATOR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_cov.h"
#include "mne_sourcespace.h"

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../fiff/fiff_types.h"
#include "../fiff/fiff_named_matrix.h"
#include "../fiff/fiff_proj.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../3rdParty/Eigen/Core"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
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

using namespace FIFFLIB;
using namespace Eigen;


//=============================================================================================================
/**
* MNE epoch data, which corresponds to an event
*
* @brief epoch data
*/
class MNESHARED_EXPORT MNEInverseOperator
{

public:
    //=========================================================================================================
    /**
    * ctor
    */
    MNEInverseOperator();

    //=========================================================================================================
    /**
    * Copy ctor
    */
    MNEInverseOperator(const MNEInverseOperator* p_pMNEInverseOperator);

    //=========================================================================================================
    /**
    * Destroys the MNEInverseOperator.
    */
    ~MNEInverseOperator();

public:
    fiff_int_t methods;
    fiff_int_t source_ori;
    fiff_int_t nsource;
    fiff_int_t nchan;
    fiff_int_t coord_frame;
    MatrixXf*  source_nn;
    VectorXf*  sing;
    bool    eigen_leads_weighted;
    FiffNamedMatrix* eigen_leads;
    FiffNamedMatrix* eigen_fields;
    MNECov* noise_cov;
    MNECov* source_cov;
    MNECov* orient_prior;
    MNECov* depth_prior;
    MNECov* fmri_prior;
    MNESourceSpace* src;
    FiffCoordTrans* mri_head_t;
    fiff_int_t nave;
    QList<FiffProj*> projs;
    MatrixXf* proj;                     /**< This is the projector to apply to the data. */
    MatrixXf* whitener;                 /**< This whitens the data */
    VectorXf* reginv;                   /**< This the diagonal matrix implementing. regularization and the inverse */
    SparseMatrix<float>* noisenorm;     /**< These are the noise-normalization factors */
};

} // NAMESPACE

#endif // MNE_INVERSE_OPERATOR_H
