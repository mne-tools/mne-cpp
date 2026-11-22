//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the FiffSimulator class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "fiffsimulator.h"

#include "FormFiles/fiffsimulatorsetupwidget.h"

#include <mne/acq/acq_device_registry.h>
#include <mne/acq/devices/acq_devices.h>
#include <mne/acq/fiff/acq_fiff.h>
#include <mne/fiff/fiff_digitizer_data.h>
#include <mne/fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator() = default;

//=============================================================================================================

FiffSimulator::~FiffSimulator()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> FiffSimulator::clone() const
{
    return QSharedPointer<FiffSimulator>(new FiffSimulator());
}

//=============================================================================================================

void FiffSimulator::init()
{
    m_pRTMSA_FiffSimulator = PluginOutputData<RealTimeMultiSampleArray>::create(this, "FiffSimulator", "Fiff Simulator Output");
    m_pRTMSA_FiffSimulator->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_FiffSimulator);
}

//=============================================================================================================

void FiffSimulator::unload()
{
}

//=============================================================================================================

std::string FiffSimulator::uri() const
{
    mne::AcqUri uri{.scheme = "rtserver", .target = {}, .params = {{"buffer", std::to_string(m_iSamplesPerBlock)}, {"alias", "mne_scan"}}};
    if (m_bSimulate) {
        uri.params.emplace_back("simulate", "1");
    } else {
        uri.target = m_host + ":" + std::to_string(m_port);
        if (!m_simfile.empty()) {
            uri.params.emplace_back("simfile", m_simfile);
        }
    }
    return uri.toString();
}

//=============================================================================================================

bool FiffSimulator::start()
{
    mne::register_rtserver_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        mne::Log::warning("[FiffSimulator::start] {}", QString::fromStdString(device.error().text()).toStdString());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        mne::Log::warning("[FiffSimulator::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }

    //The server can replay another file on every start, so it is set up here and not in init()
    auto info = std::make_shared<mne::FiffInfo>(std::move(*fiffInfo));
    if (info->dig.size() > 0) {
        // The recording's isotrak, which the HPI plugin fits the coils to
        auto digitizers = std::make_shared<mne::FiffDigitizerData>();
        digitizers->points = info->dig;
        digitizers->coord_frame = FIFFV_COORD_HEAD;
        digitizers->active.assign(info->dig.size(), true);
        digitizers->discard.assign(info->dig.size(), false);
        m_pRTMSA_FiffSimulator->measurementData()->setDigitizerData(digitizers);
    }
    m_pRTMSA_FiffSimulator->measurementData()->initFromFiffInfo(info);
    m_pRTMSA_FiffSimulator->measurementData()->setMultiArraySize(1);
    m_pRTMSA_FiffSimulator->measurementData()->setVisibility(true);
    m_pRTMSA_FiffSimulator->measurementData()->setXMLLayoutFile(QCoreApplication::applicationDirPath() +
                                                                "/../resources/mne_scan/plugins/FiffSimulator/VectorViewSimLayout.xml");

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        mne::Log::warning("[FiffSimulator::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool FiffSimulator::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_FiffSimulator->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType FiffSimulator::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString FiffSimulator::getName() const
{
    return "Fiff Simulator";
}

//=============================================================================================================

QWidget* FiffSimulator::setupWidget()
{
    return new FiffSimulatorSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void FiffSimulator::run()
{
    const std::size_t channels = m_device.stream().info()->channelCount();
    MatrixXd block(static_cast<Index>(channels), m_iSamplesPerBlock);
    while (!isInterruptionRequested()) {
        const auto read = m_reader.read(mne::AcqMatrixRef<double>::columnMajor(block.data(), channels, static_cast<std::size_t>(block.cols())),
                                        std::chrono::milliseconds(100));
        if (!read) {
            if (read.error() == mne::AcqErrorCode::Timeout || read.error() == mne::AcqErrorCode::Overrun) {
                continue;
            }
            mne::Log::warning("[FiffSimulator::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRTMSA_FiffSimulator->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString FiffSimulator::getBuildInfo()
{
    return QString(FIFFSIMULATORPLUGIN::buildDateTime()) + QString(" - ") + QString(FIFFSIMULATORPLUGIN::buildHash());
}
