//=============================================================================================================
/**
* @file     dummytoolbox.cpp
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
* @brief    Contains the implementation of the NoiseReductionOptionsWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "NoiseReductionOptionswidget.h"
#include "../noisereduction.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseReductionPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReductionOptionsWidget::NoiseReductionOptionsWidget(NoiseReduction* toolbox, QWidget* parent)
: QWidget(parent, Qt::Window)
, ui(new Ui::NoiseReductionOptionsWidgetClass)
, m_pNoiseReductionToolbox(toolbox)
{
    this->setWindowTitle("Noise reduction options");

    ui->setupUi(this);

    //Do the connects. Always connect GUI elemts after ui.setpUi has been called
    connect(ui->m_checkBox_activateSphara, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            m_pNoiseReductionToolbox, &NoiseReduction::setSpharaMode);
    connect(ui->m_spinBox_nBaseFctsMag, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &NoiseReductionOptionsWidget::onNBaseFctsChanged);
    connect(ui->m_spinBox_nBaseFctsGrad, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &NoiseReductionOptionsWidget::onNBaseFctsChanged);
}


//*************************************************************************************************************

NoiseReductionOptionsWidget::~NoiseReductionOptionsWidget()
{
    delete ui;
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::onNBaseFctsChanged()
{
    m_pNoiseReductionToolbox->setSpharaNBaseFcts(ui->m_spinBox_nBaseFctsGrad->value(), ui->m_spinBox_nBaseFctsMag->value());
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::setAcquisitionSystem(QString sSystem)
{
    if(sSystem == "VectorView") {
        ui->m_label_nBaseFctsMag->setText("Mag");
        ui->m_spinBox_nBaseFctsMag->setMaximum(102);
        ui->m_spinBox_nBaseFctsMag->setValue(102);

        ui->m_label_nBaseFctsGrad->setText("Grad");
        ui->m_spinBox_nBaseFctsGrad->setMaximum(102);
        ui->m_spinBox_nBaseFctsGrad->setValue(102);
    }

    if(sSystem == "BabyMEG") {
        ui->m_label_nBaseFctsMag->setText("Outer layer");
        ui->m_spinBox_nBaseFctsMag->setMaximum(105);
        ui->m_spinBox_nBaseFctsMag->setValue(105);

        ui->m_label_nBaseFctsGrad->setText("Inner layer");
        ui->m_spinBox_nBaseFctsGrad->setMaximum(270);
        ui->m_spinBox_nBaseFctsGrad->setValue(270);
    }
}
