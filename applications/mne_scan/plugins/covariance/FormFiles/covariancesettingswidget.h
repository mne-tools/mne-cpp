//=============================================================================================================
/**
 * @file     covariancesettingswidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the CovarianceSettingsWidget class.
 *
 */

#ifndef COVARIANCESETTINGSWIDGET_H
#define COVARIANCESETTINGSWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSpinBox>
#include <QPair>

#include <QComboBox>
#include <QCheckBox>

//=============================================================================================================
// DEFINE NAMESPACE COVARIANCEPLUGIN
//=============================================================================================================

namespace COVARIANCEPLUGIN
{

//=============================================================================================================
// COVARIANCEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class CovarianceSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<CovarianceSettingsWidget> SPtr;         /**< Shared pointer type for CovarianceAdjustmentWidget. */
    typedef QSharedPointer<CovarianceSettingsWidget> ConstSPtr;    /**< Const shared pointer type for CovarianceAdjustmentWidget. */

    explicit CovarianceSettingsWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Set current samples to gather until a new covariance is calculated.
     *
     * @param[in] iSamples     new number samples
     */
    virtual void setCurrentSamples(int iSamples);

    //=========================================================================================================
    /**
     * Set minimum number of samples to gather until a new covariance is calculated.
     *
     * @param[in] iSamples     new minimum number of samples
     */
    virtual void setMinSamples(int iSamples);

signals:
    void samplesChanged(int iSamples);

private:
    QSpinBox* m_pSpinBoxNumSamples;
};
} // NAMESPACE

#endif // COVARIANCESETTINGSWIDGET_H
