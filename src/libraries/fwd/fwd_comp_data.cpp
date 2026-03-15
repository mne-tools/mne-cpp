//=============================================================================================================
/**
 * @file     fwd_comp_data.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdCompData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_comp_data.h"

#include <mne/mne_ctf_comp_data_set.h>
#include <fiff/fiff_types.h>

namespace {
constexpr int FAIL = -1;
constexpr int OK   =  0;
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCompData::FwdCompData()
:comp_coils (nullptr)
,field      (nullptr)
,vec_field  (nullptr)
,field_grad (nullptr)
,client     (nullptr)
,set        (nullptr)
{
}

//=============================================================================================================

FwdCompData::~FwdCompData()
{
    if(this->comp_coils)
        delete this->comp_coils;
    if(this->set)
        delete this->set;
}

//=============================================================================================================

int FwdCompData::fwd_comp_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet &coils, Eigen::Ref<Eigen::VectorXf> res, void *client)
{
    FwdCompData* comp = static_cast<FwdCompData*>(client);

    if (!comp->field) {
        qWarning("Field computation function is missing in fwd_comp_field");
        return FAIL;
    }
    /*
       * First compute the field in the primary set of coils
       */
    if (comp->field(rd,Q,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
       * Compensation needed?
       */
    if (!comp->comp_coils || comp->comp_coils->ncoil() <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
       * Workspace needed?
       */
    if (comp->work.size() == 0)
        comp->work.resize(comp->comp_coils->ncoil());
    /*
       * Compute the field in the compensation coils
       */
    if (comp->field(rd,Q,*comp->comp_coils,comp->work,comp->client) == FAIL)
        return FAIL;
    /*
       * Compute the compensated field
       */
    return comp->set->apply(true, res, comp->work);
}

//=============================================================================================================

int FwdCompData::fwd_make_ctf_comp_coils(MNECTFCompDataSet *set,
                                         FwdCoilSet *coils,
                                         FwdCoilSet *comp_coils)
{
    QList<FiffChInfo> chs;
    QList<FiffChInfo> compchs;
    int        nchan   = 0;
    int        ncomp   = 0;
    int k,res;

    if (!set)
        return OK;
    if (!coils || coils->ncoil() <= 0) {
        qWarning("Coil data missing in fwd_make_ctf_comp_coils");
        return FAIL;
    }

    for (k = 0; k < coils->ncoil(); k++) {
        chs.append(FiffChInfo());
        FwdCoil* coil = coils->coils[k].get();
        chs[k].ch_name = coil->chname;
        chs[k].chpos.coil_type = coil->type;
        chs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
    }
    nchan = coils->ncoil();
    if (comp_coils && comp_coils->ncoil() > 0) {
        for (k = 0; k < comp_coils->ncoil(); k++) {
            compchs.append(FiffChInfo());
            FwdCoil* coil = comp_coils->coils[k].get();
            compchs[k].ch_name = coil->chname;
            compchs[k].chpos.coil_type = coil->type;
            compchs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        }
        ncomp = comp_coils->ncoil();
    }
    res = set->make_comp(chs,nchan,compchs,ncomp);

    return res;
}

//=============================================================================================================

FwdCompData *FwdCompData::fwd_make_comp_data(MNECTFCompDataSet *set,
                                             FwdCoilSet *coils,
                                             FwdCoilSet *comp_coils,
                                             fwdFieldFunc field,
                                             fwdVecFieldFunc vec_field,
                                             fwdFieldGradFunc field_grad,
                                             void *client)
{
    FwdCompData* comp = new FwdCompData();

    if(set)
        comp->set = new MNECTFCompDataSet(*set);
    else
        comp->set = nullptr;

    if (comp_coils) {
        comp->comp_coils = comp_coils->dup_coil_set().release();
    }
    else {
        qWarning("No coils to duplicate");
        comp->comp_coils = nullptr;
    }
    comp->field       = field;
    comp->vec_field   = vec_field;
    comp->field_grad  = field_grad;
    comp->client      = client;

    if (fwd_make_ctf_comp_coils(comp->set,
                                coils,
                                comp->comp_coils) != OK) {
        delete comp;
        return nullptr;
    }
    else {
        return comp;
    }
}

//=============================================================================================================

int FwdCompData::fwd_comp_field_vec(const Eigen::Vector3f& rd, FwdCoilSet &coils, Eigen::Ref<Eigen::MatrixXf> res, void *client)
{
    FwdCompData* comp = static_cast<FwdCompData*>(client);

    if (!comp->vec_field) {
        qWarning("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
       * First compute the field in the primary set of coils
       */
    if (comp->vec_field(rd,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
       * Compensation needed?
       */
    if (!comp->comp_coils || comp->comp_coils->ncoil() <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
       * Need workspace?
       */
    if (comp->vec_work.size() == 0)
        comp->vec_work.resize(3, comp->comp_coils->ncoil());
    /*
       * Compute the field at the compensation sensors
       */
    if (comp->vec_field(rd,*comp->comp_coils,comp->vec_work,comp->client) == FAIL)
        return FAIL;
    /*
       * Compute the compensated field of three orthogonal dipoles
       */
    for (int k = 0; k < 3; k++) {
        Eigen::VectorXf resRow = res.row(k).transpose();
        Eigen::VectorXf workRow = comp->vec_work.row(k).transpose();
        if (comp->set->apply(true, resRow, workRow) == FAIL)
            return FAIL;
        res.row(k) = resRow.transpose();
    }
    return OK;
}

//=============================================================================================================

int FwdCompData::fwd_comp_field_grad(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::Ref<Eigen::VectorXf> res, Eigen::Ref<Eigen::VectorXf> xgrad, Eigen::Ref<Eigen::VectorXf> ygrad, Eigen::Ref<Eigen::VectorXf> zgrad, void *client)
{
    FwdCompData* comp = static_cast<FwdCompData*>(client);

    if (!comp->field_grad) {
        qCritical("Field and gradient computation function is missing in fwd_comp_field_grad");
        return FAIL;
    }
    /*
     * First compute the field in the primary set of coils
     */
    if (comp->field_grad(rd,Q,coils,res,xgrad,ygrad,zgrad,comp->client) == FAIL)
        return FAIL;
    /*
     * Compensation needed?
     */
    if (!comp->comp_coils || comp->comp_coils->ncoil() <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
     * Workspace needed?
     */
    if (comp->work.size() == 0)
        comp->work.resize(comp->comp_coils->ncoil());
    if (comp->vec_work.size() == 0)
        comp->vec_work.resize(3, comp->comp_coils->ncoil());
    /*
     * Compute the field in the compensation coils
     */
    Eigen::VectorXf vw0 = comp->vec_work.row(0).transpose();
    Eigen::VectorXf vw1 = comp->vec_work.row(1).transpose();
    Eigen::VectorXf vw2 = comp->vec_work.row(2).transpose();
    if (comp->field_grad(rd,Q,*comp->comp_coils,comp->work,vw0,vw1,vw2,comp->client) == FAIL)
        return FAIL;
    comp->vec_work.row(0) = vw0.transpose();
    comp->vec_work.row(1) = vw1.transpose();
    comp->vec_work.row(2) = vw2.transpose();
    /*
     * Compute the compensated field
     */
    if (comp->set->apply(true, res, comp->work) != OK)
        return FAIL;

    vw0 = comp->vec_work.row(0).transpose();
    if (comp->set->apply(true, xgrad, vw0) != OK)
        return FAIL;

    vw1 = comp->vec_work.row(1).transpose();
    if (comp->set->apply(true, ygrad, vw1) != OK)
        return FAIL;

    vw2 = comp->vec_work.row(2).transpose();
    if (comp->set->apply(true, zgrad, vw2) != OK)
        return FAIL;

    return OK;
}
