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

using namespace EpidetectPlugin;


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
    dimVal = dimOld;
    rVal = rOld;
    nVal = nOld;
    marginVal = marginOld;
    threshold1Val = threshold1Old;
    threshold2Val = threshold2Old;
    listLengthVal = listLengthOld;
    fuzzyEnStepVal = fuzzyStepOld;
    chWheight = chWheightOld;

    this->ui->dim->setValue(dimVal);
    this->ui->r->setValue(rVal);
    this->ui->n->setValue(nVal);
    this->ui->margin->setValue(marginVal);
    this->ui->threshold1->setValue(threshold1Val);
    this->ui->threshold2->setValue(threshold2Val);
    this->ui->listlength->setValue(listLengthVal);
    this->ui->FuzzyStep->setValue(fuzzyEnStepVal);
    this->ui->chwheight->setValue(chWheight);
    connect(ui->dim, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->r, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->n, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->margin, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->threshold1, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->threshold2, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->listlength, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->FuzzyStep, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);
    connect(ui->chwheight, &QAbstractSpinBox::editingFinished, this, &EpidetectWidget::updateValues);

}


//*************************************************************************************************************

EpidetectWidget::~EpidetectWidget()
{
    delete ui;
}


//*************************************************************************************************************

void EpidetectWidget::updateValues()
{
    dimVal = this->ui->dim->value();
    rVal = this->ui->r->value();
    nVal = this->ui->n->value();
    marginVal = this->ui->margin->value();
    threshold1Val = this->ui->threshold1->value();
    threshold2Val = this->ui->threshold2->value();
    listLengthVal = this->ui->listlength->value();
    fuzzyEnStepVal = this->ui->FuzzyStep->value();
    chWheight = this->ui->chwheight->value();

    emit newValues();

}
