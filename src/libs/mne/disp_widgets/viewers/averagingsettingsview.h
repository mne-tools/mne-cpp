//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     averagingsettingsview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September 2015
 * @brief    Settings panel driving the on-line / off-line averaging engine (pre/post stim, baseline, artefact thresholds).
 *
 * AveragingSettingsView exposes the parameters needed to compute event
 * related averages from a continuous FIFF stream: stimulus channel,
 * trigger types of interest, pre- and post-stimulus window in
 * milliseconds, baseline-correction interval and an embedded artefact
 * threshold table backed by @ref mne::disp::ArtifactSettingsView. Edits propagate
 * as Qt signals that the @c rtprocessing averaging job consumes.
 */

#ifndef AVERAGINGSETTINGSVIEW_H
#define AVERAGINGSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_widgets_global.h"
#include "abstractview.h"
#include <mne/fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui
{
class AverageSettingsViewWidget;
}

namespace mne
{
class FiffEvokedSet;
class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Settings panel driving the on-line / off-line averaging engine.
 *
 * Holds stim-channel and trigger-type pickers, pre / post-stimulus
 * spinboxes (ms), baseline-correction toggles and an embedded
 * @ref ArtifactSettingsView. Parameter edits are forwarded as Qt
 * signals to the @c rtprocessing averaging job.
 */
class DISPWIDGETSSHARED_EXPORT AveragingSettingsView : public AbstractView
{
    Q_OBJECT

public:
    explicit AveragingSettingsView(const QString& sSettingsPath = "",
                                   const std::map<std::string, int>& mapStimChsIndexNames = std::map<std::string, int>(),
                                   QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the AveragingSettingsView.
     */
    ~AveragingSettingsView();

    void setStimChannels(const std::map<std::string, int>& mapStimChsIndexNames);

    std::string getCurrentStimCh();

    bool getDoBaselineCorrection();

    int getNumAverages();

    int getBaselineFromSeconds();

    int getBaselineToSeconds();

    int getStimChannelIdx();

    int getPreStimMSeconds();

    int getPostStimMSeconds();

    QString getCurrentSelectGroup();

    void setDetectedEpochs(const mne::FiffEvokedSet& evokedSet);

    bool getAutoComputeStatus();

    void clearSelectionGroup();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Redraw the GUI.
     */
    void redrawGUI();

    //=========================================================================================================
    /**
     * Fills the stim channel selection with the known stim channels and their channel indices.
     */
    void fillStimChannels();

    void onChangePreStim();
    void onChangePostStim();
    void onChangeBaselineFrom();
    void onChangeBaselineTo();
    void onChangeNumAverages();
    void onChangeBaselineActive(bool bActive);
    void onChangeStimChannel();
    void onChangeGroupSelect(int iIndex);

    std::unique_ptr<Ui::AverageSettingsViewWidget> m_pUi; /**< Holds the user interface for the AverageSettingsViewWidget.*/

    std::string m_sCurrentStimChan = "STI 014";

    std::map<std::string, int> m_mapStimChsIndexNames;

    int m_iNumAverages = 10;
    int m_iPreStimSeconds = 100;
    int m_iPostStimSeconds = 400;
    int m_iBaselineFromSeconds = 0;
    int m_iBaselineToSeconds = 0;
    bool m_bDoBaselineCorrection = false;

signals:
    void changePreStim(qint32 value);
    void changePostStim(qint32 value);
    void changeBaselineFrom(qint32 value);
    void changeBaselineTo(qint32 value);
    void changeNumAverages(qint32 value);
    void changeStimChannel(const std::string& sStimName);
    void changeBaselineActive(bool state);
    void resetAverage(bool state);
    void changeAverageMode(qint32 index);
    void calculateAverage(bool state);
    void changeDropActive(bool state);
    void setAutoCompute(bool state);
};
} // NAMESPACE

#endif // AVERAGINGSETTINGSVIEW_H
