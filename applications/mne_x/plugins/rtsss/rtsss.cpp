//=============================================================================================================
/**
* @file     rtsss.cpp
* @author   Seok Lew <slew@nmr.mgh.harvard.edu>;
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
* @brief    Contains the implementation of the RtSss class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsss.h"
#include "FormFiles/rtssssetupwidget.h"
#include "rtsssalgo.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent/QtConcurrentMap>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RtSssPlugin;
using namespace FIFFLIB;
using namespace MNEX;
using namespace XMEASLIB;



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSss::RtSss()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
{
}


//*************************************************************************************************************

RtSss::~RtSss()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> RtSss::clone() const
{
    QSharedPointer<RtSss> pRtSssClone(new RtSss);
    return pRtSssClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void RtSss::init()
{
    qDebug() << "*********** Initialization ************";

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pRtSssBuffer.isNull())
        m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr();

    // Input
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "RtSssIn", "RtSss input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &RtSss::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTMSAOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "RtSssOut", "RtSss output data");
    m_outputConnectors.append(m_pRTMSAOutput);


    //
    // Load Settings
    //
//    QSettings settings;
//    yourValue = settings.value(QString("Plugin/%1/yourValue").arg(this->getName()), 400).toInt();
}


//*************************************************************************************************************

void RtSss::unload()
{
    //
    // Store Settings
    //
//    QSettings settings;
//    settings.setValue(QString("Plugin/%1/yourValue").arg(this->getName()), yourValue);

}


//*************************************************************************************************************

bool RtSss::start()
{
    qDebug() << "*********** Start ************";

    QThread::start();
    return true;
}


//*************************************************************************************************************

bool RtSss::stop()
{
    qDebug() << "*********** Stop *************";

    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();

    if(m_pRtSssBuffer)
        m_pRtSssBuffer->clear();

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType RtSss::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString RtSss::getName() const
{
    return "RtSss Toolbox";
}


//*************************************************************************************************************

QWidget* RtSss::setupWidget()
{
    qDebug()  << "*********** SetUpWidget ************";

    RtSssSetupWidget* widget = new RtSssSetupWidget(this);  //widget is later distroyed by CentralWidget - so it has to be created everytime new

    connect(widget, &RtSssSetupWidget::signalNewLinRR, this, &RtSss::setLinRR);
    connect(widget, &RtSssSetupWidget::signalNewLoutRR, this, &RtSss::setLoutRR);
    connect(widget, &RtSssSetupWidget::signalNewLin, this, &RtSss::setLin);
    connect(widget, &RtSssSetupWidget::signalNewLout, this, &RtSss::setLout);

    LinRR = widget->getLinRR();
    LoutRR = widget->getLoutRR();
    Lin = widget->getLin();
    Lout = widget->getLout();

    return widget;
}


void RtSss::setLinRR(int val)
{
//   qDebug() <<" new LinRR: " << val;
    LinRR = val;
}

void RtSss::setLoutRR(int val)
{
//    qDebug()  <<" new LoutRR: " << val;
    LoutRR = val;
}

void RtSss::setLin(int val)
{
//    qDebug()  <<" new Lin: " << val;
    Lin = val;
}

void RtSss::setLout(int val)
{
//    qDebug()  <<" new Lout: " << val;
    Lout = val;
}

//*************************************************************************************************************

void RtSss::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
//    qDebug()  << "*********** Update ************";

    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA && m_bReceiveData)
    {
        //Check if buffer initialized
        if(!m_pRtSssBuffer)
            m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information
        if(!m_pFiffInfo)
            m_pFiffInfo = pRTMSA->getFiffInfo();

        if(m_bProcessData)
        {
            MatrixXd in_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());
            for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
                in_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

            m_pRtSssBuffer->push(&in_mat);
        }
    }
}


//*************************************************************************************************************

void RtSss::run()
{

    RtSssAlgo rsss;
    QList<MatrixXd> lineqn, sssOut;

    m_bIsRunning = true;

    // start receiving data
    //
    m_bReceiveData = true;

    // Read Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    // Initialize output
    m_pRTMSAOutput->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSAOutput->data()->setMultiArraySize(100);
    // m_pRTMSAOutput->data()->setSamplingRate(m_pFiffInfo->sfreq);
    m_pRTMSAOutput->data()->setVisibility(true);

    // Set MEG channel infomation to rtSSS
    rsss.setMEGInfo(m_pFiffInfo);

    // Get channel information
    qint32 nmegchan = rsss.getNumMEGChan();
    qint32 nmegchanused = rsss.getNumMEGChanUsed();
    qDebug() << "number of meg channels(run): " << nmegchan;
    VectorXi badch = rsss.getBadChan();

    // Load and set the number of spherical harmonics expansion
    qDebug() << "LinRR (run): " << LinRR << ", LoutRR (run): " << LoutRR <<", Lin (run): " << Lin <<", Lout (run): " << Lout;
    QList<int> expOrder;
    expOrder << LinRR << LoutRR << Lin << Lout;
    rsss.setSSSParameter(expOrder);

    // Find a starting MEG channel index fiff
    // When the MEG recording of the first channel is saved starting from the first row in the signal matrix, the startID_MEGch will be 0.
    // When the MEG recording of the first channel is saved starting not from the first row and being followed by others like EEG,
    //   the startID_MEGch will be the first encounter of MEG channel other than zero.
    //
    qint32 startID_MEGch = 0;
    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
        {
            startID_MEGch = i;
            break;
        }
//    qDebug() << "strat id: " << startID_MEGch;

    //  Build linear equation
    qDebug() << "building SSS linear equation .....";
    lineqn = rsss.buildLinearEqn();

    qDebug() << "..finished !!";

    // start processing data
    m_bProcessData = true;
    qint32 HEADMOV_COR_cnt = 2 ;

    qDebug() << "rtSSS started.....";
    while(m_bIsRunning)
    {
        // When new head movement correction presented, lineqn must be rebuilt for rtSSS
        if (HEADMOV_COR_cnt == 2)
        {
            lineqn = rsss.buildLinearEqn();
            qDebug() << "rebuilt SSS linear equation .....";
            HEADMOV_COR_cnt = 0;
        }
        else HEADMOV_COR_cnt++;

        qint32 nrows = m_pRtSssBuffer->rows();
        qDebug() << "rtsss";
        if(nrows > 0) // check if init
        {
            // * Dispatch the inputs * //
            MatrixXd in_mat = m_pRtSssBuffer->pop();
//            qDebug() << "size of in_mat (run): " << in_mat.rows() << " x " << in_mat.cols();

            //  Remove bad channel signals
            MatrixXd in_mat_used(nmegchanused, in_mat.cols());
//            qDebug() << "size of in_mat_used (run): " << in_mat_used.rows() << " x " << in_mat_used.cols();
            for(qint32 i = 0, k = 0; i < nmegchan; ++i)
                if (badch(i) == 0)
                {
                    in_mat_used.row(k) = in_mat.row(i);
                    k++;
                }

            sssOut = rsss.getSSSRR(lineqn[0], lineqn[1], lineqn[2], lineqn[3], lineqn[4]*in_mat_used);
//            sssOut = rsss.getSSSOLS(lineqn[0], lineqn[1], lineqn[3], lineqn[4]*in_mat_used);
//            sssRR = rsss.getSSSRR(lineqn[0], lineqn[1], lineqn[2], lineqn[3], lineqn[4]*in_mat_used.block(startID_MEGch,0,nmegchanused,in_mat_used.cols()));
//            sssRR = rsss.getSSSRR(lineqn[0], lineqn[1], lineqn[2], lineqn[3], lineqn[4]*in_mat.block(startID_MEGch,0,nmegchan,in_mat.cols()));
//            qDebug() <<  "size of sssRR[0] (run): " << sssRR[0].rows() << " x " << sssRR[0].cols();

            // Replace raw signal by SSS signal
            for(qint32 i = 0, k = 0; i < nmegchan; ++i)
                if (badch(i) == 0)
                {
                    in_mat.row(i) = sssOut[0].row(k);
                    k++;
                }

            // Display signal after SSS
            for(qint32 i = 0; i <in_mat.cols(); ++i)
                m_pRTMSAOutput->data()->setValue(in_mat.col(i));
        }
    }

    m_bProcessData = false;
    m_bReceiveData = false;
    qDebug() << "rtSSS stopped.";
}
