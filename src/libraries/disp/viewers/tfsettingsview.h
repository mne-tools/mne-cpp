//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     tfsettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2019
 * @brief    Time-frequency analysis window-length and overlap configuration panel.
 *
 * TfSettingsView holds the spinboxes that drive the on-line
 * time-frequency estimator: number of trials, window length and
 * overlap (in samples) for the underlying STFT / Wavelet routine.
 */

#ifndef TFSETTINGSVIEW_H
#define TFSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class TfSettingsViewWidget;
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
 * @brief Time-frequency analysis configuration panel (trials, window length, overlap).
 *
 * Spinboxes drive the on-line STFT / Wavelet estimator; values are
 * emitted as Qt signals on every edit.
 */
class DISPSHARED_EXPORT TfSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<TfSettingsView> SPtr;              /**< Shared pointer type for TfSettingsView. */
    typedef QSharedPointer<const TfSettingsView> ConstSPtr;   /**< Const shared pointer type for TfSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a TfSettingsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    TfSettingsView(const QString& sSettingsPath = "",
                   QWidget *parent = 0,
                   Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the TfSettingsView.
     */
    ~TfSettingsView();

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
     * Slot called when the trial or row number changed.
     */
    void onNumberTrialRowChanged();

    Ui::TfSettingsViewWidget* m_pUi;

    QString         m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit signal whenever trial number changed.
     *
     * @param[in] iNumberTrial        The new trial number.
     * @param[in] iNumberRow        The new row number.
     */
    void numberTrialRowChanged(int iNumberTrial, int iNumberRow);
};
} // NAMESPACE

#endif // CONNECTIVITYSETTINGSVIEW_H
