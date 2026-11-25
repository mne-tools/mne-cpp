//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     covariance.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the Covariance class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance.h"

#include "FormFiles/covariancesetupwidget.h"
#include <mne/disp_widgets/viewers/covariancesettingsview.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <mne/dsp/rt/rt_cov.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_cov.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>
#include <QtWidgets>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <numbers>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;
using namespace mne::disp;
using namespace mne::scan;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Covariance::Covariance()
: m_iEstimationSamples(2000)
, m_pCircularBuffer(std::make_shared<mne::CircularBuffer<Eigen::MatrixXd>>(40))
{
}

//=============================================================================================================

Covariance::~Covariance()
{
    if (this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Covariance::clone() const
{
    auto pCovarianceClone = QSharedPointer<Covariance>::create();
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
    m_pCovarianceOutput->measurementData()->setName(this->getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pCovarianceOutput);
}

//=============================================================================================================

void Covariance::initPluginControlWidgets()
{
    if (m_pFiffInfo) {
        std::vector<std::unique_ptr<QWidget>> controlWidgets;

        CovarianceSettingsView* pCovarianceWidget = makeControlWidget<CovarianceSettingsView>(controlWidgets, QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Covariance::guiModeChanged,
                pCovarianceWidget, &CovarianceSettingsView::setGuiMode);
        connect(pCovarianceWidget, &CovarianceSettingsView::samplesChanged,
                this, &Covariance::changeSamples);
        pCovarianceWidget->setMinSamples(m_pFiffInfo->sfreq);
        pCovarianceWidget->setCurrentSamples(m_iEstimationSamples);
        pCovarianceWidget->setObjectName("group_Settings");

        offerControlWidgets(std::move(controlWidgets));

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
    QThread::start();

    return true;
}

//=============================================================================================================

bool Covariance::stop()
{
    requestInterruption();
    wait(500);

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
    CovarianceSetupWidget* setupWidget = new CovarianceSetupWidget(this); //widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Covariance::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if (!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pCovarianceOutput->measurementData()->setFiffInfo(m_pFiffInfo);
        }

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        for (qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            // Please note that we do not need a copy here since this function will block until
            // the buffer accepts new data again. Hence, the data is not deleted in the actual
            // Measurement function after it emitted the notify signal.
            while (!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
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
    while (!isInterruptionRequested()) {
        m_mutex.lock();
        if (m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        msleep(100);
    }
    if (isInterruptionRequested()) {
        return;
    }

    MatrixXd matData;
    int iEstimationSamples = 0;
    std::optional<mne::RtCov> rtCov;
    int newSamples = 0;

    // Start processing data
    while (!isInterruptionRequested()) {
        // Get the current data
        if (m_pCircularBuffer->pop(matData)) {
            m_mutex.lock();
            const int samples = m_iEstimationSamples;
            m_mutex.unlock();
            if (!rtCov || samples != iEstimationSamples) {
                iEstimationSamples = samples;
                // An exponential window of this half-life holds iEstimationSamples effective samples
                rtCov.emplace(*m_pFiffInfo, iEstimationSamples * std::numbers::ln2 / (2.0 * m_pFiffInfo->sfreq));
                newSamples = 0;
            }

            rtCov->update(matData);
            newSamples += static_cast<int>(matData.cols());
            if (newSamples >= iEstimationSamples) {
                if (const std::optional<FiffCov> fiffCov = rtCov->covariance()) {
                    m_pCovarianceOutput->measurementData()->setValue(*fiffCov);
                    newSamples = 0;
                }
            }
        }
    }
}

//=============================================================================================================

QString Covariance::getBuildInfo()
{
    return QString(COVARIANCEPLUGIN::buildDateTime()) + QString(" - ") + QString(COVARIANCEPLUGIN::buildHash());
}

//=============================================================================================================

QVariantMap Covariance::getAttributes() const
{
    QVariantMap attrs;
    attrs[QStringLiteral("estimationSamples")] = m_iEstimationSamples;
    return attrs;
}

//=============================================================================================================

void Covariance::setAttributes(const QVariantMap& attributes)
{
    if (attributes.contains(QStringLiteral("estimationSamples")))
        m_iEstimationSamples = attributes[QStringLiteral("estimationSamples")].toInt();
}
