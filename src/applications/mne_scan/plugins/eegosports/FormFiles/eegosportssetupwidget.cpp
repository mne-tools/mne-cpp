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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
{
    ui.setupUi(this);

    QIcon impedanceIcon = QIcon(":/images/impedances.png");
    ui.m_qLabel_Impedance->setPixmap(impedanceIcon.pixmap(QSize(32,32)));
    QIcon filesIcon = QIcon(":/images/database.png");
    ui.m_qLabel_Files->setPixmap(filesIcon.pixmap(QSize(32,32)));

    //Connect device sampling properties
    connect(ui.m_comboBox_SamplingFreq, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_spinBox_BlockSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);

    //Connect impedance test
    connect(ui.m_qPushButton_Files, &QPushButton::clicked,
            m_pEEGoSports, &EEGoSports::showSetupProjectDialog);

    //Connect files dialog
    connect(ui.m_qPushButton_Impedance, &QPushButton::clicked,
            m_pEEGoSports, &EEGoSports::showImpedanceDialog);

    //Connect debug file
    connect(ui.m_checkBox_WriteDriverDebugToFile, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setWriteToFile);
}

//=============================================================================================================

EEGoSportsSetupWidget::~EEGoSportsSetupWidget()
{
}

//=============================================================================================================

void EEGoSportsSetupWidget::initGui()
{
    //Init device sampling properties
    ui.m_comboBox_SamplingFreq->setCurrentText(QString::number(m_pEEGoSports->m_iSamplingFreq));
    ui.m_spinBox_BlockSize->setValue(m_pEEGoSports->m_iSamplesPerBlock);

    //Init write to file
    ui.m_checkBox_WriteDriverDebugToFile->setChecked(m_pEEGoSports->m_bWriteDriverDebugToFile);
}

//=============================================================================================================

void EEGoSportsSetupWidget::setDeviceSamplingProperties()
{
    m_pEEGoSports->m_iSamplingFreq = ui.m_comboBox_SamplingFreq->currentText().toInt();
    m_pEEGoSports->m_iSamplesPerBlock = ui.m_spinBox_BlockSize->value();
}

//=============================================================================================================

void EEGoSportsSetupWidget::setWriteToFile()
{
    m_pEEGoSports->m_bWriteDriverDebugToFile = ui.m_checkBox_WriteDriverDebugToFile->isChecked();
}

