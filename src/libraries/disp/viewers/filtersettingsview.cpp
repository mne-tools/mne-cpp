//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file filtersettingsview.cpp
 * @since 2022
 * @date  February 2026
 * @brief Implementation of the FilterSettingsView wrapper panel.
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

    connect(m_pFilterView.data(), &FilterDesignView::updateFilterFrom,[=, this](double dFrom){
                m_pUi->m_pDoubleSpinBoxFrom->setValue(dFrom);
            });
    connect(m_pFilterView.data(), &FilterDesignView::updateFilterTo,[=, this](double dTo){
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
