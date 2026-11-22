//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg.h
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    BabyMEG class declaration.
 */

#ifndef BABYMEG_H
#define BABYMEG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

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

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
/**
 * @brief Streams the acquisition server of a BabyMEG system through the babymeg:// source of mne_acq_devices, which
 *        can also simulate it, with the system's projectors, compensators and bad channels.
 */
class BABYMEGSHARED_EXPORT BabyMEG : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "babymeg.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class BabyMEGSetupWidget;

public:
    BabyMEG();
    ~BabyMEG() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Connects to the server (or its simulator), reads its channels and gains and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and says goodbye to the server.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the system's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The babymeg:// URI of the current settings. */
    std::string uri() const;

    /** Adds the system's projectors, compensators and bad channels from the plugin's resources to @p info. */
    static void addSystemInfo(mne::FiffInfo& info);

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSABabyMEG; /**< The data. */

    mne::AcqDevice m_device; /**< The system while streaming. */
    mne::AcqReader m_reader; /**< Reads its stream in run(). */

    std::string m_host = "localhost";   /**< Host of the acquisition server. */
    std::uint16_t m_port = 6340;        /**< Its data port. */
    std::uint16_t m_commandPort = 6341; /**< Its command port. */
    int m_iSamplesPerBlock = 1000;      /**< Samples per block handed to the connected plugins. */
    bool m_bSimulate = false;           /**< Simulate the server. */
};
} // NAMESPACE

#endif // BABYMEG_H
