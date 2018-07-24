//=============================================================================================================
/**
* @file     projectorsview.cpp
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
* @brief    Definition of the ProjectorsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectorsview.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QFrame>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectorsView::ProjectorsView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
{
    this->setWindowTitle("Projectors");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void ProjectorsView::init(const FiffInfo::SPtr pFiffInfo)
{
    if(pFiffInfo) {
        m_pFiffInfo = pFiffInfo;

        //If no projectors are defined return here
        if(pFiffInfo->projs.empty()) {
            return;
        }

        m_qListProjCheckBox.clear();
        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        bool bAllActivated = true;

        qint32 i=0;

        for(i; i < pFiffInfo->projs.size(); ++i)
        {
            QCheckBox* checkBox = new QCheckBox(pFiffInfo->projs[i].desc);
            checkBox->setChecked(pFiffInfo->projs[i].active);

            if(pFiffInfo->projs[i].active == false)
                bAllActivated = false;

            m_qListProjCheckBox.append(checkBox);

            connect(checkBox, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                    this, &ProjectorsView::onCheckProjStatusChanged);

            topLayout->addWidget(checkBox, i, 0); //+2 because we already added two widgets before the first projector check box

//            if(i>m_pFiffInfo->projs.size()/2)
//                topLayout->addWidget(checkBox, i-rowCount, 1); //+2 because we already added two widgets before the first projector check box
//            else {
//                topLayout->addWidget(checkBox, i, 0); //+2 because we already added two widgets before the first projector check box
//                rowCount++;
//            }
        }

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        topLayout->addWidget(line, i+1, 0);

        m_pEnableDisableProjectors = new QCheckBox("Enable all");
        m_pEnableDisableProjectors->setChecked(bAllActivated);
        topLayout->addWidget(m_pEnableDisableProjectors, i+2, 0);
        connect(m_pEnableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &ProjectorsView::onEnableDisableAllProj);

        //Find SSP tab and add current layout
        this->setLayout(topLayout);

        //Set default activation to true
        onEnableDisableAllProj(true);
    }
}


//*************************************************************************************************************

void ProjectorsView::onEnableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for(int i=0; i<m_qListProjCheckBox.size(); i++)
        m_qListProjCheckBox.at(i)->setChecked(status);

    //Set all projection activation states to status
    for(int i=0; i < m_pFiffInfo->projs.size(); ++i)
        m_pFiffInfo->projs[i].active = status;

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(status);
    }

    emit projSelectionChanged();
}


//*************************************************************************************************************

void ProjectorsView::onCheckProjStatusChanged(bool status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if(m_qListProjCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListProjCheckBox[i]->isChecked();
    }

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(bAllActivated);
    }

    emit projSelectionChanged();
}
