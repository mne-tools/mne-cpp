//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     covariancesettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.3
 * @date     June 2020
 * @brief    Tiny settings panel exposing the noise-covariance estimation window length.
 *
 * CovarianceSettingsView holds a single @c QSpinBox whose value (in
 * samples) is forwarded as @c changeMinNumberSamples to the
 * @c rtprocessing covariance estimator that gradually accumulates a
 * @c FiffCov from the live stream.
 */

#ifndef COVARIANCESETTINGSVIEW_H
#define COVARIANCESETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSpinBox>
#include <QPair>

#include <QComboBox>
#include <QCheckBox>

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
 * @brief Settings panel exposing the noise-covariance estimation window length.
 *
 * Holds a single @c QSpinBox forwarded as @c changeMinNumberSamples
 * to the @c rtprocessing covariance estimator.
 */
class DISPSHARED_EXPORT CovarianceSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<CovarianceSettingsView> SPtr;         /**< Shared pointer type for CovarianceAdjustmentWidget. */
    typedef QSharedPointer<CovarianceSettingsView> ConstSPtr;    /**< Const shared pointer type for CovarianceAdjustmentWidget. */

    explicit CovarianceSettingsView(const QString& sSettingsPath = "",
                                    QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the CovarianceSettingsView.
     */
    ~CovarianceSettingsView();

    //=========================================================================================================
    /**
     * Set current samples to gather until a new covariance is calculated.
     *
     * @param[in] iSamples     new number samples.
     */
    void setCurrentSamples(int iSamples);

    //=========================================================================================================
    /**
     * Set minimum number of samples to gather until a new covariance is calculated.
     *
     * @param[in] iSamples     new minimum number of samples.
     */
    void setMinSamples(int iSamples);

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

signals:
    void samplesChanged(int iSamples);

private:
    QSpinBox*       m_pSpinBoxNumSamples;
    QString         m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

};
} // NAMESPACE DISPLIB

#endif // COVARIANCESETTINGSVIEW_H
