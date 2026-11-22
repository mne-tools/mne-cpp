//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natus.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Definition of the Natus class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus.h"

#include "FormFiles/natussetup.h"
#include <base/log.h>
#include <acq/acq_device_registry.h>
#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NATUSPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Natus::Natus() = default;

//=============================================================================================================

Natus::~Natus()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Natus::clone() const
{
    return QSharedPointer<Natus>(new Natus());
}

//=============================================================================================================

void Natus::init()
{
    m_pRMTSA_Natus = PluginOutputData<RealTimeMultiSampleArray>::create(this, "Natus", "EEG output data");
    m_pRMTSA_Natus->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRMTSA_Natus);
}

//=============================================================================================================

void Natus::unload()
{
}

//=============================================================================================================

std::string Natus::uri() const
{
    mne::AcqUri uri{.scheme = "natus",
                    .target = {},
                    .params = {{"channels", std::to_string(m_iNumberChannels)}, {"rate", std::to_string(m_iSamplingFreq)}}};
    if (m_bSimulate) {
        uri.params.emplace_back("simulate", "1");
    } else {
        uri.target = m_address;
        uri.params.emplace_back("port", std::to_string(m_port));
    }
    return uri.toString();
}

//=============================================================================================================

bool Natus::start()
{
    mne::register_natus_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        mne::Log::warning("[Natus::start] {}", QString::fromStdString(device.error().text()).toStdString());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        mne::Log::warning("[Natus::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }

    //The channels and rate can change on every start, so the output is set up here and not in init()
    m_pRMTSA_Natus->measurementData()->initFromFiffInfo(std::make_shared<mne::FiffInfo>(std::move(*fiffInfo)));
    m_pRMTSA_Natus->measurementData()->setMultiArraySize(1);
    m_pRMTSA_Natus->measurementData()->setVisibility(true);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        mne::Log::warning("[Natus::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool Natus::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRMTSA_Natus->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Natus::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString Natus::getName() const
{
    return "Natus EEG";
}

//=============================================================================================================

QWidget* Natus::setupWidget()
{
    return new NatusSetup(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void Natus::run()
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
            mne::Log::warning("[Natus::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRMTSA_Natus->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString Natus::getBuildInfo()
{
    return QString(NATUSPLUGIN::buildDateTime()) + QString(" - ") + QString(NATUSPLUGIN::buildHash());
}
