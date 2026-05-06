//=============================================================================================================
/**
 * @file     polhemus_connection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Polhemus digitizer connection implementation.
 */

#include "polhemus_connection.h"

#include <QtMath>

using namespace MNEALIGN;

namespace {
constexpr int    kMockTickIntervalMs = 100;
constexpr float  kMockSphereRadiusM  = 0.10f;   // 10 cm (typical adult head)
constexpr int    kMockPointsPerLap   = 60;
constexpr int    kStreamPauseMs      = 300;      // pen-button pause threshold
}

//=============================================================================================================

PolhemusConnection::PolhemusConnection(QObject* parent)
    : QObject(parent)
{
    m_mockTimer.setInterval(kMockTickIntervalMs);
    connect(&m_mockTimer, &QTimer::timeout, this, &PolhemusConnection::onMockTick);

    m_streamPauseTimer.setSingleShot(true);
    m_streamPauseTimer.setInterval(kStreamPauseMs);
    connect(&m_streamPauseTimer, &QTimer::timeout, this, &PolhemusConnection::onStreamPauseTimeout);
}

//=============================================================================================================

PolhemusConnection::~PolhemusConnection()
{
    close();
}

//=============================================================================================================

bool PolhemusConnection::open(const QString& portName, const PolhemusSerialConfig& cfg)
{
    if (m_isConnected) {
        return true;
    }

    if (!portName.isEmpty()) {
        if (openSerial(portName, cfg)) {
            m_isConnected = true;
            emit connectedChanged(true);
            return true;
        }
        // Serial open failed; do NOT silently fall back — surface the error.
        return false;
    }

    if (!openMock()) {
        return false;
    }

    m_isConnected = true;
    emit connectedChanged(true);
    return true;
}

//=============================================================================================================

void PolhemusConnection::close()
{
    if (!m_isConnected) {
        return;
    }
    if (m_pSerial) {
        closeSerial();
    } else {
        closeMock();
    }
    m_isConnected = false;
    m_backendName.clear();
    emit connectedChanged(false);
}

//=============================================================================================================

bool PolhemusConnection::isConnected() const
{
    return m_isConnected;
}

QString PolhemusConnection::backendName() const
{
    return m_backendName;
}

//=============================================================================================================
// Port discovery helpers
//=============================================================================================================

QStringList PolhemusConnection::availablePorts()
{
    QStringList names;
    const auto ports = QSerialPortInfo::availablePorts();
    names.reserve(ports.size());
    for (const auto& info : ports) {
        names << info.portName();
    }
    return names;
}

QString PolhemusConnection::autoDetectPortName()
{
    // Polhemus FastTrak/FastSCAN units use one of two USB-serial bridges:
    //   * Polhemus-branded FTDI VID 0x0F44
    //   * Generic FTDI VID 0x0403 (older / OEM units)
    // We prefer the Polhemus VID, then fall back to any FTDI device.
    constexpr quint16 kPolhemusVid = 0x0F44;
    constexpr quint16 kFtdiVid     = 0x0403;

    QString ftdiCandidate;
    for (const auto& info : QSerialPortInfo::availablePorts()) {
        if (!info.hasVendorIdentifier()) {
            continue;
        }
        const quint16 vid = info.vendorIdentifier();
        if (vid == kPolhemusVid) {
            return info.portName();
        }
        if (vid == kFtdiVid && ftdiCandidate.isEmpty()) {
            ftdiCandidate = info.portName();
        }
    }
    return ftdiCandidate;
}

//=============================================================================================================
// Mock backend
//=============================================================================================================

bool PolhemusConnection::openMock()
{
    m_backendName   = QStringLiteral("mock");
    m_mockSampleIdx = 0;
    m_mockTimer.start();
    return true;
}

void PolhemusConnection::closeMock()
{
    m_mockTimer.stop();
}

