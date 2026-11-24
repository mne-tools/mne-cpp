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

#include <rtserver_connector/iconnector.h>

#include <mne/acq/acq_device.h>
#include <mne/acq/acq_reader.h>
#include <mne/fiff/fiff_raw_data.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <memory>
#include <string>
#include <thread>

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORRTSERVERPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORRTSERVERPLUGIN
{

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulator
 *
 * @brief The FiffSimulator connector replays a FIFF file as raw buffers.
 */
class FiffSimulator : public RTSERVER::IConnector
{
public:
    /** The commands of the connector. */
    struct Commands
    {
        static constexpr const char* BUFSIZE = "bufsize";
        static constexpr const char* GETBUFSIZE = "getbufsize";
        static constexpr const char* ACCEL = "accel";
        static constexpr const char* GETACCEL = "getaccel";
        static constexpr const char* SIMFILE = "simfile";
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
    ~FiffSimulator() override;

    void connectCommandManager() override;

    RTSERVER::ConnectorID getConnectorID() const override;

    const char* getName() const override;

    const char* getMetaData() const override;

    void info(std::int32_t ID) override;

    bool start() override;

    bool stop() override;

private:
    //=========================================================================================================
    /**
     * Sends raw buffers until the connector is stopped.
     */
    void run();

    void comBufsize(const RTSERVER::Command& p_command);
    void comGetBufsize(const RTSERVER::Command& p_command);
    void comAccel(const RTSERVER::Command& p_command);
    void comGetAccel(const RTSERVER::Command& p_command);
    void comSimfile(const RTSERVER::Command& p_command);

    //=========================================================================================================
    /**
     * Reads the simulation file name from FiffSimulation.cfg.
     */
    void init();

    //=========================================================================================================
    /**
     * Reads the measurement info of the simulation file, unless it is read already.
     *
     * @return True if the measurement info is available.
     */
    bool readRawInfo();

    std::mutex mutex; /**< Guards reading the measurement info. */

    std::optional<mne::AcqDevice> m_device; /**< The fiff:// replay while running. */
    std::optional<mne::AcqReader> m_reader; /**< Reader of the replay's stream. */
    mne::FiffRawData m_RawInfo;             /**< Holds the fiff raw measurement information. */
    std::string m_sResourceDataPath;        /**< Holds the path to the Fiff resource simulation file.*/
    std::uint32_t m_uiBufferSampleSize;     /**< Sample size of the buffer. */
    float m_AccelerationFactor;             /**< Acceleration factor to simulate different sampling rates. */
    float m_TrueSamplingRate;               /**< The true sampling rate of the fif file. */
    std::atomic<bool> m_bIsRunning;         /**< Flag whether the producer is running.*/
    std::thread m_thread;                   /**< Producer thread while running. */
};

} // NAMESPACE

#endif // FIFFSIMULATOR_H
