//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    BabyMEG class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "babymeg.h"

#include "FormFiles/babymegsetupwidget.h"

#include <mne/acq/acq_device_registry.h>
#include <mne/acq/devices/acq_devices.h>
#include <mne/acq/fiff/acq_fiff.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_stream.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <fstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG() = default;

//=============================================================================================================

BabyMEG::~BabyMEG()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> BabyMEG::clone() const
{
    return QSharedPointer<BabyMEG>(new BabyMEG());
}

//=============================================================================================================

void BabyMEG::init()
{
    m_pRTMSABabyMEG = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BabyMEG Output", "BabyMEG");
    m_pRTMSABabyMEG->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSABabyMEG);
}

//=============================================================================================================

void BabyMEG::unload()
{
}

//=============================================================================================================

std::string BabyMEG::uri() const
{
    mne::AcqUri uri{.scheme = "babymeg", .target = {}, .params = {}};
    if (m_bSimulate) {
        uri.params.emplace_back("simulate", "1");
    } else {
        uri.target = m_host;
        uri.params = {{"port", std::to_string(m_port)}, {"command_port", std::to_string(m_commandPort)}};
    }
    return uri.toString();
}

//=============================================================================================================

void BabyMEG::addSystemInfo(FiffInfo& info)
{
    const std::filesystem::path resources = QCoreApplication::applicationDirPath().toStdString() + "/../resources/mne_scan/plugins/babymeg";
    if (auto header = FiffStream::open(resources / "header.fif")) {
        info.projs = header->read_proj(header->dirtree());
        for (FiffProj& proj : info.projs) {
            proj.active = false;
        }
    }
    if (auto compensators = FiffStream::open(resources / "compensator.fif")) {
        info.comps = compensators->read_ctf_comp(compensators->dirtree(), info.chs);
    }
    std::ifstream bads(resources / "babymeg_bad_channels.txt");
    for (std::string channel; std::getline(bads, channel);) {
        if (channel.ends_with('\r')) {
            channel.pop_back();
        }
        if (!channel.empty()) {
            info.bads.push_back(channel);
        }
    }
}

//=============================================================================================================

bool BabyMEG::start()
{
    mne::register_babymeg_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        Log::warning("[BabyMEG::start] {}", QString::fromStdString(device.error().text()).toStdString());
        return false;
    }
    m_device = std::move(*device);
    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        Log::warning("[BabyMEG::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }

    //The server sends its current gains on every start, so the output is set up here and not in init()
    auto info = std::make_shared<FiffInfo>(std::move(*fiffInfo));
    addSystemInfo(*info);
    m_pRTMSABabyMEG->measurementData()->initFromFiffInfo(info);
    m_pRTMSABabyMEG->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        Log::warning("[BabyMEG::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool BabyMEG::stop()
{
    requestInterruption();
    wait();
    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSABabyMEG->measurementData()->clear();
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType BabyMEG::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString BabyMEG::getName() const
{
    return "BabyMEG";
}

//=============================================================================================================

QWidget* BabyMEG::setupWidget()
{
    return new BabyMEGSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
}

//=============================================================================================================

void BabyMEG::run()
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
            Log::warning("[BabyMEG::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRTMSABabyMEG->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString BabyMEG::getBuildInfo()
{
    return QString(BABYMEGPLUGIN::buildDateTime()) + QString(" - ") + QString(BABYMEGPLUGIN::buildHash());
}
