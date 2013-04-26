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

void BabyMEGInfo::MGH_LM_Send_DataPackage(QByteArray DATA)
{
    qDebug()<<"Data Size:"<<DATA.size();
}


//*************************************************************************************************************

void BabyMEGInfo::MGH_LM_Parse_Para(QByteArray cmdstr)
{
    chnNum = 400;
    dataLength = 5000;



    // Parameters
    m_FiffInfo.file_id.version = 0; //ToDo

    m_FiffInfo.meas_date[0] = 0;
    m_FiffInfo.meas_date[1] = 0;
    m_FiffInfo.sfreq = 10000;
    m_FiffInfo.highpass = 0;
    m_FiffInfo.lowpass = m_FiffInfo.sfreq/2;
    m_FiffInfo.acq_pars = QString("BabyMEG");
    m_FiffInfo.acq_stim = QString("");
    m_FiffInfo.filename = QString("");
    m_FiffInfo.meas_id.version = 1;
    m_FiffInfo.nchan = 464;

    //MEG
    for(qint32 i = 0; i < 400; ++i)
    {
        FiffChInfo t_ch;

        t_ch.scanno = i;
        t_ch.logno = i;
        t_ch.cal = 1;
        t_ch.kind = FIFFV_MEG_CH;
        t_ch.range = 1;
        t_ch.unit = FIFF_UNITM_T;
        t_ch.unit_mul = FIFF_UNITM_NONE;
        t_ch.coil_type = FIFFV_COIL_BABY_MAG;// ToDo FIFFV_COIL_BABY_REF_MAG

        t_ch.loc.setZero(12,1);

        t_ch.ch_name = QString("MEG%1").arg(i);

        m_FiffInfo.chs.append(t_ch);

        m_FiffInfo.ch_names.append(t_ch.ch_name);
    }
    //EEG
    for(qint32 i = 0; i < 64; ++i)
    {
        FiffChInfo t_ch;

        t_ch.scanno = 400+i;
        t_ch.logno = 400+i;
        t_ch.cal = 1;
        t_ch.kind = FIFFV_EEG_CH;
        t_ch.range = 1;
        t_ch.unit = FIFF_UNIT_V;
        t_ch.unit_mul = FIFF_UNITM_NONE;
        t_ch.coil_type = FIFFV_COIL_EEG;
        t_ch.loc.setZero(12,1);

        t_ch.ch_name = QString("EEG%1").arg(i);

        m_FiffInfo.chs.append(t_ch);

        m_FiffInfo.ch_names.append(t_ch.ch_name);
    }


    emit fiffInfoAvailable(m_FiffInfo);

    return;
}
/*
void BabyMEGInfo::EnQueue(QByteArray DataIn)
{
    g_mutex.lock();
    if (g_queue.size() == g_maxlen) {
        qDebug() << "g_queue is full, data lost!";
    }
    else
    {
        g_queue.enqueue(DataIn);
        qDebug() << "Data In...[size="<<g_queue.size()<<"]";
    }
    g_mutex.unlock();

}
QByteArray BabyMEGInfo::DeQueue()
{
    QByteArray val;
    g_mutex.lock();
    if (g_queue.isEmpty()) {
        qDebug() << "g_queue empty, no data acquired!";
        val.clear();
    }
    else
    {
        val = g_queue.dequeue();

    }
    qDebug() << "Data Out...[size="<<g_queue.size()<<"]";
    g_mutex.unlock();
    return val;
}
*/
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
