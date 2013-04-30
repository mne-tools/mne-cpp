//=============================================================================================================
/**
* @file     babymeg.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
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
* @brief     implementation of the BabyMEG Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"
#include "babymegproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_types.h>
#include <utils/ioutils.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <mne/mne_epoch_data_list.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_pBabyMEGProducer(new BabyMEGProducer(this))
, m_sResourceDataPath("../../mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
, m_bIsRunning(false)
, m_uiBufferSampleSize(1000)
, m_pRawMatrixBuffer(NULL)
, pInfo(NULL)
{
    //BabyMEG Inits
    pInfo = new BabyMEGInfo();
    connect(pInfo, &BabyMEGInfo::fiffInfoAvailable, this, &BabyMEG::setFiffInfo);
    connect(pInfo, &BabyMEGInfo::SendDataPackage, this, &BabyMEG::setFiffData);

    myClient = new BabyMEGClient(6340,this);
    myClient->SetInfo(pInfo);
    myClient->start();
    myClientComm = new BabyMEGClient(6341,this);
    myClientComm->SetInfo(pInfo);
    myClientComm->start();

    myClientComm->SendCommandToBabyMEGShortConnection("INFO");

    myClient->ConnectToBabyMEG();
//    myClient->DisConnectBabyMEG();

    this->init();
}


//*************************************************************************************************************

BabyMEG::~BabyMEG()
{
    qDebug() << "Destroy BabyMEG::~BabyMEG()";

    if(m_pBabyMEGProducer)
        delete m_pBabyMEGProducer;

    if(myClient)
        delete myClient;
    if(myClientComm)
        delete myClientComm;
    if(pInfo)
        delete pInfo;

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void BabyMEG::comBufsize(Command p_command)
{
    //ToDO JSON

    quint32 t_uiBuffSize = p_command.pValues()[0].toUInt();

    if(t_uiBuffSize > 0)
    {
//        printf("bufsize %d\n", t_uiBuffSize);

            bool t_bWasRunning = m_bIsRunning;

            if(m_bIsRunning)
            {
                m_pBabyMEGProducer->stop();
                this->stop();
            }

            m_uiBufferSampleSize = t_uiBuffSize;

            if(t_bWasRunning)
                this->start();

        QString str = QString("\tSet %1 buffer sample size to %2 samples\r\n\n").arg(getName()).arg(t_uiBuffSize);

        m_commandManager["bufsize"].reply(str);
    }
    else
        m_commandManager["bufsize"].reply("Buffer size not set\r\n");
}


//*************************************************************************************************************

void BabyMEG::comGetBufsize(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if(t_bCommandIsJson)
    {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert("bufsize", QJsonValue((double)m_uiBufferSampleSize));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager["getbufsize"].reply(p_qJsonDocument.toJson());
    }
    else
    {
        QString str = QString("\t%1\r\n\n").arg(m_uiBufferSampleSize);
        m_commandManager["getbufsize"].reply(str);
    }
}


//*************************************************************************************************************

void BabyMEG::connectCommandManager()
{
    //Connect slots
    QObject::connect(&m_commandManager["bufsize"], &Command::executed, this, &BabyMEG::comBufsize);
    QObject::connect(&m_commandManager["getbufsize"], &Command::executed, this, &BabyMEG::comGetBufsize);
}


//*************************************************************************************************************

ConnectorID BabyMEG::getConnectorID() const
{
    return _BABYMEG;
}


//*************************************************************************************************************

const char* BabyMEG::getName() const
{
    return "BabyMEG";
}


//*************************************************************************************************************

void BabyMEG::setFiffInfo(FiffInfo p_FiffInfo)
{
    m_FiffInfoBabyMEG = p_FiffInfo;
}

//*************************************************************************************************************

void BabyMEG::setFiffData(QByteArray DATA)
{
    qDebug()<<"[BabyMEG]Data Size:"<<DATA.size();

    qint32 rows = m_FiffInfoBabyMEG.nchan;
    qint32 cols = (DATA.size()/8)/rows;

    Map<MatrixXd> rawData((double*)DATA.data(),rows,cols);

    for(qint32 i = 0; i < rows*cols; ++i)
        IOUtils::swap_doublep(rawData.data()+i);

    qDebug() << "Matrix " << rows << "x" << cols;

    std::cout << "first ten elements \n" << rawData.block(0,0,1,10) << std::endl;

}

//*************************************************************************************************************

void BabyMEG::init()
{

    ///////////////////////////////////////////////// OLD
//    if(m_pRawMatrixBuffer)
//        delete m_pRawMatrixBuffer;
//    m_pRawMatrixBuffer = NULL;

//    if(!m_RawInfo.isEmpty())
//        m_pRawMatrixBuffer = new RawMatrixBuffer(RAW_BUFFFER_SIZE, m_RawInfo.info.nchan, this->m_uiBufferSampleSize);
}


//*************************************************************************************************************

bool BabyMEG::start()
{
    this->init();

    // Start threads

    myClient->ConnectToBabyMEG();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::stop()
{

    myClient->DisConnectBabyMEG();

    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void BabyMEG::info(qint32 ID)
{
    if(m_FiffInfoBabyMEG.isEmpty())
        m_FiffInfoBabyMEG = pInfo->getFiffInfo();

    if(!m_FiffInfoBabyMEG.isEmpty())
        emit remitMeasInfo(ID, m_FiffInfoBabyMEG);
}


//*************************************************************************************************************

void BabyMEG::run()
{
    m_bIsRunning = true;

    quint32 uiSamplePeriod = 1000;
    quint32 count = 0;

    while(m_bIsRunning)
    {
//        MatrixXf tmp = m_pRawMatrixBuffer->pop();
        ++count;
//        printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

//        emit remitRawBuffer(tmp);
        usleep(uiSamplePeriod);
    }
}
