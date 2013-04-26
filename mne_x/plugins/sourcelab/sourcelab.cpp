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
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SourceLabPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLab::SourceLab()
: m_bIsRunning(false)
, m_qFileFwdSolution("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_annotationSet("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot", "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot")
{
    m_PLG_ID = PLG_ID::SOURCELAB;

    if(!MNEForwardSolution::read_forward_solution(m_qFileFwdSolution, m_Fwd))
        qDebug() << "Couldn't read forward solution";

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
    // Stop threads
    QThread::terminate();
    QThread::wait();

    if(m_pSourceLabBuffer)
        m_pSourceLabBuffer->clear();

    m_bIsRunning = false;

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
    return "Source Lab";
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
    if(!meas->isSingleChannel() && m_bIsRunning)
    {
        RealTimeMultiSampleArrayNew* pRTMSANew = static_cast<RealTimeMultiSampleArrayNew*>(pSubject);


        if(pRTMSANew->getID() == MSR_ID::MEGRTSERVER_OUTPUT)
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

void SourceLab::run()
{
    //
    // Cluster forward solution;
    //
//    qDebug() << "Start Clustering";
//    m_clusteredFwd = m_Fwd.cluster_forward_solution(m_annotationSet, 40);
//    qDebug() << "Clustering finished";

    //
    // start receiving data
    //
    m_bIsRunning = true;

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
    m_pRtCov = RtCov::SPtr(new RtCov(1000, m_pFiffInfo));

    m_pRtCov->start();


    qint32 count = 0;
    while(m_bIsRunning)
    {
        /* Dispatch the inputs */

//        double v = m_pECGBuffer->pop();

//        //ToDo: Implement here the algorithm

//        m_pSourceLab_Output->setValue(v);



        qint32 nrows = m_pSourceLabBuffer->rows();

        if(nrows > 0) // check if init
        {
            MatrixXd t_mat = m_pSourceLabBuffer->pop();


            qDebug() << count << ": m_pSourceLabBuffer->pop(); Matrix:" << t_mat.rows() << "x" << t_mat.cols();

            m_pRtCov->append(t_mat);


            ++count;
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

    qDebug() << "#### SourceLab Init; MEGRTSERVER_OUTPUT: " << MSR_ID::MEGRTSERVER_OUTPUT;

    this->addPlugin(PLG_ID::RTSERVER);
    Buffer::SPtr t_buf = m_pSourceLabBuffer.staticCast<Buffer>(); //unix fix
    this->addAcceptorMeasurementBuffer(MSR_ID::MEGRTSERVER_OUTPUT, t_buf);

//    m_pDummy_MSA_Output = addProviderRealTimeMultiSampleArray(MSR_ID::DUMMYTOOL_OUTPUT_II, 2);
//    m_pDummy_MSA_Output->setName("Dummy Output II");
//    m_pDummy_MSA_Output->setUnit("mV");
//    m_pDummy_MSA_Output->setMinValue(-200);
//    m_pDummy_MSA_Output->setMaxValue(360);
//    m_pDummy_MSA_Output->setSamplingRate(256.0/1.0);

}
