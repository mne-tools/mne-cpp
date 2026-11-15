//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Contains the declaration of the FtBuffer class.
 */

#ifndef FTBUFFER_H
#define FTBUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"

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
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
/**
 * FtBuffer...
 *
 * @brief Reads a FieldTrip buffer server, e.g. the one neuromag2ft fills on a Neuromag acquisition computer,
 *        through the ftbuffer:// source of mne_acq_devices, which can also simulate it.
 */
class FTBUFFER_EXPORT FtBuffer : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ftbuffer.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class FtBufferSetupWidget;

public:
    FtBuffer();
    ~FtBuffer() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Connects to the buffer (or its simulator), waits for the producer's header and starts streaming.
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
     * Hands the buffer's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The ftbuffer:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSA_BufferOutput; /**< The data. */

    mne::AcqDevice m_device; /**< The buffer while streaming. */
    mne::AcqReader m_reader; /**< Reads the buffer's stream in run(). */

    std::string m_host = "localhost"; /**< Host of the buffer server. */
    std::uint16_t m_port = 1972;      /**< Port of the buffer server. */
    int m_iSamplesPerBlock = 100;     /**< Samples per block handed to the connected plugins. */
    bool m_bSimulate = false;         /**< Simulate neuromag2ft and its buffer. */
};
} // NAMESPACE

#endif // FTBUFFER_H
