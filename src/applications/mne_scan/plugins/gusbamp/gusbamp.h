//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the declaration of the GUSBAmp class.
 */

#ifndef GUSBAMP_H
#define GUSBAMP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_global.h"

#include <acq/acq_device.h>
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
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
/**
 * GUSBAmp...
 *
 * @brief EEG connector for up to four synchronised g.tec g.USBamps through the gusbamp:// source of
 *        mne_acq_devices, which can also simulate them.
 */
class GUSBAMPSHARED_EXPORT GUSBAmp : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "gusbamp.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class GUSBAmpSetupWidget;

public:
    GUSBAmp();
    ~GUSBAmp() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Opens the amplifiers (or their simulator) and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and releases the amplifiers.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the amplifiers' blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The gusbamp:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSA_GUSBAmp; /**< The EEG data. */

    mne::AcqDevice m_device; /**< The amplifiers while streaming. */
    mne::AcqReader m_reader; /**< Reads the amplifiers' stream in run(). */

    std::string m_serials;         /**< Serial numbers separated by commas, master first; empty for the first connected unit. */
    std::string m_inputs = "1-16"; /**< Inputs of every unit, e.g. 1-8,11. */
    int m_iSampleRate = 1200;      /**< Sampling rate in Hz. */
    int m_iSamplesPerBlock = 64;   /**< Samples per block handed to the connected plugins. */
    bool m_bTrigger = false;       /**< Deliver the master's trigger line. */
    bool m_bSimulate = false;      /**< Simulate the amplifiers. */
};
} // NAMESPACE

#endif // GUSBAMP_H
