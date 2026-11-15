//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromag.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    Contains the implementation of the Neuromag class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag.h"

#include "FormFiles/neuromagsetupwidget.h"

#include <acq/acq_device_registry.h>
#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neuromag::Neuromag() = default;

//=============================================================================================================

Neuromag::~Neuromag()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Neuromag::clone() const
{
    return QSharedPointer<Neuromag>(new Neuromag());
}

//=============================================================================================================

void Neuromag::init()
{
    m_pRTMSA_Neuromag = PluginOutputData<RealTimeMultiSampleArray>::create(this, "Neuromag", "MEG output data");
    m_pRTMSA_Neuromag->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_Neuromag);
}

//=============================================================================================================

void Neuromag::unload()
{
}

//=============================================================================================================

std::string Neuromag::uri() const
{
    return mne::AcqUri{.scheme = "neuromag",
                       .target = {},
                       .params = {{"root", m_root},
                                  {"buffer", std::to_string(m_iBufferSize)},
                                  {"autostart", m_bAutostart ? "1" : "0"},
                                  {"simulate", m_bSimulate ? "1" : "0"}}}
        .toString();
}

//=============================================================================================================

bool Neuromag::start()
{
    mne::register_neuromag_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        qWarning() << "[Neuromag::start]" << QString::fromStdString(device.error().text());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        qWarning() << "[Neuromag::start]" << QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text());
        m_device.close();
        return false;
    }

    //The measurement info can change between measurements, so it is set up here and not in init()
    m_pRTMSA_Neuromag->measurementData()->initFromFiffInfo(std::make_shared<mne::FiffInfo>(std::move(*fiffInfo)));
    m_pRTMSA_Neuromag->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        qWarning() << "[Neuromag::start]" << QString::fromStdString(e.text());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool Neuromag::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_Neuromag->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Neuromag::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString Neuromag::getName() const
{
    return "Neuromag";
}

//=============================================================================================================

QWidget* Neuromag::setupWidget()
{
    return new NeuromagSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void Neuromag::run()
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
            qWarning() << "[Neuromag::run]" << QString::fromStdString(read.error().text());
            return;
        }
        m_pRTMSA_Neuromag->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString Neuromag::getBuildInfo()
{
    return QString(NEUROMAGPLUGIN::buildDateTime()) + QString(" - ") + QString(NEUROMAGPLUGIN::buildHash());
}
