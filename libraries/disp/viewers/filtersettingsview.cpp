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
: AbstractView(parent, f)
, m_pUi(new Ui::FilterSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Filter Settings");

    m_pUi->setupUi(this);

    loadSettings();

    //Create and connect design viewer
    m_pFilterView = FilterDesignView::SPtr::create(m_sSettingsPath,
                                                   Q_NULLPTR,
                                                   Qt::Dialog);

    connect(m_pFilterView.data(), &FilterDesignView::updateFilterFrom,[=](double dFrom){
                m_pUi->m_pDoubleSpinBoxFrom->setValue(dFrom);
            });
    connect(m_pFilterView.data(), &FilterDesignView::updateFilterTo,[=](double dTo){
                m_pUi->m_pDoubleSpinBoxTo->setValue(dTo);
            });

    connect(this, &FilterSettingsView::guiStyleChanged,
            m_pFilterView.data(), &FilterDesignView::guiStyleChanged);

    m_pUi->m_pDoubleSpinBoxFrom->setValue(m_pFilterView->getFrom());
    m_pUi->m_pDoubleSpinBoxTo->setValue(m_pFilterView->getTo());

    //Connect GUI elements
    connect(m_pUi->m_pCheckBoxActivateFilter, &QCheckBox::toggled,
            this, &FilterSettingsView::onFilterActivationChanged);
    connect(m_pUi->m_pPushButtonShowFilterOptions, &QPushButton::clicked,
            this, &FilterSettingsView::onShowFilterView);
    connect(m_pUi->m_pDoubleSpinBoxFrom, &QDoubleSpinBox::editingFinished,
            this, &FilterSettingsView::onFilterFromChanged);
    connect(m_pUi->m_pDoubleSpinBoxTo, &QDoubleSpinBox::editingFinished,
            this, &FilterSettingsView::onFilterToChanged);
    connect(m_pUi->m_pcomboBoxChannelTypes, &QComboBox::currentTextChanged,
            this, &FilterSettingsView::onFilterChannelTypeChanged);
}

//=============================================================================================================

FilterSettingsView::~FilterSettingsView()
{
    saveSettings();
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

void FilterSettingsView::setSamplingRate(double dSFreq)
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

    m_pFilterView->setSamplingRate(dSFreq);
}

//=============================================================================================================

void FilterSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/FilterSettingsView/filterActivated"), m_pUi->m_pCheckBoxActivateFilter->isChecked());
    settings.setValue(m_sSettingsPath + QString("/FilterSettingsView/filterFrom"), m_pUi->m_pDoubleSpinBoxFrom->value());
    settings.setValue(m_sSettingsPath + QString("/FilterSettingsView/filterTo"), m_pUi->m_pDoubleSpinBoxTo->value());
    settings.setValue(m_sSettingsPath + QString("/FilterSettingsView/filterChannelType"), m_pUi->m_pcomboBoxChannelTypes->currentText());
}

//=============================================================================================================

void FilterSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    m_pUi->m_pCheckBoxActivateFilter->setChecked(settings.value(m_sSettingsPath + QString("/FilterSettingsView/filterActivated"), false).toBool());
    m_pUi->m_pDoubleSpinBoxTo->setValue(settings.value(m_sSettingsPath + QString("/FilterSettingsView/filterTo"), 0).toDouble());
    m_pUi->m_pDoubleSpinBoxFrom->setValue(settings.value(m_sSettingsPath + QString("/FilterSettingsView/filterFrom"), 0).toDouble());
    m_pUi->m_pcomboBoxChannelTypes->setCurrentText(settings.value(m_sSettingsPath + QString("/FilterSettingsView/filterChannelType"), "All").toString());
}

//=============================================================================================================

void FilterSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            m_pUi->m_pPushButtonShowFilterOptions->hide();
            break;
        default: // default is research mode
            m_pUi->m_pPushButtonShowFilterOptions->show();
            break;
    }
}

//=============================================================================================================

void FilterSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void FilterSettingsView::onShowFilterView()
{
    if(m_pFilterView->isActiveWindow()) {
        m_pFilterView->hide();
    } else {
        m_pFilterView->activateWindow();
        m_pFilterView->show();
        m_pFilterView->updateFilterPlot();
    }
}

//=============================================================================================================

void FilterSettingsView::onFilterActivationChanged()
{
    emit filterActivationChanged(m_pUi->m_pCheckBoxActivateFilter->isChecked());

    saveSettings();
}

//=============================================================================================================

void FilterSettingsView::onFilterFromChanged()
{
    m_pFilterView->setFrom(m_pUi->m_pDoubleSpinBoxFrom->value());

    saveSettings();
}

//=============================================================================================================

void FilterSettingsView::onFilterToChanged()
{
    if(m_pUi->m_pDoubleSpinBoxFrom->value() >= 2) {
        m_pUi->m_pDoubleSpinBoxFrom->setMaximum(m_pUi->m_pDoubleSpinBoxTo->value()-1);
    } else {
        m_pUi->m_pDoubleSpinBoxFrom->setMaximum(m_pUi->m_pDoubleSpinBoxTo->value());
    }

    m_pFilterView->setTo(m_pUi->m_pDoubleSpinBoxTo->value());

    saveSettings();
}

//=============================================================================================================

void FilterSettingsView::onFilterChannelTypeChanged(const QString& sType)
{
    m_pFilterView->setChannelType(sType);

    saveSettings();
}

//=============================================================================================================

void FilterSettingsView::clearView()
{

}
