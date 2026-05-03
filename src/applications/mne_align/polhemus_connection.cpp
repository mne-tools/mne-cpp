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
 * @brief    Polhemus digitizer connection — mock backend implementation.
 */

#include "polhemus_connection.h"

#include <QtMath>

using namespace MNEALIGN;

namespace {
constexpr int    kMockTickIntervalMs = 100;
constexpr float  kMockSphereRadiusM  = 0.10f;   // 10 cm (typical adult head)
constexpr int    kMockPointsPerLap   = 60;
}

//=============================================================================================================

PolhemusConnection::PolhemusConnection(QObject* parent)
    : QObject(parent)
{
    m_mockTimer.setInterval(kMockTickIntervalMs);
    connect(&m_mockTimer, &QTimer::timeout, this, &PolhemusConnection::onMockTick);
}

//=============================================================================================================

PolhemusConnection::~PolhemusConnection()
{
    close();
}

//=============================================================================================================

bool PolhemusConnection::open(const QString& portName)
{
    if (m_isConnected) {
        return true;
    }

    if (!portName.isEmpty()) {
#ifdef MNE_ALIGN_HAS_SERIALPORT
        // Real serial backend not yet wired; fall through to mock so the
        // app remains usable on machines without a Polhemus device.
        Q_UNUSED(portName);
#endif
    }

    m_backendName = QStringLiteral("mock");
    m_mockSampleIdx = 0;
    m_mockTimer.start();
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
    m_mockTimer.stop();
    m_isConnected = false;
    m_backendName.clear();
    emit connectedChanged(false);
}

//=============================================================================================================

bool PolhemusConnection::isConnected() const
{
    return m_isConnected;
}

//=============================================================================================================

QString PolhemusConnection::backendName() const
{
    return m_backendName;
}

//=============================================================================================================

void PolhemusConnection::onMockTick()
{
    // Sweep around the equator of a 10 cm sphere centred at the origin —
    // good enough to drive the wizard's HSP step end-to-end for tests and
    // visual smoke checks.
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
