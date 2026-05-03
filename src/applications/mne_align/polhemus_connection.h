//=============================================================================================================
/**
 * @file     polhemus_connection.h
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
 * @brief    Polhemus digitizer connection abstraction.
 *
 *           Provides a virtual backend interface so the real serial-port
 *           driver (Polhemus FastSCAN ASCII protocol) can be swapped in
 *           without touching the wizard. The default implementation is a
 *           mock backend that synthesises HSP points on a unit sphere; it
 *           keeps the app usable on machines without a real digitizer
 *           and gives unit tests a deterministic source of points.
 */

#ifndef MNE_ALIGN_POLHEMUS_CONNECTION_H
#define MNE_ALIGN_POLHEMUS_CONNECTION_H

#include <QObject>
#include <QQuaternion>
#include <QScopedPointer>
#include <QString>
#include <QTimer>
#include <QVector3D>

namespace MNEALIGN
{

//=============================================================================================================
/**
 * @brief Abstract Polhemus digitizer connection.
 *
 * Real subclass reads frames from a `QSerialPort`. The mock backend
 * (default when SerialPort isn't available, or when @p portName is empty)
 * emits a scripted sequence of points over a `QTimer`.
 */
class PolhemusConnection : public QObject
{
    Q_OBJECT

public:
    explicit PolhemusConnection(QObject* parent = nullptr);
    ~PolhemusConnection() override;

    /** Open the connection. An empty @p portName selects the mock backend. */
    bool    open(const QString& portName);

    /** Close the active connection. Safe to call when already closed. */
    void    close();

    bool    isConnected() const;
    QString backendName() const;

signals:
    /**
     * @param station  Polhemus station id (1..4 for FastSCAN).
     * @param position Sample point in metres, sensor-frame.
     * @param orientation Optional orientation quaternion (identity if N/A).
     */
    void pointReceived(int station, const QVector3D& position, const QQuaternion& orientation);

    /** Emitted whenever @ref isConnected changes. */
    void connectedChanged(bool isConnected);

    /** Non-fatal protocol/IO error (already logged by the backend). */
    void errorOccurred(const QString& message);

private slots:
    void onMockTick();

private:
    bool        m_isConnected = false;
    QString     m_backendName;

    // Mock backend state ------------------------------------------------------
    QTimer      m_mockTimer;
    int         m_mockSampleIdx = 0;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_POLHEMUS_CONNECTION_H
