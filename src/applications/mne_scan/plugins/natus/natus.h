//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natus.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Contains the declaration of the Natus class.
 */

#ifndef NATUS_H
#define NATUS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus_global.h"

#include <acq/acq_device.h>
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
// DEFINE NAMESPACE NATUSPLUGIN
//=============================================================================================================

namespace NATUSPLUGIN
{

//=============================================================================================================
/**
 * @brief Receives a Natus Quantum amplifier from the datagrams of its streaming machine through the natus:// source
 *        of mne_acq_devices, which can also simulate it.
 */
class NATUSSHARED_EXPORT Natus : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "natus.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class NatusSetup;

public:
    Natus();
    ~Natus() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Receives the streaming machine's datagrams (or simulates them) and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and releases the port.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the amplifier's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The natus:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRMTSA_Natus; /**< The data. */

    mne::AcqDevice m_device; /**< The amplifier while streaming. */
    mne::AcqReader m_reader; /**< Reads its stream in run(). */

    std::string m_address = "0.0.0.0"; /**< Local address the datagrams arrive on. */
    std::uint16_t m_port = 50000;      /**< Port the streaming machine sends to. */
    int m_iNumberChannels = 46;        /**< Channels the streaming machine sends. */
    int m_iSamplingFreq = 2048;        /**< Sampling rate set on the amplifier. */
    int m_iSamplesPerBlock = 256;      /**< Samples per block handed to the connected plugins. */
    bool m_bSimulate = false;          /**< Simulate the streaming machine. */
};
} // NAMESPACE

#endif // NATUS_H
