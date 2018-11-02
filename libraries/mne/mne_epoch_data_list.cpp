//=============================================================================================================
/**
* @file     mne_epoch_data_list.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data_list.h"

#include <utils/mnemath.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEEpochDataList::MNEEpochDataList()
{

}


//*************************************************************************************************************

MNEEpochDataList::~MNEEpochDataList()
{
//    MNEEpochDataList::iterator i;
//    for( i = this->begin(); i!=this->end(); ++i) {
//        if (*i)
//            delete (*i);
//    }
}


//*************************************************************************************************************

MNEEpochDataList MNEEpochDataList::readEpochs(const FiffRawData& raw,
                                              const MatrixXi& events,
                                              float tmin,
                                              float tmax,
                                              qint32 event,
                                              double dEOGThreshold,
                                              const QString& sChType,
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
        printf("%d matching events found\n",count);
    } else {
        printf("No desired events found.\n");
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
    double min, max;

    MNEEpochData* epoch = Q_NULLPTR;

//    int iChType = FIFFV_EOG_CH;
//    int iEOGChIdx = -1;

//    for(int i = 0; i < raw.info.chs.size(); ++i) {
//        if(raw.info.chs.at(i).kind == iChType) {
//            for(int j = 0; j < picksNew.cols(); ++j) {
//                if(i == picksNew(j)) {
//                    iEOGChIdx = j;
//                    break;
//                }
//            }
//            break;
//        }
//    }

//    if(iEOGChIdx == -1) {
//        qDebug() << "No EOG channel found for epoch rejection";
//    }

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to   = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        epoch = new MNEEpochData();

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from, to, picksNew)) {
            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = event;
            epoch->tmin = ((float)(from)-(float)(raw.first_samp))/raw.info.sfreq;
            epoch->tmax = ((float)(to)-(float)(raw.first_samp))/raw.info.sfreq;

            epoch->bReject = checkForArtifact(epoch->epoch,
                                              raw.info,
                                              dEOGThreshold,
                                              "threshold",
                                              sChType);

            if (epoch->bReject) {
                dropCount++;
            }

//            if(iEOGChIdx >= 0 &&
//               iEOGChIdx < epoch->epoch.rows() &&
//               dEOGThreshold > 0.0) {
//                RowVectorXd vecRow = epoch->epoch.row(iEOGChIdx);
//                //vecRow = vecRow.array() - vecRow(0);
//                vecRow = vecRow.array() - vecRow.mean();

//                min = vecRow.minCoeff();
//                max = vecRow.maxCoeff();

//                //qDebug() << "std::fabs(min)" << std::fabs(min);
//                //qDebug() << "std::fabs(max)" << std::fabs(max);

//                //If absolute vaue of min or max if bigger than threshold -> reject
//                if((std::fabs(min) > dEOGThreshold) || (std::fabs(max) > dEOGThreshold)) {
//                    epoch->bReject = true;
//                    //qDebug() << "Epoch at sample" << event_samp << "rejected based on EOG channel";
//                }
//            }

            //Check if data block has the same size as the previous one
            if(!data.isEmpty()) {
                if(epoch->epoch.size() == data.last()->epoch.size()) {
                    data.append(MNEEpochData::SPtr(epoch));//List takes ownwership of the pointer - no delete need
                }
            } else {
                data.append(MNEEpochData::SPtr(epoch));//List takes ownwership of the pointer - no delete need
            }
        } else {
            printf("Can't read the event data segments");
        }
    }

    qDebug() << "Read total of"<< data.size() <<"epochs and dropped"<< dropCount <<"of them";

    return data;
}


//*************************************************************************************************************

FiffEvoked MNEEpochDataList::average(FiffInfo& info, fiff_int_t first, fiff_int_t last, VectorXi sel, bool proj)
{
    FiffEvoked p_evoked;

    printf("Calculate evoked... ");

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

    printf("%d averages used [done]\n ", p_evoked.nave);

    p_evoked.setInfo(info, proj);

    p_evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;

    p_evoked.first = first;
    p_evoked.last = last;

    RowVectorXf times = RowVectorXf(last-first+1);
    for (qint32 k = 0; k < times.size(); ++k) {
        times[k] = ((float)(first+k)) / info.sfreq;
    }

    p_evoked.times = times;

    p_evoked.comment = QString::number(this->at(0)->event);

    if(p_evoked.proj.rows() > 0) {
        matAverage = p_evoked.proj * matAverage;
        printf("\tSSP projectors applied to the evoked data\n");
    }

    QPair<QVariant,QVariant> pairBaselineSec;
    pairBaselineSec.first = this->first()->tmin;
    pairBaselineSec.second = this->first()->tmax;

    p_evoked.data = MNEMath::rescale(matAverage, times, pairBaselineSec, QString("mean"));

    return p_evoked;
}


//*************************************************************************************************************

void MNEEpochDataList::dropRejected()
{
    QMutableListIterator<MNEEpochData::SPtr> i(*this);
    while (i.hasNext()) {
        if (i.next()->bReject) {
            i.remove();
        }
    }
}


//*************************************************************************************************************

void MNEEpochDataList::pick_channels(const RowVectorXi& sel)
{
    QMutableListIterator<MNEEpochData::SPtr> i(*this);
    while (i.hasNext()) {
        i.next()->pick_channels(sel);
    }
}


//*************************************************************************************************************

bool MNEEpochDataList::checkForArtifact(MatrixXd& data,
                                        const FiffInfo& pFiffInfo,
                                        double dThreshold,
                                        const QString& sCheckType,
                                        const QString& sChType)
{
    bool bReject = false;

    //Prepare concurrent data handling
    QList<ArtifactRejectionData> lchData;

    int iChType = FIFFV_EOG_CH;

    if(sChType.contains("meg", Qt::CaseInsensitive)) {
        iChType = FIFFV_MEG_CH;
    }

    if(sChType.contains("eeg", Qt::CaseInsensitive)) {
        iChType = FIFFV_EEG_CH;
    }

    for(int i = 0; i < pFiffInfo.chs.size(); ++i) {
        if(pFiffInfo.chs.at(i).kind == iChType
           && !pFiffInfo.bads.contains(pFiffInfo.chs.at(i).ch_name)
           && pFiffInfo.chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG
           && pFiffInfo.chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_REF_MAG2) {
            ArtifactRejectionData tempData;
            tempData.bRejected = false;
            tempData.data = data.row(i);
            tempData.dThreshold = dThreshold;
            lchData.append(tempData);
        }
    }

//    qDebug() << "MNEEpochDataList::checkForArtifact - lchData.size()" << lchData.size();
//    qDebug() << "MNEEpochDataList::checkForArtifact - iChType" << iChType;

    if(sCheckType.contains("threshold", Qt::CaseInsensitive)) {
        //Start the concurrent processing
        QFuture<void> future = QtConcurrent::map(lchData, checkChThreshold);
        future.waitForFinished();

        for(int i = 0; i < lchData.size(); ++i) {
            if(lchData.at(i).bRejected) {
                bReject = true;
                qDebug() << "MNEEpochDataList::checkForArtifact - Reject trial";
                break;
            }
        }
    } else if(sCheckType.contains("variance", Qt::CaseInsensitive)) {
        //Start the concurrent processing
        QFuture<void> future = QtConcurrent::map(lchData, checkChVariance);
        future.waitForFinished();

        for(int i = 0; i < lchData.size(); ++i) {
            if(lchData.at(i).bRejected) {
                bReject = true;
                qDebug() << "MNEEpochDataList::checkForArtifact - Reject trial";
                break;
            }
        }
    }

    return bReject;
}


//*************************************************************************************************************

void MNEEpochDataList::checkChVariance(ArtifactRejectionData& inputData)
{
    RowVectorXd temp = inputData.data;

    double dMedian = temp.norm() / temp.cols();

    temp = temp.array() - dMedian;
    temp.array().square();

//    qDebug() << "MNEEpochDataList::checkChVariance - dMedian" << abs(dMedian);
//    qDebug() << "MNEEpochDataList::checkChVariance - m_iValueVariance * dMedian" << m_iValueVariance * abs(dMedian);
//    qDebug() << "MNEEpochDataList::checkChVariance - compare value " << abs(pairData.second.norm() / pairData.second.cols());

    //If variance is 3 times bigger than median -> reject
    if(temp.norm() / temp.cols() > (inputData.dThreshold * std::fabs(dMedian))) {
        inputData.bRejected = true;
    } else {
        inputData.bRejected = false;
    }
}


//*************************************************************************************************************

void MNEEpochDataList::checkChThreshold(ArtifactRejectionData& inputData)
{
    RowVectorXd temp = inputData.data;

    temp = temp.array() - temp(0);

    double min = temp.minCoeff();
    double max = temp.maxCoeff();

//    qDebug() << "MNEEpochDataList::checkChVariance - min" << min;
//    qDebug() << "MNEEpochDataList::checkChVariance - max" << max;
//    qDebug() << "MNEEpochDataList::checkChVariance - m_dValueThreshold" << m_dValueThreshold;

    //If absolute vaue of min or max if bigger than threshold -> reject
    if((std::fabs(min) > inputData.dThreshold) || (std::fabs(max) > inputData.dThreshold)) {
        inputData.bRejected = true;
    } else {
        inputData.bRejected = false;
    }
}
