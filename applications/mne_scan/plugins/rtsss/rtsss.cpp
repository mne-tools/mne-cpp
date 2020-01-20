//=============================================================================================================
/**
 * @file     rtsss.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RtSss class.
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

using namespace RTSSSPLUGIN;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSss::RtSss()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
, LinRR(0)
, LoutRR(0)
, Lin(0)
, Lout(0)
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
    //qDebug() << "*********** Initialization ************";

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pRtSssBuffer.isNull())
        m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr();

    // Input
    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "RtSssIn", "RtSss input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &RtSss::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTMSAOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "RtSssOut", "RtSss output data");
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
    //qDebug() << "*********** Start ************";

//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtSss::stop()
{
    //qDebug() << "*********** Stop *************";

    //Wait until this thread is stopped
    m_qMutex.lock();
    m_bIsRunning = false;

    if(m_bProcessData)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pRtSssBuffer->releaseFromPop();
        m_pRtSssBuffer->releaseFromPush();

        m_pRtSssBuffer->clear();
    }

    m_bReceiveData = false;

    m_qMutex.unlock();

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
    return "RtSss";
}


//*************************************************************************************************************

QWidget* RtSss::setupWidget()
{
    //qDebug()  << "*********** SetUpWidget ************";

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


//*************************************************************************************************************

void RtSss::setLinRR(int val)
{
//   qDebug() <<" new LinRR: " << val;
    LinRR = val;
}


//*************************************************************************************************************

void RtSss::setLoutRR(int val)
{
//    qDebug()  <<" new LoutRR: " << val;
    LoutRR = val;
}


//*************************************************************************************************************

void RtSss::setLin(int val)
{
//    qDebug()  <<" new Lin: " << val;
    Lin = val;
}


//*************************************************************************************************************

void RtSss::setLout(int val)
{
//    qDebug()  <<" new Lout: " << val;
    Lout = val;
}


//*************************************************************************************************************

void RtSss::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
//    qDebug()  << "*********** Update ************";

    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA && m_bReceiveData)
    {
        //Check if buffer initialized
        if(!m_pRtSssBuffer)
            m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(32, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));

        //Fiff information
        if(!m_pFiffInfo)
            m_pFiffInfo = pRTMSA->info();

        if(m_bProcessData)
        {
            MatrixXd in_mat;
            for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
            {
                in_mat = pRTMSA->getMultiSampleArray()[i];
                m_pRtSssBuffer->push(&in_mat);
            }
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

    //Find index vector for wanted meg channels
    QStringList exclude;
    for(int i = 0; i < m_pFiffInfo->chs.size(); i++) {
        if(m_pFiffInfo->chs.at(i).chpos.coil_type != FIFFV_COIL_BABY_MAG)
            exclude<<m_pFiffInfo->chs.at(i).ch_name;
    }

    exclude<<m_pFiffInfo->bads;

//    qDebug()<< exclude ;

    QString chType("mag");
    RowVectorXi pickedChannels = m_pFiffInfo->pick_types(chType,false, false, QStringList(),exclude);
    qDebug()<< "finished pickedChannels";
    qint32 nmegchanused = pickedChannels.cols();

//    for(int i = 0; i < nmegchanused; i++)
//        std::cout << " pickedID= " << pickedChannels(i) <<", ";

//    qDebug()<< "number of picked channels for rtSSS= " << nmegchanused;

    // Initialize output
    m_pRTMSAOutput->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSAOutput->data()->setMultiArraySize(1);
    // m_pRTMSAOutput->data()->setSamplingRate(m_pFiffInfo->sfreq);
    m_pRTMSAOutput->data()->setVisibility(true);

    // Set MEG channel infomation to rtSSS
//    rsss.setMEGInfo(m_pFiffInfo);
//    std::cout << "****** coil trans ****** " << std::endl;
//    std::cout <<  m_pFiffInfo->chs[pickedChannels(1)].coil_trans << std::endl;
//    return;
    rsss.setMEGInfo(m_pFiffInfo, pickedChannels);
//    qDebug() << " setMEGInfo finished";

    // Get channel information
    qint32 nmegchan = rsss.getNumMEGChan();
    //qint32 nmegchanused = rsss.getNumMEGChanUsed();
    //qDebug() << "number of meg channels(run): " << nmegchan;
    VectorXi badch = rsss.getBadChan();

    // Load and set the number of spherical harmonics expansion
    //qDebug() << "LinRR (run): " << LinRR << ", LoutRR (run): " << LoutRR <<", Lin (run): " << Lin <<", Lout (run): " << Lout;
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
//    qint32 startID_MEGch = 0;
//    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
//        if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
//        {
//            startID_MEGch = i;
//            break;
//        }
    //qDebug() << "strat id: " << startID_MEGch;

    //  Build linear equation
    qDebug() << "building an initial SSS linear equation .....";
    lineqn = rsss.buildLinearEqn();

    //qDebug() << "..finished !!";

    // start processing data
    m_bProcessData = true;
    qint32 HEADMOV_COR_cnt = 15 ;
    qint32 cnt=0;
    //qDebug() << "rtSSS started.....";

    bool m_bIsHeadMov = true;

    while(m_bIsRunning)
    {
//        if (m_bIsHeadMov)
//        {
//            lineqn = rsss.buildLinearEqn();
            //qDebug() << "rebuilt SSS linear equation .....";
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

            //Generate new matrix from picked channels
            MatrixXd in_mat_used(pickedChannels.cols(), in_mat.cols());

            for(qint32 i = 0; i < in_mat_used.rows(); ++i)
                in_mat_used.row(i) = in_mat.row(pickedChannels(i));

//            //  Remove bad channel signals
//            MatrixXd in_mat_used(nmegchanused, in_mat.cols());
////            qDebug() << "size of in_mat_used (run): " << in_mat_used.rows() << " x " << in_mat_used.cols();
//            for(qint32 i = 0, k = 0; i < nmegchan; ++i)
//                if (badch(i) == 0)
//                {
//                    in_mat_used.row(k) = in_mat.row(i);
//                    k++;
//                }
//            in_mat_used = in_mat.block(0,0,nmegchanused,in_mat.cols());

           in_mat_used = rt_sss(in_mat_used);

            // Implement Concurrent mapreduced for parallel processing
            // divide the in_mat_used into 2 or 4 matrices, which renders 50ms or 25ms data
//            QList<MatrixXd> list_in_mat;
//            qint32 nSubSample = 20;
//            qint32 nThread = in_mat_used.cols() / nSubSample;

//            for(qint32 ith = 0; ith < nThread; ++ ith) {
//                if(ith*nSubSample+nSubSample < in_mat_used.cols())
//                    list_in_mat.append(in_mat_used.block(0,ith*nSubSample,nmegchanused,in_mat_used.cols()-(ith*nSubSample)));
//                else
//                    list_in_mat.append(in_mat_used.block(0,ith*nSubSample,nmegchanused,nSubSample));
//            }
//            qDebug() << "rtSSS split up done!";

//            QFuture<MatrixXd> res = QtConcurrent::mapped(list_in_mat, rt_sss);
//            res.waitForFinished();

//            qDebug() << "rtSSS mapped done!";

//            for(qint32 ith = 0; ith < nThread; ++ ith)
//                in_mat_used.block(0,ith*nSubSample,nmegchanused,nSubSample)= res.resultAt(ith);

            // Replace raw signal by SSS signal
            for(qint32 i = 0; i < in_mat_used.rows(); ++i) {
                in_mat.row(pickedChannels(i)) = in_mat_used.row(i);
//                qDebug() <<    in_mat.row(pickedChannels(i));
            }

            // Output to display
            m_pRTMSAOutput->data()->setValue(0.01* in_mat);

//            cnt++;
//            qDebug() << cnt << "   " ;
        }
    }

    m_bProcessData = false;
    m_bReceiveData = false;
    //qDebug() << "rtSSS stopped.";
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


