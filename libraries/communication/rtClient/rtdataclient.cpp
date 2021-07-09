//=============================================================================================================
/**
 * @file     rtdataclient.cpp
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
 * @brief     Definition of the RtDataClient Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdataclient.h"
#include <fiff/fiff_file.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtDataClient::RtDataClient(QObject *parent)
: QTcpSocket(parent)
, m_clientID(-1)
{
    getClientId();
}

//=============================================================================================================

void RtDataClient::disconnectFromHost()
{
    QTcpSocket::disconnectFromHost();
    m_clientID = -1;
}

//=============================================================================================================

qint32 RtDataClient::getClientId()
{
    if(m_clientID == -1)
    {
//            sendFiffCommand(1);//MNE_RT.MNE_RT_GET_CLIENT_ID)

        FiffStream t_fiffStream(this);

        QString t_sCommand("");
        t_fiffStream.write_rt_command(1, t_sCommand);

        this->waitForReadyRead(100);
        // ID is send as answer
        FiffTag::SPtr t_pTag;
        t_fiffStream.read_tag(t_pTag);
        if (t_pTag->kind == FIFF_MNE_RT_CLIENT_ID)
            m_clientID = *t_pTag->toInt();
    }
    return m_clientID;
}

//=============================================================================================================

FiffInfo::SPtr RtDataClient::readInfo()
{
    FiffInfo::SPtr p_pFiffInfo(new FiffInfo());
    bool t_bReadMeasBlockStart = false;
    bool t_bReadMeasBlockEnd = false;
    QString col_names, row_names;

    FiffStream t_fiffStream(this);
    //
    // Find the start
    //
    FiffTag::SPtr t_pTag;
    while(!t_bReadMeasBlockStart)
    {
        t_fiffStream.read_rt_tag(t_pTag);
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_START FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockStart = true;
        }
    }

    //
    // Parse until the endblock
    //

    bool dev_head_t_read = false;
    bool ctf_head_t_read = false;

    while(!t_bReadMeasBlockEnd)
    {
        t_fiffStream.read_rt_tag(t_pTag);
        //
        //  megacq parameters
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_DACQ_PARS)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_DACQ_PARS)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_DACQ_PARS)
                    p_pFiffInfo->acq_pars = t_pTag->toString();
                else if(t_pTag->kind == FIFF_DACQ_STIM)
                    p_pFiffInfo->acq_stim = t_pTag->toString();
            }
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (t_pTag->kind == FIFF_COORD_TRANS)
        {
            if (!dev_head_t_read)
            {
                p_pFiffInfo->dev_head_t = t_pTag->toCoordTrans();
                dev_head_t_read = true;
            }
            else if (!ctf_head_t_read)
            {
                p_pFiffInfo->ctf_head_t = t_pTag->toCoordTrans();
                ctf_head_t_read = true;
            }
        }
        //
        //    Polhemus data
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_ISOTRAK)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_ISOTRAK)
            {
                t_fiffStream.read_rt_tag(t_pTag);

                if(t_pTag->kind == FIFF_DIG_POINT)
                    p_pFiffInfo->dig.append(t_pTag->toDigPoint());
            }
        }
        //
        //    Projectors
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM)
                {
                    FiffProj proj;
                    qint32 countProj = p_pFiffInfo->projs.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ_ITEM)
                    {
                        t_fiffStream.read_rt_tag(t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_NAME: // First proj -> Proj is created
                            proj = FiffProj();
                            p_pFiffInfo->projs.append(proj);
                            p_pFiffInfo->projs[countProj].desc = t_pTag->toString();
                            break;
                        case FIFF_PROJ_ITEM_KIND:
                            p_pFiffInfo->projs[countProj].kind = *(t_pTag->toInt());
                            break;
                        case FIFF_NCHAN: // First data -> FiffNamedMatrix is created
                            p_pFiffInfo->projs[countProj].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_NVEC:
                            p_pFiffInfo->projs[countProj].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_PROJ_ITEM_ACTIVE:
                            p_pFiffInfo->projs[countProj].active = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_CH_NAME_LIST:
                            p_pFiffInfo->projs[countProj].data->col_names = FiffStream::split_name_list(t_pTag->toString());
                            break;
                        case FIFF_PROJ_ITEM_VECTORS:
                            //ToDo: Test; Float Matrix
                            p_pFiffInfo->projs[countProj].data->data = t_pTag->toFloatMatrix().transpose().cast<double>();
                            break;
                        }
                    }
                }
            }
        }
        // Check consisty
        for(qint32 i = 0; i < p_pFiffInfo->projs.size(); ++i)
        {
            if(p_pFiffInfo->projs[i].data->data.rows() != p_pFiffInfo->projs[i].data->nrow)
                p_pFiffInfo->projs[i].data->data.transposeInPlace();
        }

        //
        //    CTF compensation info
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP_DATA)
                {
                    FiffCtfComp comp;
                    qint32 countComp = p_pFiffInfo->comps.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP_DATA)
                    {
                        t_fiffStream.read_rt_tag(t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_MNE_CTF_COMP_KIND: //First comp -> create comp
                            comp = FiffCtfComp();
                            p_pFiffInfo->comps.append(comp);
                            p_pFiffInfo->comps[countComp].ctfkind = *(t_pTag->toInt());

                            if (p_pFiffInfo->comps[countComp].ctfkind == 1194410578) //hex2dec('47314252')
                                p_pFiffInfo->comps[countComp].kind = 1;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194476114) //hex2dec('47324252')
                                p_pFiffInfo->comps[countComp].kind = 2;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194541650) //hex2dec('47334252')
                                p_pFiffInfo->comps[countComp].kind = 3;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194479433)
                                p_pFiffInfo->comps[countComp].kind = 4;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194544969)
                                p_pFiffInfo->comps[countComp].kind = 5;
                            else
                                p_pFiffInfo->comps[countComp].kind = p_pFiffInfo->comps[countComp].ctfkind;
                            break;
                        case FIFF_MNE_CTF_COMP_CALIBRATED:
                            p_pFiffInfo->comps[countComp].save_calibrated = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_NROW:
                            p_pFiffInfo->comps[countComp].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_NCOL:
                            p_pFiffInfo->comps[countComp].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_ROW_NAMES:
                            row_names = t_pTag->toString();
                            if (!row_names.isEmpty())
                                p_pFiffInfo->comps[countComp].data->row_names = FiffStream::split_name_list(row_names);
                            break;
                        case FIFF_MNE_COL_NAMES:
                            col_names = t_pTag->toString();
                            if (!col_names.isEmpty())
                                p_pFiffInfo->comps[countComp].data->col_names = FiffStream::split_name_list(col_names);
                            break;
                        case FIFF_MNE_CTF_COMP_DATA:
                            //ToDo: Test; Float Matrix
                            p_pFiffInfo->comps[countComp].data->data = t_pTag->toFloatMatrix().transpose().cast<double>();
                            break;
                        }
                    }
                }
            }
        }
        //
        //    Bad channels
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_BAD_CHANNELS)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_BAD_CHANNELS)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_MNE_CH_NAME_LIST)
                    p_pFiffInfo->bads = FiffStream::split_name_list(t_pTag->data());
            }
        }
        //
        //    General
        //
        switch(t_pTag->kind)
        {
        case FIFF_SFREQ:
            p_pFiffInfo->sfreq = *(t_pTag->toFloat());
            break;
        case FIFF_LINE_FREQ:
            p_pFiffInfo->linefreq = *(t_pTag->toFloat());
            break;
        case FIFF_HIGHPASS:
            p_pFiffInfo->highpass = *(t_pTag->toFloat());
            break;
        case FIFF_LOWPASS:
            p_pFiffInfo->lowpass = *(t_pTag->toFloat());
            break;
        case FIFF_NCHAN:
            p_pFiffInfo->nchan = *(t_pTag->toInt());
            break;
        case FIFF_MEAS_DATE:
            p_pFiffInfo->meas_date[0] = t_pTag->toInt()[0];
            p_pFiffInfo->meas_date[1] = t_pTag->toInt()[1];
            break;
        case FIFF_PROJ_ID:
            p_pFiffInfo->proj_id = *t_pTag->toInt();
            break;
        case FIFF_PROJ_NAME:
            p_pFiffInfo->proj_name = t_pTag->toString();
            break;
        case FIFF_XPLOTTER_LAYOUT:
            p_pFiffInfo->xplotter_layout = t_pTag->toString();
            break;
        case FIFF_EXPERIMENTER:
            p_pFiffInfo->experimenter = t_pTag->toString();
            break;
        case FIFF_DESCRIPTION:
            p_pFiffInfo->description = t_pTag->toString();
            break;
        case FIFF_GANTRY_ANGLE:
            p_pFiffInfo->gantry_angle = *t_pTag->toInt();
            break;
        case FIFF_UTC_OFFSET:
            p_pFiffInfo->utc_offset = t_pTag->toString();
            break;
        }

        if (t_pTag->kind == FIFF_CH_INFO)
            p_pFiffInfo->chs.append(t_pTag->toChInfo());

        // END MEAS
        if(t_pTag->kind == FIFF_BLOCK_END && *t_pTag->toInt() == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_END FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockEnd = true;
        }
    }

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    for (qint32 c = 0; c < p_pFiffInfo->nchan; ++c)
        p_pFiffInfo->ch_names << p_pFiffInfo->chs[c].ch_name;

    return p_pFiffInfo;
}

//=============================================================================================================

Metadata RtDataClient::readMetadata()
{
    FiffDigitizerData::SPtr p_pDigData(new FiffDigitizerData());
    FiffInfo::SPtr p_pFiffInfo(new FiffInfo());

    bool t_bReadMeasBlockStart = false;
    bool t_bReadMeasBlockEnd = false;
    QString col_names, row_names;

    FiffStream t_fiffStream(this);
    //
    // Find the start
    //
    FiffTag::SPtr t_pTag;
    while(!t_bReadMeasBlockStart)
    {
        t_fiffStream.read_rt_tag(t_pTag);
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_START FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockStart = true;
        }
    }

    //
    // Parse until the endblock
    //

    bool dev_head_t_read = false;
    bool ctf_head_t_read = false;

    while(!t_bReadMeasBlockEnd)
    {
        t_fiffStream.read_rt_tag(t_pTag);
        //
        //  megacq parameters
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_DACQ_PARS)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_DACQ_PARS)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_DACQ_PARS)
                    p_pFiffInfo->acq_pars = t_pTag->toString();
                else if(t_pTag->kind == FIFF_DACQ_STIM)
                    p_pFiffInfo->acq_stim = t_pTag->toString();
            }
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (t_pTag->kind == FIFF_COORD_TRANS)
        {
            if (!dev_head_t_read)
            {
                p_pFiffInfo->dev_head_t = t_pTag->toCoordTrans();
                dev_head_t_read = true;
            }
            else if (!ctf_head_t_read)
            {
                p_pFiffInfo->ctf_head_t = t_pTag->toCoordTrans();
                ctf_head_t_read = true;
            }
        }
        //
        //    Polhemus data
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_ISOTRAK)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_ISOTRAK)
            {
                t_fiffStream.read_rt_tag(t_pTag);

                if(t_pTag->kind == FIFF_DIG_POINT){
                    p_pFiffInfo->dig.append(t_pTag->toDigPoint());
                    p_pDigData->points.append(t_pTag->toDigPoint());
                }
                if(t_pTag->kind == FIFF_MNE_COORD_FRAME){
                    p_pDigData->coord_frame = *t_pTag->toInt();
                }
            }
        }
        //
        //    Projectors
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM)
                {
                    FiffProj proj;
                    qint32 countProj = p_pFiffInfo->projs.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ_ITEM)
                    {
                        t_fiffStream.read_rt_tag(t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_NAME: // First proj -> Proj is created
                            proj = FiffProj();
                            p_pFiffInfo->projs.append(proj);
                            p_pFiffInfo->projs[countProj].desc = t_pTag->toString();
                            break;
                        case FIFF_PROJ_ITEM_KIND:
                            p_pFiffInfo->projs[countProj].kind = *(t_pTag->toInt());
                            break;
                        case FIFF_NCHAN: // First data -> FiffNamedMatrix is created
                            p_pFiffInfo->projs[countProj].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_NVEC:
                            p_pFiffInfo->projs[countProj].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_PROJ_ITEM_ACTIVE:
                            p_pFiffInfo->projs[countProj].active = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_CH_NAME_LIST:
                            p_pFiffInfo->projs[countProj].data->col_names = FiffStream::split_name_list(t_pTag->toString());
                            break;
                        case FIFF_PROJ_ITEM_VECTORS:
                            //ToDo: Test; Float Matrix
                            p_pFiffInfo->projs[countProj].data->data = t_pTag->toFloatMatrix().transpose().cast<double>();
                            break;
                        }
                    }
                }
            }
        }
        // Check consisty
        for(qint32 i = 0; i < p_pFiffInfo->projs.size(); ++i)
        {
            if(p_pFiffInfo->projs[i].data->data.rows() != p_pFiffInfo->projs[i].data->nrow)
                p_pFiffInfo->projs[i].data->data.transposeInPlace();
        }

        //
        //    CTF compensation info
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP_DATA)
                {
                    FiffCtfComp comp;
                    qint32 countComp = p_pFiffInfo->comps.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP_DATA)
                    {
                        t_fiffStream.read_rt_tag(t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_MNE_CTF_COMP_KIND: //First comp -> create comp
                            comp = FiffCtfComp();
                            p_pFiffInfo->comps.append(comp);
                            p_pFiffInfo->comps[countComp].ctfkind = *(t_pTag->toInt());

                            if (p_pFiffInfo->comps[countComp].ctfkind == 1194410578) //hex2dec('47314252')
                                p_pFiffInfo->comps[countComp].kind = 1;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194476114) //hex2dec('47324252')
                                p_pFiffInfo->comps[countComp].kind = 2;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194541650) //hex2dec('47334252')
                                p_pFiffInfo->comps[countComp].kind = 3;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194479433)
                                p_pFiffInfo->comps[countComp].kind = 4;
                            else if (p_pFiffInfo->comps[countComp].ctfkind == 1194544969)
                                p_pFiffInfo->comps[countComp].kind = 5;
                            else
                                p_pFiffInfo->comps[countComp].kind = p_pFiffInfo->comps[countComp].ctfkind;
                            break;
                        case FIFF_MNE_CTF_COMP_CALIBRATED:
                            p_pFiffInfo->comps[countComp].save_calibrated = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_NROW:
                            p_pFiffInfo->comps[countComp].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_NCOL:
                            p_pFiffInfo->comps[countComp].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_ROW_NAMES:
                            row_names = t_pTag->toString();
                            if (!row_names.isEmpty())
                                p_pFiffInfo->comps[countComp].data->row_names = FiffStream::split_name_list(row_names);
                            break;
                        case FIFF_MNE_COL_NAMES:
                            col_names = t_pTag->toString();
                            if (!col_names.isEmpty())
                                p_pFiffInfo->comps[countComp].data->col_names = FiffStream::split_name_list(col_names);
                            break;
                        case FIFF_MNE_CTF_COMP_DATA:
                            //ToDo: Test; Float Matrix
                            p_pFiffInfo->comps[countComp].data->data = t_pTag->toFloatMatrix().transpose().cast<double>();
                            break;
                        }
                    }
                }
            }
        }
        //
        //    Bad channels
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_BAD_CHANNELS)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_BAD_CHANNELS)
            {
                t_fiffStream.read_rt_tag(t_pTag);
                if(t_pTag->kind == FIFF_MNE_CH_NAME_LIST)
                    p_pFiffInfo->bads = FiffStream::split_name_list(t_pTag->data());
            }
        }
        //
        //    General
        //
        switch(t_pTag->kind)
        {
        case FIFF_SFREQ:
            p_pFiffInfo->sfreq = *(t_pTag->toFloat());
            break;
        case FIFF_LINE_FREQ:
            p_pFiffInfo->linefreq = *(t_pTag->toFloat());
            break;
        case FIFF_HIGHPASS:
            p_pFiffInfo->highpass = *(t_pTag->toFloat());
            break;
        case FIFF_LOWPASS:
            p_pFiffInfo->lowpass = *(t_pTag->toFloat());
            break;
        case FIFF_NCHAN:
            p_pFiffInfo->nchan = *(t_pTag->toInt());
            break;
        case FIFF_MEAS_DATE:
            p_pFiffInfo->meas_date[0] = t_pTag->toInt()[0];
            p_pFiffInfo->meas_date[1] = t_pTag->toInt()[1];
            break;
        case FIFF_PROJ_ID:
            p_pFiffInfo->proj_id = *t_pTag->toInt();
            break;
        case FIFF_PROJ_NAME:
            p_pFiffInfo->proj_name = t_pTag->toString();
            break;
        case FIFF_XPLOTTER_LAYOUT:
            p_pFiffInfo->xplotter_layout = t_pTag->toString();
            break;
        case FIFF_EXPERIMENTER:
            p_pFiffInfo->experimenter = t_pTag->toString();
            break;
        case FIFF_DESCRIPTION:
            p_pFiffInfo->description = t_pTag->toString();
            break;
        case FIFF_GANTRY_ANGLE:
            p_pFiffInfo->gantry_angle = *t_pTag->toInt();
            break;
        case FIFF_UTC_OFFSET:
            p_pFiffInfo->utc_offset = t_pTag->toString();
            break;
        }

        if (t_pTag->kind == FIFF_CH_INFO)
            p_pFiffInfo->chs.append(t_pTag->toChInfo());

        // END MEAS
        if(t_pTag->kind == FIFF_BLOCK_END && *t_pTag->toInt() == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_END FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockEnd = true;
        }
    }

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    for (qint32 c = 0; c < p_pFiffInfo->nchan; ++c)
        p_pFiffInfo->ch_names << p_pFiffInfo->chs[c].ch_name;

    p_pDigData->npoint = p_pDigData->points.size();

    for (int k = 0; k < p_pDigData->npoint; k++) {
        std::cout << "Apending true: " << k << "\n";
        p_pDigData->active.append(1);
        p_pDigData->discard.append(0);
    }

    Metadata data;

    data.info = p_pFiffInfo;
    data.dig = p_pDigData;

    return data;
}

//=============================================================================================================

void RtDataClient::readRawBuffer(qint32 p_nChannels,
                                 MatrixXf& data,
                                 fiff_int_t& kind)
{
    FiffStream t_fiffStream(this);
    //
    // Find the start
    //
    FiffTag::SPtr t_pTag;

    t_fiffStream.read_rt_tag(t_pTag);

    kind = t_pTag->kind;

    if(kind == FIFF_DATA_BUFFER)
    {
        qint32 nSamples = (t_pTag->size()/4)/p_nChannels;
        data = MatrixXf(Map< MatrixXf >(t_pTag->toFloat(), p_nChannels, nSamples));
    }
//        else
//            data = tag.data;
}

//=============================================================================================================

void RtDataClient::setClientAlias(const QString &p_sAlias)
{
    FiffStream t_fiffStream(this);
    t_fiffStream.write_rt_command(2, p_sAlias);//MNE_RT.MNE_RT_SET_CLIENT_ALIAS, alias);
    this->flush();
}
