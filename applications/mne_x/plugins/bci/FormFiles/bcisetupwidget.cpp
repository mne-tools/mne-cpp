//=============================================================================================================
/**
* @file     bcisetupwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*			Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the BCISetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bcisetupwidget.h"
#include "bciaboutwidget.h"
#include "../bci.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BCISetupWidget::BCISetupWidget(BCI* pBCI, QWidget* parent)
: QWidget(parent)
, m_pBCI(pBCI)
{
    ui.setupUi(this);

    //Connect general options
    connect(ui.m_checkBox_UseSourceData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_checkBox_UseSensorData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);

    //Connect processing options
    connect(ui.m_spinBox_SlidingWindowSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_spinBox_BaseLineWindowSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &BCISetupWidget::setProcessingOptions);

    //Connect classification options
    connect(ui.m_pushButton_LoadSensorBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSourceBoundary);
    connect(ui.m_pushButton_LoadSourceBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSensorBoundary);
}


//*************************************************************************************************************

BCISetupWidget::~BCISetupWidget()
{

}


//*************************************************************************************************************

void BCISetupWidget::initGui()
{
}


//*************************************************************************************************************

void BCISetupWidget::setGeneralOptions()
{
//    BCISetupWidget->m_bUseChExponent = ui.m_checkBox_UseChExponent->isChecked();
//    BCISetupWidget->m_bUseUnitGain = ui.m_checkBox_UseUnitGain->isChecked();
}


//*************************************************************************************************************

void BCISetupWidget::setProcessingOptions()
{
//    BCISetupWidget->m_iSamplingFreq = ui.m_spinBox_SamplingFreq->value();
//    BCISetupWidget->m_iNumberOfChannels = ui.m_spinBox_NumberOfChannels->value();
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSourceBoundary()
{
//    QString path = QFileDialog::getSaveFileName(
//                this,
//                "Save to fif file",
//                "mne_x_plugins/resources/tmsi/EEG_data_001_raw.fif",
//                 tr("Fif files (*.fif)"));

//    if(path==NULL)
//        path = ui.m_lineEdit_outputDir->text();

//    ui.m_lineEdit_outputDir->setText(path);
//    BCISetupWidget->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSensorBoundary()
{
//    QString path = QFileDialog::getSaveFileName(
//                this,
//                "Save to fif file",
//                "mne_x_plugins/resources/tmsi/EEG_data_001_raw.fif",
//                 tr("Fif files (*.fif)"));

//    if(path==NULL)
//        path = ui.m_lineEdit_outputDir->text();

//    ui.m_lineEdit_outputDir->setText(path);
//    BCISetupWidget->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void BCISetupWidget::showAboutDialog()
{
    BCIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
