//=============================================================================================================
/**
 * @file     covariancesettingsview.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020 Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the CovarianceSettingsView class.
 *
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
 * User GUI control for Covariance estimation.
 *
 * @brief User GUI control for Covariance estimation.
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
