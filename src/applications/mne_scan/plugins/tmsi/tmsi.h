//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsi.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    Contains the declaration of the TMSI class.
 */

#ifndef TMSI_H
#define TMSI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_global.h"

#include "FormFiles/tmsiimpedancewidget.h"
#include "FormFiles/tmsimanualannotationwidget.h"
#include "FormFiles/tmsisetupprojectwidget.h"
#include "FormFiles/tmsisetupwidget.h"

#include <acq/acq_device.h>
#include <scShared/Plugins/abstractsensor.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

namespace mne::scan
{
class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
/**
 * TMSI...
 *
 * @brief EEG connector for TMSi Refa and Porti amplifiers through the tmsi:// source of mne_acq_devices, which can
 *        also simulate the amplifier.
 */
class TMSISHARED_EXPORT TMSI : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "tmsi.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class TMSISetupWidget;
    friend class TMSIImpedanceWidget;
    friend class TMSISetupProjectWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a TMSI.
     */
    TMSI();

    //=========================================================================================================
    /**
     * Destroys the TMSI.
     */
    ~TMSI() override;

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    QSharedPointer<AbstractPlugin> clone() const override;

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
     * Opens the amplifier (or its simulator) and starts streaming, or measuring impedances.
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

    //=========================================================================================================
    /**
     * Writes a trigger code into the stimulus channel of the next block.
     *
     * @param[in] type   The code: 254 left, 253 right.
     */
    void setKeyboardTriggerType(int type);

protected:
    //=========================================================================================================
    /**
     * Hands the amplifier's blocks, or its impedances, to the connected plugins and widgets until stop().
     */
    void run() override;

    //=========================================================================================================
    /**
     * Opens a widget to check the impedance values
     */
    void showImpedanceDialog();

    //=========================================================================================================
    /**
     * Opens a dialog to setup the project to check the impedance values
     */
    void showSetupProjectDialog();

private:
    /** @return The tmsi:// URI of the current settings. */
    std::string uri() const;

    /** Names and places the EEG channels after the electrodes of the cap's .elc file. */
    void applyElectrodeLayout(mne::FiffInfo& info) const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRMTSA_TMSI; /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<TMSIManualAnnotationWidget> m_pTmsiManualAnnotationWidget;                       /**< Widget for manually annotation the trigger during session.*/
    QSharedPointer<TMSIImpedanceWidget> m_pTmsiImpedanceWidget;                                     /**< Widget for checking the impedances*/
    QSharedPointer<TMSISetupProjectWidget> m_pTmsiSetupProjectWidget;                               /**< Widget for checking the impedances*/

    mne::AcqDevice m_device; /**< The amplifier while streaming or measuring impedances. */
    mne::AcqReader m_reader; /**< Reads the amplifier's stream in run(). */
    int m_iStimRow = -1;     /**< Row of the stimulus channel in the blocks, -1 if it is not delivered. */

    int m_iSamplingFreq = 2048;      /**< Sampling rate in Hz; the amplifier runs at the supported rate nearest to it.*/
    int m_iNumberOfChannels = 0;     /**< Leading amplifier channels to deliver; 0 = all.*/
    int m_iSamplesPerBlock = 16;     /**< Samples per block handed to the connected plugins.*/
    bool m_bUseCommonAverage = true; /**< Reference the ExG inputs to their common average.*/
    bool m_bSimulate = false;        /**< Simulate the amplifier.*/

    int m_iTriggerInterval = 5000;      /**< The gap between the trigger signals which request the subject to do something (in ms).*/
    QElapsedTimer m_qTimerTrigger;      /**< Time stemp of the last trigger event (in ms).*/
    bool m_bBeepTrigger = false;        /**< Flag for using a trigger input.*/
    bool m_bUseKeyboardTrigger = false; /**< Flag for using the keyboard as a trigger input.*/
    bool m_bCheckImpedances = false;    /**< Flag for checking the impedances of the EEG amplifier.*/
    std::atomic<int> m_iTriggerType{0}; /**< Trigger code for the next block, 0 for none.*/

    QString m_sElcFilePath; /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/

    QAction* m_pActionImpedance = nullptr;    /**< shows impedance widget. */
    QAction* m_pActionSetupProject = nullptr; /**< shows setup project dialog. */
};
} // NAMESPACE

#endif // TMSI_H
