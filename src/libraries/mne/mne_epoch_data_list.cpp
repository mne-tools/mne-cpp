//=============================================================================================================
/**
 * @file     mne_epoch_data_list.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the MNEEpochDataList Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data_list.h"

#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QtConcurrent>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEEpochDataList::MNEEpochDataList()
{
}

//=============================================================================================================

MNEEpochDataList::~MNEEpochDataList()
{
//    MNEEpochDataList::iterator i;
//    for( i = this->begin(); i!=this->end(); ++i) {
//        if (*i)
//            delete (*i);
//    }
}

//=============================================================================================================

MNEEpochDataList MNEEpochDataList::readEpochs(const FiffRawData& raw,
                                              const MatrixXi& events,
                                              float tmin,
                                              float tmax,
                                              qint32 event,
                                              const QMap<QString,double>& mapReject,
                                              const QStringList& lExcludeChs,
                                              const RowVectorXi& picks)
{
    MNEEpochDataList data;

    // Select the desired events
    qint32 count = 0;
    qint32 p;
    MatrixXi selected = MatrixXi::Zero(1, events.rows());
    for (p = 0; p < events.rows(); ++p)
    {
        if (events(p,1) == 0 && events(p,2) == event)
        {
            selected(0,count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0) {
        qInfo("[MNEEpochDataList::readEpochs] %d matching events found",count);
    } else {
        qWarning("[MNEEpochDataList::readEpochs] No desired events found.");
        return MNEEpochDataList();
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

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to   = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        epoch.reset(new MNEEpochData());

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from, to, picksNew)) {
            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = event;
            epoch->tmin = tmin;
            epoch->tmax = tmax;

            epoch->bReject = checkForArtifact(epoch->epoch,
                                              raw.info,
                                              mapReject,
                                              lExcludeChs);

            if (epoch->bReject) {
                dropCount++;
            }

            //Check if data block has the same size as the previous one
            if(!data.isEmpty()) {
                if(epoch->epoch.size() == data.last()->epoch.size()) {
                    data.append(MNEEpochData::SPtr(epoch.take()));//List takes ownwership of the pointer - no delete need
                }
            } else {
                data.append(MNEEpochData::SPtr(epoch.take()));//List takes ownwership of the pointer - no delete need
            }
        } else {
            qWarning("[MNEEpochDataList::readEpochs] Can't read the event data segments.");
        }
    }

    qInfo().noquote() << "[MNEEpochDataList::readEpochs] Read a total of"<< data.size() <<"epochs of type" << event << "and marked"<< dropCount <<"for rejection.";

    return data;
}

//=============================================================================================================

FiffEvoked MNEEpochDataList::average(const FiffInfo& info,
                                     fiff_int_t first,
                                     fiff_int_t last,
                                     VectorXi sel,
                                     bool proj)
{
    FiffEvoked p_evoked;

    qInfo("[MNEEpochDataList::average] Calculate evoked. ");

    MatrixXd matAverage;

    if(this->size() > 0) {
        matAverage = MatrixXd::Zero(this->at(0)->epoch.rows(), this->at(0)->epoch.cols());
    } else {
        p_evoked.aspect_kind = FIFFV_ASPECT_STD_ERR;
        return p_evoked;
    }

    if(sel.size() > 0) {
        p_evoked.nave = sel.size();

        for(qint32 i = 0; i < sel.size(); ++i) {
            matAverage.array() += this->at(sel(i))->epoch.array();
        }
    } else {
        p_evoked.nave = this->size();

        for(qint32 i = 0; i < this->size(); ++i) {
            matAverage.array() += this->at(i)->epoch.array();
        }
    }
    matAverage.array() /= p_evoked.nave;

    qInfo("[MNEEpochDataList::average] %d averages used [done]", p_evoked.nave);

    p_evoked.setInfo(info, proj);

    p_evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;

    p_evoked.first = first;
    p_evoked.last = last;

    p_evoked.times = RowVectorXf::LinSpaced(this->first()->epoch.cols(), this->first()->tmin, this->first()->tmax);

    p_evoked.times[static_cast<int>(this->first()->tmin * -1 * info.sfreq)] = 0;

    p_evoked.comment = QString::number(this->at(0)->event);

    if(p_evoked.proj.rows() > 0) {
        matAverage = p_evoked.proj * matAverage;
        qInfo("[MNEEpochDataList::average] SSP projectors applied to the evoked data");
    }

    p_evoked.data = matAverage;

    return p_evoked;
}

//=============================================================================================================

void MNEEpochDataList::applyBaselineCorrection(const QPair<float, float> &baseline)
{
    // Run baseline correction
    QMutableListIterator<MNEEpochData::SPtr> i(*this);
    while (i.hasNext()) {
        i.next()->applyBaselineCorrection(baseline);
    }
}

//=============================================================================================================

void MNEEpochDataList::dropRejected()
{
    QMutableListIterator<MNEEpochData::SPtr> i(*this);
    while (i.hasNext()) {
        if (i.next()->bReject) {
            i.remove();
        }
    }
}

//=============================================================================================================

void MNEEpochDataList::pick_channels(const RowVectorXi& sel)
{
    QMutableListIterator<MNEEpochData::SPtr> i(*this);
    while (i.hasNext()) {
        i.next()->pick_channels(sel);
    }
}

//=============================================================================================================

bool MNEEpochDataList::checkForArtifact(const MatrixXd& data,
                                        const FiffInfo& pFiffInfo,
                                        const QMap<QString,double>& mapReject,
                                        const QStringList& lExcludeChs)
{
    //qDebug() << "MNEEpochDataList::checkForArtifact - Doing artifact reduction for" << mapReject;

    bool bReject = false;

    //Prepare concurrent data handling
    QList<ArtifactRejectionData> lchData;
    QList<int> lChTypes;

    if(mapReject.contains("grad") ||
       mapReject.contains("mag") ) {
        lChTypes << FIFFV_MEG_CH;
    }

    if(mapReject.contains("eeg")) {
        lChTypes << FIFFV_EEG_CH;
    }

    if(mapReject.contains("eog")) {
        lChTypes << FIFFV_EOG_CH;
    }

    if(lChTypes.isEmpty()) {
        return bReject;
    }

    for(int i = 0; i < pFiffInfo.chs.size(); ++i) {
        if(lChTypes.contains(pFiffInfo.chs.at(i).kind)
           && !lExcludeChs.contains(pFiffInfo.chs.at(i).ch_name)
           && !pFiffInfo.bads.contains(pFiffInfo.chs.at(i).ch_name)
           && pFiffInfo.chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG
           && pFiffInfo.chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG2) {
            ArtifactRejectionData tempData;
            tempData.data = data.row(i);

            switch (pFiffInfo.chs.at(i).kind) {
            case FIFFV_MEG_CH:
                if(pFiffInfo.chs.at(i).unit == FIFF_UNIT_T) {
                    tempData.dThreshold = mapReject["mag"];
                } else if(pFiffInfo.chs.at(i).unit == FIFF_UNIT_T_M) {
                    tempData.dThreshold = mapReject["grad"];
                }
            break;

            case FIFFV_EEG_CH:
                tempData.dThreshold = mapReject["eeg"];
            break;

            case FIFFV_EOG_CH:
                tempData.dThreshold = mapReject["eog"];
            break;
            }

            tempData.sChName = pFiffInfo.chs.at(i).ch_name;
            lchData.append(tempData);
        }
    }

    if(lchData.isEmpty()) {
        qWarning() << "[MNEEpochDataList::checkForArtifact] No channels found to scan for artifacts. Do not reject. Returning.";

        return bReject;
    }

    //qDebug() << "MNEEpochDataList::checkForArtifact - lchData.size()" << lchData.size();

    //Start the concurrent processing
    QFuture<void> future = QtConcurrent::map(lchData, checkChThreshold);
    future.waitForFinished();

    for(int i = 0; i < lchData.size(); ++i) {
        if(lchData.at(i).bRejected) {
            bReject = true;
            qInfo().noquote() << "[MNEEpochDataList::checkForArtifact] Reject trial because of channel"<<lchData.at(i).sChName;
            break;
        }
    }

    return bReject;
}

//=============================================================================================================

void MNEEpochDataList::checkChThreshold(ArtifactRejectionData& inputData)
{
    RowVectorXd temp = inputData.data;

    // Remove offset
    //temp = temp.array() - temp(0);

    double min = temp.minCoeff();
    double max = temp.maxCoeff();

    // Peak to Peak
    double pp = max - min;

    if(std::fabs(pp) > inputData.dThreshold) {
        inputData.bRejected = true;
    } else {
        inputData.bRejected = false;
    }

//    qDebug() << "MNEEpochDataList::checkChThreshold - min" << min;
//    qDebug() << "MNEEpochDataList::checkChThreshold - max" << max;
//    qDebug() << "MNEEpochDataList::checkChThreshold - pp" << pp;
//    qDebug() << "MNEEpochDataList::checkChThreshold - inputData.dThreshold" << inputData.dThreshold;

//    //If absolute vaue of min or max if bigger than threshold -> reject
//    if((std::fabs(min) > inputData.dThreshold) || (std::fabs(max) > inputData.dThreshold)) {
//        inputData.bRejected = true;
//    } else {
//        inputData.bRejected = false;
//    }
}
