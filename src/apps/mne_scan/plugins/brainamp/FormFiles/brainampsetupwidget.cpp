//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the implementation of the BrainAMPSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampsetupwidget.h"
#include "../brainamp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMPSetupWidget::BrainAMPSetupWidget(BrainAMP* pBrainAMP, QWidget* parent)
: QWidget(parent)
, m_pBrainAMP(pBrainAMP)
{
    ui.setupUi(this);

    connect(ui.m_spinBox_Channels, &QSpinBox::valueChanged, this, [this](int value) { m_pBrainAMP->m_iChannels = value; });
    connect(ui.m_comboBox_Resolution, &QComboBox::currentTextChanged, this, [this](const QString& text) { m_pBrainAMP->m_sResolution = text; });
    connect(ui.m_checkBox_Simulate, &QCheckBox::toggled, this, [this](bool checked) { m_pBrainAMP->m_bSimulate = checked; });
    connect(ui.m_spinBox_BlockSize, &QSpinBox::valueChanged, this, [this](int value) { m_pBrainAMP->m_iSamplesPerBlock = value; });
}

//=============================================================================================================

BrainAMPSetupWidget::~BrainAMPSetupWidget()
{
}

//=============================================================================================================

void BrainAMPSetupWidget::initGui()
{
    ui.m_spinBox_Channels->setValue(m_pBrainAMP->m_iChannels);
    ui.m_comboBox_Resolution->setCurrentText(m_pBrainAMP->m_sResolution);
    ui.m_checkBox_Simulate->setChecked(m_pBrainAMP->m_bSimulate);
    ui.m_spinBox_BlockSize->setValue(m_pBrainAMP->m_iSamplesPerBlock);
}
