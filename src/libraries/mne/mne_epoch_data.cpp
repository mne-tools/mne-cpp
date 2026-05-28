//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_epoch_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref MNELIB::MNEEpochData.
 *
 * Provides constructors, deep-copy semantics for the sensor matrix and
 * the application of SSP / baseline correction to one epoch in place.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data.h"

#include <math/numerics.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEEpochData::MNEEpochData()
: event(-1)
, eventSample(-1)
, tmin(-1)
, tmax(-1)
, bReject(false)
, bUserReject(false)
{
}

//=============================================================================================================

MNEEpochData::MNEEpochData(const MNEEpochData &p_MNEEpochData)
: epoch(p_MNEEpochData.epoch)
, event(p_MNEEpochData.event)
, eventSample(p_MNEEpochData.eventSample)
, tmin(p_MNEEpochData.tmin)
, tmax(p_MNEEpochData.tmax)
, bReject(p_MNEEpochData.bReject)
, bUserReject(p_MNEEpochData.bUserReject)
{
}

//=============================================================================================================

MNEEpochData::~MNEEpochData()
{
}

//=============================================================================================================

void MNEEpochData::applyBaselineCorrection(const QPair<float, float>& baseline)
{
    // Run baseline correction
    RowVectorXf times = RowVectorXf::LinSpaced(this->epoch.cols(), this->tmin, this->tmax);
    this->epoch = Numerics::rescale(this->epoch, times, baseline, QString("mean"));
}

//=============================================================================================================

void MNEEpochData::pick_channels(const RowVectorXi& sel)
{
    if (sel.cols() == 0) {
        qWarning("MNEEpochData::pick_channels - Warning : No channels were provided.\n");
        return;
    }

    // Reduce data set
    MatrixXd selBlock(1,1);

    if(selBlock.rows() != sel.cols() || selBlock.cols() != epoch.cols()) {
        selBlock.resize(sel.cols(), epoch.cols());
    }

    for(qint32 l = 0; l < sel.cols(); ++l) {
        if(sel(l) <= epoch.rows()) {
            selBlock.row(l) = epoch.row(sel(0,l));
        } else {
            qWarning("FiffEvoked::pick_channels - Warning : Selected channel index out of bound.\n");
        }
    }

    epoch = selBlock;
}
