//=============================================================================================================
/**
 * @file     fiffrawviewsettings.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the FiffRawViewSettings Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffrawviewsettings.h"

#include "ui_fiffrawviewsettings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColorDialog>
#include <QSettings>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawViewSettings::FiffRawViewSettings(const QString &sSettingsPath,
                                         QWidget *parent,
                                         Qt::WindowFlags f)
: QWidget(parent, f)
, m_pUi(new Ui::FiffRawViewSettingsWidget)
, m_sSettingsPath(sSettingsPath)
{
    m_pUi->setupUi(this);

    this->setWindowTitle("Channel Data View Settings");
    this->setMinimumWidth(330);

    loadSettings(m_sSettingsPath);
}

//=============================================================================================================

FiffRawViewSettings::~FiffRawViewSettings()
{
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FiffRawViewSettings::setWidgetList(const QStringList& lVisibleWidgets)
{
    if(lVisibleWidgets.contains("numberChannels", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Number of visible channels
        connect(m_pUi->m_doubleSpinBox_numberVisibleChannels, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &FiffRawViewSettings::zoomChanged);
    } else {
        m_pUi->m_doubleSpinBox_numberVisibleChannels->hide();
        m_pUi->label_numberChannels->hide();
    }

    if(lVisibleWidgets.contains("windowSize", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Window size
        connect(m_pUi->m_spinBox_windowSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &FiffRawViewSettings::timeWindowChanged);
    } else {
        m_pUi->m_spinBox_windowSize->hide();
        m_pUi->label_windowSize->hide();
    }

    if(lVisibleWidgets.contains("distanceSpacers", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Distance for timer spacer
        connect(m_pUi->m_comboBox_distaceTimeSpacer, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &FiffRawViewSettings::onDistanceTimeSpacerChanged);
    } else {
        m_pUi->m_comboBox_distaceTimeSpacer->hide();
        m_pUi->label_timeSpacers->hide();
    }

    if(lVisibleWidgets.contains("backgroundColor", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Background Colors
        connect(m_pUi->m_pushButton_backgroundColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &FiffRawViewSettings::onViewColorButtonClicked);
    } else {
        m_pUi->m_pushButton_backgroundColor->hide();
        m_pUi->label_backgroundColor->hide();
    }

    if(lVisibleWidgets.contains("signalColor", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Signal Colors
        connect(m_pUi->m_pushButton_signalColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &FiffRawViewSettings::onViewColorButtonClicked);
    } else {
        m_pUi->m_pushButton_signalColor->hide();
        m_pUi->label_signalColor->hide();
    }

    if(lVisibleWidgets.contains("screenshot", Qt::CaseInsensitive) || lVisibleWidgets.isEmpty()) {
        //Signal Colors
        connect(m_pUi->m_pushButton_makeScreenshot, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &FiffRawViewSettings::onMakeScreenshot);
    } else {
        m_pUi->m_pushButton_makeScreenshot->hide();
        m_pUi->m_comboBox_imageType->hide();
    }

    this->adjustSize();
}

//=============================================================================================================

void FiffRawViewSettings::setWindowSize(int windowSize)
{
    m_pUi->m_spinBox_windowSize->setValue(windowSize);

    timeWindowChanged(windowSize);
}

//=============================================================================================================

void FiffRawViewSettings::setZoom(double zoomFactor)
{
    m_pUi->m_doubleSpinBox_numberVisibleChannels->setValue(zoomFactor);

    zoomChanged(zoomFactor);
}

//=============================================================================================================

int FiffRawViewSettings::getDistanceTimeSpacer()
{
    return m_pUi->m_comboBox_distaceTimeSpacer->currentText().toInt();
}

//=============================================================================================================

void FiffRawViewSettings::setDistanceTimeSpacer(int value)
{
    m_pUi->m_comboBox_distaceTimeSpacer->setCurrentText(QString::number(value));
}

//=============================================================================================================

void FiffRawViewSettings::setBackgroundColor(const QColor& backgroundColor)
{
    m_pUi->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(backgroundColor.red()).arg(backgroundColor.green()).arg(backgroundColor.blue()));

    m_colCurrentBackgroundColor = backgroundColor;
}

//=============================================================================================================

void FiffRawViewSettings::setSignalColor(const QColor& signalColor)
{
    m_pUi->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(signalColor.red()).arg(signalColor.green()).arg(signalColor.blue()));

    m_colCurrentSignalColor = signalColor;
}

//=============================================================================================================

const QColor& FiffRawViewSettings::getSignalColor()
{
    return m_colCurrentSignalColor;
}

//=============================================================================================================

const QColor& FiffRawViewSettings::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}

//=============================================================================================================

double FiffRawViewSettings::getZoom()
{
    return m_pUi->m_doubleSpinBox_numberVisibleChannels->value();
}

//=============================================================================================================

int FiffRawViewSettings::getWindowSize()
{
    return m_pUi->m_spinBox_windowSize->value();
}

//=============================================================================================================

void FiffRawViewSettings::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/viewZoomFactor"), getZoom());
    settings.setValue(settingsPath + QString("/viewWindowSize"), getWindowSize());
    settings.setValue(settingsPath + QString("/signalColor"), getSignalColor());
    settings.setValue(settingsPath + QString("/backgroundColor"), getBackgroundColor());
    settings.setValue(settingsPath + QString("/distanceTimeSpacer"), getDistanceTimeSpacer());
}

//=============================================================================================================

void FiffRawViewSettings::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
    setZoom(settings.value(settingsPath + QString("/viewZoomFactor"), 0.3).toDouble());
    setWindowSize(settings.value(settingsPath + QString("/viewWindowSize"), 10).toInt());
    QColor color = Qt::blue;
    setSignalColor(settings.value(settingsPath + QString("/signalColor"), color).value<QColor>());
    color = Qt::white;
    setBackgroundColor(settings.value(settingsPath + QString("/backgroundColor"), color).value<QColor>());
    setDistanceTimeSpacer(settings.value(settingsPath + QString("/distanceTimeSpacer"), 1000).toInt());
}

//=============================================================================================================

void FiffRawViewSettings::onDistanceTimeSpacerChanged(qint32 value)
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

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FiffRawViewSettings::onViewColorButtonClicked()
{
    QColorDialog* pDialog = new QColorDialog(this);

    QObject* obj = sender();
    if(obj == m_pUi->m_pushButton_signalColor) {
        pDialog->setCurrentColor(m_colCurrentSignalColor);
        pDialog->setWindowTitle("Signal Color");

        pDialog->exec();
        m_colCurrentSignalColor = pDialog->currentColor();

        //Set color of button new new scene color
        m_pUi->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentSignalColor.red()).arg(m_colCurrentSignalColor.green()).arg(m_colCurrentSignalColor.blue()));

        emit signalColorChanged(m_colCurrentSignalColor);
    }

    if( obj == m_pUi->m_pushButton_backgroundColor ) {
        pDialog->setCurrentColor(m_colCurrentBackgroundColor);
        pDialog->setWindowTitle("Background Color");

        pDialog->exec();
        m_colCurrentBackgroundColor = pDialog->currentColor();

        //Set color of button new new scene color
        m_pUi->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentBackgroundColor.red()).arg(m_colCurrentBackgroundColor.green()).arg(m_colCurrentBackgroundColor.blue()));

        emit backgroundColorChanged(m_colCurrentBackgroundColor);
    }

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FiffRawViewSettings::onTimeWindowChanged(int value)
{
    emit timeWindowChanged(value);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FiffRawViewSettings::onZoomChanged(double value)
{
    emit zoomChanged(value);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void FiffRawViewSettings::onMakeScreenshot()
{
    emit makeScreenshot(m_pUi->m_comboBox_imageType->currentText());
}

