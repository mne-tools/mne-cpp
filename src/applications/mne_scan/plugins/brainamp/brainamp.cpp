//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainamp.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the implementation of the BrainAMP class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp.h"

#include "FormFiles/brainampsetupwidget.h"
#include "FormFiles/brainampsetupprojectwidget.h"

#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <fiff/fiff.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMP::BrainAMP()
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &BrainAMP::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);
}

//=============================================================================================================

BrainAMP::~BrainAMP()
{
    //If the program is closed while the sampling is in process
    if (this->isRunning()) {
        this->stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> BrainAMP::clone() const
{
    QSharedPointer<BrainAMP> pBrainAMPClone(new BrainAMP());
    return pBrainAMPClone;
}

//=============================================================================================================

void BrainAMP::init()
{
    m_pRMTSA_BrainAMP = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainAMP", "EEG output data");
    m_pRMTSA_BrainAMP->measurementData()->setName(this->getName()); //Provide name to auto store widget settings

    m_outputConnectors.append(m_pRMTSA_BrainAMP);

    QSettings settings("MNECPP");
    m_iChannels = settings.value(QString("BRAINAMP/channels"), 32).toInt();
    m_sResolution = settings.value(QString("BRAINAMP/resolution"), QString("100nV")).toString();
    m_bSimulate = settings.value(QString("BRAINAMP/simulate"), false).toBool();
    m_iSamplesPerBlock = settings.value(QString("BRAINAMP/samplesPerBlock"), 500).toInt();

    m_sElcFilePath = settings.value(QString("BRAINAMP/elcFilePath"), QString("../resources/general/3DLayouts/standard_waveguard64_duke.elc")).toString();

    m_sCardinalFilePath = settings.value(QString("BRAINAMP/cardinalFilePath"), QString("")).toString();

    m_dLPAShift = settings.value(QString("BRAINAMP/LPAShift"), 0.0).toFloat();
    m_dRPAShift = settings.value(QString("BRAINAMP/RPAShift"), 0.0).toFloat();
    m_dNasionShift = settings.value(QString("BRAINAMP/NasionShift"), 0.0).toFloat();
    m_sLPA = settings.value(QString("BRAINAMP/LPAElectrode"), QString("")).toString();
    m_sRPA = settings.value(QString("BRAINAMP/RPAElectrode"), QString("")).toString();
    m_sNasion = settings.value(QString("BRAINAMP/NasionElectrode"), QString("")).toString();
    m_bUseTrackedCardinalMode = settings.value(QString("BRAINAMP/useTrackedCardinalsMode"), true).toBool();
    m_bUseElectrodeShiftMode = settings.value(QString("BRAINAMP/useElectrodeshiftMode"), false).toBool();
}

//=============================================================================================================

void BrainAMP::unload()
{
}

//=============================================================================================================

std::string BrainAMP::uri() const
{
    return std::format("brainamp://?channels={}&resolution={}&simulate={}", m_iChannels, m_sResolution.toStdString(),
                       m_bSimulate ? 1 : 0);
}

//=============================================================================================================

bool BrainAMP::start()
{
    mne::registerBrainAmpScheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        qWarning() << "[BrainAMP::start]" << QString::fromStdString(device.error().text());
        return false;
    }
    auto fiffInfo = mne::toFiffInfo(*device->stream().info());
    auto reader = device->stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        qWarning() << "[BrainAMP::start]" << QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text());
        return false;
    }

    //The user can change the channels between runs, so the measurement is set up here and not in init()
    m_pRMTSA_BrainAMP->measurementData()->initFromFiffInfo(std::make_shared<FiffInfo>(std::move(*fiffInfo)));
    m_pRMTSA_BrainAMP->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = device->start(); !e.ok()) {
        qWarning() << "[BrainAMP::start]" << QString::fromStdString(e.text());
        return false;
    }
    m_device = std::move(*device);
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool BrainAMP::stop()
{
    requestInterruption();
    wait();

    m_device.close();
    m_reader = mne::AcqReader();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRMTSA_BrainAMP->measurementData()->clear();

    //Store settings for next use
    QSettings settings("MNECPP");
    settings.setValue(QString("BRAINAMP/channels"), m_iChannels);
    settings.setValue(QString("BRAINAMP/resolution"), m_sResolution);
    settings.setValue(QString("BRAINAMP/simulate"), m_bSimulate);
    settings.setValue(QString("BRAINAMP/samplesPerBlock"), m_iSamplesPerBlock);
    settings.setValue(QString("BRAINAMP/LPAShift"), m_dLPAShift);
    settings.setValue(QString("BRAINAMP/RPAShift"), m_dRPAShift);
    settings.setValue(QString("BRAINAMP/NasionShift"), m_dNasionShift);
    settings.setValue(QString("BRAINAMP/LPAElectrode"), m_sLPA);
    settings.setValue(QString("BRAINAMP/RPAElectrode"), m_sRPA);
    settings.setValue(QString("BRAINAMP/NasionElectrode"), m_sNasion);
    settings.setValue(QString("BRAINAMP/elcFilePath"), m_sElcFilePath);
    settings.setValue(QString("BRAINAMP/cardinalFilePath"), m_sCardinalFilePath);
    settings.setValue(QString("BRAINAMP/useTrackedCardinalsMode"), m_bUseTrackedCardinalMode);
    settings.setValue(QString("BRAINAMP/useElectrodeshiftMode"), m_bUseElectrodeShiftMode);

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType BrainAMP::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString BrainAMP::getName() const
{
    return "BrainAMP";
}

//=============================================================================================================

QWidget* BrainAMP::setupWidget()
{
    BrainAMPSetupWidget* widget = new BrainAMPSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}

//=============================================================================================================

void BrainAMP::onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion)
{
    m_dLPAShift = dLPA;
    m_dRPAShift = dRPA;
    m_dNasionShift = dNasion;

    m_sLPA = sLPA;
    m_sRPA = sRPA;
    m_sNasion = sNasion;
}

