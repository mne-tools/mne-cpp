//=============================================================================================================
/**
 * @file     fiff_info.cpp
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
 * @brief    Definition of the FiffInfo Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"
#include "fiff_stream.h"
#include "fiff_file.h"

#include <utils/ioutils.h>
#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfo::FiffInfo()
: FiffInfoBase()//nchan(-1)
, sfreq(-1.0)
, linefreq(-1.0)
, highpass(-1.0)
, lowpass(-1.0)
, gantry_angle(-1)
, acq_pars("")
, acq_stim("")
{
    meas_date[0] = -1;
}

//=============================================================================================================

FiffInfo::FiffInfo(const FiffInfo& p_FiffInfo)
: FiffInfoBase(p_FiffInfo)
, file_id(p_FiffInfo.file_id)
, sfreq(p_FiffInfo.sfreq)
, linefreq(p_FiffInfo.linefreq)
, highpass(p_FiffInfo.highpass)
, lowpass(p_FiffInfo.lowpass)
, proj_id(p_FiffInfo.proj_id)
, proj_name(p_FiffInfo.proj_name)
, xplotter_layout(p_FiffInfo.xplotter_layout)
, experimenter(p_FiffInfo.experimenter)
, description(p_FiffInfo.description)
, utc_offset(p_FiffInfo.utc_offset)
, gantry_angle(p_FiffInfo.gantry_angle)
, dev_ctf_t(p_FiffInfo.dev_ctf_t)
, dig(p_FiffInfo.dig)
, dig_trans(p_FiffInfo.dig_trans)
, projs(p_FiffInfo.projs)
, comps(p_FiffInfo.comps)
, acq_pars(p_FiffInfo.acq_pars)
, acq_stim(p_FiffInfo.acq_stim)
{
    meas_date[0] = p_FiffInfo.meas_date[0];
    meas_date[1] = p_FiffInfo.meas_date[1];
}

//=============================================================================================================

FiffInfo::~FiffInfo()
{
}

//=============================================================================================================

void FiffInfo::clear()
{
    FiffInfoBase::clear();
    file_id = FiffId();
    meas_date[0] = -1;
    sfreq = -1.0;
    linefreq = -1.0;
    highpass = -1.0;
    lowpass = -1.0;
    proj_id = -1;
    proj_name = "";
    xplotter_layout = "";
    experimenter = "";
    description = "";
    utc_offset = "";         /**< UTC offset of related meas_date (sHH:MM).*/
    gantry_angle = -1;
    dev_ctf_t.clear();
    dig.clear();
    dig_trans.clear();
    projs.clear();
    comps.clear();
    acq_pars = "";
    acq_stim = "";
}

//=============================================================================================================

qint32 FiffInfo::get_current_comp()
{
    qint32 comp = 0;
    qint32 first_comp = -1;

    qint32 k = 0;
    for (k = 0; k < this->nchan; ++k)
    {
        if (this->chs[k].kind == FIFFV_MEG_CH)
        {
            comp = this->chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (comp != first_comp)
                printf("Compensation is not set equally on all MEG channels");
        }
    }
    return comp;
}

//=============================================================================================================

bool FiffInfo::make_compensator(fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs) const
{
    MatrixXd C1, C2, comp_tmp;

//    if(ctf_comp.data)
//        delete ctf_comp.data;
    ctf_comp.data->clear();

    if (from == to)
    {
        ctf_comp.data->data = MatrixXd::Identity(this->nchan, this->nchan);
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
    } else {
        ctf_comp.data->data = comp_tmp;
    }

    return true;
}

//=============================================================================================================

