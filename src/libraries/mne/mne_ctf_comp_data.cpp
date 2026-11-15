//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_ctf_comp_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNECTFCompData.
 *
 * Implements FIFF read/write and the calibration toggle that switches
 * the matrix between raw and calibrated units.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data.h"
#include <base/log.h>

#include <vector>
#include "mne_types.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>


#include <Eigen/Core>

constexpr int FAIL = -1;
constexpr int OK = 0;

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNECTFCompData::MNECTFCompData()
: kind(MNE_CTFV_COMP_UNKNOWN)
, mne_kind(MNE_CTFV_COMP_UNKNOWN)
, calibrated(false)
{
}

//=============================================================================================================

MNECTFCompData::MNECTFCompData(const MNECTFCompData& comp)
: kind(MNE_CTFV_COMP_UNKNOWN)
, mne_kind(MNE_CTFV_COMP_UNKNOWN)
, calibrated(false)
{
    kind = comp.kind;
    mne_kind = comp.mne_kind;
    calibrated = comp.calibrated;
    if (comp.data)
        data = std::make_unique<MNENamedMatrix>(*comp.data);
    if (comp.presel)
        presel = std::make_unique<FiffSparseMatrix>(*comp.presel);
    if (comp.postsel)
        postsel = std::make_unique<FiffSparseMatrix>(*comp.postsel);
}

//=============================================================================================================

MNECTFCompData::~MNECTFCompData()
{
}

//=============================================================================================================

int MNECTFCompData::calibrate(const std::vector<mne::FiffChInfo>& chs, int nch, bool do_it)
{
    Eigen::VectorXf col_cals(this->data->ncol());
    Eigen::VectorXf row_cals(this->data->nrow());
    int j, k, p, found;

    if (calibrated)
        return OK;

    for (j = 0; j < this->data->nrow(); j++) {
        const std::string& name = this->data->rowlist[j];
        found = false;
        for (p = 0; p < nch; p++)
            if ((name == chs[p].ch_name)) {
                row_cals[j] = chs[p].range * chs[p].cal;
                found = true;
                break;
            }
        if (!found) {
            mne::Log::critical("Channel {} not found. Cannot calibrate the compensation matrix.", name);
            return FAIL;
        }
    }
    for (k = 0; k < this->data->ncol(); k++) {
        const std::string& name = this->data->collist[k];
        found = false;
        for (p = 0; p < nch; p++)
            if ((name == chs[p].ch_name)) {
                col_cals[k] = chs[p].range * chs[p].cal;
                found = true;
                break;
            }
        if (!found) {
            mne::Log::critical("Channel {} not found. Cannot calibrate the compensation matrix.", name);
            return FAIL;
        }
    }
    if (do_it) {
        for (j = 0; j < this->data->nrow(); j++)
            for (k = 0; k < this->data->ncol(); k++)
                this->data->data(j, k) = row_cals[j] * this->data->data(j, k) / col_cals[k];
    } else {
        for (j = 0; j < this->data->nrow(); j++)
            for (k = 0; k < this->data->ncol(); k++)
                this->data->data(j, k) = col_cals[k] * this->data->data(j, k) / row_cals[j];
    }
    return OK;
}
