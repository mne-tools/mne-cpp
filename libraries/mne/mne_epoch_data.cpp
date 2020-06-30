//=============================================================================================================
/**
 * @file     mne_epoch_data.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the MNEEpochData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data.h"

#include <utils/mnemath.h>

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
, tmin(-1)
, tmax(-1)
, bReject(false)
{
}

//=============================================================================================================

MNEEpochData::MNEEpochData(const MNEEpochData &p_MNEEpochData)
: epoch(p_MNEEpochData.epoch)
, event(p_MNEEpochData.event)
, tmin(p_MNEEpochData.tmin)
, tmax(p_MNEEpochData.tmax)
, bReject(p_MNEEpochData.bReject)
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
    this->epoch = MNEMath::rescale(this->epoch, times, baseline, QString("mean"));
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
