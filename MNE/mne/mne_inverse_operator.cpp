//=============================================================================================================
/**
* @file     mne_inverse_operator.cpp
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
* @brief    Contains the implementation of the MNEInverseOperator Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_inverse_operator.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEInverseOperator::MNEInverseOperator()
: methods(-1)
, source_ori(-1)
, nsource(-1)
, nchan(-1)
, coord_frame(-1)
, source_nn(NULL)
, sing(NULL)
, eigen_leads_weighted(false)
, eigen_leads(NULL)
, eigen_fields(NULL)
, noise_cov(NULL)
, source_cov(NULL)
, orient_prior(NULL)
, depth_prior(NULL)
, fmri_prior(NULL)
, src(NULL)
, mri_head_t(NULL)
, nave(-1)
, proj(NULL)
, whitener(NULL)
, reginv(NULL)
, noisenorm(NULL)
{

}


//*************************************************************************************************************

MNEInverseOperator::MNEInverseOperator(const MNEInverseOperator* p_pMNEInverseOperator)
: methods(p_pMNEInverseOperator->methods)
, source_ori(p_pMNEInverseOperator->source_ori)
, nsource(p_pMNEInverseOperator->nsource)
, nchan(p_pMNEInverseOperator->nchan)
, coord_frame(p_pMNEInverseOperator->coord_frame)
, source_nn(p_pMNEInverseOperator->source_nn ? new MatrixXf(*p_pMNEInverseOperator->source_nn) : NULL)
, sing(p_pMNEInverseOperator->sing ? new VectorXf(*p_pMNEInverseOperator->sing) : NULL)
, eigen_leads_weighted(p_pMNEInverseOperator->eigen_leads_weighted)
, eigen_leads(p_pMNEInverseOperator->eigen_leads ? new FiffNamedMatrix(p_pMNEInverseOperator->eigen_leads) : NULL)
, eigen_fields(p_pMNEInverseOperator->eigen_fields ? new FiffNamedMatrix(p_pMNEInverseOperator->eigen_fields) : NULL)
, noise_cov(p_pMNEInverseOperator->noise_cov ? new MNECov(p_pMNEInverseOperator->noise_cov) : NULL)
, source_cov(p_pMNEInverseOperator->source_cov ? new MNECov(p_pMNEInverseOperator->source_cov) : NULL)
, orient_prior(p_pMNEInverseOperator->orient_prior ? new MNECov(p_pMNEInverseOperator->orient_prior) : NULL)
, depth_prior(p_pMNEInverseOperator->depth_prior ? new MNECov(p_pMNEInverseOperator->depth_prior) : NULL)
, fmri_prior(p_pMNEInverseOperator->fmri_prior ? new MNECov(p_pMNEInverseOperator->fmri_prior) : NULL)
, src(p_pMNEInverseOperator->src ? new MNESourceSpace(p_pMNEInverseOperator->src) : NULL)
, mri_head_t(p_pMNEInverseOperator->mri_head_t ? new FiffCoordTrans(p_pMNEInverseOperator->mri_head_t) : NULL)
, nave(p_pMNEInverseOperator->nave)
, proj(p_pMNEInverseOperator->proj ? new MatrixXf(*p_pMNEInverseOperator->proj) : NULL)
, whitener(p_pMNEInverseOperator->whitener ? new MatrixXf(*p_pMNEInverseOperator->whitener) : NULL)
, reginv(p_pMNEInverseOperator->reginv ? new VectorXf(*p_pMNEInverseOperator->reginv) : NULL)
, noisenorm(p_pMNEInverseOperator->noisenorm ? new SparseMatrix<float>(*p_pMNEInverseOperator->noisenorm) : NULL)
{

}


//*************************************************************************************************************

MNEInverseOperator::~MNEInverseOperator()
{
    if(source_nn)
        delete source_nn;
    if(sing)
        delete sing;
    if(eigen_leads)
        delete eigen_leads;
    if(eigen_fields)
        delete eigen_fields;
    if(noise_cov)
        delete noise_cov;
    if(source_cov)
        delete source_cov;
    if(orient_prior)
        delete orient_prior;
    if(depth_prior)
        delete depth_prior;
    if(fmri_prior)
        delete fmri_prior;
    if(src)
        delete src;
    if(mri_head_t)
        delete mri_head_t;
    if(proj)
        delete proj;
    if(whitener)
        delete whitener;
    if(reginv)
        delete reginv;
    if(noisenorm)
        delete noisenorm;
}
