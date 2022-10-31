//=============================================================================================================
/**
 * @file     averaging.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief     Averaging defintions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"

#include "helpers/filterkernel.h"
#include "filter.h"

#include <mne/mne_epoch_data_list.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE GLOBAL RTPROCESSINGLIB METHODS
//=============================================================================================================

FiffEvoked RTPROCESSINGLIB::computeAverage(const FiffRawData& raw,
                                           const MatrixXi& matEvents,
                                           float fTMinS,
                                           float fTMaxS,
                                           qint32 eventType,
                                           bool bApplyBaseline,
                                           float fTBaselineFromS,
                                           float fTBaselineToS,
                                           const QMap<QString,double>& mapReject,
                                           const QStringList& lExcludeChs,
                                           const RowVectorXi& picks)
{

    MNEEpochDataList lstEpochDataList = MNEEpochDataList::readEpochs(raw,
                                                                     matEvents,
                                                                     fTMinS,
                                                                     fTMaxS,
                                                                     eventType,
                                                                     mapReject,
                                                                     lExcludeChs,
                                                                     picks);

    if(bApplyBaseline){
        QPair<float, float> baselinePair(fTBaselineFromS, fTBaselineToS);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if(!mapReject.isEmpty()){
        lstEpochDataList.dropRejected();
    }

    return lstEpochDataList.average(raw.info,
                                    0,
                                    lstEpochDataList.first()->epoch.cols());
}

//=============================================================================================================

FiffEvoked RTPROCESSINGLIB::computeFilteredAverage(const FiffRawData& raw,
                                                   const MatrixXi& matEvents,
                                                   float fTMinS,
                                                   float fTMaxS,
                                                   qint32 eventType,
                                                   bool bApplyBaseline,
                                                   float fTBaselineFromS,
                                                   float fTBaselineToS,
                                                   const QMap<QString,double>& mapReject,
                                                   const FilterKernel& filterKernel,
                                                   const QStringList& lExcludeChs,
                                                   const RowVectorXi& picks)
{
    MNEEpochDataList lstEpochDataList;

    // Select the desired events
    qint32 count = 0;
    qint32 p;
    MatrixXi selected = MatrixXi::Zero(1, matEvents.rows());
    for (p = 0; p < matEvents.rows(); ++p)
    {
        if (matEvents(p,1) == 0 && matEvents(p,2) == eventType)
        {
            selected(0,count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0) {
        qInfo("[RTPROCESSINGLIB::computeFilteredAverage] %d matching events found",count);
    }

    // If picks are empty, pick all
    RowVectorXi picksNew = picks;
    if(picks.cols() <= 0) {
        picksNew.resize(raw.info.chs.size());
        for(int i = 0; i < raw.info.chs.size(); ++i) {
            picksNew(i) = i;
        }
    }

    fiff_int_t event_samp, from, to;
    fiff_int_t dropCount = 0;
    MatrixXd timesDummy;
    MatrixXd times;

    QScopedPointer<MNEEpochData> epoch(Q_NULLPTR);
    int iFilterDelay = filterKernel.getFilterOrder()/2;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = matEvents(selected(p),0);
        from = event_samp + fTMinS*raw.info.sfreq;
        to   = event_samp + floor(fTMaxS*raw.info.sfreq + 0.5);

        epoch.reset(new MNEEpochData());

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from - iFilterDelay, to + iFilterDelay, picksNew)) {
            // Filter the data
            epoch->epoch = RTPROCESSINGLIB::filterData(epoch->epoch,filterKernel).block(0, iFilterDelay, epoch->epoch.rows(), to-from);

            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = eventType;
            epoch->tmin = fTMinS;
            epoch->tmax = fTMaxS;

            epoch->bReject = MNEEpochDataList::checkForArtifact(epoch->epoch,
                                                                raw.info,
                                                                mapReject,
                                                                lExcludeChs);

            if (epoch->bReject) {
                dropCount++;
            }

            //Check if data block has the same size as the previous one
            if(!lstEpochDataList.isEmpty()) {
                if(epoch->epoch.size() == lstEpochDataList.last()->epoch.size()) {
                    lstEpochDataList.append(MNEEpochData::SPtr(epoch.take()));//List takes ownwership of the pointer - no delete need
                }
            } else {
                lstEpochDataList.append(MNEEpochData::SPtr(epoch.take()));//List takes ownwership of the pointer - no delete need
            }
        } else {
            qWarning("[MNEEpochDataList::readEpochs] Can't read the event data segments.");
        }
    }

    qInfo().noquote() << "[MNEEpochDataList::readEpochs] Read a total of"<< lstEpochDataList.size() <<"epochs of type" << eventType << "and marked"<< dropCount <<"for rejection.";

    if(bApplyBaseline){
        QPair<float, float> baselinePair(fTBaselineFromS, fTBaselineToS);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if(!mapReject.isEmpty()){
        lstEpochDataList.dropRejected();
    }

    return lstEpochDataList.average(raw.info,
                                    0,
                                    lstEpochDataList.first()->epoch.cols());
}
