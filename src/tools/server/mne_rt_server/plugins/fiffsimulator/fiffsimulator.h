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
#include "../../mne_rt_server/IConnector.h"

#include <fiff/fiff_raw_data.h>
#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMutex>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORRTSERVERPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORRTSERVERPLUGIN
{

//=============================================================================================================
// FIFFSIMULATORRTSERVERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class FiffProducer;

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulator
 *
 * @brief The FiffSimulator class provides a Fiff data simulator.
 */
class FIFFSIMULATORSHARED_EXPORT FiffSimulator : public RTSERVER::IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "fiffsimulator.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(RTSERVER::IConnector)

    friend class FiffProducer;

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
    void comBufsize(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Returns the buffer sample size
     *
     * @param[in] p_command  The buffer sample size command.
     */
    void comGetBufsize(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Sets the acceleration factor
     *
     * @param[in] p_command  The acceleration factor command.
     */
    void comAccel(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Returns the acceleration factor
     *
     * @param[in] p_command  The acceleration factor command.
     */
    void comGetAccel(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Sets the fiff simulation file
     *
     * @param[in] p_command  The fiff simulation file command.
     */
    void comSimfile(COMLIB::Command p_command);

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

    FiffProducer*                           m_pFiffProducer;        /**< Holds the DataProducer.*/
    UTILSLIB::CircularBuffer_Matrix_float*  m_pRawMatrixBuffer;     /**< The Circular Raw Matrix Buffer. */
    FIFFLIB::FiffRawData                    m_RawInfo;              /**< Holds the fiff raw measurement information. */
    QString                                 m_sResourceDataPath;    /**< Holds the path to the Fiff resource simulation file directory.*/
    quint32                                 m_uiBufferSampleSize;   /**< Sample size of the buffer. */
    float                                   m_AccelerationFactor;   /**< Acceleration factor to simulate different sampling rates. */
    float                                   m_TrueSamplingRate;     /**< The true sampling rate of the fif file. */
    bool                                    m_bIsRunning;           /**< Flag whether the producer is running.*/
};
} // NAMESPACE

#endif // FIFFSIMULATOR_H
