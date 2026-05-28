//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averagingsettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September 2018
 * @brief    Settings panel driving the on-line / off-line averaging engine (pre/post stim, baseline, artefact thresholds).
 *
 * AveragingSettingsView exposes the parameters needed to compute event
 * related averages from a continuous FIFF stream: stimulus channel,
 * trigger types of interest, pre- and post-stimulus window in
 * milliseconds, baseline-correction interval and an embedded artefact
 * threshold table backed by @ref ArtifactSettingsView. Edits propagate
 * as Qt signals that the @c rtprocessing averaging job consumes.
 */

#ifndef AVERAGINGSETTINGSVIEW_H
#define AVERAGINGSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QMap>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class AverageSettingsViewWidget;
}

namespace FIFFLIB {
    class FiffEvokedSet;
    class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
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
class DISPSHARED_EXPORT AveragingSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<AveragingSettingsView> SPtr;         /**< Shared pointer type for AveragingAdjustmentWidget. */
    typedef QSharedPointer<AveragingSettingsView> ConstSPtr;    /**< Const shared pointer type for AveragingAdjustmentWidget. */

    explicit AveragingSettingsView(const QString& sSettingsPath = "",
                                   const QMap<QString, int>& mapStimChsIndexNames = QMap<QString, int>(),
                                   QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the AveragingSettingsView.
     */
    ~AveragingSettingsView();

    void setStimChannels(const QMap<QString, int> &mapStimChsIndexNames);

    QString getCurrentStimCh();

    bool getDoBaselineCorrection();

    int getNumAverages();

    int getBaselineFromSeconds();

    int getBaselineToSeconds();

    int getStimChannelIdx();

    int getPreStimMSeconds();

    int getPostStimMSeconds();

    QString getCurrentSelectGroup();

    void setDetectedEpochs(const FIFFLIB::FiffEvokedSet& evokedSet);

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

    void onChangePreStim();
    void onChangePostStim();
    void onChangeBaselineFrom();
    void onChangeBaselineTo();
    void onChangeNumAverages();    
    void onChangeStimChannel();
    void onChangeGroupSelect(int iIndex);

    Ui::AverageSettingsViewWidget* m_pUi;              /**< Holds the user interface for the AverageSettingsViewWidget.*/

    QString             m_sCurrentStimChan;

    QMap<QString,int>   m_mapStimChsIndexNames;

    int                 m_iNumAverages;
    int                 m_iPreStimSeconds;
    int                 m_iPostStimSeconds;
    int                 m_iBaselineFromSeconds;
    int                 m_iBaselineToSeconds;
    bool                m_bDoBaselineCorrection;

signals:
    void changePreStim(qint32 value);
    void changePostStim(qint32 value);
    void changeBaselineFrom(qint32 value);
    void changeBaselineTo(qint32 value);
    void changeNumAverages(qint32 value);
    void changeStimChannel(const QString& sStimName);
    void changeBaselineActive(bool state);
    void resetAverage(bool state);
    void changeAverageMode(qint32 index);
    void calculateAverage(bool state);
    void changeDropActive(bool state);
    void setAutoCompute(bool state);
};
} // NAMESPACE

#endif // AVERAGINGSETTINGSVIEW_H
