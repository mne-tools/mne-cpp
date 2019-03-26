//=============================================================================================================
/**
* @file     filtersettingsview.cpp
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
* @brief    Definition of the FilterSettingsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtersettingsview.h"

#include "filterdesignview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>


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

FilterSettingsView::FilterSettingsView(const QString& sSettingsPath,
                                       QWidget *parent,
                                       Qt::WindowFlags f)
: QWidget(parent, f)
, m_sSettingsPath(sSettingsPath)
{
    this->setWindowTitle("Filter Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    m_pFilterView = FilterDesignView::SPtr::create(m_sSettingsPath,
                                                   this,
                                                   Qt::Window);

    //Delete all widgets in the filter layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    //Add filters
    m_pCheckBox = new QCheckBox("Activate filter");

    connect(m_pCheckBox.data(), &QCheckBox::toggled,
            this, &FilterSettingsView::onFilterActivationChanged);

    topLayout->addWidget(m_pCheckBox, 0, 0);

    //Add push button for filter options
    QPushButton* pShowFilterOptions = new QPushButton();
    pShowFilterOptions->setText("Filter options");
    pShowFilterOptions->setCheckable(false);
    connect(pShowFilterOptions, &QPushButton::clicked,
            this, &FilterSettingsView::onShowFilterView);

    topLayout->addWidget(pShowFilterOptions, 1, 0);

    //Find Filter tab and add current layout
    this->setLayout(topLayout);

    loadSettings(m_sSettingsPath);
}

//*************************************************************************************************************

FilterSettingsView::~FilterSettingsView()
{
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

QSharedPointer<FilterDesignView> FilterSettingsView::getFilterView()
{
    return m_pFilterView;
}


//*************************************************************************************************************

bool FilterSettingsView::getFilterActive()
{
    return m_pCheckBox->isChecked();
}


//*************************************************************************************************************

void FilterSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/filterActivated"), m_pCheckBox->isChecked());
}


//*************************************************************************************************************

void FilterSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    m_pCheckBox->setChecked(settings.value(settingsPath + QString("/filterActivated"), false).toBool());
}


//*************************************************************************************************************

void FilterSettingsView::onShowFilterView()
{
    if(m_pFilterView->isActiveWindow()) {
        m_pFilterView->hide();
    } else {
        m_pFilterView->activateWindow();
        m_pFilterView->show();
    }
}


//*************************************************************************************************************

void FilterSettingsView::onFilterActivationChanged()
{
    emit filterActivationChanged(m_pCheckBox->isChecked());

    saveSettings(m_sSettingsPath);
}
