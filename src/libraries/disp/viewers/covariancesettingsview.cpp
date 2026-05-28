//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     covariancesettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.3
 * @date     June 2020
 * @brief    Implementation of the CovarianceSettingsView covariance-length panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancesettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovarianceSettingsView::CovarianceSettingsView(const QString& sSettingsPath,
                                               QWidget *parent)
: AbstractView(parent)
, m_sSettingsPath(sSettingsPath)
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
            this, &CovarianceSettingsView::samplesChanged);
    t_pGridLayout->addWidget(m_pSpinBoxNumSamples,0,1,1,1);
    this->setLayout(t_pGridLayout);

    loadSettings();
}

//=============================================================================================================

CovarianceSettingsView::~CovarianceSettingsView()
{
    saveSettings();
}

//=============================================================================================================

void CovarianceSettingsView::setCurrentSamples(int iSamples)
{
    m_pSpinBoxNumSamples->setValue(iSamples);
}

//=============================================================================================================

void CovarianceSettingsView::setMinSamples(int iSamples)
{
    m_pSpinBoxNumSamples->setMinimum(iSamples);
    m_pSpinBoxNumSamples->setMaximum(iSamples*60);
}

//=============================================================================================================

void CovarianceSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void CovarianceSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void CovarianceSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void CovarianceSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void CovarianceSettingsView::clearView()
{

}
