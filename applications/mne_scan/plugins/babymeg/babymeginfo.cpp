//=============================================================================================================
/**
 * @file     babymeginfo.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Limin Sun, Lorenz Esch. All rights reserved.
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
 * @brief     BabyMEGInfo class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeginfo.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGInfo::BabyMEGInfo()
: chnNum(0)
, dataLength(0)
, sfreq(0)
, g_maxlen(500)
{
}
//=============================================================================================================

void BabyMEGInfo::MGH_LM_Send_CMDPackage(QByteArray DATA)
{
//    qDebug()<<"[BabyMEGInfo]CMD Size:"<<DATA.size();
    emit SendCMDPackage(DATA);
}
//=============================================================================================================

void BabyMEGInfo::MGH_LM_Send_DataPackage(QByteArray DATA)
{
//    qDebug()<<"[BabyMEGInfo]Data Size:"<<DATA.size();
    emit SendDataPackage(DATA);
}

//=============================================================================================================

QByteArray BabyMEGInfo::MGH_LM_Get_Field(QByteArray cmdstr)
{
    bool Start = false;
    qint32 bPos = 0;
    qint32 ePos = 0;
    qint32 cn = 0;
    for(qint32 i=0;i<cmdstr.size();i++)
    {
        if (cmdstr[i] == ':')
        { // get channel number
            Start = !Start;
            if (Start)
            {
                bPos = i;
            }
            else
            {
                ePos = i;
            }
            cn ++;
        }
        if (cn == 2)
        { // find the first ":" and the next ":"
            break;
        }

    }

    return cmdstr.mid(bPos,ePos-bPos);
}

//=============================================================================================================

QStringList BabyMEGInfo::MGH_LM_Exact_Single_Channel_Info(QByteArray cmdstr)
{
    QStringList sList;
    qint32 sp =0;
    qint32 ep =0;
    //extract single channel information by char ';'
    for (qint32 i=0;i<cmdstr.size();i++)
    {
        if (cmdstr[i]==';')
        {
            ep = i;
            QString t =  cmdstr.mid(sp,ep-sp);
            //qDebug()<<"[BabyMEGInfo] chan-name"<<t;
            sList.append(t);
            sp = i+1;
        }
    }

    return sList;
}

//=============================================================================================================

void BabyMEGInfo::MGH_LM_Get_Channel_Info(QByteArray cmdstr)
{
    //operation about lm_ch_names
    if (cmdstr[0]==':')
        cmdstr.remove(0,1);

    QStringList sList = MGH_LM_Exact_Single_Channel_Info(cmdstr);

    lm_ch_names.clear();
    lm_ch_scales.clear();
    lm_ch_pos1.clear();
    lm_ch_pos2.clear();
    lm_ch_pos3.clear();
    lm_ch_pos4.clear();
    lm_ch_pos5.clear();
    lm_ch_pos6.clear();
    lm_ch_pos7.clear();
    lm_ch_pos8.clear();
    lm_ch_pos9.clear();
    lm_ch_pos10.clear();
    lm_ch_pos11.clear();
    lm_ch_pos12.clear();
    lm_ch_coiltype.clear();
    lm_ch_calicoef.clear();
    lm_ch_gain.clear();

    // parse the information for each channel
    for(qint32 k =0; k<sList.size(); k++)
    {
        QString t = sList.at(k);
        for (qint32 z=0;z<t.size();z++)
        {
            if (t[z]=='|')
            {
                lm_ch_names.append(t.left(z));
                //qDebug()<<t.left(z);
                //extract the substring contained channel information: scale and coil positions
                QString tt = t.mid(z);
                //qDebug()<<tt;
                //extract value array from tt by separated char ","
                QStringList schp = tt.split(",");
                //qDebug()<<schp;
                // scale
                lm_ch_scales.append(schp.at(0));
                // positions : x,y,z and units of x,y,z
                lm_ch_pos1.append(schp.at(1));
                lm_ch_pos2.append(schp.at(2));
                lm_ch_pos3.append(schp.at(3));
                lm_ch_pos4.append(schp.at(4));
                lm_ch_pos5.append(schp.at(5));
                lm_ch_pos6.append(schp.at(6));
                lm_ch_pos7.append(schp.at(7));
                lm_ch_pos8.append(schp.at(8));
                lm_ch_pos9.append(schp.at(9));
                lm_ch_pos10.append(schp.at(10));
                lm_ch_pos11.append(schp.at(11));
                lm_ch_pos12.append(schp.at(12));
                //coil type
                lm_ch_coiltype.append(schp.at(13));
                //calibration coefficient
                lm_ch_calicoef.append(schp.at(14));
                //gain
                lm_ch_gain.append(schp.at(15));

//                qDebug()<<lm_ch_scales;
//                qDebug()<<lm_ch_pos2;
//                qDebug()<<"coiltype"<<lm_ch_coiltype<<"calicoef"<<lm_ch_calicoef;
            }
        }
    }
    return;
}

//=============================================================================================================

void BabyMEGInfo::MGH_LM_Parse_Para(QByteArray cmdstr)
{

    QByteArray CMD = cmdstr.left(4);
    if (CMD == "INFO")
    {
        //remove INFO
        cmdstr.remove(0,4);
        //ACQ the number of channels
        QByteArray T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        chnNum = T.toInt();
        //ACQ the length of data package
        T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        dataLength = T.toInt();
        // ACQ sampling rate
        T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        sfreq = T.toDouble();
        qDebug()<<"[babyMEGinfo] chnNum:" << chnNum << "Data Length" <<dataLength<<"sampling rate"<<sfreq;
        //qDebug()<<"cmdstr"<<cmdstr;
        // Start to acquire the channel's name and channel's scale
        MGH_LM_Get_Channel_Info(cmdstr);

    }
    else
    {
        chnNum = 464;
        dataLength = 5000;
        sfreq = 10000;
    }

    // Parameters
    m_FiffInfo.file_id = FiffId::new_file_id();

    m_FiffInfo.meas_date[0] = 0;
    m_FiffInfo.meas_date[1] = 0;
    m_FiffInfo.sfreq = sfreq;
    m_FiffInfo.highpass = 0;
    m_FiffInfo.lowpass = m_FiffInfo.sfreq/2;
    m_FiffInfo.acq_pars = QString("BabyMEG");
    m_FiffInfo.acq_stim = QString("");
    m_FiffInfo.filename = QString("");
    m_FiffInfo.meas_id = FiffId::new_file_id();
    m_FiffInfo.nchan = chnNum; //464;
    m_FiffInfo.dev_head_t.from =FIFFV_COORD_DEVICE;//1;  //* should be from dev to head 7/18/2016 Limin
    m_FiffInfo.dev_head_t.to =FIFFV_COORD_HEAD;//4;

    //set the identified matrix
    for (int li=0;li<4;li++)
        for(int lj=0;lj<4;lj++)
            if (li==lj) m_FiffInfo.dev_head_t.trans(li,lj) = 1.0f;
            else m_FiffInfo.dev_head_t.trans(li,lj) = 0.0f;

    //MEG
    for(qint32 i = 0; i < chnNum; i++)
    {
        FiffChInfo t_ch;

        t_ch.ch_name = lm_ch_names.at(i); //QString("MEG%1").arg(i);
        //qDebug()<<t_ch.ch_name;
        t_ch.scanNo = i;
        t_ch.logNo = i+1;
        t_ch.cal = lm_ch_calicoef.at(i).toFloat();
        t_ch.range =1.0f/lm_ch_gain.at(i).toFloat();//1; // set gain

        //qDebug()<<i<<":="<<t_ch.ch_name<<","<<t_ch.range<<","<<t_ch.cal;
        //t_ch.loc.setZero(12,1);

        //set loc
        t_ch.chpos.r0[0] = lm_ch_pos1.at(i).toDouble();
        t_ch.chpos.r0[1] = lm_ch_pos2.at(i).toDouble();
        t_ch.chpos.r0[2] = lm_ch_pos3.at(i).toDouble();
        t_ch.chpos.ex[0] = lm_ch_pos4.at(i).toDouble();
        t_ch.chpos.ex[1] = lm_ch_pos5.at(i).toDouble();
        t_ch.chpos.ex[2] = lm_ch_pos6.at(i).toDouble();
        t_ch.chpos.ey[0] = lm_ch_pos7.at(i).toDouble();
        t_ch.chpos.ey[1] = lm_ch_pos8.at(i).toDouble();
        t_ch.chpos.ey[2] = lm_ch_pos9.at(i).toDouble();
        t_ch.chpos.ez[0] = lm_ch_pos10.at(i).toDouble();
        t_ch.chpos.ez[1] = lm_ch_pos11.at(i).toDouble();
        t_ch.chpos.ez[2] = lm_ch_pos12.at(i).toDouble();

        //qDebug()<<t_ch.loc(0,0)<<t_ch.loc(1,0)<<t_ch.loc(2,0);

        int type = lm_ch_coiltype.at(i).toInt();
        int ntype = 0;

        if (type == FIFFV_COIL_BABY_MAG) //inner layer MEG
            ntype = 1;
        else if (type == FIFFV_COIL_BABY_REF_MAG)
            ntype = 2;
        else if (type == FIFFV_COIL_BABY_REF_MAG2)
            ntype = 3;
        else if (type == FIFFV_STIM_CH)
            ntype = 4;
        else if (type == FIFFV_EEG_CH)
            ntype = 5;
        else if (type >= FIFFV_QUAT_1 && type <= FIFFV_QUAT_6)
            ntype = 6;
        else if (type == FIFFV_HPI_G)
            ntype = 7;

        switch (ntype)
        {
        case 1: // inner layer meg sensors
            t_ch.kind = FIFFV_MEG_CH;
            t_ch.unit = FIFF_UNIT_T;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_BABY_MAG;
            break;
        case 2: // outer layer meg sensors
            t_ch.kind = FIFFV_REF_MEG_CH;
            t_ch.unit = FIFF_UNIT_T;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_BABY_REF_MAG;

            break;
        case 3: // reference meg sensors
            t_ch.kind = FIFFV_REF_MEG_CH;
            t_ch.unit = FIFF_UNIT_T;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_BABY_REF_MAG2;

            break;
        case 4: // trigger lines
            t_ch.kind = FIFFV_STIM_CH;
            t_ch.unit = FIFF_UNIT_V;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_STIM_CH;
            break;
        case 5: // EEG channels
            t_ch.kind = FIFFV_EEG_CH;
            t_ch.unit = FIFF_UNIT_V;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_EEG;

            break;
        case 6: // HPI channels
            t_ch.kind = type;
            t_ch.unit = FIFF_UNIT_V;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_NONE;

            break;
        case 7: // HPI G channels
            t_ch.kind = FIFFV_HPI_G;
            t_ch.unit = FIFF_UNIT_V;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_NONE;

            break;

        default: // other unknown type sensors
            t_ch.kind = FIFFV_MEG_CH;
            t_ch.unit = FIFF_UNIT_T;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.chpos.coil_type = FIFFV_COIL_NONE;

            break;
        }

        /*  Add the coiltrans for each sensor */
        /* x-axis normal vector */
        t_ch.coil_trans(0,0) = t_ch.chpos.ex[0];
        t_ch.coil_trans(1,0) = t_ch.chpos.ex[1];
        t_ch.coil_trans(2,0) = t_ch.chpos.ex[2];
        /* y-axis normal vector */
        t_ch.coil_trans(0,1) = t_ch.chpos.ey[0];
        t_ch.coil_trans(1,1) = t_ch.chpos.ey[1];
        t_ch.coil_trans(2,1) = t_ch.chpos.ey[2];
        /* z-axis normal vector */
        t_ch.coil_trans(0,2) = t_ch.chpos.ez[0];
        t_ch.coil_trans(1,2) = t_ch.chpos.ez[1];
        t_ch.coil_trans(2,2) = t_ch.chpos.ez[2];
        /* x,y,z coordinates */
        t_ch.coil_trans(0,3) = t_ch.chpos.r0[0];
        t_ch.coil_trans(1,3) = t_ch.chpos.r0[0];
        t_ch.coil_trans(2,3) = t_ch.chpos.r0[0];

        /* 0 0 0 1 */
        t_ch.coil_trans(3,0) = 0.0;
        t_ch.coil_trans(3,1) = 0.0;
        t_ch.coil_trans(3,2) = 0.0;
        t_ch.coil_trans(3,3) = 1.0;

        m_FiffInfo.chs.append(t_ch);
        m_FiffInfo.ch_names.append(t_ch.ch_name);

    }

    emit fiffInfoAvailable(m_FiffInfo);

    return;
}

