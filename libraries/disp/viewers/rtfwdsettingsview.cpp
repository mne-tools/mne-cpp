//=============================================================================================================
/**
 * @file     rtfwdsettingsview.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    RtFwdSettingsView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwdsettingsview.h"
#include "ui_rtfwdsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwdSettingsView::RtFwdSettingsView(const QString& sSettingsPath,
                                     QWidget *parent,
                                     Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui(new Ui::RtFwdSettingsViewWidget)
    , m_sSettingsPath(sSettingsPath)
{
    m_ui->setupUi(this);

    // init
    m_ui->m_spinBox_Movement->setValue(3);      // movement threshols = 3 mm
    m_ui->m_spinBox_Rotation->setValue(5);      // rotation threshols = 5°

    // connect
    connect(m_ui->m_spinBox_Movement, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &RtFwdSettingsView::allowedMoveThresholdChanged);

    // load settings
    loadSettings(m_sSettingsPath);
}

//=============================================================================================================

RtFwdSettingsView::~RtFwdSettingsView()
{
    saveSettings(m_sSettingsPath);

    delete m_ui;
}

//=============================================================================================================

void RtFwdSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
    QVariant defaultData;

//    defaultData.setValue(m_vCoilFreqs);
//    m_vCoilFreqs = settings.value(settingsPath + QString("/coilFreqs"), defaultData).value<QVector<int> >();
//    emit coilFrequenciesChanged(m_vCoilFreqs);

//    m_ui->m_checkBox_useSSP->setChecked(settings.value(settingsPath + QString("/useSSP"), false).toBool());
//    m_ui->m_checkBox_useComp->setChecked(settings.value(settingsPath + QString("/useCOMP"), false).toBool());
//    m_ui->m_doubleSpinBox_maxHPIContinousDist->setValue(settings.value(settingsPath + QString("/maxError"), 10.0).toDouble());
}

//=============================================================================================================

void RtFwdSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
    QVariant data;

//    data.setValue(m_vCoilFreqs);
//    settings.setValue(settingsPath + QString("/coilFreqs"), data);

//    data.setValue(m_ui->m_checkBox_useSSP->isChecked());
//    settings.setValue(settingsPath + QString("/useSSP"), data);

//    data.setValue(m_ui->m_checkBox_useComp->isChecked());
//    settings.setValue(settingsPath + QString("/useCOMP"), data);

//    data.setValue(m_ui->m_doubleSpinBox_maxHPIContinousDist->value());
//    settings.setValue(settingsPath + QString("/maxError"), data);
}

//=============================================================================================================

void RtFwdSettingsView::allowedMoveThresholdChanged(double dThreshMove)
{

}

//=============================================================================================================

void RtFwdSettingsView::allowedRotThresholdChanged(double dThreshRot)
{

}
