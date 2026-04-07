//=============================================================================================================
/**
 * @file     mne_ctf_comp_data.cpp
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
 * @brief    Definition of the MNECTFCompData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

#include <QFile>

#include <Eigen/Core>

constexpr int FAIL = -1;
constexpr int OK   =  0;

#define MNE_CTFV_COMP_UNKNOWN -1
#define MNE_CTFV_COMP_NONE    0
#define MNE_CTFV_COMP_G1BR    0x47314252
#define MNE_CTFV_COMP_G2BR    0x47324252
#define MNE_CTFV_COMP_G3BR    0x47334252
#define MNE_CTFV_COMP_G2OI    0x47324f49
#define MNE_CTFV_COMP_G3OI    0x47334f49

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNECTFCompData::MNECTFCompData()
:kind(MNE_CTFV_COMP_UNKNOWN)
,mne_kind(MNE_CTFV_COMP_UNKNOWN)
,calibrated(false)
{
}

//=============================================================================================================

MNECTFCompData::MNECTFCompData(const MNECTFCompData& comp)
:kind(MNE_CTFV_COMP_UNKNOWN)
,mne_kind(MNE_CTFV_COMP_UNKNOWN)
,calibrated(false)
{
    kind       = comp.kind;
    mne_kind   = comp.mne_kind;
    calibrated = comp.calibrated;
    if (comp.data)
        data       = std::make_unique<MNENamedMatrix>(*comp.data);
    if (comp.presel)
        presel     = std::make_unique<FiffSparseMatrix>(*comp.presel);
    if (comp.postsel)
        postsel    = std::make_unique<FiffSparseMatrix>(*comp.postsel);
}

//=============================================================================================================

MNECTFCompData::~MNECTFCompData()
{
}

//=============================================================================================================

int MNECTFCompData::calibrate(const QList<FIFFLIB::FiffChInfo>& chs, int nch, bool do_it)
{
    Eigen::VectorXf col_cals(this->data->ncol);
    Eigen::VectorXf row_cals(this->data->nrow);
    int   j,k,p,found;
    QString name;

    if (calibrated)
        return OK;

    for (j = 0; j < this->data->nrow; j++) {
        name = this->data->rowlist[j];
        found = false;
        for (p = 0; p < nch; p++)
            if (QString::compare(name,chs[p].ch_name) == 0) {
                row_cals[j] = chs[p].range*chs[p].cal;
                found = true;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
            return FAIL;
        }
    }
    for (k = 0; k < this->data->ncol; k++) {
        name = this->data->collist[k];
        found = false;
        for (p = 0; p < nch; p++)
            if (QString::compare(name,chs[p].ch_name) == 0) {
                col_cals[k] = chs[p].range*chs[p].cal;
                found = true;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
            return FAIL;
        }
    }
    if (do_it) {
        for (j = 0; j < this->data->nrow; j++)
            for (k = 0; k < this->data->ncol; k++)
                this->data->data(j, k) = row_cals[j]*this->data->data(j, k)/col_cals[k];
    }
    else {
        for (j = 0; j < this->data->nrow; j++)
            for (k = 0; k < this->data->ncol; k++)
                this->data->data(j, k) = col_cals[k]*this->data->data(j, k)/row_cals[j];
    }
    return OK;
}
