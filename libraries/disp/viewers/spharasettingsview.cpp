//=============================================================================================================
/**
* @file     spharasettingsview.cpp
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
* @brief    Definition of the SpharaSettingsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spharasettingsview.h"

#include "ui_spharasettingsview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


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

SpharaSettingsView::SpharaSettingsView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::SpharaSettingsViewWidget)
{
    ui->setupUi(this);

    //Sphara activation changed
    connect(ui->m_checkBox_activateSphara, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &SpharaSettingsView::onSpharaButtonClicked);

    //Sphara options changed
    connect(ui->m_comboBox_spharaSystem, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    connect(ui->m_spinBox_spharaFirst, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    connect(ui->m_spinBox_spharaSecond, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &SpharaSettingsView::onSpharaOptionsChanged);

    this->setWindowTitle("SPHARA Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

SpharaSettingsView::~SpharaSettingsView()
{
    delete ui;
}


//*************************************************************************************************************

void SpharaSettingsView::onSpharaButtonClicked(bool state)
{
    emit spharaActivationChanged(state);
}


//*************************************************************************************************************

void SpharaSettingsView::onSpharaOptionsChanged()
{
    ui->m_label_spharaFirst->show();
    ui->m_spinBox_spharaFirst->show();

    ui->m_label_spharaSecond->show();
    ui->m_spinBox_spharaSecond->show();

    if(ui->m_comboBox_spharaSystem->currentText() == "VectorView") {
        ui->m_label_spharaFirst->setText("Mag");
        ui->m_spinBox_spharaFirst->setMaximum(102);

        ui->m_label_spharaSecond->setText("Grad");
        ui->m_spinBox_spharaSecond->setMaximum(102);
    }

    if(ui->m_comboBox_spharaSystem->currentText() == "BabyMEG") {
        ui->m_label_spharaFirst->setText("Inner layer");
        ui->m_spinBox_spharaFirst->setMaximum(270);

        ui->m_label_spharaSecond->setText("Outer layer");
        ui->m_spinBox_spharaSecond->setMaximum(105);
    }

    if(ui->m_comboBox_spharaSystem->currentText() == "EEG") {
        ui->m_label_spharaFirst->setText("EEG");
        ui->m_spinBox_spharaFirst->setMaximum(256);

        ui->m_label_spharaSecond->hide();
        ui->m_spinBox_spharaSecond->hide();
    }

    emit spharaOptionsChanged(ui->m_comboBox_spharaSystem->currentText(),
                              ui->m_spinBox_spharaFirst->value(),
                              ui->m_spinBox_spharaSecond->value());
}
