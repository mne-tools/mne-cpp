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
    this->setWindowTitle("Projections");
    if(m_pFiffInfo)
    {
        m_qListCheckBox.clear();
        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        m_enableDisableProjectors = new QCheckBox("Enable all");

        topLayout->addWidget(m_enableDisableProjectors, 0, 1);
        connect(m_enableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                this, &ProjectorWidget::enableDisableAll);

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        topLayout->addWidget(line, 1, 1);

        bool bAllActivated = true;

        for(qint32 i=0; i < m_pFiffInfo->projs.size(); ++i)
        {
            QCheckBox* checkBox = new QCheckBox(m_pFiffInfo->projs[i].desc);
            checkBox->setChecked(m_pFiffInfo->projs[i].active);

            if(m_pFiffInfo->projs[i].active == false)
                bAllActivated = false;

            m_qListCheckBox.append(checkBox);

            connect(checkBox, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
                    this, &ProjectorWidget::checkStatusChanged);

            topLayout->addWidget(checkBox, i+2, 1); //+2 because we already added two widgets before the first projector check box
        }

        m_enableDisableProjectors->setChecked(bAllActivated);

        setLayout(topLayout);

        //Set default activation to true
        enableDisableAll(true);
    }
}


//*************************************************************************************************************

void ProjectorWidget::checkStatusChanged(int status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListCheckBox.size(); ++i) {
        if(m_qListCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListCheckBox[i]->isChecked();
    }

    m_enableDisableProjectors->setChecked(bAllActivated);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void ProjectorWidget::enableDisableAll(bool status)
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
