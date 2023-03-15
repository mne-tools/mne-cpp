//=============================================================================================================
/**
 * @file     triggerdetectionview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the TriggerDetectionView Class.
 *
 */

#ifndef TRIGGERDETECTIONVIEW_H
#define TRIGGERDETECTIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class TriggerDetectionViewWidget;
}

namespace FIFFLIB {
    class FiffInfo;
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
 * DECLARE CLASS TriggerDetectionView
 *
 * @brief The TriggerDetectionView class provides a view to control the trigger detection
 */
class DISPSHARED_EXPORT TriggerDetectionView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<TriggerDetectionView> SPtr;              /**< Shared pointer type for TriggerDetectionView. */
    typedef QSharedPointer<const TriggerDetectionView> ConstSPtr;   /**< Const shared pointer type for TriggerDetectionView. */

    //=========================================================================================================
    /**
     * Constructs a TriggerDetectionView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    TriggerDetectionView(const QString& sSettingsPath = "",
                         QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the TriggerDetectionView.
     */
    ~TriggerDetectionView();

    //=========================================================================================================
    /**
     * Init the view.
     */
    void init(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Set total number of detected triggers and trigger types.
     *
     * @param[in] totalNumberDetections     The numger of detected triggers.
     * @param[in] mapDetectedTriggers       The currently detected triggers.
     */
    void setNumberDetectedTriggersAndTypes(int totalNumberDetections,
                                           const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);

    //=========================================================================================================
    /**
     * Gets currently selected STIM channel in GUI
     *
     * @return currently selected STIM channel.
     */
    QString getSelectedStimChannel();

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
     * Clears the view
     */
    void clearView();

protected:
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
     * Slot called when trigger info changed
     */
    void onTriggerInfoChanged();

    //=========================================================================================================
    /**
     * Slot called when trigger detection color button was clicked
     */
    void onRealTimeTriggerColorChanged(bool state);

    //=========================================================================================================
    /**
     * Slot called when trigger type changed
     */
    void onRealTimeTriggerColorTypeChanged(const QString& value);

    //=========================================================================================================
    /**
     * Slot called when reset number of detected triggers was pressed
     */
    void onResetTriggerNumbers();

    //=========================================================================================================
    /**
     * Slot called when detect triggers is pressed
     */
    void onDetectTriggers();

    Ui::TriggerDetectionViewWidget*                     m_pUi;

    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                    /**< Connected fiff info. */

    QMap<double, QColor>                                m_qMapTriggerColor;             /**< Trigger colors per detected type. */

    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user pressed the trigger counter.
     */
    void resetTriggerCounter();

    //=========================================================================================================
    /**
     * Emit this signal whenever the trigger information changed.
     */
    void triggerInfoChanged(const QMap<double,
                            QColor>& value,
                            bool bActive,
                            const QString& sTriggerCh,
                            double dThreshold);

    //=========================================================================================================
    /**
     * Emit this signal when user clicks detect trigger button.
     */
    void detectTriggers(const QString& sChannelName,
                        double iThreshold);
};
} // NAMESPACE

#endif // TRIGGERDETECTIONVIEW_H
