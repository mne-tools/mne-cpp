//=============================================================================================================
/**
* @file     sourcelab.cpp
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
* @brief    Contains the implementation of the SourceLab class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelab.h"

#include <xMeas/Measurement/sngchnmeasurement.h>
#include <xMeas/Measurement/realtimesamplearray.h>
#include <xMeas/Measurement/realtimemultisamplearray_new.h>

#include "FormFiles/sourcelabsetupwidget.h"
#include "FormFiles/sourcelabrunwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
//#include <QtConcurrent>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SourceLabPlugin;
using namespace FIFFLIB;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLab::SourceLab()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_qFileFwdSolution("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_pFwd(new MNEForwardSolution(m_qFileFwdSolution))
, m_annotationSet("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot", "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot")
, m_iStimChan(0)
{
    m_PLG_ID = PLG_ID::SOURCELAB;
}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    stop();
}


//*************************************************************************************************************

bool SourceLab::start()
{
    // Initialize displaying widgets
    init();

    QThread::start();
    return true;
}


//*************************************************************************************************************

bool SourceLab::stop()
{
    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();

    if(m_pRtCov->isRunning())
        m_pRtCov->stop();

    if(m_pRtInvOp->isRunning())
        m_pRtInvOp->stop();

    if(m_pSourceLabBuffer)
        m_pSourceLabBuffer->clear();

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

Type SourceLab::getType() const
{
    return _IRTAlgorithm;
}


//*************************************************************************************************************

const char* SourceLab::getName() const
{
    return "SourceLab";
}


//*************************************************************************************************************

QWidget* SourceLab::setupWidget()
{
    SourceLabSetupWidget* setupWidget = new SourceLabSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

QWidget* SourceLab::runWidget()
{
    SourceLabRunWidget* runWidget = new SourceLabRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return runWidget;
}


//*************************************************************************************************************

void SourceLab::update(Subject* pSubject)
{
    Measurement* meas = static_cast<Measurement*>(pSubject);

    //MEG
    if(!meas->isSingleChannel() && m_bReceiveData)
    {
        RealTimeMultiSampleArrayNew* pRTMSANew = static_cast<RealTimeMultiSampleArrayNew*>(pSubject);


        if(pRTMSANew->getID() == MSR_ID::MEGMNERTCLIENT_OUTPUT)
        {
            //Check if buffer initialized
            if(!m_pSourceLabBuffer)
            {
                m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSANew->getNumChannels(), pRTMSANew->getMultiArraySize()));
                Buffer::SPtr t_buf = m_pSourceLabBuffer.staticCast<Buffer>();// unix fix
                setAcceptorMeasurementBuffer(pRTMSANew->getID(), t_buf);
            }

            //Fiff information
            if(!m_pFiffInfo)
                m_pFiffInfo = pRTMSANew->getFiffInfo();

            MatrixXd t_mat(pRTMSANew->getNumChannels(), pRTMSANew->getMultiArraySize());

            //ToDo: Cast to specific Buffer
            for(unsigned char i = 0; i < pRTMSANew->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSANew->getMultiSampleArray()[i];

            getAcceptorMeasurementBuffer(pRTMSANew->getID()).staticCast<CircularMatrixBuffer<double> >()
                    ->push(&t_mat);
        }

    }
}


//*************************************************************************************************************

void SourceLab::appendEvoked(FiffEvoked::SPtr p_pEvoked)
{
    if(p_pEvoked->comment == QString("Stim %1").arg(m_iStimChan))
    {
        qDebug() << p_pEvoked->comment << "append";

        mutex.lock();
        m_qVecEvokedData.push_back(p_pEvoked);
        mutex.unlock();
    }
}


//*************************************************************************************************************

void SourceLab::updateFiffCov(FiffCov::SPtr p_pFiffCov)
{
    m_pFiffCov = p_pFiffCov;

    if(m_pRtInvOp)
        m_pRtInvOp->appendNoiseCov(m_pFiffCov);
}


//*************************************************************************************************************

void SourceLab::updateInvOp(MNEInverseOperator::SPtr p_pInvOp)
{
    m_pInvOp = p_pInvOp;

    double snr = 3.0;
    double lambda2 = 1.0 / pow(snr, 2); //ToDO estimate lambda using covariance

    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

    mutex.lock();
    m_pMinimumNorm = MinimumNorm::SPtr(new MinimumNorm(*m_pInvOp.data(), lambda2, method));
    mutex.unlock();
}


//*************************************************************************************************************

void SourceLab::run()
{
    m_bIsRunning = true;

    //
    // Cluster forward solution;
    //
//    qDebug() << "Start Clustering";
//    QFuture<MNEForwardSolution> future = QtConcurrent::run(this->m_pFwd.data(), &MNEForwardSolution::cluster_forward_solution, m_annotationSet, 40);
//    qDebug() << "Run Clustering";
//    future.waitForFinished();
//    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(future.result()));

    emit statMsg("Start Clustering");
    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(m_annotationSet, 40)));
    emit statMsg("Clustering finished");

    //
    // start receiving data
    //
    m_bReceiveData = true;

    //
    // Read Fiff Info
    //
    while(!m_pFiffInfo)
    {
        msleep(10);
        qDebug() << "Wait for fiff Info";
    }

    qDebug() << "Fiff Info received.";

    //
    // Init Real-Time Covariance estimator
    //
    m_pRtCov = RtCov::SPtr(new RtCov(5000, m_pFiffInfo));
    connect(m_pRtCov.data(), &RtCov::covCalculated, this, &SourceLab::updateFiffCov);

    //
    // Init Real-Time inverse estimator
    //
    m_pRtInvOp = RtInvOp::SPtr(new RtInvOp(m_pFiffInfo, m_pClusteredFwd));
    connect(m_pRtInvOp.data(), &RtInvOp::invOperatorCalculated, this, &SourceLab::updateInvOp);

    //
    // Init Real-Time average
    //
    m_pRtAve = RtAve::SPtr(new RtAve(750, 750, m_pFiffInfo));
    connect(m_pRtAve.data(), &RtAve::evokedStim, this, &SourceLab::appendEvoked);

    //
    // Start the rt helpers
    //
    m_pRtCov->start();
    m_pRtInvOp->start();
    m_pRtAve->start();

//    // Replace this with a rt average class
//    FiffEvoked t_evoked;
//    t_evoked.setInfo(*m_pFiffInfo);
//    t_evoked.nave = 1;
//    t_evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
//    t_evoked.comment = QString("Real-time average");

//    qint32 t_iSampleCount = 0;
//    qint32 i = 0;
//    float T = 1/m_pFiffInfo->sfreq;

//    qint32 matSize = 100; //80 critical when print to console, its recommended to have a higher buffer size
//    qint32 curSize = 0;
//    MatrixXd curMat;
//    bool bMatInit = false;
//    QVector<MatrixXd> t_evokedDataVec;

    while(m_bIsRunning)
    {
        qint32 nrows = m_pSourceLabBuffer->rows();

        if(nrows > 0) // check if init
        {
            /* Dispatch the inputs */
            MatrixXd t_mat = m_pSourceLabBuffer->pop();

            //Add to covariance estimation
            m_pRtCov->append(t_mat);
            m_pRtAve->append(t_mat);

            if(m_pMinimumNorm && m_qVecEvokedData.size() > 0)
            {
                FiffEvoked t_evoked = *m_qVecEvokedData[0].data();
                SourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_evoked);

                std::cout << "SourceEstimated:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;

                //emit source estimates sample wise
                for(qint32 i = 0; i < sourceEstimate.data.cols(); ++i)
                    m_pRTSE_SourceLab->setValue(sourceEstimate.data.col(i));

                mutex.lock();
                m_qVecEvokedData.pop_front();
                mutex.unlock();
            }

