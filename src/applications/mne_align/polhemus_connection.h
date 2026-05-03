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
 *           Two backends share one signal interface:
 *
 *           1. **Mock** — emits a scripted sweep over a 10 cm sphere
 *              every @c kMockTickIntervalMs. Always available so the app
 *              and tests run on machines without a real digitizer.
 *
 *           2. **FastTrak / FastSCAN** — opens a `QSerialPort`, sends
 *              the continuous-output command, and decodes the ASCII
 *              record stream via @ref FastTrakParser. Compiled in iff
 *              the `Qt::SerialPort` module is available (controlled by
 *              the `MNE_ALIGN_HAS_SERIALPORT` define).
 *
 *           Backend selection is implicit: an empty @c portName picks
 *           mock; a non-empty name and SerialPort support pick the
 *           hardware backend (falling back to mock when SerialPort is
 *           not built in).
 */

#ifndef MNE_ALIGN_POLHEMUS_CONNECTION_H
#define MNE_ALIGN_POLHEMUS_CONNECTION_H

#include "fasttrak_parser.h"

#include <QObject>
#include <QQuaternion>
#include <QString>
#include <QTimer>
#include <QVector3D>

#ifdef MNE_ALIGN_HAS_SERIALPORT
#include <QSerialPort>
#endif

namespace MNEALIGN
{

//=============================================================================================================
/**
 * @brief Settings used to open a real Polhemus serial connection.
 */
struct PolhemusSerialConfig
{
    int                       baudRate = 115200;
    FastTrakParser::Units     units    = FastTrakParser::Units::Centimetres;
    /** Stream control command sent right after opening the port. The
     *  factory default for FastTrak is `"C\r"` (continuous ASCII output);
     *  use `"P\r"` if the application drives polled mode itself. */
    QByteArray                streamCommand = QByteArrayLiteral("C\r");
};

//=============================================================================================================
/**
 * @brief Polhemus digitizer connection (mock + serial-port backends).
 */
class PolhemusConnection : public QObject
{
    Q_OBJECT

public:
    explicit PolhemusConnection(QObject* parent = nullptr);
    ~PolhemusConnection() override;

    /**
     * Open the connection.
     *
     * @param portName  Empty → mock backend. Non-empty → hardware
     *                  backend (when compiled in).
     * @param cfg       Serial transport configuration; ignored by the
     *                  mock backend.
     */
    bool    open(const QString& portName,
                 const PolhemusSerialConfig& cfg = PolhemusSerialConfig{});

    /** Close the active connection. Safe to call when already closed. */
    void    close();

    bool    isConnected() const;
    QString backendName() const;

signals:
    /**
     * @param station     Polhemus station id (1..4 for FastSCAN).
     * @param position    Sample point in metres, sensor-frame.
     * @param orientation Sensor orientation; identity if N/A.
     */
    void pointReceived(int station, const QVector3D& position, const QQuaternion& orientation);

    /** Emitted whenever @ref isConnected changes. */
    void connectedChanged(bool isConnected);

    /** Non-fatal protocol/IO error (already logged by the backend). */
    void errorOccurred(const QString& message);

private slots:
    void onMockTick();
#ifdef MNE_ALIGN_HAS_SERIALPORT
    void onSerialReadyRead();
    void onSerialError(QSerialPort::SerialPortError err);
#endif

private:
    bool openMock();
    void closeMock();

#ifdef MNE_ALIGN_HAS_SERIALPORT
    bool openSerial(const QString& portName, const PolhemusSerialConfig& cfg);
    void closeSerial();
    void drainParser();
#endif

    bool        m_isConnected = false;
    QString     m_backendName;

    // Mock backend ------------------------------------------------------------
    QTimer      m_mockTimer;
    int         m_mockSampleIdx = 0;

#ifdef MNE_ALIGN_HAS_SERIALPORT
    // Serial backend ----------------------------------------------------------
    QSerialPort*    m_pSerial = nullptr;
    FastTrakParser  m_parser;
#endif
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_POLHEMUS_CONNECTION_H
