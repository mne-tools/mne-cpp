//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natussetup.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Contains the implementation of the NatusSetup class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natussetup.h"
#include "../natus.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NATUSPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NatusSetup::NatusSetup(Natus* pNatus, QWidget* parent)
: QWidget(parent)
, m_pNatus(pNatus)
{
    ui.setupUi(this);

    //Connect device sampling properties
    connect(ui.m_comboBox_SamplingFreq, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &NatusSetup::setSamplingFreq);
    connect(ui.m_comboBox_blockSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &NatusSetup::setSamplesPerBlock);
    connect(ui.m_spinBox_numberChannels, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &NatusSetup::setNumberChannels);
}

//=============================================================================================================

NatusSetup::~NatusSetup()
{
}

//=============================================================================================================

void NatusSetup::initGui()
{
    //Init device sampling properties
    ui.m_comboBox_SamplingFreq->setCurrentText(QString::number(m_pNatus->m_iSamplingFreq));
    ui.m_comboBox_blockSize->setCurrentText(QString::number(m_pNatus->m_iSamplesPerBlock));
    ui.m_spinBox_numberChannels->setValue(m_pNatus->m_iNumberChannels);
}

//=============================================================================================================

void NatusSetup::setSamplingFreq()
{
    m_pNatus->m_iSamplingFreq = ui.m_comboBox_SamplingFreq->currentText().toInt();
}

//=============================================================================================================

void NatusSetup::setNumberChannels()
{
    m_pNatus->m_iNumberChannels = ui.m_spinBox_numberChannels->value();
}

//=============================================================================================================

void NatusSetup::setSamplesPerBlock()
{
    m_pNatus->m_iSamplesPerBlock = ui.m_comboBox_blockSize->currentText().toInt();
}
