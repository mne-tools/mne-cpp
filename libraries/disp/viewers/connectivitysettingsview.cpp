//=============================================================================================================
/**
* @file     connectivitysettingsview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the ConnectivitySettingsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettingsview.h"

#include "ui_connectivitysettingsview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettingsView::ConnectivitySettingsView(QWidget *parent,
                                                   Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::ConnectivitySettingsViewWidget)
{
    ui->setupUi(this);

    connect(ui->m_comboBox_method, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onMetricChanged);

    connect(ui->m_comboBox_windowType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onWindowTypeChanged);

    connect(ui->m_spinBox_numberTrials, &QSpinBox::editingFinished,
            this, &ConnectivitySettingsView::onNumberTrialsChanged);

    connect(ui->m_comboBox_triggerType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onTriggerTypeChanged);

    connect(ui->m_spinBox_freqLow, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    connect(ui->m_spinBox_freqHigh, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    this->setWindowTitle("Connectivity Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

ConnectivitySettingsView::~ConnectivitySettingsView()
{
    delete ui;
}


//*************************************************************************************************************

void ConnectivitySettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    for(const QString &sTriggerType : lTriggerTypes) {
        if(ui->m_comboBox_triggerType->findText(sTriggerType) == -1) {
            ui->m_comboBox_triggerType->addItem(sTriggerType);
        }
    }
}


//*************************************************************************************************************

void ConnectivitySettingsView::setNumberTrials(int iNumberTrials)
{
    ui->m_spinBox_numberTrials->setValue(iNumberTrials);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onMetricChanged(const QString& sMetric)
{
    emit connectivityMetricChanged(sMetric);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onWindowTypeChanged(const QString& sWindowType)
{
    emit windowTypeChanged(sWindowType);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onNumberTrialsChanged()
{
    emit numberTrialsChanged(ui->m_spinBox_numberTrials->value());
}


//*************************************************************************************************************

void ConnectivitySettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onFrequencyBandChanged(int value)
{
    //Q_UNUSED(value)
    emit freqBandChanged(ui->m_spinBox_freqLow->value(),
                         ui->m_spinBox_freqHigh->value());
}
