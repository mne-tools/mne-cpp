//=============================================================================================================
/**
 * @file     covariancesettingswidget.cpp
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
 * @brief    Definition of the CovarianceSettingsWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancesettingswidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovarianceSettingsWidget::CovarianceSettingsWidget(QWidget *parent)
: QWidget(parent)
{
    this->setWindowTitle("Covariance Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    QLabel* t_pLabelNumSamples = new QLabel;
    t_pLabelNumSamples->setText("Number of Samples");
    t_pGridLayout->addWidget(t_pLabelNumSamples,0,0,1,1);

    qint32 minSamples = 600;

    m_pSpinBoxNumSamples = new QSpinBox;
    m_pSpinBoxNumSamples->setMinimum(minSamples);
    m_pSpinBoxNumSamples->setMaximum(minSamples*60);
    m_pSpinBoxNumSamples->setSingleStep(minSamples);
    connect(m_pSpinBoxNumSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CovarianceSettingsWidget::samplesChanged);
    t_pGridLayout->addWidget(m_pSpinBoxNumSamples,0,1,1,1);
    this->setLayout(t_pGridLayout);
}

//=============================================================================================================

void CovarianceSettingsWidget::setCurrentSamples(int iSamples)
{
    m_pSpinBoxNumSamples->setValue(iSamples);
}

//=============================================================================================================

void CovarianceSettingsWidget::setMinSamples(int iSamples)
{
    m_pSpinBoxNumSamples->setMinimum(iSamples);
    m_pSpinBoxNumSamples->setMaximum(iSamples*60);

}
