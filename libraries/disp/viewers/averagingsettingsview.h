//=============================================================================================================
/**
 * @file     averagingsettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the AveragingSettingsView class.
 *
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

/**
 * DECLARE CLASS AveragingSettingsView
 *
 * @brief The AveragingSettingsView class provides a averaging settings view.
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