void PolhemusConnection::onMockTick()
{
    // Sweep around the equator of a 10 cm sphere centred at the origin —
    // good enough to drive the wizard's HSP step end-to-end for tests
    // and visual smoke checks.
    const double phase = (2.0 * M_PI * (m_mockSampleIdx % kMockPointsPerLap))
                         / static_cast<double>(kMockPointsPerLap);
    const double elevation = 0.3 * std::sin(phase * 3.0);
    const QVector3D pos(
        kMockSphereRadiusM * static_cast<float>(std::cos(phase)),
        kMockSphereRadiusM * static_cast<float>(std::sin(phase)),
        kMockSphereRadiusM * static_cast<float>(elevation));
    ++m_mockSampleIdx;
    emit pointReceived(/*station*/ 1, pos, QQuaternion());
}

//=============================================================================================================
// Serial / FastTrak backend
//=============================================================================================================

bool PolhemusConnection::openSerial(const QString& portName, const PolhemusSerialConfig& cfg)
{
    auto* port = new QSerialPort(this);
    port->setPortName(portName);
    port->setBaudRate(cfg.baudRate);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);

    if (!port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QStringLiteral("Serial open failed (%1): %2")
                               .arg(portName, port->errorString()));
        port->deleteLater();
        return false;
    }

    m_pSerial = port;
    m_parser.reset();
    m_parser.setUnits(cfg.units);

    connect(m_pSerial, &QSerialPort::readyRead, this, &PolhemusConnection::onSerialReadyRead);
    connect(m_pSerial, &QSerialPort::errorOccurred, this, &PolhemusConnection::onSerialError);

    if (!cfg.streamCommand.isEmpty()) {
        m_pSerial->write(cfg.streamCommand);
    }

    m_backendName = QStringLiteral("FastTrak (%1 @ %2 baud)")
                        .arg(portName)
                        .arg(cfg.baudRate);
    return true;
}

void PolhemusConnection::closeSerial()
{
    if (!m_pSerial) {
        return;
    }
    m_streamPauseTimer.stop();
    if (m_pSerial->isOpen()) {
        // Stop continuous streaming politely before closing.
        m_pSerial->write(QByteArrayLiteral("P\r"));
        m_pSerial->flush();
        m_pSerial->close();
    }
    m_pSerial->deleteLater();
    m_pSerial = nullptr;
    m_parser.reset();
}

void PolhemusConnection::onSerialReadyRead()
{
    if (!m_pSerial) {
        return;
    }
    m_parser.append(m_pSerial->readAll());
    drainParser();
}

void PolhemusConnection::onSerialError(QSerialPort::SerialPortError err)
{
    if (err == QSerialPort::NoError) {
        return;
    }
    if (!m_pSerial) {
        return;
    }
    const QString msg = QStringLiteral("Serial error: %1").arg(m_pSerial->errorString());
    emit errorOccurred(msg);

    if (err == QSerialPort::ResourceError || err == QSerialPort::DeviceNotFoundError) {
        // Hardware has gone away — tear down so the UI reflects it.
        close();
    }
}

void PolhemusConnection::drainParser()
{
    FastTrakSample s;
    while (m_parser.nextSample(s)) {
        m_lastSamples[s.station] = {s.position, s.orientation};
        emit pointReceived(s.station, s.position, s.orientation);
    }
    // (Re-)start the pause timer on every batch of data.
    // If the stream goes silent for kStreamPauseMs the pen button was pressed.
    if (m_pSerial) {
        m_streamPauseTimer.start();
    }
}

void PolhemusConnection::onStreamPauseTimeout()
{
    if (!m_pSerial || !m_isConnected) return;

    // The continuous stream has been silent — pen button was pressed.
    // Emit for every station that has accumulated data; the wizard
    // filters by pen-station so only the relevant one triggers capture.
    for (auto it = m_lastSamples.constBegin(); it != m_lastSamples.constEnd(); ++it) {
        emit penButtonPressed(it.key(), it.value().position, it.value().orientation);
    }

    // Restart continuous mode so the live feed resumes on release.
    m_pSerial->write(QByteArrayLiteral("C\r"));
}
