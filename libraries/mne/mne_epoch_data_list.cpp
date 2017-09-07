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
* @brief     implementation of the MNEEpochDataList Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data_list.h"


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
