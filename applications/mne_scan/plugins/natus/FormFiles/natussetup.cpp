//=============================================================================================================
/**
 * @file     natussetup.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the implementation of the NatusSetup class.
 *
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
