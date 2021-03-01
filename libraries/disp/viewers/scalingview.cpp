//=============================================================================================================
/**
 * @file     scalingview.cpp
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
 * @brief    Definition of the ScalingView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scalingview.h"

#include "ui_scalingview.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include <QDebug>
#include <QSettings>
#include <QKeyEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE GLOBAL DISPLIB METHODS
//=============================================================================================================

/**
 * Default scales for each channel by type.
 */
const static float m_fScaleMAG = 1e-12f;      /**< Default scale for channel kind and unit of MAG */
const static float m_fScaleGRAD = 1e-15f;     /**< Default scale for channel kind and unit of GRAD */
const static float m_fScaleEEG = 1e-5f;       /**< Default scale for channel kind and unit of EEG */
const static float m_fScaleEOG = 1e-6f;       /**< Default scale for channel kind and unit of EOG */
const static float m_fScaleECG = 1e-2f;       /**< Default scale for channel kind and unit of ECG */
const static float m_fScaleSTIM = 1e-3f;      /**< Default scale for channel kind and unit of STIM */
const static float m_fScaleMISC = 1e-3f;      /**< Default scale for channel kind and unit of MISC */
const static float m_fScaleEMG = 1e-3f;       /**< Default scale for channel kind and unit of EMG */

float DISPLIB::getDefaultScalingValue(int iChannelKind,
                                      int iChannelUnit)
{
    float fMaxScale(1e-9f);

    switch(iChannelKind) {
        case FIFFV_MEG_CH: {
            if( iChannelUnit == FIFF_UNIT_T_M ) { //Gradiometers
                    fMaxScale = m_fScaleGRAD;
                }
            else if( iChannelUnit == FIFF_UNIT_T ) { //Magnetometers
                    fMaxScale = m_fScaleMAG;
                }
            break;
        }

        case FIFFV_REF_MEG_CH: {
            fMaxScale = m_fScaleMAG;
            break;
        }

        case FIFFV_EEG_CH: { //EEG Channels
            fMaxScale = m_fScaleEEG;
            break;
        }

        case FIFFV_ECG_CH: { //ECG Channels
            fMaxScale = m_fScaleECG;
            break;
        }
        case FIFFV_EMG_CH: { //EMG Channels
            fMaxScale = m_fScaleEMG;
            break;
        }
        case FIFFV_EOG_CH: { //EOG Channels
            fMaxScale = m_fScaleEOG;
            break;
        }

        case FIFFV_STIM_CH: { //STIM Channels
            fMaxScale = m_fScaleSTIM;
            break;
        }

        case FIFFV_MISC_CH: { //MISC Channels
            fMaxScale = m_fScaleMISC;
            break;
        }
    }

    return fMaxScale;
}

//=============================================================================================================

float DISPLIB::getScalingValue(const QMap<qint32, float>& qMapChScaling,
                                   int iChannelKind,
                                   int iChannelUnit)
{
   float fMaxScale = qMapChScaling.value(iChannelKind);

   if(iChannelKind == FIFFV_MEG_CH) {
        fMaxScale = qMapChScaling.value(iChannelUnit);
    }

    if(qIsNaN(fMaxScale) || fMaxScale == 0) {
        fMaxScale = DISPLIB::getDefaultScalingValue(iChannelKind, iChannelUnit);
     }
    return fMaxScale;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

double ScalingView::m_dMAGtoGRADSpinboxConverter(100);

ScalingView::ScalingView(const QString& sSettingsPath,
                         QWidget *parent,
                         Qt::WindowFlags f,
                         const QStringList& lChannelsToShow)
: AbstractView(parent, f)
, m_lChannelTypesToShow(lChannelsToShow)
, m_pUi(new Ui::ScalingViewWidget)
, m_bIsShiftKeyPressed(false)
, m_bManagingSpinBoxChange(false)
, m_bManagingSliderChange(false)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Scaling");
    this->setMinimumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

ScalingView::~ScalingView()
{
    saveSettings();
    delete m_pUi;
}

//=============================================================================================================

void ScalingView::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Shift)
    {
        m_bIsShiftKeyPressed = false;
    }
    QWidget::keyReleaseEvent(event);
}

//=============================================================================================================

void ScalingView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Shift)
    {
        m_bIsShiftKeyPressed = true;
    }
    QWidget::keyPressEvent(event);
}

//=============================================================================================================

QMap<qint32,float> ScalingView::getScaleMap() const
{
    return m_qMapChScaling;
}

//=============================================================================================================

void ScalingView::setScaleMap(const QMap<qint32,float>& qMapChScaling)
{
    m_qMapChScaling = qMapChScaling;

    redrawGUI();
}

//=============================================================================================================

