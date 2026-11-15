//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromag.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    Contains the declaration of the Neuromag class.
 */

#ifndef NEUROMAG_H
#define NEUROMAG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_global.h"

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
// DEFINE NAMESPACE NEUROMAGPLUGIN
//=============================================================================================================

namespace NEUROMAGPLUGIN
{

//=============================================================================================================
/**
 * Neuromag...
 *
 * @brief MEG connector for the data acquisition of Neuromag (MEGIN) Vectorview and TRIUX systems through the
 *        neuromag:// source of mne_acq_devices, which can also simulate it.
 */
class NEUROMAGSHARED_EXPORT Neuromag : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuromag.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class NeuromagSetupWidget;

public:
    Neuromag();
    ~Neuromag() override;

    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;

    //=========================================================================================================
    /**
     * Opens the acquisition (or its simulator), which starts a measurement, and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and the measurement.
     */
    bool stop() override;

    mne::scan::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

protected:
    //=========================================================================================================
    /**
     * Hands the acquisition's blocks to the connected plugins until stop().
     */
    void run() override;

private:
    /** @return The neuromag:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSA_Neuromag; /**< The MEG data. */

    mne::AcqDevice m_device; /**< The acquisition while streaming. */
    mne::AcqReader m_reader; /**< Reads the acquisition's stream in run(). */

    std::string m_root = "/neuro/dacq"; /**< Directory of the acquisition's sockets, shared memory and files. */
    int m_iBufferSize = 0;              /**< Samples per data buffer to set; 0 keeps the acquisition's setting. */
    int m_iSamplesPerBlock = 100;       /**< Samples per block handed to the connected plugins. */
    bool m_bAutostart = true;           /**< Start and stop measurements through the collector. */
    bool m_bSimulate = false;           /**< Simulate the acquisition. */
};
} // NAMESPACE

#endif // NEUROMAG_H