//            if(m_pMinimumNorm && t_mat.cols() > 0)
//            {
//                if(!bMatInit)
//                    curMat = MatrixXd::Zero(t_mat.rows(), matSize);

//                // assemble matrix to matrices of matSize cols
//                if(curSize + t_mat.cols() < matSize)
//                {
//                    curMat.block(0,curSize,t_mat.rows(),t_mat.cols()) = t_mat;
//                    curSize += t_mat.cols();
//                }
//                else
//                {
//                    //Fill last part
//                    curMat.block(0,curSize,t_mat.rows(),matSize-curSize) = t_mat.block(0,0,t_mat.rows(),matSize-curSize);

//                    t_evokedDataVec.push_back(curMat);

//                    //Fill first part of new matrix
//                    qint32 iOffset = t_mat.cols()-(matSize-curSize);

//                    if(iOffset != 0)
//                    {
//                        curMat.block(0,0,t_mat.rows(),iOffset) = t_mat.block(0,matSize-curSize,t_mat.rows(),iOffset);
//                        curSize = iOffset;
//                    }
//                    else
//                        curSize = 0;
//                }

//                qDebug() << "Evoked Data Vector size" << t_evokedDataVec.size();

//                if(t_evokedDataVec.size() > 0)
//                {
//                    MatrixXd bufferedMat = t_evokedDataVec[0];

//                    // without average -> maybe not fast enough

//                    RowVectorXf times(bufferedMat.cols());
//                    times[0] = T*t_iSampleCount;
//                    for(i = 1; i < bufferedMat.cols(); ++i)
//                        times[i] = times[i-1] + T;

//                    t_evoked.first = times[0];
//                    t_evoked.last = times[bufferedMat.cols()-1];
//                    t_evoked.times = times;
//                    t_evoked.data = bufferedMat;


//                    //
//                    // calculate the inverse
//                    //
//                    SourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_evoked);

//                    qDebug() << t_iSampleCount << " : SourceEstimate";

//                    t_iSampleCount += bufferedMat.cols();

//                    t_evokedDataVec.pop_front();
//                }
//            }

        }
    }
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void SourceLab::init()
{
    //Delete Buffer - will be initailzed with first incoming data
    if(m_pSourceLabBuffer)
        m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr();

    qDebug() << "#### SourceLab Init; MEGRTCLIENT_OUTPUT: " << MSR_ID::MEGMNERTCLIENT_OUTPUT;

    this->addPlugin(PLG_ID::MNERTCLIENT);
    Buffer::SPtr t_buf = m_pSourceLabBuffer.staticCast<Buffer>(); //unix fix
    this->addAcceptorMeasurementBuffer(MSR_ID::MEGMNERTCLIENT_OUTPUT, t_buf);



    m_pRTSE_SourceLab = addProviderRealTimeSourceEstimate(MSR_ID::SOURCELAB_OUTPUT);
//    m_pRTSE_SourceLab->initFromFiffInfo(m_pFiffInfo);
    m_pRTSE_SourceLab->setArraySize(10);
    m_pRTSE_SourceLab->setVisibility(true);



//    m_pDummy_MSA_Output = addProviderRealTimeMultiSampleArray(MSR_ID::DUMMYTOOL_OUTPUT_II, 2);
//    m_pDummy_MSA_Output->setName("Dummy Output II");
//    m_pDummy_MSA_Output->setUnit("mV");
//    m_pDummy_MSA_Output->setMinValue(-200);
//    m_pDummy_MSA_Output->setMaxValue(360);
//    m_pDummy_MSA_Output->setSamplingRate(256.0/1.0);

}
