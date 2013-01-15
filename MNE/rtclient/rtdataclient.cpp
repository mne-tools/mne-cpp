//=============================================================================================================
/**
* @file     rtdataclient.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           To Be continued...
*
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
* @brief     implementation of the RtDataClient Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdataclient.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCLIENTLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtDataClient::RtDataClient(QObject *parent)
: QTcpSocket(parent)
, m_clientID(-1)
{
    getClientId();
}


//*************************************************************************************************************

void RtDataClient::connectToHost(const QString& p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4218);
}


//*************************************************************************************************************

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
        FiffTag* t_pTag = NULL;
        FiffTag::read_tag(&t_fiffStream, t_pTag);
        if (t_pTag->kind == FIFF_MNE_RT_CLIENT_ID)
            m_clientID = *t_pTag->toInt();

        delete t_pTag;
    }
    return m_clientID;
}


//*************************************************************************************************************

FiffInfo RtDataClient::readInfo()
{
    FiffInfo p_FiffInfo;
    bool t_bReadMeasBlockStart = false;
    bool t_bReadMeasBlockEnd = false;

    FiffStream t_fiffStream(this);
    //
    // Find the start
    //
    FiffTag* t_pTag = NULL;
    while(!t_bReadMeasBlockStart)
    {
        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
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
        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
        //
        //  megacq parameters
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_DACQ_PARS)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_DACQ_PARS)
            {
                FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                if(t_pTag->kind == FIFF_DACQ_PARS)
                    p_FiffInfo.acq_pars = t_pTag->toString();
                else if(t_pTag->kind == FIFF_DACQ_STIM)
                    p_FiffInfo.acq_stim = t_pTag->toString();
            }
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (t_pTag->kind == FIFF_COORD_TRANS)
        {
            if (!dev_head_t_read)
            {
                p_FiffInfo.dev_head_t = t_pTag->toCoordTrans();
                dev_head_t_read = true;
            }
            else if (!ctf_head_t_read)
            {
                p_FiffInfo.ctf_head_t = t_pTag->toCoordTrans();
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
                FiffTag::read_rt_tag(&t_fiffStream, t_pTag);

                if(t_pTag->kind == FIFF_DIG_POINT)
                    p_FiffInfo.dig.append(t_pTag->toDigPoint());
            }
        }
        //
        //    Projectors
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ)
            {
                FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM)
                {
                    FiffProj proj;
                    qint32 countProj = p_FiffInfo.projs.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ_ITEM)
                    {
                        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_NAME: // First proj -> Proj is created
                            proj = FiffProj();
                            p_FiffInfo.projs.append(proj);
                            p_FiffInfo.projs[countProj].desc = t_pTag->toString();
                            break;
                        case FIFF_PROJ_ITEM_KIND:
                            p_FiffInfo.projs[countProj].kind = *(t_pTag->toInt());
                            break;
                        case FIFF_NCHAN: // First data -> FiffNamedMatrix is created
//                                p_FiffInfo.projs[countProj].data = FiffNamedMatrix();//obsolete
                            p_FiffInfo.projs[countProj].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_NVEC:
                            p_FiffInfo.projs[countProj].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_PROJ_ITEM_ACTIVE:
                            p_FiffInfo.projs[countProj].active = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_CH_NAME_LIST:
                            p_FiffInfo.projs[countProj].data->col_names = FiffStream::split_name_list(t_pTag->toString());
                            break;
                        case FIFF_PROJ_ITEM_VECTORS:
                            p_FiffInfo.projs[countProj].data->data = t_pTag->toFloatMatrix();
                            break;
                        }
                    }
                }
            }
        }
        //
        //    CTF compensation info
        //
        if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP)
        {
            while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP)
            {
                FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP_DATA)
                {
                    FiffCtfComp comp;
                    qint32 countComp = p_FiffInfo.comps.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP_DATA)
                    {
                        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_MNE_CTF_COMP_KIND: //First comp -> create comp
                            comp = FiffCtfComp();
                            p_FiffInfo.comps.append(comp);
                            p_FiffInfo.comps[countComp].ctfkind = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_CTF_COMP_CALIBRATED:
                            p_FiffInfo.comps[countComp].save_calibrated = *(t_pTag->toInt());
                            break;
//                            case FIFF_MNE_CTF_COMP_DATA:
//                                p_FiffInfo.comps[countComp]->data = *(t_pTag->toNamedMatrix());
//                                break;
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
                FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                if(t_pTag->kind == FIFF_MNE_CH_NAME_LIST)
                    p_FiffInfo.bads = FiffStream::split_name_list(t_pTag->data());
            }
        }
        //
        //    General
        //
        switch(t_pTag->kind)
        {
        case FIFF_SFREQ:
            p_FiffInfo.sfreq = *(t_pTag->toFloat());
            break;
        case FIFF_HIGHPASS:
            p_FiffInfo.highpass = *(t_pTag->toFloat());
            break;
        case FIFF_LOWPASS:
            p_FiffInfo.lowpass = *(t_pTag->toFloat());
            break;
        case FIFF_NCHAN:
            p_FiffInfo.nchan = *(t_pTag->toInt());
            break;
        case FIFF_MEAS_DATE:
            p_FiffInfo.meas_date[0] = t_pTag->toInt()[0];
            p_FiffInfo.meas_date[1] = t_pTag->toInt()[1];
            break;
        }

        if (t_pTag->kind == FIFF_CH_INFO)
            p_FiffInfo.chs.append(t_pTag->toChInfo());

        // END MEAS
        if(t_pTag->kind == FIFF_BLOCK_END && *t_pTag->toInt() == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_END FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockEnd = true;
        }
    }
    return p_FiffInfo;
}


//*************************************************************************************************************

void RtDataClient::readRawBuffer(qint32 p_nChannels, MatrixXf& data, fiff_int_t& kind)
{
//        data = [];

    FiffStream t_fiffStream(this);
    //
    // Find the start
    //
    FiffTag* t_pTag = NULL;

    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);

    kind = t_pTag->kind;

    if(kind == FIFF_DATA_BUFFER)
    {
        qint32 nSamples = (t_pTag->size()/4)/p_nChannels;
        data = MatrixXf(Map< MatrixXf >(t_pTag->toFloat(), p_nChannels, nSamples));
    }
//        else
//            data = tag.data;
}


//*************************************************************************************************************

void RtDataClient::setClientAlias(const QString &p_sAlias)
{
    FiffStream t_fiffStream(this);
    t_fiffStream.write_rt_command(2, p_sAlias);//MNE_RT.MNE_RT_SET_CLIENT_ALIAS, alias);
    this->flush();
}
