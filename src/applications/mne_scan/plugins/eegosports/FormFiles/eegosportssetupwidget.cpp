//=============================================================================================================
/**
 * @file     eegosportssetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsSetupWidget class.
 *
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

