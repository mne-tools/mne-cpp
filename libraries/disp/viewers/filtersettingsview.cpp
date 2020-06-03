//=============================================================================================================
/**
 * @file     filtersettingsview.cpp
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
 * @brief    Definition of the FilterSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtersettingsview.h"

#include "filterdesignview.h"

#include "ui_filtersettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
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

FilterSettingsView::FilterSettingsView(const QString& sSettingsPath,
                                       QWidget *parent,
                                       Qt::WindowFlags f)
: QWidget(parent, f)
, m_sSettingsPath(sSettingsPath)
, m_pUi(new Ui::FilterSettingsViewWidget)
{
    this->setWindowTitle("Filter Settings");

    m_pUi->setupUi(this);

    loadSettings(m_sSettingsPath);

    m_pFilterView = FilterDesignView::SPtr::create(m_sSettingsPath,
                                                   this,
                                                   Qt::Window);

    //Connect GUI elements
    connect(m_pUi->m_pCheckBoxActivateFilter, &QCheckBox::toggled,
            this, &FilterSettingsView::onFilterActivationChanged);
    connect(m_pUi->m_pPushButtonShowFilterOptions, &QPushButton::clicked,
            this, &FilterSettingsView::onShowFilterView);
    connect(m_pUi->m_pDoubleSpinBoxFrom, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &FilterSettingsView::onFilterParametersChanged);
    connect(m_pUi->m_pDoubleSpinBoxTo, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &FilterSettingsView::onFilterParametersChanged);
}

//=============================================================================================================

FilterSettingsView::~FilterSettingsView()
{
    saveSettings(m_sSettingsPath);
    delete m_pUi;
}

//=============================================================================================================

QSharedPointer<FilterDesignView> FilterSettingsView::getFilterView()
{
    return m_pFilterView;
}

//=============================================================================================================

bool FilterSettingsView::getFilterActive()
{
    return m_pUi->m_pCheckBoxActivateFilter->isChecked();
}

//=============================================================================================================

void FilterSettingsView::init(double dSFreq)
{
    //Update min max of spin boxes to nyquist
    double nyquistFrequency = dSFreq/2;

    m_pUi->m_pDoubleSpinBoxFrom->setMaximum(nyquistFrequency);
    m_pUi->m_pDoubleSpinBoxTo->setMaximum(nyquistFrequency);

    if(m_pUi->m_pDoubleSpinBoxFrom->value() > dSFreq/2) {
        m_pUi->m_pDoubleSpinBoxFrom->setValue(dSFreq/2);
    }

    if(m_pUi->m_pDoubleSpinBoxTo->value() > dSFreq/2) {
        m_pUi->m_pDoubleSpinBoxTo->setValue(dSFreq/2);
    }

    m_pFilterView->init(dSFreq);
}

//=============================================================================================================

void FilterSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/FilterSettingsView/filterActivated"), m_pUi->m_pCheckBoxActivateFilter->isChecked());
    settings.setValue(settingsPath + QString("/FilterSettingsView/filterFrom"), m_pUi->m_pDoubleSpinBoxFrom->value());
    settings.setValue(settingsPath + QString("/FilterSettingsView/filterTo"), m_pUi->m_pDoubleSpinBoxTo->value());
}

//=============================================================================================================

void FilterSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    m_pUi->m_pCheckBoxActivateFilter->setChecked(settings.value(settingsPath + QString("/FilterSettingsView/filterActivated"), false).toBool());
    m_pUi->m_pDoubleSpinBoxTo->setValue(settings.value(settingsPath + QString("/FilterSettingsView/filterTo"), 0).toDouble());
    m_pUi->m_pDoubleSpinBoxFrom->setValue(settings.value(settingsPath + QString("/FilterSettingsView/filterFrom"), 0).toDouble());
}

//=============================================================================================================

void FilterSettingsView::onShowFilterView()
{
    if(m_pFilterView->isActiveWindow()) {
        m_pFilterView->hide();
    } else {
        m_pFilterView->activateWindow();
        m_pFilterView->show();
    }
}

//=============================================================================================================

void FilterSettingsView::onFilterActivationChanged()
{
    emit filterActivationChanged(m_pUi->m_pCheckBoxActivateFilter->isChecked());

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FilterSettingsView::onFilterParametersChanged(double dValue)
{
    Q_UNUSED(dValue)
    m_pUi->m_pDoubleSpinBoxFrom->setMaximum(m_pUi->m_pDoubleSpinBoxTo->value());

    m_pFilterView->setFilterParameters(m_pUi->m_pDoubleSpinBoxFrom->value(),
                                       m_pUi->m_pDoubleSpinBoxTo->value(),
                                       2048,
                                       1, //Cosine
                                       0.1,
                                       "All");

    saveSettings(m_sSettingsPath);
}
