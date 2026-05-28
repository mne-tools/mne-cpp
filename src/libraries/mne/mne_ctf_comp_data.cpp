//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_ctf_comp_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNECTFCompData.
 *
 * Implements FIFF read/write and the calibration toggle that switches
 * the matrix between raw and calibrated units.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

#include <QFile>
#include <QDebug>

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
            qCritical("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
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
            qCritical("Channel %s not found. Cannot calibrate the compensation matrix.",name.toUtf8().constData());
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
