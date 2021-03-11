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

const static float m_fDefaultMagGradLink = m_fScaleMAG / m_fScaleGRAD;

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

ScalingView::ScalingView(const QString& sSettingsPath,
                         QWidget *parent,
                         Qt::WindowFlags f,
                         const QStringList& lChannelsToShow)
: AbstractView(parent, f)
, m_lChannelTypesToShow(lChannelsToShow)
, m_pUi(new Ui::ScalingViewWidget)
, m_bIsShiftKeyPressed(false)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Scaling");
    this->setMinimumWidth(330);
    //this->setMaximumWidth(330);

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
    if(event->key() == 16777248)
    {
        m_bIsShiftKeyPressed = false;
    }
}

//=============================================================================================================

void ScalingView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == 16777248)
    {
        m_bIsShiftKeyPressed = true;
    }
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

void ScalingView::emitScalingChangedAndSaveSettings()
{
    emit scalingChanged(m_qMapChScaling);
    saveSettings();
}

//=============================================================================================================

static double sensibility(7.);
static int sliderMax(1000);
static double spinboxMin(0.);
static double spinboxMax(2.);
static double spinboxStep(0.005);
static double spinboxDefault(1.);

static inline double atanMap(double d)
{
    return atan(sensibility * (d - spinboxDefault));
}

static double mapSliderToSpinbox(int s)
{
    return spinboxDefault + tan((s * (atanMap(spinboxMax) - atanMap(spinboxMin) ) / sliderMax) + atanMap(spinboxMin)) / sensibility;
}

static int mapSpinBoxToSlider(double d)
{
    return ( atanMap(d) - atanMap(spinboxMin) ) * sliderMax / (atanMap(spinboxMax) - atanMap(spinboxMin) );
}

void ScalingView::MAGScaleSpinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    double scale = mapSpinBoxToSlider(value);
    qDebug() << "spinbox value: " << value << " -> scale: " << scale;
    if(!m_bManagingSliderChange)
    {
        m_qMapScalingSlider[FIFF_UNIT_T]->setValue(scale);
    }
    m_qMapChScaling.insert(FIFF_UNIT_T, (-value+ spinboxMax + spinboxStep) * m_fScaleMAG);
    emitScalingChangedAndSaveSettings();
    m_bManagingSpinBoxChange = false;
}

//=============================================================================================================

void ScalingView::GRADScaleSpinBoxChanged(double value)
{
   m_qMapScalingSlider[FIFF_UNIT_T_M]->setValue(value * 500.0 / 400.0);
    m_qMapChScaling.insert(FIFF_UNIT_T_M, (-0.35*value + 300) * m_fScaleGRAD * 100.0);//*100 because data in fiff files is stored as fT/m not fT/cm
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EEGScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_EEG_CH]->setValue(value * 500.0 / 30.0);
    m_qMapChScaling.insert(FIFFV_EEG_CH, (-value + 60) * m_fScaleEEG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EOGScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_EOG_CH]->setValue(value * 10.0);
    m_qMapChScaling.insert(FIFFV_EEG_CH, value * m_fScaleEOG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EMGScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_EMG_CH]->setValue(value * 10.0);
    m_qMapChScaling.insert(FIFFV_EMG_CH, value * m_fScaleEOG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::ECGScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_ECG_CH]->setValue(value * 10.0);
    m_qMapChScaling.insert(FIFFV_ECG_CH, value * m_fScaleEOG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::MISCScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_MISC_CH]->setValue(value * 10.0);
    m_qMapChScaling.insert(FIFFV_MISC_CH, value * m_fScaleEOG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::STIMScaleSpinBoxChanged(double value)
{
    m_qMapScalingSlider[FIFFV_STIM_CH]->setValue(value * 10.0);
    m_qMapChScaling.insert(FIFFV_STIM_CH, value * m_fScaleEOG);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::MAGScaleSliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
//        int value = m_qMapScalingSlider[FIFF_UNIT_T]->value();
        //=(TAN(PI() *B1 / 1000 - PI() /2)) / 10 + 1
        double m(tan(3.1416 * value / 1001.0 - 3.1416 / 2.0) / 5.0 + 1.0);
        qDebug() << "scale value: " << value << " -> spinbox: " << m;

        m_qMapScalingDoubleSpinBox[FIFF_UNIT_T]->setValue(m);
    //    m_qMapScalingDoubleSpinBox[FIFF_UNIT_T]->setValue(value / 500.0);
        emitScalingChangedAndSaveSettings();
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

void ScalingView::GRADScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFF_UNIT_T_M]->setValue( mapSliderToSpinbox(value));
//    m_qMapScalingDoubleSpinBox[FIFF_UNIT_T_M]->setValue(value * 400.0 / 500.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EEGScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_EEG_CH]->setValue(value * 10.0 / 500.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EOGScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_EOG_CH]->setValue(value / 10.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::EMGScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_EMG_CH]->setValue(value / 10.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::ECGScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_ECG_CH]->setValue(value / 10.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::MISCScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_MISC_CH]->setValue(value / 10.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::STIMScaleSliderChanged(int value)
{
    m_qMapScalingDoubleSpinBox[FIFFV_STIM_CH]->setValue(value / 10.0);
    emitScalingChangedAndSaveSettings();
}

//=============================================================================================================

void ScalingView::redrawGUI()
{
    qint32 i = 0;
    //MAG
    if(m_qMapChScaling.contains(FIFF_UNIT_T) && (m_lChannelTypesToShow.contains("mag") || m_lChannelTypesToShow.contains("all")))
    {
        QLabel* t_pLabelModality = new QLabel("MAG (pT)");
        m_pUi->m_formLayout_Scaling->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pDoubleSpinBoxScale->setKeyboardTracking(false);
        t_pDoubleSpinBoxScale->setMinimum(0.0001);
        t_pDoubleSpinBoxScale->setMaximum(2);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.0001);
        t_pDoubleSpinBoxScale->setDecimals(3);
        //t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(m_fScaleMAG));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::MAGScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        //t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(m_fScaleMAG)*10);
        m_qMapScalingSlider.insert(FIFF_UNIT_T,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::MAGScaleSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //GRAD
    if(m_qMapChScaling.contains(FIFF_UNIT_T_M) && (m_lChannelTypesToShow.contains("grad") || m_lChannelTypesToShow.contains("all")))
    {
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
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::GRADScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(m_fScaleGRAD * 100));
        m_qMapScalingSlider.insert(FIFF_UNIT_T_M,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::GRADScaleSliderChanged);
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
        t_pDoubleSpinBoxScale->setMinimum(0.01);
        t_pDoubleSpinBoxScale->setMaximum(25000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(2);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(m_fScaleEEG));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::EEGScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(m_fScaleEEG)*10);
        m_qMapScalingSlider.insert(FIFFV_EEG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::EEGScaleSliderChanged);
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
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::EOGScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(m_fScaleEOG)*10);
        m_qMapScalingSlider.insert(FIFFV_EOG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::EOGScaleSliderChanged);
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
        m_qMapScalingDoubleSpinBox.insert(FIFFV_ECG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::ECGScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_ECG_CH)/(m_fScaleECG)*10);
        m_qMapScalingSlider.insert(FIFFV_ECG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::ECGScaleSliderChanged);
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
        m_qMapScalingDoubleSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::STIMScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_STIM_CH)*10);
        m_qMapScalingSlider.insert(FIFFV_STIM_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::STIMScaleSliderChanged);
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
        m_qMapScalingDoubleSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::MISCScaleSpinBoxChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(10000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_MISC_CH)/10);
        m_qMapScalingSlider.insert(FIFFV_MISC_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::MISCScaleSliderChanged);
        m_pUi->m_formLayout_Scaling->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }
}

