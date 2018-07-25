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


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>


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

FilterSettingsView::FilterSettingsView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
, m_pShowFilterOptions(Q_NULLPTR)
{
    this->setWindowTitle("Compensators");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void FilterSettingsView::filterGroupChanged(QList<QCheckBox*> list)
{
    m_qFilterListCheckBox.clear();

    for(int u = 0; u < list.size(); ++u) {
        QCheckBox* tempCheckBox = new QCheckBox(list[u]->text());
        tempCheckBox->setChecked(list[u]->isChecked());

        connect(tempCheckBox, &QCheckBox::toggled,
                list[u], &QCheckBox::setChecked);

        if(tempCheckBox->text() == "Activate user designed filter")
            connect(tempCheckBox, &QCheckBox::toggled,
                    this, &FilterSettingsView::onUserFilterToggled);

        connect(list[u], &QCheckBox::toggled,
                tempCheckBox, &QCheckBox::setChecked);

        m_qFilterListCheckBox.append(tempCheckBox);
    }

    //Delete all widgets in the filter layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    //Add filters
    int u = 0;

    for(u; u < m_qFilterListCheckBox.size(); ++u) {
        topLayout->addWidget(m_qFilterListCheckBox[u], u, 0);
    }

    //Add push button for filter options
    m_pShowFilterOptions = new QPushButton();
//        m_pShowFilterOptions->setText("Open Filter options");
    m_pShowFilterOptions->setText("Filter options");
    m_pShowFilterOptions->setCheckable(false);
    connect(m_pShowFilterOptions, &QPushButton::clicked,
            this, &FilterSettingsView::onShowFilterOptions);

    topLayout->addWidget(m_pShowFilterOptions, u+1, 0);

    //Find Filter tab and add current layout
    this->setLayout(topLayout);
}


//*************************************************************************************************************

void FilterSettingsView::onUserFilterToggled(bool state)
{
    Q_UNUSED(state);
    //qDebug()<<"onUserFilterToggled";
    //emit updateConnectedView();
}


//*************************************************************************************************************

void FilterSettingsView::onShowFilterOptions(bool state)
{
//    if(state)
//        m_pShowFilterOptions->setText("Close filter options");
//    else
//        m_pShowFilterOptions->setText("Open filter options");

//    m_pShowFilterOptions->setChecked(state);

//    emit showFilterOptions(state);

    Q_UNUSED(state);
    emit showFilterOptions(true);
}
