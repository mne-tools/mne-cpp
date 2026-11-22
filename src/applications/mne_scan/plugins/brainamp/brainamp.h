//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainamp.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the BrainAMP class.
 */

#ifndef BRAINAMP_H
#define BRAINAMP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_global.h"
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
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{

//=============================================================================================================
// BRAINAMPPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMPSetupWidget;
class BrainAMPSetupProjectWidget;

//=============================================================================================================
/**
 * BrainAMP...
 *
 * @brief EEG connector for Brain Products BrainAmp amplifiers through the brainamp:// source of mne_acq_devices,
 *        which can also simulate the amplifier.
 */
class BRAINAMPSHARED_EXPORT BrainAMP : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainamp.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class BrainAMPSetupWidget;
    friend class BrainAMPSetupProjectWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a BrainAMP.
     */
    BrainAMP();

    //=========================================================================================================
    /**
     * Destroys the BrainAMP.
     */
    ~BrainAMP() override;

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    QSharedPointer<mne::scan::AbstractPlugin> clone() const override;

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    void init() override;

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    void unload() override;

    //=========================================================================================================
    /**
     * Opens the amplifier (or its simulator) and starts streaming.
     */
    bool start() override;

    //=========================================================================================================
    /**
     * Stops streaming and releases the amplifier.
     */
    bool stop() override;

    AbstractPlugin::PluginType getType() const override;
    QString getName() const override;

    QWidget* setupWidget() override;

    QString getBuildInfo() override;

protected slots:
    //=========================================================================================================
    /**
     * Update cardinal points
     *
     * @param[in] sLPA  The channel name to take as the LPA.
     * @param[in] dLPA  The amount (in m) to translate the LPA channel position on the z axis.
     * @param[in] sRPA  The channel name to take as the RPA.
     * @param[in] dRPA  The amount (in m) to translate the RPA channel position on the z axis.
     * @param[in] sNasion  The channel name to take as the Nasion.
     * @param[in] dNasion  The amount (in m) to translate the Nasion channel position on the z axis.
     */
    void onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion);

protected:
    //=========================================================================================================
    /**
     * Hands the amplifier's blocks to the connected plugins until stop().
     */
    void run() override;

    //=========================================================================================================
    /**
     * Opens a dialog to setup the project to check the impedance values
     */
    void showSetupProjectDialog();

private:
    /** @return The brainamp:// URI of the current settings. */
    std::string uri() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRMTSA_BrainAMP; /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<BrainAMPSetupProjectWidget> m_pBrainAMPSetupProjectWidget;                           /**< Widget for checking the impedances*/

    mne::AcqDevice m_device; /**< The amplifier while streaming. */
    mne::AcqReader m_reader; /**< Reads the amplifier's stream in run(). */

    int m_iChannels = 32;            /**< EEG channels, 32 per amplifier.*/
    QString m_sResolution = "100nV"; /**< Step of one bit.*/
    bool m_bSimulate = false;        /**< Simulate the amplifier.*/
    int m_iSamplesPerBlock = 500;    /**< Samples per block handed to the connected plugins.*/

    double m_dLPAShift = 0.01;    /**< The shift in m in to generate the LPA.*/
    double m_dRPAShift = 0.01;    /**< The shift in m in to generate the RPA.*/
    double m_dNasionShift = 0.06; /**< The shift in m in to generate the Nasion.*/

    bool m_bUseTrackedCardinalMode = true; /**< Flag for using the tracked cardinal mode.*/
    bool m_bUseElectrodeShiftMode = false; /**< Flag for using the electrode shift mode.*/

    QString m_sElcFilePath;      /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QString m_sCardinalFilePath; /**< Holds the path for the .elc file holding the cardinals/fiducials (electrode positions). Defined by the user via the GUI.*/
    QString m_sLPA = "2LD";      /**< The electrode to take to function as the LPA.*/
    QString m_sRPA = "2RD";      /**< The electrode to take to function as the RPA.*/
    QString m_sNasion = "0Z";    /**< The electrode to take to function as the Nasion.*/

    QAction* m_pActionSetupProject = nullptr; /**< shows setup project dialog. */
};
} // NAMESPACE

#endif // BRAINAMP_H
