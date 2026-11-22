//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     brainflowboard.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the BrainFlowBoard class.
 */

#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowboard_global.h"

#include <mne/acq/acq_device.h>
#include <scShared/Plugins/abstractsensor.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::scan
{
class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE BRAINFLOWBOARDPLUGIN
//=============================================================================================================

namespace BRAINFLOWBOARDPLUGIN
{

//=============================================================================================================
/**
 * @brief Streams any board BrainFlow supports through the brainflow:// source of mne_acq_devices, which loads an
 *        installed BrainFlow at run time.
 */
class BRAINFLOWBOARD_EXPORT BrainFlowBoard : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainflowboard.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class BrainFlowSetupWidget;

public:
    BrainFlowBoard();
    ~BrainFlowBoard() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Opens the board through BrainFlow and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and releases the board.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the board's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The brainflow:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pOutput; /**< The data. */

    mne::AcqDevice m_device; /**< The board while streaming. */
    mne::AcqReader m_reader; /**< Reads its stream in run(). */

    std::string m_board = "synthetic"; /**< BrainFlow board id or name. */
    std::string m_serial_port;         /**< BrainFlow input parameters, empty when not needed. */
    std::string m_mac_address;
    std::string m_ip_address;
    int m_ip_port = 0;
    int m_ip_protocol = 0; /**< 0 none, 1 UDP, 2 TCP. */
    std::string m_other_info;
    std::string m_serial_number;
    std::string m_file;
    std::string m_library;         /**< Path of BrainFlow's BoardController; empty to search for it. */
    int m_samples_per_block = 100; /**< Samples per block handed to the connected plugins. */
};
} // NAMESPACE

#endif // BRAINFLOWBOARD_H