bool FiffInfo::make_compensator(fiff_int_t kind, MatrixXd& this_comp) const//private method
{
    FiffNamedMatrix::SDPtr this_data;
    MatrixXd presel, postsel;
    qint32 k, col, c, ch=0, row, row_ch=0, channelAvailable;

    for (k = 0; k < this->comps.size(); ++k)
    {
        if (this->comps[k].kind == kind)
        {
            this_data = this->comps[k].data;

            //
            //   Create the preselector
            //
            presel = MatrixXd::Zero(this_data->ncol,this->nchan);

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
                    if (QString::compare(this->ch_names.at(c),this_data->row_names.at(row)) == 0)
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

//=============================================================================================================

FiffInfo FiffInfo::pick_info(const RowVectorXi &sel) const
{
    FiffInfo res = *this;//new FiffInfo(this);
    if (sel.size() == 0)
        return res;

    //ToDo when pointer List do delation
    res.chs.clear();
    res.ch_names.clear();

    qint32 idx;
    for(qint32 i = 0; i < sel.size(); ++i)
    {
        idx = sel[i];
        res.chs.append(this->chs[idx]);
        res.ch_names.append(this->ch_names[idx]);
    }
    res.nchan  = static_cast<int>(sel.size());

    return res;
}

//=============================================================================================================

QList<FiffChInfo> FiffInfo::set_current_comp(QList<FiffChInfo>& listFiffChInfo, fiff_int_t value)
{
    QList<FiffChInfo> newList;
    qint32 k;
    fiff_int_t coil_type;

    for(k = 0; k < listFiffChInfo.size(); ++k)
        newList.append(listFiffChInfo[k]);

    qint32 lower_half = 65535;// hex2dec('FFFF');
    for (k = 0; k < listFiffChInfo.size(); ++k)
    {
        if (listFiffChInfo[k].kind == FIFFV_MEG_CH)
        {
            coil_type = listFiffChInfo[k].chpos.coil_type & lower_half;
            newList[k].chpos.coil_type = (coil_type | (value << 16));
        }
    }
    return newList;
}

//=============================================================================================================

void FiffInfo::writeToStream(FiffStream* p_pStream) const
{
    //
    //   We will always write floats
    //
    fiff_int_t data_type = 4;
    qint32 k;
    QList<FiffChInfo> chs;

    for(k = 0; k < this->nchan; ++k)
        chs << this->chs[k];

    fiff_int_t nchan = chs.size();

    //
    // write the essentials
    //
    p_pStream->start_block(FIFFB_MEAS);//4
    p_pStream->write_id(FIFF_BLOCK_ID);//5
    if(this->meas_id.version != -1)
    {
        p_pStream->write_id(FIFF_PARENT_BLOCK_ID,this->meas_id);//6
    }
    //
    //    Measurement info
    //
    p_pStream->start_block(FIFFB_MEAS_INFO);//7

    //
    //    Blocks from the original -> skip this
    //
//        QList<fiff_int_t> blocks;
//        blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY;
    bool have_hpi_result = false;
    bool have_isotrak    = false;
    //
    //    megacq parameters
    //
    if (!this->acq_pars.isEmpty() || !this->acq_stim.isEmpty())
    {
        p_pStream->start_block(FIFFB_DACQ_PARS);
        if (!this->acq_pars.isEmpty())
            p_pStream->write_string(FIFF_DACQ_PARS, this->acq_pars);

        if (!this->acq_stim.isEmpty())
            p_pStream->write_string(FIFF_DACQ_STIM, this->acq_stim);

        p_pStream->end_block(FIFFB_DACQ_PARS);
    }
    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result)
    {
        if (!this->dev_head_t.isEmpty())
            p_pStream->write_coord_trans(this->dev_head_t);

        if (!this->ctf_head_t.isEmpty())
            p_pStream->write_coord_trans(this->ctf_head_t);
    }
    //
    //    Polhemus data
    //
    if (this->dig.size() > 0 && !have_isotrak)
    {
        p_pStream->start_block(FIFFB_ISOTRAK);
        for (qint32 k = 0; k < this->dig.size(); ++k)
            p_pStream->write_dig_point(this->dig[k]);

        p_pStream->end_block(FIFFB_ISOTRAK);
    }
    //
    //    Projectors
    //
    p_pStream->write_proj(this->projs);
    //
    //    CTF compensation info
    //
    p_pStream->write_ctf_comp(this->comps);
    //
    //    Bad channels
    //
    if (this->bads.size() > 0)
    {
        p_pStream->start_block(FIFFB_MNE_BAD_CHANNELS);
        p_pStream->write_name_list(FIFF_MNE_CH_NAME_LIST,this->bads);
        p_pStream->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //    General
    //
    p_pStream->write_float(FIFF_SFREQ,&this->sfreq);
    p_pStream->write_float(FIFF_LINE_FREQ,&this->linefreq);
    p_pStream->write_float(FIFF_HIGHPASS,&this->highpass);
    p_pStream->write_float(FIFF_LOWPASS,&this->lowpass);
    p_pStream->write_string(FIFF_EXPERIMENTER,this->experimenter);
    p_pStream->write_string(FIFF_DESCRIPTION,this->description);
    p_pStream->write_string(FIFF_UNIT_C,this->utc_offset);
    p_pStream->write_string(FIFF_PROJ_NAME,this->proj_name);
    p_pStream->write_int(FIFF_PROJ_ID,&this->proj_id);
    p_pStream->write_int(FIFF_GANTRY_ANGLE,&this->gantry_angle);
    p_pStream->write_int(FIFF_NCHAN,&nchan);
    p_pStream->write_int(FIFF_DATA_PACK,&data_type);
    if (this->meas_date[0] != -1)
        p_pStream->write_int(FIFF_MEAS_DATE,this->meas_date, 2);
    //
    //    Channel info
    //
    MatrixXd cals(1,nchan);

    for(k = 0; k < nchan; ++k)
    {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanNo = k+1;//+1 because
//        chs[k].range  = 1.0f;//Why? -> cause its already calibrated through reading
        cals(0,k) = static_cast<double>(chs[k].cal); //ToDo whats going on with cals?
        p_pStream->write_ch_info(chs[k]);
    }
    //
    //
    p_pStream->end_block(FIFFB_MEAS_INFO);
}

//=============================================================================================================

void FiffInfo::print()
{
    std::cout << "Sample frequency: " << sfreq << "\n";
    std::cout << "LineFreq: " << linefreq << " | Highpass: " << highpass << " | Lowpass: " << lowpass << "\n";
    std::cout << "Number of digitizer points: " << dig.size() << "\n";
    for (auto& point : dig){
        if (point.kind == FIFFV_POINT_HPI){
            std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
        }
    }
}
