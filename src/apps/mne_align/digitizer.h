//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     digitizer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    The Polhemus digitizer of MNE Align: a polhemus:// stream delivered as Qt signals.
 */

#ifndef MNEALIGN_DIGITIZER_H
#define MNEALIGN_DIGITIZER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq_device.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNEALIGN
//=============================================================================================================

namespace MNEALIGN
{

//=============================================================================================================
/**
 * Reads a Polhemus Fastrak or its simulator through polhemus:// on the GUI thread's event loop.
 */
class Digitizer : public QObject
{
    Q_OBJECT

public:
    explicit Digitizer(QObject* parent = nullptr);
    ~Digitizer() override;

    //=========================================================================================================
    /**
     * Opens the digitizer with stylus (1) and tracker (2) and starts its output.
     *
     * @param[in] port   Serial port of a Fastrak; empty for the simulator.
     *
     * @return Whether the digitizer is streaming; errorOccurred() tells why not.
     */
    bool open(const QString& port);

    /** Stops and closes the digitizer. */
    void close();

    bool isConnected() const;

    /** @return The digitizer, e.g. "Polhemus Fastrak on /dev/cu.usbserial-1". */
    QString description() const;

    /** @return The serial ports of this machine, USB-serial adapters first. */
    static QStringList availablePorts();

signals:
    /** The pose of a station in metres in the transmitter frame. */
    void pointReceived(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);

    /** The stylus button was pressed while the stations were at these poses (sent for every station). */
    void penButtonPressed(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);

    void connectedChanged(bool isConnected);

    void errorOccurred(const QString& message);

private:
    void poll();

    mne::AcqDevice m_device;
    mne::AcqReader m_reader;
    QTimer m_timer;
    std::vector<float> m_frames; // decoded frames, one column per update
    QString m_description;
};

} // namespace MNEALIGN

#endif // MNEALIGN_DIGITIZER_H
