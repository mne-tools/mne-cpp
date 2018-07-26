//=============================================================================================================
/**
* @file     channeldatasettingsview.cpp
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
* @brief    Definition of the ChannelDataSettingsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldatasettingsview.h"

#include "ui_channeldatasettingsview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QColorDialog>


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

ChannelDataSettingsView::ChannelDataSettingsView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::ChannelDataSettingsViewWidget)
{
    ui->setupUi(this);

    this->setWindowTitle("Channel Data View Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void ChannelDataSettingsView::init()
{
    //Number of visible channels
    connect(ui->m_doubleSpinBox_numberVisibleChannels, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &ChannelDataSettingsView::zoomChanged);

    //Window size
    connect(ui->m_spinBox_windowSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ChannelDataSettingsView::timeWindowChanged);

    //Distance for timer spacer
    connect(ui->m_comboBox_distaceTimeSpacer, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ChannelDataSettingsView::onDistanceTimeSpacerChanged);

    //Colors
    connect(ui->m_pushButton_backgroundColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &ChannelDataSettingsView::onViewColorButtonClicked);

    connect(ui->m_pushButton_signalColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &ChannelDataSettingsView::onViewColorButtonClicked);

    connect(ui->m_pushButton_makeScreenshot, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &ChannelDataSettingsView::onMakeScreenshot);
}


//*************************************************************************************************************

void ChannelDataSettingsView::setViewParameters(double zoomFactor, int windowSize)
{
    ui->m_doubleSpinBox_numberVisibleChannels->setValue(zoomFactor);
    ui->m_spinBox_windowSize->setValue(windowSize);

    zoomChanged(zoomFactor);
    timeWindowChanged(windowSize);
}


//*************************************************************************************************************

QString ChannelDataSettingsView::getDistanceTimeSpacer()
{
    return ui->m_comboBox_distaceTimeSpacer->currentText();
}


//*************************************************************************************************************

void ChannelDataSettingsView::setDistanceTimeSpacer(int value)
{
    ui->m_comboBox_distaceTimeSpacer->setCurrentText(QString::number(value));
}


//*************************************************************************************************************

void ChannelDataSettingsView::setSignalBackgroundColors(const QColor& signalColor, const QColor& backgroundColor)
{
    ui->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(backgroundColor.red()).arg(backgroundColor.green()).arg(backgroundColor.blue()));
    ui->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(signalColor.red()).arg(signalColor.green()).arg(signalColor.blue()));

    m_colCurrentBackgroundColor = backgroundColor;
    m_colCurrentSignalColor = signalColor;
}


//*************************************************************************************************************

const QColor& ChannelDataSettingsView::getSignalColor()
{
    return m_colCurrentSignalColor;
}


//*************************************************************************************************************

const QColor& ChannelDataSettingsView::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}


//*************************************************************************************************************

void ChannelDataSettingsView::onDistanceTimeSpacerChanged(qint32 value)
{
    switch(value) {
        case 0:
            emit distanceTimeSpacerChanged(100);
        break;

        case 1:
            emit distanceTimeSpacerChanged(200);
        break;

        case 2:
            emit distanceTimeSpacerChanged(500);
        break;

        case 3:
            emit distanceTimeSpacerChanged(1000);
        break;
    }

    //emit updateConnectedView();
}


//*************************************************************************************************************

void ChannelDataSettingsView::onViewColorButtonClicked()
{
    QColorDialog* pDialog = new QColorDialog(this);

    QObject* obj = sender();
    if(obj == ui->m_pushButton_signalColor) {
        pDialog->setCurrentColor(m_colCurrentSignalColor);
        pDialog->setWindowTitle("Select Signal Color");

        pDialog->exec();
        m_colCurrentSignalColor = pDialog->currentColor();

        //Set color of button new new scene color
        ui->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentSignalColor.red()).arg(m_colCurrentSignalColor.green()).arg(m_colCurrentSignalColor.blue()));

        emit signalColorChanged(m_colCurrentSignalColor);
    }

    if( obj == ui->m_pushButton_backgroundColor ) {
        pDialog->setCurrentColor(m_colCurrentBackgroundColor);
        pDialog->setWindowTitle("Select Background Color");

        pDialog->exec();
        m_colCurrentBackgroundColor = pDialog->currentColor();

        //Set color of button new new scene color
        ui->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentBackgroundColor.red()).arg(m_colCurrentBackgroundColor.green()).arg(m_colCurrentBackgroundColor.blue()));

        emit backgroundColorChanged(m_colCurrentBackgroundColor);
    }
}


//*************************************************************************************************************

void ChannelDataSettingsView::onTimeWindowChanged(int value)
{
    emit timeWindowChanged(value);
}


//*************************************************************************************************************

void ChannelDataSettingsView::onZoomChanged(double value)
{
    emit zoomChanged(value);
}


//*************************************************************************************************************

void ChannelDataSettingsView::onMakeScreenshot()
{
    emit makeScreenshot(ui->m_comboBox_imageType->currentText());
}