//=============================================================================================================

void BrainAMP::run()
{
    const std::size_t channels = m_reader.channelCount();
    MatrixXd block(static_cast<Index>(channels), m_iSamplesPerBlock);
    while (!isInterruptionRequested()) {
        const auto read = m_reader.read(mne::AcqMatrixRef<double>::columnMajor(block.data(), channels, static_cast<std::size_t>(block.cols())),
                                        std::chrono::milliseconds(100));
        if (read) {
            m_pRMTSA_BrainAMP->measurementData()->setValue(block);
        } else if (read.error() != mne::AcqErrorCode::Timeout && read.error() != mne::AcqErrorCode::Overrun) {
            qWarning() << "[BrainAMP::run]" << QString::fromStdString(read.error().text());
            return;
        }
    }
}

//=============================================================================================================

void BrainAMP::showSetupProjectDialog()
{
    // Open setup project widget
    if (m_pBrainAMPSetupProjectWidget == nullptr) {
        m_pBrainAMPSetupProjectWidget = QSharedPointer<BrainAMPSetupProjectWidget>(new BrainAMPSetupProjectWidget(this));

        connect(m_pBrainAMPSetupProjectWidget.data(), &BrainAMPSetupProjectWidget::cardinalPointsChanged,
                this, &BrainAMP::onUpdateCardinalPoints);
    }

    if (!m_pBrainAMPSetupProjectWidget->isVisible()) {
        m_pBrainAMPSetupProjectWidget->setWindowTitle("BrainAMP EEG Connector - Setup project");
        m_pBrainAMPSetupProjectWidget->show();
        m_pBrainAMPSetupProjectWidget->raise();
    }
}

//=============================================================================================================

QString BrainAMP::getBuildInfo()
{
    return QString(BRAINAMPPLUGIN::buildDateTime()) + QString(" - ") + QString(BRAINAMPPLUGIN::buildHash());
}
