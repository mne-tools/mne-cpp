//=============================================================================================================
/**
 * @file     covariance.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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

#include <disp/viewers/covariancesettingsview.h>

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
using namespace DISPLIB;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Covariance::Covariance()
: m_iEstimationSamples(2000)
, m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr::create(40))
, m_bProcessOutput(false)
{
}

//=============================================================================================================

Covariance::~Covariance()
{
    if(m_bProcessOutput)
        stop();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Covariance::clone() const
{
    QSharedPointer<Covariance> pCovarianceClone(new Covariance);
    return pCovarianceClone;
}

//=============================================================================================================

void Covariance::init()
{
    // Load Settings
    QSettings settings("MNECPP");
    m_iEstimationSamples = settings.value(QString("MNESCAN/%1/estimationSamples").arg(this->getName()), 5000).toInt();

    // Input
    m_pCovarianceInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "CovarianceIn", "Covariance input data");
    connect(m_pCovarianceInput.data(), &PluginInputConnector::notify,
            this, &Covariance::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pCovarianceInput);

    // Output
    m_pCovarianceOutput = PluginOutputData<RealTimeCov>::create(this, "CovarianceOut", "Covariance output data");
    m_pCovarianceOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pCovarianceOutput);
}

//=============================================================================================================

void Covariance::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        CovarianceSettingsView* pCovarianceWidget = new CovarianceSettingsView(QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Covariance::guiModeChanged,
                pCovarianceWidget, &CovarianceSettingsView::setGuiMode);
        connect(pCovarianceWidget, &CovarianceSettingsView::samplesChanged,
                this, &Covariance::changeSamples);
        pCovarianceWidget->setMinSamples(m_pFiffInfo->sfreq);
        pCovarianceWidget->setCurrentSamples(m_iEstimationSamples);
        pCovarianceWidget->setObjectName("group_Settings");
        plControlWidgets.append(pCovarianceWidget);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void Covariance::unload()
{
    // Save Settings
    QSettings settings("MNECPP");
    settings.setValue(QString("MNESCAN/%1/estimationSamples").arg(this->getName()), m_iEstimationSamples);
}

//=============================================================================================================

bool Covariance::start()
{
    // Start thread
    m_bProcessOutput = true;
    m_OutputProcessingThread = std::thread(&Covariance::run, this);

    return true;
}

//=============================================================================================================

bool Covariance::stop()
{
    m_bProcessOutput = false;

    if(m_OutputProcessingThread.joinable()){
        m_OutputProcessingThread.join();
    }

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Covariance::getType() const
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
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pCovarianceOutput->measurementData()->setFiffInfo(m_pFiffInfo);
        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            // Please note that we do not need a copy here since this function will block until
            // the buffer accepts new data again. Hence, the data is not deleted in the actual
            // Measurement function after it emitted the notify signal.
            while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                //Do nothing until the circular buffer is ready to accept new data again
            }
        }
    }
}

//=============================================================================================================

void Covariance::changeSamples(qint32 samples)
{
    m_iEstimationSamples = samples;
}

//=============================================================================================================

void Covariance::run()
{
    // Wait for fiff info
    while(true) {
        m_mutex.lock();
        if(m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    MatrixXd matData;
    FiffCov fiffCov;
    m_mutex.lock();
    int iEstimationSamples = m_iEstimationSamples;
    m_mutex.unlock();
    RTPROCESSINGLIB::RtCov rtCov(m_pFiffInfo);

    // Start processing data
    while(m_bProcessOutput) {
        // Get the current data
        if(m_pCircularBuffer->pop(matData)) {
            m_mutex.lock();
            iEstimationSamples = m_iEstimationSamples;
            m_mutex.unlock();

            fiffCov = rtCov.estimateCovariance(matData, iEstimationSamples);
            if(!fiffCov.names.isEmpty()) {
                m_pCovarianceOutput->measurementData()->setValue(fiffCov);
            }
        }
    }
}

//=============================================================================================================

QString Covariance::getBuildInfo()
{
    return QString(COVARIANCEPLUGIN::buildDateTime()) + QString(" - ")  + QString(COVARIANCEPLUGIN::buildHash());
}