void ScalingView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    if(m_qMapChScaling.contains(FIFF_UNIT_T)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleMAG"), m_qMapChScaling[FIFF_UNIT_T]);
    }

    if(m_qMapChScaling.contains(FIFF_UNIT_T_M)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleGRAD"), m_qMapChScaling[FIFF_UNIT_T_M]);
    }

    if(m_qMapChScaling.contains(FIFFV_EEG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleEEG"), m_qMapChScaling[FIFFV_EEG_CH]);
    }

    if(m_qMapChScaling.contains(FIFFV_EOG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleEOG"), m_qMapChScaling[FIFFV_EOG_CH]);
    }

    if(m_qMapChScaling.contains(FIFFV_ECG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleECG"), m_qMapChScaling[FIFFV_ECG_CH]);
    }

    if(m_qMapChScaling.contains(FIFFV_STIM_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleSTIM"), m_qMapChScaling[FIFFV_STIM_CH]);
    }

    if(m_qMapChScaling.contains(FIFFV_MISC_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleMISC"), m_qMapChScaling[FIFFV_MISC_CH]);
    }
}

//=============================================================================================================

void ScalingView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    float val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleMAG"), m_fScaleMAG * 3).toFloat();
    m_qMapChScaling.insert(FIFF_UNIT_T, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleGRAD"), m_fScaleGRAD * 300 * 100/*convert cm to m*/).toFloat();
    m_qMapChScaling.insert(FIFF_UNIT_T_M, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleEEG"), m_fScaleEEG * 10).toFloat();
    m_qMapChScaling.insert(FIFFV_EEG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleEOG"), m_fScaleEOG).toFloat();
    m_qMapChScaling.insert(FIFFV_EOG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleECG"), m_fScaleECG).toFloat();
    m_qMapChScaling.insert(FIFFV_ECG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleSTIM"), m_fScaleSTIM).toFloat();
    m_qMapChScaling.insert(FIFFV_STIM_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleMISC"), m_fScaleMISC).toFloat();
    m_qMapChScaling.insert(FIFFV_MISC_CH, val);
}

//=============================================================================================================

void ScalingView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ScalingView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ScalingView::processScalingChange()
{
    emit scalingChanged(m_qMapChScaling);
    saveSettings();
}

//=============================================================================================================

void ScalingView::MAGSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange || m_bManagingLinkMagToGrad)
    {
        m_qMapSlider[FIFF_UNIT_T]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFF_UNIT_T, value * m_fScaleMAG);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
    linkMagToGrad();
}

//=============================================================================================================

void ScalingView::linkMagToGrad()
{
    m_bManagingLinkMagToGrad = true;
    if(m_bIsShiftKeyPressed)
    {
        m_dMAGtoGRADSpinboxConverter = m_qMapSpinBox[FIFF_UNIT_T_M]->value() / m_qMapSpinBox[FIFF_UNIT_T]->value();
    }
    m_qMapSpinBox[FIFF_UNIT_T_M]->setValue(m_dMAGtoGRADSpinboxConverter * m_qMapSpinBox[FIFF_UNIT_T]->value());
    m_bManagingLinkMagToGrad = false;
}

//=============================================================================================================

void ScalingView::linkGradToMag()
{
    if(!m_bManagingLinkMagToGrad)
    {
        m_bManagingLinkMagToGrad = true;
        if(m_bIsShiftKeyPressed)
        {
            m_dMAGtoGRADSpinboxConverter = m_qMapSpinBox[FIFF_UNIT_T_M]->value() / m_qMapSpinBox[FIFF_UNIT_T]->value();
        }
        m_qMapSpinBox[FIFF_UNIT_T]->setValue( m_qMapSpinBox[FIFF_UNIT_T_M]->value() / m_dMAGtoGRADSpinboxConverter);
        m_bManagingLinkMagToGrad = false;
    }
}

//=============================================================================================================

float ScalingView::sliderNonLinearMap(float minInput, float maxInput, float sensitivity, float centerPoint, float minOutput, float maxOutput, float x)
{
    float Y = atanf(sensitivity * (minInput - centerPoint));
    float K = (maxOutput - minOutput) / (atanf(sensitivity * (maxInput-centerPoint)) - Y);
    return K * atanf(sensitivity * (x-centerPoint)) - Y;
}

//=============================================================================================================

float ScalingView::sliderNonLinearUnMap(float minInput, float maxInput, float sensitivity, float centerPoint, float minOutput, float maxOutput, float x)
{
    float Y = atanf(sensitivity * (minInput - centerPoint));
    float K = (maxOutput - minOutput) / (atanf(sensitivity * (maxInput-centerPoint)) - Y);
    return (1/sensitivity) * tanf((x / K) + Y) + centerPoint;
}

//=============================================================================================================

void ScalingView::GRADSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange || m_bManagingLinkMagToGrad)
    {
        m_qMapSlider[FIFF_UNIT_T_M]->setValue(-value * 1.0);
    }
    m_qMapChScaling.insert(FIFF_UNIT_T_M, value * m_fScaleGRAD * 100.0);//*100 because data in fiff files is stored as fT/m not fT/cm
    processScalingChange();
    m_bManagingSpinBoxChange = false;
    linkGradToMag();
}

//=============================================================================================================

void ScalingView::EEGSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_EEG_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_EEG_CH, value * m_fScaleEEG);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::EOGSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_EOG_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_EOG_CH, value * m_fScaleEOG);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::EMGSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_EMG_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_EMG_CH, value * m_fScaleEMG);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::ECGSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_ECG_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_ECG_CH, value * m_fScaleECG);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::MISCSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_MISC_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_MISC_CH, value * m_fScaleMISC);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::STIMSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_qMapSlider[FIFFV_STIM_CH]->setValue(-value * 10.0);
    }
    m_qMapChScaling.insert(FIFFV_STIM_CH, value * m_fScaleSTIM);
    processScalingChange();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::MAGSliderChanged(int value)
{
    qDebug() << "slider mag : " << value;
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFF_UNIT_T]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::GRADSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFF_UNIT_T_M]->setValue(abs(value) / 1.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::EEGSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_EEG_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::EOGSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_EOG_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::EMGSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_EMG_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::ECGSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_ECG_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::MISCSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_MISC_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::STIMSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_qMapSpinBox[FIFFV_STIM_CH]->setValue(abs(value) / 10.0);
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::redrawGUI()
{
    qint32 i = 0;
    //MAG
    if(m_qMapChScaling.contains(FIFF_UNIT_T) && (m_lChannelTypesToShow.contains("mag") || m_lChannelTypesToShow.contains("all")))
    {
        QString tip("Press SHIFT to unlock link with GRADs.");
        QLabel* t_pLabelModality = new QLabel("MAG (pT)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setKeyboardTracking(false);
        t_pDoubleSpinBoxScale->setMinimum(0.001);
        t_pDoubleSpinBoxScale->setMaximum(50000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.01);
        t_pDoubleSpinBoxScale->setDecimals(3);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(m_fScaleMAG));
        t_pDoubleSpinBoxScale->setToolTip(tip);
        m_qMapSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::MAGSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-1000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(-m_qMapChScaling.value(FIFF_UNIT_T)/(m_fScaleMAG)*10);
        t_pHorizontalSlider->setToolTip(tip);
        m_qMapSlider.insert(FIFF_UNIT_T,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::MAGSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //GRAD
    if(m_qMapChScaling.contains(FIFF_UNIT_T_M) && (m_lChannelTypesToShow.contains("grad") || m_lChannelTypesToShow.contains("all")))
    {
        QString tip("Press SHIFT to unlock link with MAGs.");
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD (fT/cm)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(1);
        t_pDoubleSpinBoxScale->setMaximum(500000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(m_fScaleGRAD * 100));
        t_pDoubleSpinBoxScale->setToolTip(tip);
        m_qMapSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::GRADSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-2000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(m_fScaleGRAD * 100));
        t_pHorizontalSlider->setToolTip(tip);
        m_qMapSlider.insert(FIFF_UNIT_T_M,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::GRADSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EEG
    if(m_qMapChScaling.contains(FIFFV_EEG_CH) && (m_lChannelTypesToShow.contains("eeg") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG (uV)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(25000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(2);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(m_fScaleEEG));
        m_qMapSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::EEGSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-1000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(m_fScaleEEG)*10);
        m_qMapSlider.insert(FIFFV_EEG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::EEGSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EOG
    if(m_qMapChScaling.contains(FIFFV_EOG_CH) && (m_lChannelTypesToShow.contains("eog") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EOG (uV)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(102500e14);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(m_fScaleEOG));
        m_qMapSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::EOGSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-1000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(m_fScaleEOG)*10);
        m_qMapSlider.insert(FIFFV_EOG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::EOGSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //ECG
    if(m_qMapChScaling.contains(FIFFV_ECG_CH) && (m_lChannelTypesToShow.contains("ecg") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("ECG (uV)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(102500e14);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_ECG_CH)/(m_fScaleECG));
        m_qMapSpinBox.insert(FIFFV_ECG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::ECGSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-1000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_ECG_CH)/(m_fScaleECG)*10);
        m_qMapSlider.insert(FIFFV_ECG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::ECGSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //STIM
    if(m_qMapChScaling.contains(FIFFV_STIM_CH) && (m_lChannelTypesToShow.contains("stim") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(0.001);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.001);
        t_pDoubleSpinBoxScale->setDecimals(3);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_STIM_CH));
        m_qMapSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::STIMSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-1000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_STIM_CH)*10);
        m_qMapSlider.insert(FIFFV_STIM_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::STIMSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //MISC
    if(m_qMapChScaling.contains(FIFFV_MISC_CH) && (m_lChannelTypesToShow.contains("misc") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(10000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_MISC_CH));
        m_qMapSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::MISCSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(-10000);
        t_pHorizontalSlider->setMaximum(-1);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_MISC_CH)/10);
        m_qMapSlider.insert(FIFFV_MISC_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::MISCSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }
}

//=============================================================================================================

void ScalingView::clearView()
{

}
