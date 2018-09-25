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


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEEpochDataList::MNEEpochDataList()
{

}


//*************************************************************************************************************

MNEEpochDataList::MNEEpochDataList(const FiffRawData& raw,
                                   const MatrixXi& events,
                                   const RowVectorXi& picks,
                                   float tmin,
                                   float tmax,
                                   qint32 event)
{
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
        return;
    }

    fiff_int_t event_samp, from, to;
    MatrixXd timesDummy;
    MatrixXd times;

    MNEEpochData* epoch = Q_NULLPTR;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to   = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        epoch = new MNEEpochData();

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from, to, picks)) {
            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = event;
            epoch->tmin = ((float)(from)-(float)(raw.first_samp))/raw.info.sfreq;
            epoch->tmax = ((float)(to)-(float)(raw.first_samp))/raw.info.sfreq;

            this->append(MNEEpochData::SPtr(epoch));//List takes ownwership of the pointer - no delete need
        } else {
            printf("Can't read the event data segments");
        }
    }
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

FiffEvoked MNEEpochDataList::average(FiffInfo& info, fiff_int_t first, fiff_int_t last, VectorXi sel, bool proj)
{
    FiffEvoked p_evoked;

    printf("Calculate evoked... ");

    MatrixXd matAverage;
    if(this->size() > 0)
        matAverage = MatrixXd::Zero(this->at(0)->epoch.rows(), this->at(0)->epoch.cols());
    else
    {
        p_evoked.aspect_kind = FIFFV_ASPECT_STD_ERR;
        return p_evoked;
    }

    if(sel.size() > 0)
    {
        p_evoked.nave = sel.size();

        for(qint32 i = 0; i < sel.size(); ++i)
            matAverage.array() += this->at(sel(i))->epoch.array();
    }
    else
    {
        p_evoked.nave = this->size();

        for(qint32 i = 0; i < this->size(); ++i)
            matAverage.array() += this->at(i)->epoch.array();
    }
    matAverage.array() /= p_evoked.nave;

    printf("%d averages used [done]\n ", p_evoked.nave);

    p_evoked.setInfo(info, proj);

    p_evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;

    p_evoked.first = first;
    p_evoked.last = last;

    RowVectorXf times = RowVectorXf(last-first+1);
    for (qint32 k = 0; k < times.size(); ++k)
        times[k] = ((float)(first+k)) / info.sfreq;
    p_evoked.times = times;

    p_evoked.comment = QString::number(this->at(0)->event);

    if(p_evoked.proj.rows() > 0)
    {
        matAverage = p_evoked.proj * matAverage;
        printf("\tSSP projectors applied to the evoked data\n");
    }

    p_evoked.data = matAverage;

    return p_evoked;
}
