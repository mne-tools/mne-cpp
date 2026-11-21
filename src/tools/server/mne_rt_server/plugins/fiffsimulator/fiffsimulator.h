//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffSimulator class.
 */

#ifndef FIFFSIMULATOR_H
#define FIFFSIMULATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"
#include <rtserver_connector/iconnector.h>
#include <acq/acq_device.h>
#include <acq/acq_reader.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMutex>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORRTSERVERPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORRTSERVERPLUGIN
{

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulator
 *
 * @brief The FiffSimulator class serves a FIFF file in a loop, replayed through the fiff:// acquisition source.
 */
class FIFFSIMULATORSHARED_EXPORT FiffSimulator : public RTSERVER::IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "fiffsimulator.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(RTSERVER::IConnector)

public:
    struct Commands
    {
        static const QString BUFSIZE;
        static const QString GETBUFSIZE;
        static const QString ACCEL;
        static const QString GETACCEL;
        static const QString SIMFILE;
    };

    //=========================================================================================================
    /**
     * Constructs a FiffSimulator.
     */
    FiffSimulator();

    //=========================================================================================================
    /**
     * Destroys the FiffSimulator.
     */
    virtual ~FiffSimulator();

    virtual void connectCommandManager();

    virtual RTSERVER::ConnectorID getConnectorID() const;

    virtual const char* getName() const;

    virtual void info(qint32 ID);

    virtual bool start();

    virtual bool stop();

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
     * Sets the buffer sample size
     *
     * @param[in] p_command  The buffer sample size command.
     */
    void comBufsize(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Returns the buffer sample size
     *
     * @param[in] p_command  The buffer sample size command.
     */
    void comGetBufsize(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Sets the acceleration factor
     *
     * @param[in] p_command  The acceleration factor command.
     */
    void comAccel(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Returns the acceleration factor
     *
     * @param[in] p_command  The acceleration factor command.
     */
    void comGetAccel(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Sets the fiff simulation file
     *
     * @param[in] p_command  The fiff simulation file command.
     */
    void comSimfile(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Initialise the FiffSimulator.
     */
    void init();

    //=========================================================================================================
    /**
     * Read the raw FiffInfo.
     */
    bool readRawInfo();

    QMutex mutex;

    std::optional<mne::AcqDevice> m_device; /**< The fiff:// replay while running. */
    std::optional<mne::AcqReader> m_reader; /**< Reader of the replay's stream. */
    mne::FiffRawData m_RawInfo;             /**< Holds the fiff raw measurement information. */
    QString m_sResourceDataPath;            /**< Holds the path to the Fiff resource simulation file directory.*/
    quint32 m_uiBufferSampleSize;           /**< Sample size of the buffer. */
    float m_AccelerationFactor;             /**< Acceleration factor to simulate different sampling rates. */
    float m_TrueSamplingRate;               /**< The true sampling rate of the fif file. */
    bool m_bIsRunning;                      /**< Flag whether the producer is running.*/
};
} // NAMESPACE

#endif // FIFFSIMULATOR_H
