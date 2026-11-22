//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Definition of the FtBuffer class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer.h"

#include "FormFiles/ftbuffersetupwidget.h"
#include <base/log.h>
#include <acq/acq_device_registry.h>
#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBuffer::FtBuffer() = default;

//=============================================================================================================

FtBuffer::~FtBuffer()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> FtBuffer::clone() const
{
    return QSharedPointer<FtBuffer>(new FtBuffer());
}

//=============================================================================================================

void FtBuffer::init()
{
    m_pRTMSA_BufferOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "FtBuffer", "Output data");
    m_pRTMSA_BufferOutput->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_BufferOutput);
}

//=============================================================================================================

void FtBuffer::unload()
{
}

//=============================================================================================================

std::string FtBuffer::uri() const
{
    mne::AcqUri uri{.scheme = "ftbuffer", .target = {}, .params = {{"simulate", "1"}}};
    if (!m_bSimulate) {
        uri.target = m_host + ":" + std::to_string(m_port);
        uri.params.clear();
    }
    return uri.toString();
}

//=============================================================================================================

bool FtBuffer::start()
{
    mne::register_ftbuffer_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        mne::Log::warning("[FtBuffer::start] {}", QString::fromStdString(device.error().text()).toStdString());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        mne::Log::warning("[FtBuffer::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }

    //The buffer can hold another measurement on every start, so it is set up here and not in init()
    auto info = std::make_shared<mne::FiffInfo>(std::move(*fiffInfo));
    if (info->dig.size() > 0) {
        // neuromag2ft puts the acquisition's isotrak, which the HPI plugin fits the coils to
        auto digitizers = std::make_shared<mne::FiffDigitizerData>();
        digitizers->points = info->dig;
        digitizers->coord_frame = FIFFV_COORD_HEAD;
        digitizers->active.assign(info->dig.size(), true);
        digitizers->discard.assign(info->dig.size(), false);
        m_pRTMSA_BufferOutput->measurementData()->setDigitizerData(digitizers);
    }
    m_pRTMSA_BufferOutput->measurementData()->initFromFiffInfo(info);
    m_pRTMSA_BufferOutput->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        mne::Log::warning("[FtBuffer::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool FtBuffer::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_BufferOutput->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType FtBuffer::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString FtBuffer::getName() const
{
    return "FtBuffer";
}

//=============================================================================================================

QWidget* FtBuffer::setupWidget()
{
    return new FtBufferSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void FtBuffer::run()
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
            mne::Log::warning("[FtBuffer::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRTMSA_BufferOutput->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString FtBuffer::getBuildInfo()
{
    return QString(FTBUFFERPLUGIN::buildDateTime()) + QString(" - ") + QString(FTBUFFERPLUGIN::buildHash());
}
