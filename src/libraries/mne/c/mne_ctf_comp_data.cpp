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
 * @brief    Definition of the MneCTFCompData Class.
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

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#define MALLOC_31(x,t) (t *)malloc((x)*sizeof(t))

#define FREE_31(x) if ((char *)(x) != NULL) free((char *)(x))

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

MneCTFCompData::MneCTFCompData()
:kind(MNE_CTFV_COMP_UNKNOWN)
,mne_kind(MNE_CTFV_COMP_UNKNOWN)
,calibrated(FALSE)
,data(NULL)
,presel(NULL)
,postsel(NULL)
,presel_data(NULL)
,comp_data(NULL)
,postsel_data(NULL)
{
}

//=============================================================================================================

MneCTFCompData::MneCTFCompData(const MneCTFCompData& comp)
:kind(MNE_CTFV_COMP_UNKNOWN)
,mne_kind(MNE_CTFV_COMP_UNKNOWN)
,calibrated(FALSE)
,data(NULL)
,presel(NULL)
,postsel(NULL)
,presel_data(NULL)
,comp_data(NULL)
,postsel_data(NULL)
{
    kind       = comp.kind;
    mne_kind   = comp.mne_kind;
    calibrated = comp.calibrated;
    data       = new MneNamedMatrix(*comp.data);

    presel     = new FiffSparseMatrix(*comp.presel);
    postsel    = new FiffSparseMatrix(*comp.postsel);
}

//=============================================================================================================

MneCTFCompData::~MneCTFCompData()
{
    if(data)
        delete data;
    if(presel)
        delete presel;
    if(postsel)
        delete postsel;
    FREE_31(presel_data);
    FREE_31(postsel_data);
    FREE_31(comp_data);
}

//=============================================================================================================

int MneCTFCompData::mne_calibrate_ctf_comp(MneCTFCompData *one, const QList<FIFFLIB::FiffChInfo>& chs, int nch, int do_it)
/*
     * Calibrate or decalibrate a compensation data set
     */
{
    float *col_cals,*row_cals;
    int   j,k,p,found;
    QString name;
    float **data;

    if (!one)
        return OK;
    if (one->calibrated)
        return OK;

    row_cals = MALLOC_31(one->data->nrow,float);
    col_cals = MALLOC_31(one->data->ncol,float);

    for (j = 0; j < one->data->nrow; j++) {
        name = one->data->rowlist[j];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (QString::compare(name,chs[p].ch_name) == 0) {
                row_cals[j] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
            return FAIL;
        }
    }
    for (k = 0; k < one->data->ncol; k++) {
        name = one->data->collist[k];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (QString::compare(name,chs[p].ch_name) == 0) {
                col_cals[k] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
            return FAIL;
        }
    }
    data = one->data->data;
    if (do_it) {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = row_cals[j]*data[j][k]/col_cals[k];
    }
    else {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = col_cals[k]*data[j][k]/row_cals[j];
    }
    return OK;
}
