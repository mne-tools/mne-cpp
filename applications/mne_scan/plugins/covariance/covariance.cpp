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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance.h"

#include "FormFiles/covariancesetupwidget.h"
#include "FormFiles/covariancesettingswidget.h"

#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <rtprocessing/rtcov.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>
#include <QtWidgets>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Covariance::Covariance()
: m_iEstimationSamples(5000)
{
}

//=============================================================================================================

Covariance::~Covariance()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<IPlugin> Covariance::clone() const
{
    QSharedPointer<Covariance> pCovarianceClone(new Covariance);
    return pCovarianceClone;
}

//=============================================================================================================

void Covariance::init()
{
    // Load Settings
    QSettings settings;
    m_iEstimationSamples = settings.value(QString("MNESCAN/%1/estimationSamples").arg(this->getName()), 5000).toInt();

    // Input
    m_pCovarianceInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "CovarianceIn", "Covariance input data");
    connect(m_pCovarianceInput.data(), &PluginInputConnector::notify,
            this, &Covariance::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pCovarianceInput);

    // Output
    m_pCovarianceOutput = PluginOutputData<RealTimeCov>::create(this, "CovarianceOut", "Covariance output data");
    m_pCovarianceOutput->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pCovarianceOutput);
}

//=============================================================================================================

void Covariance::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        CovarianceSettingsWidget* pCovarianceWidget = new CovarianceSettingsWidget();
        connect(pCovarianceWidget, &CovarianceSettingsWidget::samplesChanged,
                this, &Covariance::changeSamples);
        pCovarianceWidget->setMinSamples(m_pFiffInfo->sfreq);
        pCovarianceWidget->setCurrentSamples(m_iEstimationSamples);
        pCovarianceWidget->setObjectName("group_Settings");
        plControlWidgets.append(pCovarianceWidget);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    }
}

//=============================================================================================================

void Covariance::unload()
{
    // Store Settings
    QSettings settings;
    settings.setValue(QString("Plugin/%1/estimationSamples").arg(this->getName()), m_iEstimationSamples);
}

//=============================================================================================================

bool Covariance::start()
{
    //Start thread as soon as we have received the first data block. See update().

    return true;
}

//=============================================================================================================

bool Covariance::stop()
{
    requestInterruption();
    wait();

    return true;
}

//=============================================================================================================

IPlugin::PluginType Covariance::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Covariance::getName() const
{
    return "Covariance";
}

//=============================================================================================================

void Covariance::showCovarianceWidget()
{
}

//=============================================================================================================

QWidget* Covariance::setupWidget()
{
    CovarianceSetupWidget* setupWidget = new CovarianceSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Covariance::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            initPluginControlWidgets();

            m_pCovarianceOutput->data()->setFiffInfo(m_pFiffInfo);

            m_pRtCov = RtCov::SPtr(new RtCov(m_iEstimationSamples, m_pFiffInfo));
            connect(m_pRtCov.data(), &RtCov::covCalculated,
                    this, &Covariance::appendCovariance);

            if(!m_pCircularBuffer) {
                m_pCircularBuffer = CircularBuffer<FIFFLIB::FiffCov>::SPtr::create(10);
            }

            // Start thread
            QThread::start();
        }

        MatrixXd matData;
        for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            // This extra copy is necessary since the referenced data is getting deleted as soon as
            // m_pRtAve->append() returns. m_pRtAve->append() returns without a copy since it communicates
            // via signals with the worker thread of RtCov.
            matData = pRTMSA->getMultiSampleArray()[i];
            m_pRtCov->append(matData);
        }
    }
}

//=============================================================================================================

void Covariance::appendCovariance(const FiffCov& covariance)
{
    while(!m_pCircularBuffer->push(covariance)) {
        //Do nothing until the circular buffer is ready to accept new data again
    }
}

//=============================================================================================================

void Covariance::changeSamples(qint32 samples)
{
    m_iEstimationSamples = samples;
    if(m_pRtCov) {
        m_pRtCov->setSamples(m_iEstimationSamples);
    }
}

//=============================================================================================================

void Covariance::run()
{
    FiffCov covariance;

    // Start processing data
    while(!isInterruptionRequested()) {
        // Get the current data
        if(m_pCircularBuffer->pop(covariance)) {
            m_pCovarianceOutput->data()->setValue(covariance);
        }
    }
}

