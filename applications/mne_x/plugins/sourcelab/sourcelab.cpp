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
, m_iNumAverages(10)
{

}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> SourceLab::clone() const
{
    QSharedPointer<SourceLab> pSourceLabClone(new SourceLab());
    return pSourceLabClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void SourceLab::init()
{
    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pSourceLabBuffer.isNull())
        m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr();

    // Input
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "SourceLabIn", "SourceLab input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &SourceLab::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);



//    this->addPlugin(PLG_ID::MNERTCLIENT);
//    Buffer::SPtr t_buf = m_pSourceLabBuffer.staticCast<Buffer>(); //unix fix
//    this->addAcceptorMeasurementBuffer(MSR_ID::MEGMNERTCLIENT_OUTPUT, t_buf);



//    m_pRTSE_SourceLab = addProviderRealTimeSourceEstimate(MSR_ID::SOURCELAB_OUTPUT);
//    m_pRTSE_SourceLab->setName("Real-Time Source Estimate");
////    m_pRTSE_SourceLab->initFromFiffInfo(m_pFiffInfo);
//    m_pRTSE_SourceLab->setArraySize(10);
//    m_pRTSE_SourceLab->setVisibility(true);



}


//*************************************************************************************************************

bool SourceLab::start()
{
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

IPlugin::PluginType SourceLab::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString SourceLab::getName() const
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

void SourceLab::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    //MEG
    if(pRTMSA && m_bReceiveData)
    {

        //Check if buffer initialized
        if(!m_pSourceLabBuffer)
            m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information
        if(!m_pFiffInfo)
            m_pFiffInfo = pRTMSA->getFiffInfo();

        MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
            t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

        m_pSourceLabBuffer->push(&t_mat);

////            getAcceptorMeasurementBuffer(pRTMSANew->getID()).staticCast<CircularMatrixBuffer<double> >()
////                    ->push(&t_mat);
    }
}


//*************************************************************************************************************

void SourceLab::appendEvoked(FiffEvoked::SPtr p_pEvoked)
{
    if(p_pEvoked->comment == QString("Stim %1").arg(m_iStimChan))
    {
        std::cout << p_pEvoked->comment.toLatin1().constData() << " append" << std::endl;

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
    //
    //   Set up the inverse according to the parameters
    //
    m_pMinimumNorm->doInverseSetup(m_iNumAverages,false);
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

    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(m_annotationSet, 40)));

    //
    // start receiving data
    //
    m_bReceiveData = true;

    //
    // Read Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

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
    m_pRtAve = RtAve::SPtr(new RtAve(m_iNumAverages, 750, 750, m_pFiffInfo));
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

            mutex.lock();
            if(m_pMinimumNorm && m_qVecEvokedData.size() > 0)
            {
                FiffEvoked t_fiffEvoked = *m_qVecEvokedData[0].data();

                float tmin = ((float)t_fiffEvoked.first) / t_fiffEvoked.info.sfreq;
                float tstep = 1/t_fiffEvoked.info.sfreq;

                SourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_fiffEvoked.data, tmin, tstep);

                std::cout << "SourceEstimated:\n" << std::endl;
//                std::cout << "SourceEstimated:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;

//                //emit source estimates sample wise
//                for(qint32 i = 0; i < sourceEstimate.data.cols(); ++i)
//                    m_pRTSE_SourceLab->setValue(sourceEstimate.data.col(i));


                m_qVecEvokedData.pop_front();
            }
            mutex.unlock();

            //Continous Data

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
