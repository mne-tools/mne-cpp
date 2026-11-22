//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportssetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2014
 * @brief    Contains the implementation of the EEGoSportsSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportssetupwidget.h"
#include "../eegosports.h"
#include "ui_eegosportssetup.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsSetupWidget::EEGoSportsSetupWidget(EEGoSports* pEEGoSports, QWidget* parent)
: QWidget(parent)
, m_pEEGoSports(pEEGoSports)
, m_pUi(std::make_unique<Ui::EEGoSportsSetupClass>())
{
    m_pUi->setupUi(this);

    QIcon impedanceIcon = QIcon(":/images/impedances.png");
    m_pUi->m_qLabel_Impedance->setPixmap(impedanceIcon.pixmap(QSize(32, 32)));
    QIcon filesIcon = QIcon(":/images/database.png");
    m_pUi->m_qLabel_Files->setPixmap(filesIcon.pixmap(QSize(32, 32)));

    //Connect device sampling properties
    connect(m_pUi->m_comboBox_SamplingFreq, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_spinBox_BlockSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);

    connect(m_pUi->m_qPushButton_Files, &QPushButton::clicked,
            m_pEEGoSports, &EEGoSports::showSetupProjectDialog);
    connect(m_pUi->m_qPushButton_Impedance, &QPushButton::clicked,
            m_pEEGoSports, &EEGoSports::showImpedanceDialog);
    connect(m_pUi->m_checkBox_Simulate, &QCheckBox::toggled, this, [this](bool on) { m_pEEGoSports->m_bSimulate = on; });
}

//=============================================================================================================

EEGoSportsSetupWidget::~EEGoSportsSetupWidget()
{
}

//=============================================================================================================

void EEGoSportsSetupWidget::initGui()
{
    //Init device sampling properties
    m_pUi->m_comboBox_SamplingFreq->setCurrentText(QString::number(m_pEEGoSports->m_iSamplingFreq));
    m_pUi->m_spinBox_BlockSize->setValue(m_pEEGoSports->m_iSamplesPerBlock);

    m_pUi->m_checkBox_Simulate->setChecked(m_pEEGoSports->m_bSimulate);
}

//=============================================================================================================

void EEGoSportsSetupWidget::setDeviceSamplingProperties()
{
    m_pEEGoSports->m_iSamplingFreq = m_pUi->m_comboBox_SamplingFreq->currentText().toInt();
    m_pEEGoSports->m_iSamplesPerBlock = m_pUi->m_spinBox_BlockSize->value();
}
