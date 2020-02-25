//=============================================================================================================
/**
 * @file     minimumnormsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     September, 2018
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
 * @brief    Definition of the MinimumNormSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "minimumnormsettingsview.h"

#include "ui_minimumnormsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimumNormSettingsView::MinimumNormSettingsView(QWidget *parent,
                                                 Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::MinimumNormSettingsViewWidget)
{
    ui->setupUi(this);

    connect(ui->m_comboBox_method, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &MinimumNormSettingsView::onMethodChanged);

    connect(ui->m_comboBox_triggerType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &MinimumNormSettingsView::onTriggerTypeChanged);

    connect(ui->m_spinBox_timepoint, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MinimumNormSettingsView::onTimePointValueChanged);

    this->setWindowTitle("MinimumNorm Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

MinimumNormSettingsView::~MinimumNormSettingsView()
{
    delete ui;
}

//=============================================================================================================

void MinimumNormSettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    for(const QString &sTriggerType : lTriggerTypes) {
        if(ui->m_comboBox_triggerType->findText(sTriggerType) == -1) {
            ui->m_comboBox_triggerType->addItem(sTriggerType);
        }
    }
}

//=============================================================================================================

void MinimumNormSettingsView::onMethodChanged(const QString& method)
{
    emit methodChanged(method);
}

//=============================================================================================================

void MinimumNormSettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
}

//=============================================================================================================

void MinimumNormSettingsView::onTimePointValueChanged(int iTimePointMs)
{
    emit timePointChanged(iTimePointMs);
}
