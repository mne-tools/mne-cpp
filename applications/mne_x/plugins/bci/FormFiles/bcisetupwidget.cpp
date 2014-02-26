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

    // Connect general options
    connect(ui.m_checkBox_UseSourceData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_checkBox_UseSensorData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);

    // Connect processing options
    connect(ui.m_doubleSpinBox_SlidingWindowSize, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_doubleSpinBox_BaseLineWindowSize, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_spinBox_NumberSubSignals, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &BCISetupWidget::setProcessingOptions);

    // Connect classification options
    connect(ui.m_pushButton_LoadSensorBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSensorBoundary);
    connect(ui.m_pushButton_LoadSourceBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSourceBoundary);

    //Connect about button
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &BCISetupWidget::showAboutDialog);

    //Fill info box
    QFile file(m_pBCI->m_qStringResourcePath+"readme.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        ui.m_qTextBrowser_Information->insertHtml(line);
        ui.m_qTextBrowser_Information->insertHtml("<br>");
    }
}


//*************************************************************************************************************

BCISetupWidget::~BCISetupWidget()
{
}


//*************************************************************************************************************

void BCISetupWidget::initGui()
{
    // General options
    ui.m_checkBox_UseSensorData->setChecked(m_pBCI->m_bUseSensorData);
    ui.m_checkBox_UseSourceData->setChecked(m_pBCI->m_bUseSourceData);

    // Processing options
    ui.m_doubleSpinBox_SlidingWindowSize->setValue(m_pBCI->m_dSlidingWindowSize);
    ui.m_doubleSpinBox_BaseLineWindowSize->setValue(m_pBCI->m_dBaseLineWindowSize);
    ui.m_spinBox_NumberSubSignals->setValue(m_pBCI->m_iNumberSubSignals);

    // Classification options
    ui.m_lineEdit_SensorBoundary->setText(m_pBCI->m_sSensorBoundaryPath);
    ui.m_lineEdit_SourceBoundary->setText(m_pBCI->m_sSourceBoundaryPath);
}


//*************************************************************************************************************

void BCISetupWidget::setGeneralOptions()
{
    m_pBCI->m_bUseSensorData = ui.m_checkBox_UseSourceData->isChecked();
    m_pBCI->m_bUseSourceData = ui.m_checkBox_UseSensorData->isChecked();
}


//*************************************************************************************************************

void BCISetupWidget::setProcessingOptions()
{
    m_pBCI->m_dSlidingWindowSize = ui.m_doubleSpinBox_SlidingWindowSize->value();
    m_pBCI->m_dBaseLineWindowSize = ui.m_doubleSpinBox_BaseLineWindowSize->value();
    m_pBCI->m_iNumberSubSignals = ui.m_spinBox_NumberSubSignals->value();
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSensorBoundary()
{
    QString path = QFileDialog::getOpenFileName(
                this,
                "Load decision boundary for sensor level",
                "mne_x_plugins/resources/tmsi/",
                 tr("Text files (*.txt)"));

    if(path==NULL)
        path = ui.m_lineEdit_SensorBoundary->text();

    ui.m_lineEdit_SensorBoundary->setText(path);
    m_pBCI->m_sSensorBoundaryPath = ui.m_lineEdit_SensorBoundary->text();
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSourceBoundary()
{
    QString path = QFileDialog::getOpenFileName(
                this,
                "Load decision boundary for source level",
                "mne_x_plugins/resources/tmsi/",
                 tr("Text files (*.txt)"));

    if(path==NULL)
        path = ui.m_lineEdit_SourceBoundary->text();

    ui.m_lineEdit_SourceBoundary->setText(path);
    m_pBCI->m_sSourceBoundaryPath = ui.m_lineEdit_SourceBoundary->text();
}


//*************************************************************************************************************

void BCISetupWidget::showAboutDialog()
{
    BCIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
