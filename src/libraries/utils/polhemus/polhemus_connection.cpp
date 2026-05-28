//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file polhemus_connection.cpp
 * @since May 2026
 * @brief Backend implementation for @ref UTILSLIB::PolhemusConnection — mock sweep generator and live @c QSerialPort driver.
 *
 * The hardware backend opens the serial port with the
 * documented Fastrak defaults (115 200 8N1, no flow control),
 * issues the continuous-output start command on @c open() and
 * the stop command on @c close(), then forwards every chunk of
 * received bytes to @ref FastrakParser. The mock backend
 * fabricates a deterministic spiral over a 10 cm sphere on a
 * @c QTimer tick so unit tests and demo apps still produce
 * geometry without a tracker attached.
 */

#include "polhemus_connection.h"

#include <QtMath>

using namespace UTILSLIB;

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
    const double phase = (2.0 * M_PI * (m_mockSampleIdx % kMockPointsPerLap))
                         / static_cast<double>(kMockPointsPerLap);
    const double elevation = 0.3 * std::sin(phase * 3.0);
    const QVector3D pos(
        kMockSphereRadiusM * static_cast<float>(std::cos(phase)),
        kMockSphereRadiusM * static_cast<float>(std::sin(phase)),
        kMockSphereRadiusM * static_cast<float>(elevation));
    ++m_mockSampleIdx;

    // Station 1 = pen (stylus — matches default penStation=1)
    const QVector3D penPos(
        kMockSphereRadiusM * static_cast<float>(std::cos(phase + 0.3)),
        kMockSphereRadiusM * static_cast<float>(std::sin(phase + 0.3)),
        kMockSphereRadiusM * static_cast<float>(elevation) + 0.02f);
    const QQuaternion penOri = QQuaternion::fromAxisAndAngle(
        QVector3D(0, 0, 1), static_cast<float>(phase * 180.0 / M_PI));
    emit pointReceived(/*station*/ 1, penPos, penOri);

    // Station 2 = tracker (device pose — matches default trackerStation=2)
    emit pointReceived(/*station*/ 2, pos, QQuaternion());
}

//=============================================================================================================
// Serial / Fastrak backend
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

    // Ensure all four stations are enabled and output position + Euler angles.
    // Fastrak command: "l<station>,1\r" — enable station output
    // Fastrak command: "O<station>,2,4,1\r" — output position (2), Euler angles (4), CRLF (1)
    // NOTE: item 11 (quaternion) is not supported by all Fastrak firmware versions.
    // Harmless for stations without a sensor physically connected.
    for (int st = 1; st <= 4; ++st) {
        m_pSerial->write(QStringLiteral("l%1,1\r").arg(st).toLatin1());
        m_pSerial->write(QStringLiteral("O%1,2,4,1\r").arg(st).toLatin1());
    }
    m_pSerial->flush();

    // Set tracking hemisphere per station if configured (non-zero vector).
    // Fastrak command: "H<station>,x,y,z\r" — prevents position jumps
    // when a sensor crosses the default hemisphere boundary.
    if (!cfg.hemisphere.isNull()) {
        const QVector3D& h = cfg.hemisphere;
        for (int st = 1; st <= 4; ++st) {
            const QByteArray cmd = QStringLiteral("H%1,%2,%3,%4\r")
                .arg(st)
                .arg(h.x(), 0, 'f', 1).arg(h.y(), 0, 'f', 1).arg(h.z(), 0, 'f', 1)
                .toLatin1();
            m_pSerial->write(cmd);
        }
        m_pSerial->flush();
        qInfo() << "Fastrak: hemisphere set to" << cfg.hemisphere;
    }

    if (!cfg.streamCommand.isEmpty()) {
        m_pSerial->write(cfg.streamCommand);
    }

    m_backendName = QStringLiteral("Fastrak (%1 @ %2 baud)")
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
    m_pSerial->disconnect(this);
    if (m_pSerial->isOpen()) {
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
        close();
    }
}

void PolhemusConnection::drainParser()
{
    FastrakSample s;
    while (m_parser.nextSample(s)) {
        m_lastSamples[s.station] = {s.position, s.orientation};
        emit pointReceived(s.station, s.position, s.orientation);
    }
    if (m_pSerial) {
        m_streamPauseTimer.start();
    }
}

void PolhemusConnection::onStreamPauseTimeout()
{
    if (!m_pSerial || !m_isConnected) return;

    for (auto it = m_lastSamples.constBegin(); it != m_lastSamples.constEnd(); ++it) {
        emit penButtonPressed(it.key(), it.value().position, it.value().orientation);
    }

    m_pSerial->write(QByteArrayLiteral("C\r"));
}