//=============================================================================================================

void BabyMEGInfo::MGH_LM_Get_Channel_Infg(QByteArray cmdstr)
{
    //operation about lm_ch_names
    if (cmdstr[0]==':')
        cmdstr.remove(0,1);

    QStringList sList = MGH_LM_Exact_Single_Channel_Info(cmdstr);

    lm_ch_names.clear();
    lm_ch_gain.clear();

    // parse the information for each channel
    for(qint32 k =0; k<sList.size(); k++)
    {
        QString t = sList.at(k);
        for (qint32 z=0;z<t.size();z++)
        {
            if (t[z]=='|')
            {
                lm_ch_names.append(t.left(z));
                //qDebug()<<t.left(z);
                //extract the substring contained channel information: scale and coil positions
                QString tt = t.mid(z+1);
                qDebug()<<tt;
                //gain
                lm_ch_gain.append(tt);
                qDebug()<<t.left(z)<<"----"<<tt;

            }
        }
    }
    return;
}

//=============================================================================================================

void BabyMEGInfo::MGH_LM_Parse_Para_Infg(QByteArray cmdstr)
{

    QByteArray CMD = cmdstr.left(4);
    if (CMD == "INFG")
    {
        //remove INFG
        cmdstr.remove(0,4);
        //ACQ the number of channels
        QByteArray T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        chnNum = T.toInt();
        //ACQ the length of data package
        T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        dataLength = T.toInt();
        // ACQ sampling rate
        T = MGH_LM_Get_Field(cmdstr);
        cmdstr.remove(0,T.size());
        T.remove(0,1);
        sfreq = T.toDouble();
        qDebug()<<"[babyMEG_INFG] chnNum:" << chnNum << "Data Length" <<dataLength<<"sampling rate"<<sfreq;
        //qDebug()<<"cmdstr"<<cmdstr;
        // Start to acquire the channel's name and channel's scale
        MGH_LM_Get_Channel_Infg(cmdstr);

        //emit gain info
        emit GainInfoUpdate(lm_ch_gain);

    }

    return;
}

