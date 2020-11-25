//=============================================================================================================
/**
 * @file     spharasettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
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
 * @brief    Definition of the SpharaSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spharasettingsview.h"

#include "ui_spharasettingsview.h"

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

SpharaSettingsView::SpharaSettingsView(const QString& sSettingsPath,
                                       QWidget *parent,
                                       Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::SpharaSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    //Sphara activation changed
    connect(m_pUi->m_checkBox_activateSphara, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &SpharaSettingsView::onSpharaButtonClicked);

    //Sphara options changed
    connect(m_pUi->m_comboBox_spharaSystem, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    connect(m_pUi->m_spinBox_spharaFirst, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    connect(m_pUi->m_spinBox_spharaSecond, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    this->setWindowTitle("SPHARA Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
    loadSettings();
}

//=============================================================================================================

SpharaSettingsView::~SpharaSettingsView()
{
    saveSettings();
    delete m_pUi;
}

//=============================================================================================================

void SpharaSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpharaSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpharaSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void SpharaSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void SpharaSettingsView::onSpharaButtonClicked(bool state)
{
    emit spharaActivationChanged(state);
}

//=============================================================================================================

void SpharaSettingsView::onSpharaOptionsChanged()
{
    m_pUi->m_label_spharaFirst->show();
    m_pUi->m_spinBox_spharaFirst->show();

    m_pUi->m_label_spharaSecond->show();
    m_pUi->m_spinBox_spharaSecond->show();

    if(m_pUi->m_comboBox_spharaSystem->currentText() == "VectorView") {
        m_pUi->m_label_spharaFirst->setText("Mag");
        m_pUi->m_spinBox_spharaFirst->setMaximum(102);

        m_pUi->m_label_spharaSecond->setText("Grad");
        m_pUi->m_spinBox_spharaSecond->setMaximum(102);
    }

    if(m_pUi->m_comboBox_spharaSystem->currentText() == "BabyMEG") {
        m_pUi->m_label_spharaFirst->setText("Inner layer");
        m_pUi->m_spinBox_spharaFirst->setMaximum(270);

        m_pUi->m_label_spharaSecond->setText("Outer layer");
        m_pUi->m_spinBox_spharaSecond->setMaximum(105);
    }

    if(m_pUi->m_comboBox_spharaSystem->currentText() == "EEG") {
        m_pUi->m_label_spharaFirst->setText("EEG");
        m_pUi->m_spinBox_spharaFirst->setMaximum(256);

        m_pUi->m_label_spharaSecond->hide();
        m_pUi->m_spinBox_spharaSecond->hide();
    }

    emit spharaOptionsChanged(m_pUi->m_comboBox_spharaSystem->currentText(),
                              m_pUi->m_spinBox_spharaFirst->value(),
                              m_pUi->m_spinBox_spharaSecond->value());
}

//=============================================================================================================

void SpharaSettingsView::clearView()
{

}
