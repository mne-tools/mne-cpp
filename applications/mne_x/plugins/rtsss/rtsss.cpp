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
#include "rtsssalgo.h"
#include "FormFiles/rtssssetupwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent/QtConcurrentMap>
#include <QSettings>

RtSssAlgo rsss;

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
            m_pFiffInfo = pRTMSA->info();

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


MatrixXd rt_sss(const MatrixXd &p_mat)
{
    MatrixXd out;
    out = rsss.getSSSRR(p_mat);
    return out;
}


//*************************************************************************************************************

void RtSss::run()
{
//    QList<MatrixXd> lineqn;
    MatrixXd lineqn;

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

//    // Find out if coils are all gradiometers, all magnetometers, or both.
//    // When both gradiometers and magnetometers are used,
//    //      MagScale facor of 100 must be appiled to magnetomters.
//    float MagScale;
//    if ((0 < CoilGrad.sum()) && (CoilGrad.sum() < NumCoil))  MagScale = 100;
//    else MagScale = 1;

//    VectorXd CoilScale;
//    CoilScale.setOnes(NumCoil);
//    for(int i=0; i<NumCoil; i++)
//    {
//        if (CoilGrad(i) == 0) CoilScale(i) = MagScale;
////        std::cout <<  "i=" << i << "CoilGrad: " << CoilGrad(i) << ",  CoilScale: " << CoilScale(i) << std::endl;
//    }

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
    qint32 HEADMOV_COR_cnt = 15 ;
    qint32 cnt=0;
    qDebug() << "rtSSS started.....";

    bool m_bIsHeadMov = true;

    while(m_bIsRunning)
    {
//        if (m_bIsHeadMov)
//        {
            lineqn = rsss.buildLinearEqn();
            qDebug() << "rebuilt SSS linear equation .....";
//            m_bIsHeadMov = false;
//        }
//        else
//        {
//            m_bIsHeadMov = true;
//        }

        qint16 nrows = m_pRtSssBuffer->rows();

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
//            in_mat_used = in_mat.block(0,0,nmegchanused,in_mat.cols());

            // Implement Concurrent mapreduced for parallel processing
            // divide the in_mat_used into 2 or 4 matrices, which renders 50ms or 25ms data
            QList<MatrixXd> list_in_mat;
            qint32 nSubSample = 20;
            qint32 nThread = in_mat.cols() / nSubSample;

            for(qint32 ith = 0; ith < nThread; ++ ith)
                list_in_mat.append(in_mat_used.block(0,ith*nSubSample,nmegchanused,nSubSample));

            QFuture<MatrixXd> res = QtConcurrent::mapped(list_in_mat, rt_sss);
            res.waitForFinished();

            for(qint32 ith = 0; ith < nThread; ++ ith)
                in_mat_used.block(0,ith*nSubSample,nmegchanused,nSubSample)= res.resultAt(ith);

            // Replace raw signal by SSS signal
            for(qint32 i = 0, k = 0; i < nmegchan; ++i)
                if (badch(i) == 0)
                {
                    in_mat.row(i) = in_mat_used.row(k);
                    k++;
                }

            // Output to display
            for(qint32 i = 0; i <in_mat.cols(); ++i)
                m_pRTMSAOutput->data()->setValue(1e7 * in_mat.col(i));
//                m_pRTMSAOutput->data()->setValue(1e-16 * in_mat.col(i));

            cnt++;
            qDebug() << cnt;
        }
    }

    m_bProcessData = false;
    m_bReceiveData = false;
    qDebug() << "rtSSS stopped.";
}



//
// A possible way of passing parameters to QtConcurrent::mapped

//QFuture<MatrixXd> thumbnails = QtConcurrent::mapped(list_in_mat, rsss.getSSSRR);


//class obj
//{
//    MatrixXd data;
//    int lineq1;
//    int lineq2;
//    int lineq3;
//    int lineq4;

//    MatrixXd getSSSRR()
//    {

//        lineq1;
//        lineq2;
//        lineq3;
//        lineq4;
//        MatrixXd res;
//        return res;
//    }
//};


//QList<obj> qListObj;

////            {
//obj obj0;
//obj0.data = list_in_mat(0);
//obj0.lineq1 = 0; obj0.lineq2 = 0; obj0.lineq3 = 0; obj0.lineq4 = 0;

//obj obj1;
//obj1.data = list_in_mat(1);
//obj1.lineq1 = 0; obj1.lineq2 = 0; obj1.lineq3 = 1; obj1.lineq4 = 0;

//qListObj << obj0;
//qListObj << obj1;
////            }

//QFuture<MatrixXd> thumbnails = QtConcurrent::mapped(qListObj, &obj::getSSSRR);


