//=============================================================================================================
/**
* @file     compensatorview.cpp
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
* @brief    Definition of the CompensatorView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compensatorview.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QSignalMapper>


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

CompensatorView::CompensatorView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
, m_pCompSignalMapper(new QSignalMapper(this))
{
    this->setWindowTitle("Compensators");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void CompensatorView::init(const FiffInfo::SPtr pFiffInfo)
{
    if(pFiffInfo) {
        m_pFiffInfo = pFiffInfo;

        m_qListCompCheckBox.clear();

        // Compensation Selection
        QGridLayout *topLayout = new QGridLayout;

        qint32 i=0;

        for(i; i < m_pFiffInfo->comps.size(); ++i)
        {
            QString numStr;
            QCheckBox* checkBox = new QCheckBox(numStr.setNum(m_pFiffInfo->comps[i].kind));

            m_qListCompCheckBox.append(checkBox);

            connect(checkBox, SIGNAL(clicked()),
                        m_pCompSignalMapper, SLOT(map()));

            m_pCompSignalMapper->setMapping(checkBox, numStr);

            topLayout->addWidget(checkBox, i, 0);

        }

        connect(m_pCompSignalMapper, SIGNAL(mapped(const QString &)),
                    this, SIGNAL(compClicked(const QString &)));

        connect(this, &CompensatorView::compClicked,
                this, &CompensatorView::onCheckCompStatusChanged);

        //Find Comp tab and add current layout
        this->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void CompensatorView::onCheckCompStatusChanged(const QString & compName)
{
    bool currentState = false;

    for(int i = 0; i < m_qListCompCheckBox.size(); ++i) {
        if(m_qListCompCheckBox[i]->text() != compName) {
            m_qListCompCheckBox[i]->setChecked(false);
        } else {
            currentState = m_qListCompCheckBox[i]->isChecked();
        }
    }

    if(currentState) {
        emit compSelectionChanged(compName.toInt());
    } else { //If none selected
        emit compSelectionChanged(0);
    }
}
