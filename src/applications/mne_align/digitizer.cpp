//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     digitizer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    Digitizer: polhemus:// polled on the event loop.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizer.h"

#include <acq/acq_convert.h>
#include <acq/acq_device_registry.h>
#include <acq/devices/acq_devices.h>

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEALIGN;
using namespace mne;

namespace
{

constexpr std::size_t kStations = 2;
constexpr std::size_t kWidth = 7 * kStations; // per station X Y Z, QW QX QY QZ
constexpr int kPollMs = 15;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Digitizer::Digitizer(QObject* parent)
: QObject(parent)
{
    m_timer.setInterval(kPollMs);
    connect(&m_timer, &QTimer::timeout, this, &Digitizer::poll);
}

//=============================================================================================================

Digitizer::~Digitizer() = default;

//=============================================================================================================

bool Digitizer::open(const QString& port)
{
    if (isConnected()) {
        return true;
    }
    register_polhemus_scheme();
    const AcqUri uri{.scheme = "polhemus", .target = port.toStdString(), .params = {{"stations", "1,2"}, {"simulate", port.isEmpty() ? "1" : "0"}}};
    auto device = AcqDevice::open(uri.toString());
    if (!device) {
        emit errorOccurred(QString::fromStdString(device.error().text()));
        return false;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader) {
        emit errorOccurred(QString::fromStdString(reader.error().text()));
        return false;
    }
    if (AcqError e = device->start(); !e.ok()) {
        emit errorOccurred(QString::fromStdString(e.text()));
        return false;
    }
    m_description = QString::fromStdString(device->description().device);
    m_device = std::move(*device);
    m_reader = std::move(*reader);
    m_timer.start();
    emit connectedChanged(true);
    return true;
}

//=============================================================================================================

void Digitizer::close()
{
    if (!isConnected()) {
        return;
    }
    m_timer.stop();
    m_reader = AcqReader();
    m_device.close();
    m_description.clear();
    emit connectedChanged(false);
}

//=============================================================================================================

bool Digitizer::isConnected() const
{
    return m_timer.isActive();
}

//=============================================================================================================

QString Digitizer::description() const
{
    return m_description;
}

//=============================================================================================================

QStringList Digitizer::availablePorts()
{
    register_polhemus_scheme();
    QStringList ports;
    for (const AcqDiscoveredDevice& found : AcqDeviceRegistry::instance().discover(std::chrono::milliseconds(200))) {
        if (found.scheme == "polhemus") {
            ports << QString::fromStdString(found.uri.substr(std::string_view("polhemus://").size()));
        }
    }
    return ports;
}

//=============================================================================================================

void Digitizer::poll()
{
    auto view = m_reader.lease(0, std::chrono::nanoseconds(0));
    if (!view) {
        if (view.error() != AcqErrorCode::Timeout) {
            emit errorOccurred(QString::fromStdString(view.error().text()));
            close();
        }
        return;
    }
    const AcqBlockView& block = **view;
    const std::size_t frames = block.sampleCount();
    m_frames.resize(kWidth * frames);
    if (AcqError e = decode(block, AcqMatrixRef<float>::columnMajor(m_frames.data(), kWidth, frames)); !e.ok()) {
        emit errorOccurred(QString::fromStdString(e.text()));
        return;
    }
    const auto pose = [&](std::size_t frame, std::size_t station) {
        const float* p = &m_frames[frame * kWidth + station * 7];
        return std::pair(Eigen::Vector3f(p[0], p[1], p[2]), Eigen::Quaternionf(p[3], p[4], p[5], p[6]));
    };
    for (std::size_t i = 0; i < frames; ++i) {
        for (std::size_t s = 0; s < kStations; ++s) {
            const auto [position, orientation] = pose(i, s);
            if (position.allFinite()) {
                emit pointReceived(static_cast<int>(s + 1), position, orientation);
            }
        }
    }
    for (std::size_t e = 0; e < block.events().size(); ++e) {
        const std::size_t frame = static_cast<std::size_t>(block.events()[e].sample - block.firstSample());
        for (std::size_t s = 0; s < kStations && frame < frames; ++s) {
            const auto [position, orientation] = pose(frame, s);
            if (position.allFinite()) {
                emit penButtonPressed(static_cast<int>(s + 1), position, orientation);
            }
        }
    }
}
