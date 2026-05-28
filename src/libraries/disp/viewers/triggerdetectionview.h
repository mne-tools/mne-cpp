//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     triggerdetectionview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Stim-channel chooser plus threshold spinbox for on-line trigger / event detection.
 *
 * TriggerDetectionView lets the user pick the stim channel to monitor,
 * set the rising-edge threshold and reset the detected-event list.
 * Detected events feed the averaging pipeline and the
 * @ref ChannelDataView trigger overlay.
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
 * @brief Stim-channel chooser plus threshold spinbox for on-line trigger detection.
 *
 * Picks the stim channel to monitor, sets the rising-edge threshold
 * and resets the detected-event list; detected events feed the
 * averaging pipeline and the @ref ChannelDataView trigger overlay.
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
