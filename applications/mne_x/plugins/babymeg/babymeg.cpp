//=============================================================================================================
/**
* @file     babymeg.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BabyMEG class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"

#include "FormFiles/babymegsetupwidget.h"

#include <utils/ioutils.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QList>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_iBufferSize(-1)
, m_pRawMatrixBuffer(0)
, m_bIsRunning(false)
{

}


//*************************************************************************************************************

BabyMEG::~BabyMEG()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> BabyMEG::clone() const
{
    QSharedPointer<BabyMEG> pBabyMEGClone(new BabyMEG());
    return pBabyMEGClone;
}


//*************************************************************************************************************

void BabyMEG::init()
{
    //BabyMEG Inits
    pInfo = QSharedPointer<BabyMEGInfo>(new BabyMEGInfo());
    connect(pInfo.data(), &BabyMEGInfo::fiffInfoAvailable, this, &BabyMEG::setFiffInfo);
    connect(pInfo.data(), &BabyMEGInfo::SendDataPackage, this, &BabyMEG::setFiffData);
    connect(pInfo.data(), &BabyMEGInfo::SendCMDPackage, this, &BabyMEG::setCMDData);

    myClient = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6340,this));
    myClient->SetInfo(pInfo);
    myClient->start();
    myClientComm = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6341,this));
    myClientComm->SetInfo(pInfo);
    myClientComm->start();

    myClientComm->SendCommandToBabyMEGShortConnection("INFO");


    myClient->ConnectToBabyMEG();

//    myClient->ConnectToBabyMEG();
////    myClient->DisConnectBabyMEG();
//    m_bIsRunning = true;
//    QThread::start();

    //init channels when fiff info is available
    connect(this, &BabyMEG::fiffInfoAvailable, this, &BabyMEG::initConnector);
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void BabyMEG::initConnector()
{


    qDebug() << "BabyMEG::init()";

//    if(m_pFiffInfo)
//    {
////        m_pFiffInfo->sfreq /= 100;
//        m_pRTMSA_BabyMEG = addProviderRealTimeMultiSampleArray_New(MSR_ID::MEGBabyMEG_OUTPUT);
//        m_pRTMSA_BabyMEG->initFromFiffInfo(m_pFiffInfo);
//        m_pRTMSA_BabyMEG->setMultiArraySize(10);
//    }


    if(m_pFiffInfo)
    {
        m_pRTMSABabyMEG = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "RtClient", "MNE Rt Client");

        m_pRTMSABabyMEG->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSABabyMEG->data()->setMultiArraySize(10);

        m_pRTMSABabyMEG->data()->setVisibility(true);

        m_outputConnectors.append(m_pRTMSABabyMEG);
    }

}


//*************************************************************************************************************

void BabyMEG::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}

//*************************************************************************************************************

void BabyMEG::setFiffData(QByteArray DATA)
{
//    qDebug() << "[BabyMEG] Data Size:"<<DATA.size();

    //get the first byte -- the data format
    int dformat = DATA.left(1).toInt();

    DATA.remove(0,1);
    qint32 rows = m_pFiffInfo->nchan;
    qint32 cols = (DATA.size()/dformat)/rows;

    qDebug() << "[BabyMEG] Matrix " << rows << "x" << cols << " [Data bytes:" << dformat << "]";

    MatrixXf rawData(Map<MatrixXf>( (float*)DATA.data(),rows, cols ));

    for(qint32 i = 0; i < rows*cols; ++i)
        IOUtils::swap_floatp(rawData.data()+i);



    if(m_bIsRunning)
    {
        if(!m_pRawMatrixBuffer)
            m_pRawMatrixBuffer = CircularMatrixBuffer<float>::SPtr(new CircularMatrixBuffer<float>(40, rows, cols));

        m_pRawMatrixBuffer->push(&rawData);
    }
    else
    {
//        std::cout << "Data coming" << std::endl; //"first ten elements \n" << rawData.block(0,0,1,10) << std::endl;

        emit DataToSquidCtrlGUI(rawData);
    }
}


//*************************************************************************************************************

void BabyMEG::setFiffInfo(FiffInfo p_FiffInfo)
{
    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(p_FiffInfo));

    m_iBufferSize = pInfo->dataLength;
}

//*************************************************************************************************************

void BabyMEG::setCMDData(QByteArray DATA)
{
    qDebug()<<"------"<<DATA;
//    m_commandManager["FLL"].reply(DATA);
    emit SendCMDDataToSQUIDControl(DATA);
    qDebug()<<"Data has been received.";
}


//*************************************************************************************************************
void BabyMEG::comFLL(QString t_sFLLControlCommand)
{
    qDebug()<<"FLL commands";

    qDebug() << "BabyMeg Received" << t_sFLLControlCommand;
    int strlen = t_sFLLControlCommand.size();
    QByteArray Scmd = myClientComm->MGH_LM_Int2Byte(strlen);
    QByteArray SC = QByteArray("COMS")+Scmd;
    SC.append(t_sFLLControlCommand);
    myClientComm->SendCommandToBabyMEGShortConnection(SC);
}


//*************************************************************************************************************

bool BabyMEG::start()
{/*
    // Initialize real time measurements
    init();*/

    // Start threads
    m_bIsRunning = true;
    myClient->ConnectToBabyMEG();

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::stop()
{
    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pRawMatrixBuffer->clear();

    myClient->DisConnectBabyMEG();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType BabyMEG::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString BabyMEG::getName() const
{
    return "BabyMEG";
}


//*************************************************************************************************************

QWidget* BabyMEG::setupWidget()
{
    BabyMEGSetupWidget* widget = new BabyMEGSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog

    return widget;
}


//*************************************************************************************************************

void BabyMEG::run()
{

    MatrixXf matValue;
    while(true)
    {
        if(m_pRawMatrixBuffer)
        {
            //pop matrix
            matValue = m_pRawMatrixBuffer->pop();

            //emit values
            for(qint32 i = 0; i < matValue.cols(); ++i)
                m_pRTMSABabyMEG->data()->setValue(matValue.col(i).cast<double>());
        }
    }
}
