//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the FiffSimulator class.
 */

#ifndef FIFFSIMULATOR_H
#define FIFFSIMULATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"

#include <mne/acq/acq_device.h>
#include <scShared/Plugins/abstractsensor.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::scan
{
class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{

//=============================================================================================================
/**
 * @brief Streams the FIFF file simulator of an mne_rt_server through the rtserver:// source of mne_acq_devices,
 *        which can also simulate the server.
 */
class FIFFSIMULATORSHARED_EXPORT FiffSimulator : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "fiffsimulator.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class FiffSimulatorSetupWidget;

public:
    FiffSimulator();
    ~FiffSimulator() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Connects to the server (or its simulator), reads the measurement info and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and disconnects.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the server's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The rtserver:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSA_FiffSimulator; /**< The data. */

    mne::AcqDevice m_device; /**< The server while streaming. */
    mne::AcqReader m_reader; /**< Reads the server's stream in run(). */

    std::string m_host = "127.0.0.1"; /**< Host of mne_rt_server. */
    std::uint16_t m_port = 4217;      /**< Its command port; the data port is the next one. */
    std::string m_simfile;            /**< File the server replays; empty keeps its file. */
    int m_iSamplesPerBlock = 100;     /**< Samples per buffer of the server and per block handed on. */
    bool m_bSimulate = false;         /**< Simulate the server. */
};
} // NAMESPACE

#endif // FIFFSIMULATOR_H
