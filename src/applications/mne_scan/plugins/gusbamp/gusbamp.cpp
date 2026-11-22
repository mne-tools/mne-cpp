//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the implementation of the GUSBAmp class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp.h"

#include "FormFiles/gusbampsetupwidget.h"
#include <mne/base/log.h>
#include <mne/acq/devices/acq_devices.h>
#include <mne/acq/fiff/acq_fiff.h>
#include <mne/fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAMPPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmp::GUSBAmp() = default;

//=============================================================================================================

GUSBAmp::~GUSBAmp()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> GUSBAmp::clone() const
{
    return QSharedPointer<GUSBAmp>(new GUSBAmp());
}

//=============================================================================================================

void GUSBAmp::init()
{
    m_pRTMSA_GUSBAmp = PluginOutputData<RealTimeMultiSampleArray>::create(this, "GUSBAmp", "EEG output data");
    m_pRTMSA_GUSBAmp->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_GUSBAmp);
}

//=============================================================================================================

void GUSBAmp::unload()
{
}

//=============================================================================================================

std::string GUSBAmp::uri() const
{
    return std::format("gusbamp://{}?rate={}&inputs={}&trigger={}&simulate={}", m_serials, m_iSampleRate, m_inputs, m_bTrigger ? 1 : 0,
                       m_bSimulate ? 1 : 0);
}

//=============================================================================================================

bool GUSBAmp::start()
{
    mne::register_gusbamp_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        mne::Log::warning("[GUSBAmp::start] {}", QString::fromStdString(device.error().text()).toStdString());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        mne::Log::warning("[GUSBAmp::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }

    //The user can change the channels between runs, so the measurement is set up here and not in init()
    m_pRTMSA_GUSBAmp->measurementData()->initFromFiffInfo(std::make_shared<mne::FiffInfo>(std::move(*fiffInfo)));
    m_pRTMSA_GUSBAmp->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        mne::Log::warning("[GUSBAmp::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool GUSBAmp::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_GUSBAmp->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType GUSBAmp::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString GUSBAmp::getName() const
{
    return "GUSBAmp";
}

//=============================================================================================================

QWidget* GUSBAmp::setupWidget()
{
    return new GUSBAmpSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void GUSBAmp::run()
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
            mne::Log::warning("[GUSBAmp::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRTMSA_GUSBAmp->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString GUSBAmp::getBuildInfo()
{
    return QString(GUSBAMPPLUGIN::buildDateTime()) + QString(" - ") + QString(GUSBAMPPLUGIN::buildHash());
}
