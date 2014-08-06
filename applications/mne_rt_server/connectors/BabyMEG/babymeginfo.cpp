//=============================================================================================================
/**
* @file     babymeginfo.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the BabyMEGInfo Class.
*
*/

#include "babymeginfo.h"



//*************************************************************************************************************

BabyMEGInfo::BabyMEGInfo()
: g_maxlen(500)
{
}
//*************************************************************************************************************

void BabyMEGInfo::MGH_LM_Send_CMDPackage(QByteArray DATA)
{
//    qDebug()<<"[BabyMEGInfo]CMD Size:"<<DATA.size();
    emit SendCMDPackage(DATA);
}
//*************************************************************************************************************

void BabyMEGInfo::MGH_LM_Send_DataPackage(QByteArray DATA)
{
//    qDebug()<<"[BabyMEGInfo]Data Size:"<<DATA.size();
    emit SendDataPackage(DATA);
}

//*************************************************************************************************************

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

//*************************************************************************************************************

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

//*************************************************************************************************************

void BabyMEGInfo::MGH_LM_Get_Channel_Info(QByteArray cmdstr)
{
    //operation about lm_ch_names
    if (cmdstr[0]==':')
        cmdstr.remove(0,1);

    QStringList sList = MGH_LM_Exact_Single_Channel_Info(cmdstr);

    lm_ch_names.clear();
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
            }
        }
    }
    return;
}

//*************************************************************************************************************

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
    m_FiffInfo.file_id.version = 0; //ToDo

    m_FiffInfo.meas_date[0] = 0;
    m_FiffInfo.meas_date[1] = 0;
    m_FiffInfo.sfreq = sfreq;
    m_FiffInfo.highpass = 0;
    m_FiffInfo.lowpass = m_FiffInfo.sfreq/2;
    m_FiffInfo.acq_pars = QString("BabyMEG");
    m_FiffInfo.acq_stim = QString("");
    m_FiffInfo.filename = QString("");
    m_FiffInfo.meas_id.version = 1;
    m_FiffInfo.nchan = chnNum; //464;

    //MEG
    for(qint32 i = 0; i < chnNum; i++)
    {
        FiffChInfo t_ch;

        t_ch.ch_name = lm_ch_names.at(i); //QString("MEG%1").arg(i);
        //qDebug()<<t_ch.ch_name;
        t_ch.scanno = i;
        t_ch.logno = i+1;
        t_ch.cal = 1;
        t_ch.range = 1;
        t_ch.loc.setZero(12,1);

        QString type = t_ch.ch_name.left(3);
        int ntype = 0;
        if (type == "MEG")
            ntype = 1;
        else if (type == "EEG")
            ntype = 2;
        switch (ntype)
        {
        case 1:
                t_ch.kind = FIFFV_MEG_CH;
                t_ch.unit = FIFF_UNIT_T;
                t_ch.unit_mul = FIFF_UNITM_NONE;
                t_ch.coil_type = FIFFV_COIL_BABY_MAG;// ToDo FIFFV_COIL_BABY_REF_MAG
            break;
        case 2:
                t_ch.kind = FIFFV_EEG_CH;
                t_ch.unit = FIFF_UNIT_V;
                t_ch.unit_mul = FIFF_UNITM_NONE;
                t_ch.coil_type = FIFFV_COIL_EEG;
            break;
        default:
            t_ch.kind = FIFFV_MEG_CH;
            t_ch.unit = FIFF_UNIT_T;
            t_ch.unit_mul = FIFF_UNITM_NONE;
            t_ch.coil_type = FIFFV_COIL_BABY_MAG;// ToDo FIFFV_COIL_BABY_REF_MAG

            break;
        }
        m_FiffInfo.chs.append(t_ch);
        m_FiffInfo.ch_names.append(t_ch.ch_name);
    }

    emit fiffInfoAvailable(m_FiffInfo);

    return;
}

//*************************************************************************************************************

void BabyMEGInfo::EnQueue(QByteArray DataIn)
{
    g_mutex.lock();
    if (g_queue.size() == g_maxlen) {
        qDebug() << "g_queue is full, waiting!";
        g_queueNotFull.wait(&g_mutex);
    }
    g_queue.enqueue(DataIn);
    g_queueNotEmpty.wakeAll();
    g_mutex.unlock();
    qDebug() << "Data In...[size="<<g_queue.size()<<"]";
}
//*************************************************************************************************************

QByteArray BabyMEGInfo::DeQueue()
{
    QMutexLocker locker(&g_mutex);
    if (g_queue.isEmpty()) {
    qDebug() << "g_queue empty, waiting!";
    g_queueNotEmpty.wait(&g_mutex);
    }
    QByteArray val = g_queue.dequeue();
    g_queueNotFull.wakeAll();
    qDebug() << "Data Out...[size="<<g_queue.size()<<"]";
    return val;
}
