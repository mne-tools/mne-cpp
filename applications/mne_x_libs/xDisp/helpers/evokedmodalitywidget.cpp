//=============================================================================================================
/**
* @file     evokedmodalitywidget.cpp
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
* @brief    Implementation of the EvokedModalityWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "evokedmodalitywidget.h"
#include "../realtimeevokedwidget.h"
#include "sensoritem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QGridLayout>
#include <QDoubleValidator>

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

EvokedModalityWidget::EvokedModalityWidget(RealTimeEvokedWidget *toolbox)
: m_pRealTimeEvokedWidget(toolbox)
{
    this->setWindowTitle("Covariance Modality Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    for(qint32 i = 0; i < m_pRealTimeEvokedWidget->m_qListModalities.size(); ++i)
    {
        QString mod = m_pRealTimeEvokedWidget->m_qListModalities[i].m_sName;

        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText(mod);
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QCheckBox* t_pCheckBoxModality = new QCheckBox;
        t_pCheckBoxModality->setChecked(m_pRealTimeEvokedWidget->m_qListModalities[i].m_bActive);
        m_qListModalityCheckBox << t_pCheckBoxModality;
        connect(t_pCheckBoxModality,&QCheckBox::stateChanged,this,&EvokedModalityWidget::updateCheckbox);
        t_pGridLayout->addWidget(t_pCheckBoxModality,i,1,1,1);


        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRealTimeEvokedWidget->m_qListModalities[i].m_fNorm));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&EvokedModalityWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);

    }

    this->setLayout(t_pGridLayout);

}


//*************************************************************************************************************

void EvokedModalityWidget::updateCheckbox(qint32 state)
{
    Q_UNUSED(state)

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
    {
        if(m_qListModalityCheckBox[i]->isChecked())
            m_pRealTimeEvokedWidget->m_qListModalities[i].m_bActive = true;
        else
            m_pRealTimeEvokedWidget->m_qListModalities[i].m_bActive = false;
    }

    emit settingsChanged();
}


//*************************************************************************************************************

void EvokedModalityWidget::updateLineEdit(const QString & text)
{
    Q_UNUSED(text)

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
        m_pRealTimeEvokedWidget->m_qListModalities[i].m_fNorm = (float)m_qListModalityLineEdit[i]->text().toDouble();

    emit settingsChanged();
}
