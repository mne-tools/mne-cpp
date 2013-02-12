//=============================================================================================================
/**
* @file     fiff_info.cpp
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the FiffInfo Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfo::FiffInfo()
: FiffInfoBase()//nchan(-1)
, sfreq(-1.0)
, highpass(-1.0)
, lowpass(-1.0)
, acq_pars("")
, acq_stim("")
, filename("")
{
    meas_date[0] = -1;
}


//*************************************************************************************************************

FiffInfo::FiffInfo(const FiffInfo& p_FiffInfo)
: FiffInfoBase(p_FiffInfo)
, file_id(FiffId(p_FiffInfo.file_id))//: QSharedData(p_FiffInfo)
, meas_id(FiffId(p_FiffInfo.meas_id))
, sfreq(p_FiffInfo.sfreq)
, highpass(p_FiffInfo.highpass)
, lowpass(p_FiffInfo.lowpass)
, dev_ctf_t(p_FiffInfo.dev_ctf_t)
, dig_trans(p_FiffInfo.dig_trans)
, acq_pars(p_FiffInfo.acq_pars)
, acq_stim(p_FiffInfo.acq_stim)
, filename(p_FiffInfo.filename)
{
    meas_date[0] = p_FiffInfo.meas_date[0];
    meas_date[1] = p_FiffInfo.meas_date[1];

    qint32 i;
    for(i = 0; i < p_FiffInfo.dig.size(); ++i)
        dig.append(FiffDigPoint(p_FiffInfo.dig[i]));

    for(i = 0; i < p_FiffInfo.projs.size(); ++i)
        projs.append(p_FiffInfo.projs[i]);

    for(i = 0; i < p_FiffInfo.comps.size(); ++i)
        comps.append(p_FiffInfo.comps[i]);
}


//*************************************************************************************************************

FiffInfo::~FiffInfo()
{

}


//*************************************************************************************************************

void FiffInfo::clear()
{
    FiffInfoBase::clear();
    file_id.clear();
    meas_id.clear();
    meas_date[0] = -1;
    sfreq = -1.0;
    highpass = -1.0;
    lowpass = -1.0;
    dev_ctf_t.clear();
    dig.clear();
    dig_trans.clear();
    projs.clear();
    comps.clear();
    acq_pars = "";
    acq_stim = "";
    filename = "";
}


//*************************************************************************************************************

qint32 FiffInfo::get_current_comp()
{
    qint32 comp = 0;
    qint32 first_comp = -1;

    qint32 k = 0;
    for (k = 0; k < this->nchan; ++k)
    {
        if (this->chs[k].kind == FIFFV_MEG_CH)
        {
            comp = this->chs[k].coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (comp != first_comp)
                printf("Compensation is not set equally on all MEG channels");
        }
    }
    return comp;
}


//*************************************************************************************************************

bool FiffInfo::make_compensator(fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs) const
{
    qDebug() << "make_compensator not debugged jet";

    MatrixXd C1, C2, comp_tmp;

    qDebug() << "Todo add all need ctf variables.";
//    if(ctf_comp.data)
//        delete ctf_comp.data;
    ctf_comp.data->clear();

    if (from == to)
    {
        ctf_comp.data->data = MatrixXd::Zero(this->nchan, this->nchan);
        return false;
    }

    if (from == 0)
        C1 = MatrixXd::Zero(this->nchan,this->nchan);
    else
    {
        if (!this->make_compensator(from, C1))
        {
            printf("Cannot create compensator C1\n");
            printf("Desired compensation matrix (kind = %d) not found\n",from);
            return false;
        }
    }

    if (to == 0)
        C2 = MatrixXd::Zero(this->nchan,this->nchan);
    else
    {
        if (!this->make_compensator(to, C2))
        {
            printf("Cannot create compensator C2\n");
            printf("Desired compensation matrix (kind = %d) not found\n",to);
            return false;
        }
    }
    //
    //   s_orig = s_from + C1*s_from = (I + C1)*s_from
    //   s_to   = s_orig - C2*s_orig = (I - C2)*s_orig
    //   s_to   = (I - C2)*(I + C1)*s_from = (I + C1 - C2 - C2*C1)*s_from
    //
    comp_tmp = MatrixXd::Identity(this->nchan,this->nchan) + C1 - C2 - C2*C1;

    qint32 k;
    if (exclude_comp_chs)
    {
        VectorXi pick  = MatrixXi::Zero(1,this->nchan);
        qint32 npick = 0;
        for (k = 0; k < this->nchan; ++k)
        {
            if (this->chs[k].kind != FIFFV_REF_MEG_CH)
            {
                pick(npick) = k;
                ++npick;
            }
        }
        if (npick == 0)
        {
            printf("Nothing remains after excluding the compensation channels\n");
            return false;
        }

        ctf_comp.data->data.resize(npick,this->nchan);
        for (k = 0; k < npick; ++k)
            ctf_comp.data->data.row(k) = comp_tmp.block(pick(k), 0, 1, this->nchan);
    }
    return true;
}


//*************************************************************************************************************

bool FiffInfo::make_compensator(fiff_int_t kind, MatrixXd& this_comp) const//private method
{
    qDebug() << "make_compensator not debugged jet";
    FiffNamedMatrix::SDPtr this_data;
    MatrixXd presel, postsel;
    qint32 k, col, c, ch, row, row_ch, channelAvailable;
    for (k = 0; k < this->comps.size(); ++k)
    {
        if (this->comps[k].kind == kind)
        {                this_data = this->comps[k].data;
            //
            //   Create the preselector
            //
            presel  = MatrixXd::Zero(this_data->ncol,this->nchan);
            for(col = 0; col < this_data->ncol; ++col)
            {
                channelAvailable = 0;
                for (c = 0; c < this->ch_names.size(); ++c)
                {
                    if (QString::compare(this_data->col_names.at(col),this->ch_names.at(c)) == 0)
                    {
                        ++channelAvailable;
                        ch = c;
                    }
                }
                if (channelAvailable == 0)
                {
                    printf("Channel %s is not available in data\n",this_data->col_names.at(col).toUtf8().constData());
                    return false;
                }
                else if (channelAvailable > 1)
                {
                    printf("Ambiguous channel %s",this_data->col_names.at(col).toUtf8().constData());
                    return false;
                }
                presel(col,ch) = 1.0;
            }
            //
            //   Create the postselector
            //
            postsel = MatrixXd::Zero(this->nchan,this_data->nrow);
            for (c = 0; c  < this->nchan; ++c)
            {
                channelAvailable = 0;
                for (row = 0; row < this_data->row_names.size(); ++row)
                {
                    if (QString::compare(this_data->col_names.at(c),this->ch_names.at(row)) == 0)
                    {
                        ++channelAvailable;
                        row_ch = row;
                    }
                }
                if (channelAvailable > 1)
                {
                    printf("Ambiguous channel %s", this->ch_names.at(c).toUtf8().constData());
                    return false;
                }
                else if (channelAvailable == 1)
                {
                    postsel(c,row_ch) = 1.0;
                }
            }
            this_comp = postsel*this_data->data*presel;
            return true;
        }
    }
    this_comp = defaultMatrixXd;
    return false;
}


//*************************************************************************************************************

FiffInfo FiffInfo::pick_info(const MatrixXi* sel) const
{
    FiffInfo res = *this;//new FiffInfo(this);
    if (sel == NULL)
        return res;

    //ToDo when pointer List do delation
    res.chs.clear();
    res.ch_names.clear();

    qint32 idx;
    for(qint32 i = 0; i < sel->cols(); ++i)
    {
        idx = (*sel)(0,i);
        res.chs.append(this->chs[idx]);
        res.ch_names.append(this->ch_names[idx]);
    }
    res.nchan  = sel->cols();

    return res;
}


//*************************************************************************************************************

QList<FiffChInfo> FiffInfo::set_current_comp(QList<FiffChInfo>& chs, fiff_int_t value)
{
    QList<FiffChInfo> new_chs;
    qint32 k;
    fiff_int_t coil_type;

    for(k = 0; k < chs.size(); ++k)
        new_chs.append(chs[k]);

    qint32 lower_half = 65535;// hex2dec('FFFF');
    for (k = 0; k < chs.size(); ++k)
    {
        if (chs[k].kind == FIFFV_MEG_CH)
        {
            coil_type = chs[k].coil_type & lower_half;
            new_chs[k].coil_type = (coil_type | (value << 16));
        }
    }
    return new_chs;
}
