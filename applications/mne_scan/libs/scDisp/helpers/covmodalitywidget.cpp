//=============================================================================================================
/**
* @file     covmodalitywidget.cpp
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
* @brief    Definition of the CovModalityWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covmodalitywidget.h"
#include "../realtimecovwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QGridLayout>
#include <QStringList>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovModalityWidget::CovModalityWidget(RealTimeCovWidget *parent)
: m_pRealTimeCovWidget(parent)
{
    this->setWindowTitle("Covariance Modality Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    QStringList t_qListModalities;
    t_qListModalities << "MEG" << "EEG";


    qint32 count = 0;
    foreach (const QString &mod, t_qListModalities) {

        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText(mod);
        t_pGridLayout->addWidget(t_pLabelModality, count,0,1,1);
        m_qListModalities << mod;

        QCheckBox* t_pCheckBoxModality = new QCheckBox;
        if(m_pRealTimeCovWidget->m_qListPickTypes.contains(mod))
            t_pCheckBoxModality->setChecked(true);

        m_qListModalityCheckBox << t_pCheckBoxModality;

        connect(t_pCheckBoxModality,&QCheckBox::stateChanged,this,&CovModalityWidget::updateSelection);

        t_pGridLayout->addWidget(t_pCheckBoxModality,count,1,1,1);

        ++count;
    }

    this->setLayout(t_pGridLayout);

}


//*************************************************************************************************************

void CovModalityWidget::updateSelection(qint32 state)
{
    Q_UNUSED(state)

    m_pRealTimeCovWidget->m_qListPickTypes.clear();

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
        if(m_qListModalityCheckBox[i]->isChecked())
            m_pRealTimeCovWidget->m_qListPickTypes << m_qListModalities[i];

    m_pRealTimeCovWidget->m_bInitialized = false;
}
