//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosports.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the declaration of the EEGoSports class.
 */

#ifndef EEGOSPORTS_H
#define EEGOSPORTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosports_global.h"

#include "FormFiles/eegosportsimpedancewidget.h"
#include "FormFiles/eegosportssetupprojectwidget.h"
#include "FormFiles/eegosportssetupwidget.h"

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

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
/**
 * EEGoSports.
 *
 * @brief EEG connector for ANT Neuro eego amplifiers through the eego:// source of mne_acq_devices, which can also
 *        simulate the amplifier.
 */
class EEGOSPORTSSHARED_EXPORT EEGoSports : public mne::scan::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "eegosports.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractSensor)

    friend class EEGoSportsImpedanceWidget;
    friend class EEGoSportsSetupWidget;
    friend class EEGoSportsSetupProjectWidget;

public:
    EEGoSports();
    ~EEGoSports() override;

    QSharedPointer<AbstractPlugin> clone() const override;
    void init() override;
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
    void onUpdateCardinalPoints(const QString& sLPA,
                                double dLPA,
                                const QString& sRPA,
                                double dRPA,
                                const QString& sNasion,
                                double dNasion);

protected:
    //=========================================================================================================
    /**
     * Opens a widget to check the impedance values.
     */
    void showImpedanceDialog();

    //=========================================================================================================
    /**
     * Opens a dialog to set up the electrode layout and the cardinal points.
     */
    void showSetupProjectDialog();

    //=========================================================================================================
    /**
     * Hands the amplifier's blocks, or its impedances, to the connected plugins and widgets until stop().
     */
    void run() override;

private:
    /** @return The eego:// URI of the current settings. */
    std::string uri() const;

    /** Names and places the EEG channels after the cap's .elc file and sets the cardinal points. */
    void applyElectrodeLayout(mne::FiffInfo& info) const;

    /** Stores the settings for the next session. */
    void saveSettings() const;

    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeMultiSampleArray>> m_pRMTSA_EEGoSports; /**< The EEG data. */
    QSharedPointer<EEGoSportsImpedanceWidget> m_pEEGoSportsImpedanceWidget;                               /**< Widget for checking the impedances. */
    QSharedPointer<EEGoSportsSetupProjectWidget> m_pEEGoSportsSetupProjectWidget;                         /**< Widget for the layout and cardinals. */

    mne::AcqDevice m_device; /**< The amplifier while streaming or measuring impedances. */
    mne::AcqReader m_reader; /**< Reads the amplifier's stream in run(). */

    int m_iSamplingFreq = 512;       /**< Sampling rate in Hz, one the amplifier supports. */
    int m_iSamplesPerBlock = 512;    /**< Samples per block handed to the connected plugins. */
    bool m_bSimulate = false;        /**< Simulate the amplifier. */
    bool m_bCheckImpedances = false; /**< Measure impedances instead of streaming. */

    double m_dLPAShift = 0.0;    /**< Lowering of the LPA electrode in m (electrode shift mode). */
    double m_dRPAShift = 0.0;    /**< Lowering of the RPA electrode in m (electrode shift mode). */
    double m_dNasionShift = 0.0; /**< Lowering of the nasion electrode in m (electrode shift mode). */

    bool m_bUseTrackedCardinalMode = true; /**< Take the cardinal points from the cardinal file. */
    bool m_bUseElectrodeShiftMode = false; /**< Take the cardinal points from lowered electrodes. */

    QString m_sElcFilePath;      /**< The cap's .elc file (electrode positions). */
    QString m_sCardinalFilePath; /**< The .elc file of the tracked cardinal points LPA, Nasion and RPA. */
    QString m_sLPA;              /**< The electrode taken as the LPA. */
    QString m_sRPA;              /**< The electrode taken as the RPA. */
    QString m_sNasion;           /**< The electrode taken as the nasion. */
};
} // NAMESPACE

#endif // EEGOSPORTS_H
