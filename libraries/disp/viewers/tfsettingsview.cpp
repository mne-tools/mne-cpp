//=============================================================================================================
/**
 * @file     tfsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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
: AbstractView(parent, f)
, m_pUi(new Ui::TfSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    loadSettings();

    connect(m_pUi->m_spinBox_trialNumber, static_cast<void (QSpinBox::*)(const QString&)>(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    connect(m_pUi->m_spinBox_rowNumber, static_cast<void (QSpinBox::*)(const QString&)>(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    this->setWindowTitle("Time frequency Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

TfSettingsView::~TfSettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void TfSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void TfSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void TfSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::onNumberTrialRowChanged()
{
    emit numberTrialRowChanged(m_pUi->m_spinBox_trialNumber->value(), m_pUi->m_spinBox_rowNumber->value());
    saveSettings();
}

//=============================================================================================================

void TfSettingsView::clearView()
{

}
