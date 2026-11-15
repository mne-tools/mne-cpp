//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     brainflowboard.cpp
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Definition of the BrainFlowBoard class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowboard.h"

#include "FormFiles/brainflowsetupwidget.h"

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

using namespace BRAINFLOWBOARDPLUGIN;
using namespace mne::scan;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowBoard::BrainFlowBoard() = default;

//=============================================================================================================

BrainFlowBoard::~BrainFlowBoard()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> BrainFlowBoard::clone() const
{
    return QSharedPointer<BrainFlowBoard>(new BrainFlowBoard());
}

//=============================================================================================================

void BrainFlowBoard::init()
{
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainFlowBoard", "BrainFlow Board Output");
    m_pOutput->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pOutput);
}

//=============================================================================================================

void BrainFlowBoard::unload()
{
}

//=============================================================================================================

std::string BrainFlowBoard::uri() const
{
    // Only the parameters that are set: BrainFlow's defaults stand for the others
    mne::AcqUri uri{.scheme = "brainflow", .target = m_board, .params = {}};
    const auto add = [&uri](const char* key, const std::string& value) {
        if (!value.empty()) {
            uri.params.emplace_back(key, value);
        }
    };
    add("serial_port", m_serial_port);
    add("mac_address", m_mac_address);
    add("ip_address", m_ip_address);
    add("ip_port", m_ip_port != 0 ? std::to_string(m_ip_port) : std::string());
    add("ip_protocol", m_ip_protocol != 0 ? std::to_string(m_ip_protocol) : std::string());
    add("other_info", m_other_info);
    add("serial_number", m_serial_number);
    add("file", m_file);
    add("library", m_library);
    return uri.toString();
}

//=============================================================================================================

bool BrainFlowBoard::start()
{
    mne::register_brainflow_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        qWarning() << "[BrainFlowBoard::start]" << QString::fromStdString(device.error().text());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        qWarning() << "[BrainFlowBoard::start]" << QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text());
        m_device.close();
        return false;
    }

    //Every board has its own rows, so the output is set up here and not in init()
    m_pOutput->measurementData()->initFromFiffInfo(std::make_shared<mne::FiffInfo>(std::move(*fiffInfo)));
    m_pOutput->measurementData()->setMultiArraySize(1);
    m_pOutput->measurementData()->setVisibility(true);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        qWarning() << "[BrainFlowBoard::start]" << QString::fromStdString(e.text());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool BrainFlowBoard::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pOutput->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType BrainFlowBoard::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString BrainFlowBoard::getName() const
{
    return "BrainFlow";
}

//=============================================================================================================

QWidget* BrainFlowBoard::setupWidget()
{
    return new BrainFlowSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void BrainFlowBoard::run()
{
    const std::size_t channels = m_device.stream().info()->channelCount();
    MatrixXd block(static_cast<Index>(channels), m_samples_per_block);
    while (!isInterruptionRequested()) {
        const auto read = m_reader.read(mne::AcqMatrixRef<double>::columnMajor(block.data(), channels, static_cast<std::size_t>(block.cols())),
                                        std::chrono::milliseconds(100));
        if (!read) {
            if (read.error() == mne::AcqErrorCode::Timeout || read.error() == mne::AcqErrorCode::Overrun) {
                continue;
            }
            qWarning() << "[BrainFlowBoard::run]" << QString::fromStdString(read.error().text());
            return;
        }
        m_pOutput->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString BrainFlowBoard::getBuildInfo()
{
    return QString(BRAINFLOWBOARDPLUGIN::buildDateTime()) + QString(" - ") + QString(BRAINFLOWBOARDPLUGIN::buildHash());
}
