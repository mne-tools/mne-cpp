//=============================================================================================================
/**
* @file     projectorwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the ProjectorWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectorwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectorWidget::ProjectorWidget(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
}


//*************************************************************************************************************

void ProjectorWidget::createUI()
{
    if(m_pFiffInfo)
    {
        m_qListCheckBox.clear();
        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        qint32 i;
        for(i = 0; i < m_pFiffInfo->projs.size(); ++i)
        {
            QCheckBox* checkBox = new QCheckBox(m_pFiffInfo->projs[i].desc);
            checkBox->setChecked(m_pFiffInfo->projs[i].active);

            m_qListCheckBox.append(checkBox);

            connect(checkBox, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &ProjectorWidget::checkStatusChanged);
            topLayout->addWidget(checkBox, i, 1);
        }

        QCheckBox *enableDisableProjectors = new QCheckBox("Enable/Disable all");

        topLayout->addWidget(enableDisableProjectors, i+1, 1);
        connect(enableDisableProjectors, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
                this, &ProjectorWidget::enableDisableAll);

        setLayout(topLayout);
    }
}


//*************************************************************************************************************

void ProjectorWidget::checkStatusChanged(int status)
{
    Q_UNUSED(status)

    for(qint32 i = 0; i < m_qListCheckBox.size(); ++i)
        this->m_pFiffInfo->projs[i].active = m_qListCheckBox[i]->isChecked();

    emit projSelectionChanged();
}


//*************************************************************************************************************

void ProjectorWidget::enableDisableAll(int status)
{
    for(int i=0; i<m_qListCheckBox.size(); i++)
        m_qListCheckBox.at(i)->setChecked(status);
}


//*************************************************************************************************************

void ProjectorWidget::setFiffInfo(FIFFLIB::FiffInfo::SPtr& p_pFiffInfo)
{
    this->m_pFiffInfo = p_pFiffInfo;
    createUI();
}
