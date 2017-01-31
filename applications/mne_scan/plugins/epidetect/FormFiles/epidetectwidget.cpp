//=============================================================================================================
/**
* @file     epidetecttoolbox.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the EpidetectWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epidetectwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EPIDETECTPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EpidetectWidget::EpidetectWidget(QWidget *parent)
: QWidget(parent)
, ui(new Ui::EpidetectToolbarWidget)
{
    //EpidetectWidget(2, 0.3, 2, 1.2, 0.3, 10, 10, 0, 2);

}


//*************************************************************************************************************

EpidetectWidget::EpidetectWidget(int dimOld, double rOld, int nOld, double marginOld, double threshold1Old, double threshold2Old, int listLengthOld, int fuzzyStepOld,  int chWheightOld, QWidget *parent)
: QWidget(parent)
, ui(new Ui::EpidetectToolbarWidget)
{
    ui->setupUi(this);
    m_iDimVal = dimOld;
    m_dRVal = rOld;
    m_iNVal = nOld;
    m_dMarginVal = marginOld;
    m_dThreshold1Val = threshold1Old;
    m_dThreshold2Val = threshold2Old;
    m_iListLengthVal = listLengthOld;
    m_iFuzzyEnStepVal = fuzzyStepOld;
    m_iChWeight = chWheightOld;

    this->ui->m_iDim->setValue(m_iDimVal);
    this->ui->m_dR->setValue(m_dRVal);
    this->ui->m_iN->setValue(m_iNVal);
    this->ui->m_dMargin->setValue(m_dMarginVal);
    this->ui->m_dThreshold1->setValue(m_dThreshold1Val);
    this->ui->m_dThreshold2->setValue(m_dThreshold2Val);
    this->ui->m_iListlength->setValue(m_iListLengthVal);
    this->ui->m_iFuzzyStep->setValue(m_iFuzzyEnStepVal);
    this->ui->m_iChweight->setValue(m_iChWeight);
    connect(ui->m_iDim, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_dR, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_iN, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_dMargin, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_dThreshold1, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_dThreshold2, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_iListlength, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_iFuzzyStep, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->m_iChweight, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);

}


//*************************************************************************************************************

EpidetectWidget::~EpidetectWidget()
{
    delete ui;
}


//*************************************************************************************************************

void EpidetectWidget::updateValues()
{
    m_iDimVal = this->ui->m_iDim->value();
    m_dRVal = this->ui->m_dR->value();
    m_iNVal = this->ui->m_iN->value();
    m_dMarginVal = this->ui->m_dMargin->value();
    m_dThreshold1Val = this->ui->m_dThreshold1->value();
    m_dThreshold2Val = this->ui->m_dThreshold2->value();
    m_iListLengthVal = this->ui->m_iListlength->value();
    m_iFuzzyEnStepVal = this->ui->m_iFuzzyStep->value();
    m_iChWeight = this->ui->m_iChweight->value();

    emit newValues();

}
