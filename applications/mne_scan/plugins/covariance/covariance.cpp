//=============================================================================================================
/**
 * @file     covariance.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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
 * @brief    Definition of the Covariance class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance.h"

#include "FormFiles/covariancesetupwidget.h"
#include "FormFiles/covariancesettingswidget.h"

#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <rtprocessing/rtcov.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>
#include <QMutexLocker>
#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Covariance::Covariance()
: m_bIsRunning(false)
, m_bProcessData(false)
, m_pCovarianceInput(NULL)
, m_pCovarianceOutput(NULL)
, m_iEstimationSamples(5000)
{
    m_pActionShowAdjustment = new QAction(QIcon(":/images/covadjustments.png"), tr("Covariance Adjustments"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionShowAdjustment->setStatusTip(tr("Covariance Adjustments"));
    connect(m_pActionShowAdjustment, &QAction::triggered, this, &Covariance::showCovarianceWidget);
    addPluginAction(m_pActionShowAdjustment);
//    m_pActionShowAdjustment->setVisible(false);
}


//*************************************************************************************************************

Covariance::~Covariance()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Covariance::clone() const
{
    QSharedPointer<Covariance> pCovarianceClone(new Covariance);
    return pCovarianceClone;
}


//*************************************************************************************************************

void Covariance::init()
{
    // Load Settings
    QSettings settings;
    m_iEstimationSamples = settings.value(QString("Plugin/%1/estimationSamples").arg(this->getName()), 5000).toInt();

    // Input
    m_pCovarianceInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "CovarianceIn", "Covariance input data");
    connect(m_pCovarianceInput.data(), &PluginInputConnector::notify, this, &Covariance::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pCovarianceInput);

    // Output
    m_pCovarianceOutput = PluginOutputData<RealTimeCov>::create(this, "CovarianceOut", "Covariance output data");
    m_outputConnectors.append(m_pCovarianceOutput);
}


//*************************************************************************************************************

void Covariance::unload()
{
    // Store Settings
    QSettings settings;
    settings.setValue(QString("Plugin/%1/estimationSamples").arg(this->getName()), m_iEstimationSamples);
}


//*************************************************************************************************************

bool Covariance::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning()) {
//        QThread::wait();
//    }

    m_bIsRunning = true;

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Covariance::stop()
{
    //Wait until this thread is stopped
    m_bIsRunning = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Covariance::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString Covariance::getName() const
{
    return "Covariance";
}


//*************************************************************************************************************

void Covariance::showCovarianceWidget()
{
    m_pCovarianceWidget = CovarianceSettingsWidget::SPtr(new CovarianceSettingsWidget(this));
    m_pCovarianceWidget->show();
}


//*************************************************************************************************************

QWidget* Covariance::setupWidget()
{
    CovarianceSetupWidget* setupWidget = new CovarianceSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Covariance::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pCovarianceOutput->data()->setFiffInfo(m_pFiffInfo);

            //Set m_iEstimationSamples so that we alwyas wait for 5 secs
            m_iEstimationSamples = m_pFiffInfo->sfreq * 5;
            m_pRtCov = RtCov::SPtr(new RtCov(m_iEstimationSamples, m_pFiffInfo));
            connect(m_pRtCov.data(), &RtCov::covCalculated,
                    this, &Covariance::appendCovariance);
        }


        if(m_bProcessData) {
            MatrixXd t_mat;

            for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
                t_mat = pRTMSA->getMultiSampleArray()[i];
                m_pRtCov->append(t_mat);
            }
        }
    }
}


//*************************************************************************************************************

void Covariance::appendCovariance(const FiffCov& p_pCovariance)
{
    QMutexLocker locker(&mutex);
    m_qVecCovData.push_back(p_pCovariance);
}


//*************************************************************************************************************

void Covariance::changeSamples(qint32 samples)
{
    m_iEstimationSamples = samples;
    if(m_pRtCov) {
        m_pRtCov->setSamples(m_iEstimationSamples);
    }
}


//*************************************************************************************************************

void Covariance::run()
{
    // Read Fiff Info
    while(!m_pFiffInfo) {
        msleep(10);// Wait for fiff Info
    }

    m_pCovarianceOutput->data()->setFiffInfo(m_pFiffInfo);

    // Start processing data
    m_bProcessData = true;

    while (m_bIsRunning) {
        if(m_bProcessData) {
            //Add to covariance estimation
            QMutexLocker locker(&mutex);
            if(!m_qVecCovData.isEmpty()) {
                m_pCovarianceOutput->data()->setValue(m_qVecCovData.takeFirst());
            }
        }
    }
}