//=============================================================================================================

//void ScalingView::onUpdateSpinBoxScaling(double value)
//{
//    Q_UNUSED(value)

//    QMap<qint32, QDoubleSpinBox*>::iterator it;
//    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
//    {
//        double scaleValue = 0;

//        switch(it.key())
//        {
//            case FIFF_UNIT_T:
//                //MAG
//                scaleValue = m_fScaleMAG;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFF_UNIT_T_M:
//                //GRAD
//                scaleValue = m_fScaleGRAD * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*1);
//                break;
//            case FIFFV_EEG_CH:
//                //EEG
//                scaleValue = m_fScaleEEG;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFFV_EOG_CH:
//                //EOG
//                scaleValue = m_fScaleEOG;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFFV_EMG_CH:
//                //EMG
//                scaleValue = m_fScaleEMG;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFFV_ECG_CH:
//                //ECG
//                scaleValue = m_fScaleECG;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFFV_MISC_CH:
//                //MISC
//                scaleValue = m_fScaleMISC;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            case FIFFV_STIM_CH:
//                //STIM
//                scaleValue = m_fScaleSTIM;
//                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
//                break;
//            default:
//                scaleValue = 1.0;
//        }

//        //if(m_qMapScalingSlider[it.key()]->maximum()<it.value()->value()*10)
//            m_qMapChScaling.insert(it.key(), it.value()->value() * scaleValue);
////        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
//    }

//    emit scalingChanged(m_qMapChScaling);

//    saveSettings();
//}

//=============================================================================================================

//void ScalingView::onUpdateSliderScaling(int value)
//{
//    Q_UNUSED(value)

//    QMap<qint32, QDoubleSpinBox*>::iterator it;
//    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
//    {
//        double scaleValue = 0;

//        switch(it.key())
//        {
//            case FIFF_UNIT_T:
//                //MAG
//                scaleValue = m_fScaleMAG;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFF_UNIT_T_M:
//                //GRAD
//                scaleValue = m_fScaleGRAD * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/1));
//                break;
//            case FIFFV_EEG_CH:
//                //EEG
//                scaleValue = m_fScaleEEG;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFFV_EOG_CH:
//                //EOG
//                scaleValue = m_fScaleEOG;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFFV_EMG_CH:
//                //EMG
//                scaleValue = m_fScaleEMG;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFFV_ECG_CH:
//                //ECG
//                scaleValue = m_fScaleECG;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFFV_MISC_CH:
//                //MISC
//                scaleValue = m_fScaleMISC;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            case FIFFV_STIM_CH:
//                //STIM
//                scaleValue = m_fScaleMISC;
//                it.value()->setValue(static_cast<double>(m_qMapScalingSlider[it.key()]->value()/10));
//                break;
//            default:
//                scaleValue = 1.0;
//        }

////        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
//    }

//    emit scalingChanged(m_qMapChScaling);

//    saveSettings();
//}

//=============================================================================================================

void ScalingView::clearView()
{

}
