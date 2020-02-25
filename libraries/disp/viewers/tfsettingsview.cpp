//=============================================================================================================
/**
 * @file     tfsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the TfSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfsettingsview.h"

#include "ui_tfsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>

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

TfSettingsView::TfSettingsView(const QString& sSettingsPath,
                               QWidget *parent,
                               Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::TfSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
{
    ui->setupUi(this);

    loadSettings(m_sSettingsPath);

    connect(ui->m_spinBox_trialNumber, static_cast<void (QSpinBox::*)(const QString&)>(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    connect(ui->m_spinBox_rowNumber, static_cast<void (QSpinBox::*)(const QString&)>(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    this->setWindowTitle("Time frequency Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

TfSettingsView::~TfSettingsView()
{
    saveSettings(m_sSettingsPath);

    delete ui;
}

//=============================================================================================================

void TfSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
}

//=============================================================================================================

void TfSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
}

//=============================================================================================================

void TfSettingsView::onNumberTrialRowChanged()
{
    emit numberTrialRowChanged(ui->m_spinBox_trialNumber->value(), ui->m_spinBox_rowNumber->value());
    saveSettings(m_sSettingsPath);
}

