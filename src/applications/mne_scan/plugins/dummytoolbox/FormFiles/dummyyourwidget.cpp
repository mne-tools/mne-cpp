//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     dummyyourwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the DummyYourWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummyyourwidget.h"
#include "ui_dummyyourwidget.h"

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

using namespace DUMMYTOOLBOXPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DummyYourWidget::DummyYourWidget(const QString& sSettingsPath,
                                 QWidget *parent)
: QWidget(parent)
, m_pUi(new Ui::DummyYourWidgetGui)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    loadSettings();
}

//=============================================================================================================

DummyYourWidget::~DummyYourWidget()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void DummyYourWidget::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/valueName"), m_pUi->m_pDoubleSpinBox_dummy->value());
}

//=============================================================================================================

void DummyYourWidget::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    m_pUi->m_pDoubleSpinBox_dummy->setValue(settings.value(m_sSettingsPath + QString("/valueName"), 10).toInt());
}
