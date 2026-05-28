//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file scalecontrol.cpp
 * @since 2022
 * @date  October 2022
 * @brief Implementation of the ScaleControl slider + spinbox composite.
 */

#include "scalecontrol.h"
#include "ui_scalecontrol.h"
#include <math.h>

#include <QInputDialog>

using namespace DISPLIB;

const static float  m_dDefaultMin(0.0f);
const static float  m_dDefaultMax(1.0f);
const static float  m_dDefaultMaxSensitivityPoint((m_dDefaultMin+m_dDefaultMax)/2.0f);
const static float  m_dDefaultSensitivity(0.3f);
const static int    m_iDefaultSliderMin(1);
const static int    m_iDefaultSliderMax(1000);
const static int    m_iDefaultSliderStep(1);
const static int    m_iDefaultSliderPageStep(10);

//=============================================================================================================

ScaleControl::ScaleControl(const char* label)
: ScaleControl(label, nullptr, m_dDefaultMin, m_dDefaultMax)
{ }

//=============================================================================================================

ScaleControl::ScaleControl(const char* label,
                           QWidget* parent)
: ScaleControl(label, parent, m_dDefaultMin, m_dDefaultMax)
{ }

//=============================================================================================================

ScaleControl::ScaleControl(const char* label,
                           QWidget* parent,
                           double min,
                           double max)
: QWidget(parent)
, m_pUi(new Ui::ScaleControlWidget)
, m_bManagingSpinBoxChange(false)
, m_bManagingSliderChange(false)
, m_fSensitivity(m_dDefaultSensitivity)
, m_fMaxSensitivityPoint(m_dDefaultMaxSensitivityPoint)
, m_fMapYconstant(0.0f)
, m_fMapKconstant(0.0f)
, m_bSliderInverted(false)
{
    m_pUi->setupUi(this);
    initLabel(label);
    initSpinBox();
    initSlider();
    initMenu();
    updateNLMapConstants();
    setRange(min, max);
}

//=============================================================================================================

void ScaleControl::initLabel(const char* charTextLabel)
{
    m_pUi->label->setText(charTextLabel);
}

//=============================================================================================================

void ScaleControl::initSpinBox()
{
    m_pUi->spinBox->setKeyboardTracking(false);
    m_pUi->spinBox->setPrefix("+/- ");

    connect(m_pUi->spinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleControl::spinBoxChanged);
}

//=============================================================================================================

void ScaleControl::initSlider()
{
    m_pUi->slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_pUi->slider->setSingleStep(m_iDefaultSliderStep);
    m_pUi->slider->setPageStep(m_iDefaultSliderPageStep);
    setSliderRange(m_iDefaultSliderMin, m_iDefaultSliderMax);

    connect(m_pUi->slider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
            this,&ScaleControl::sliderChanged);
}

//=============================================================================================================

Ui::ScaleControlWidget* ScaleControl::getUI()
{
    return m_pUi;
}

//=============================================================================================================

void ScaleControl::setValue(double dScale)
{
    m_pUi->spinBox->setValue(dScale);
}

//=============================================================================================================

void ScaleControl::setMaxSensitivityPoint(double s)
{
    if(m_pUi->spinBox->minimum() < s && s < m_pUi->spinBox->maximum())
    {
        m_fMaxSensitivityPoint = s;
        updateNLMapConstants();
    }
}

//=============================================================================================================

void ScaleControl::setSensitivity(double s)
{
    if(0.0 < s && s < 1.0)
    {
        m_fSensitivity = s;
        updateNLMapConstants();
    }
}

//=============================================================================================================

void ScaleControl::setRange(double min,
                            double max)
{
    m_pUi->spinBox->setRange(min, max);
    if(m_fMaxSensitivityPoint < m_pUi->spinBox->minimum() || m_pUi->spinBox->maximum() < m_fMaxSensitivityPoint)
    {
        m_fMaxSensitivityPoint = (m_pUi->spinBox->minimum() + m_pUi->spinBox->maximum())/2;
    }
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::invertSlider(bool inverted)
{
    m_bSliderInverted = inverted;
}

//=============================================================================================================

void ScaleControl::setSliderRange(int min,
                                  int max)
{
    if( (0 < min) && (min < max) )
    {
        m_pUi->slider->setRange(min, max);
        updateNLMapConstants();
    }
}

//=============================================================================================================

void ScaleControl::setDecimals(int d)
{
    m_pUi->spinBox->setDecimals(d);
}

//=============================================================================================================

void ScaleControl::spinBoxChanged(double dScale)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_pUi->slider->setValue(mapSpinBoxToSlider(dScale));
    }
    m_bManagingSpinBoxChange = false;
    emit valueChanged(dScale);
}

//=============================================================================================================

void ScaleControl::sliderChanged(int dScale)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_pUi->spinBox->setValue(mapSliderToSpinBox(dScale));
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

inline float ScaleControl::weightedSensitivity(float s)
{
    return s * s * s * 100 / m_pUi->spinBox->maximum();
}

//=============================================================================================================

void ScaleControl::updateNLMapConstants()
{
    m_fMapYconstant = atanf(weightedSensitivity(m_fSensitivity) * (m_pUi->spinBox->minimum() - m_fMaxSensitivityPoint));
    m_fMapKconstant = (m_pUi->slider->maximum() - m_pUi->slider->minimum()) / (atanf(weightedSensitivity(m_fSensitivity) * (m_pUi->spinBox->maximum() - m_fMaxSensitivityPoint)) - m_fMapYconstant);
}

//=============================================================================================================

int ScaleControl::mapSpinBoxToSlider(double dScale)
{
    float map = m_fMapKconstant * (atanf(weightedSensitivity(m_fSensitivity) * (static_cast<float>(dScale) - m_fMaxSensitivityPoint)) - m_fMapYconstant);
    int out;
    if(m_bSliderInverted)
    {
        out = m_pUi->slider->maximum() - static_cast<int>(roundf(map));
    } else {
        out = static_cast<int> (roundf(map));
    }
    return out;
}

//=============================================================================================================

double ScaleControl::mapSliderToSpinBox(int dScale)
{
    int valueCorrected = m_bSliderInverted? m_pUi->slider->maximum() - dScale : dScale;
    float map = (1/weightedSensitivity(m_fSensitivity)) * tanf((static_cast<float>(valueCorrected) / m_fMapKconstant) + m_fMapYconstant) + m_fMaxSensitivityPoint;
    return static_cast<double>(map);
}

//=============================================================================================================

double ScaleControl::value() const
{
    return m_pUi->spinBox->value();
}

//=============================================================================================================

void ScaleControl::initMenu()
{
    m_pSettingsMenu = new QMenu(this); //Scale Control takes ownership of this QMenu and will trigger its deletion.

    QAction* minVal = new QAction("Edit minimum value");
    connect(minVal, &QAction::triggered,
            this, &ScaleControl::promptMinValueChange,
            Qt::UniqueConnection);
    m_pSettingsMenu->addAction(minVal);//takes ownership

    QAction* maxVal = new QAction("Edit maximum value");
    connect(maxVal, &QAction::triggered,
            this, &ScaleControl::promptMaxValueChange,
            Qt::UniqueConnection);
    m_pSettingsMenu->addAction(maxVal);//takes ownership

    m_pUi->toolButton->setMenu(m_pSettingsMenu);
    m_pUi->toolButton->setPopupMode(QToolButton::InstantPopup);
    m_pUi->toolButton->setArrowType(Qt::ArrowType::NoArrow);
}

//=============================================================================================================

void ScaleControl::promptMinValueChange()
{
    bool ok;
    double min = QInputDialog::getDouble(this, tr("Minimum Value"),
                                         tr("Please input a new minumum value:"),
                                         m_pUi->spinBox->minimum(), 0.01, 100000,
                                         2, &ok, Qt::WindowFlags(), 1);
    if(ok && min <= m_pUi->spinBox->maximum()){
        setRange(min, m_pUi->spinBox->maximum());
    }
}

//=============================================================================================================

void ScaleControl::promptMaxValueChange()
{
    bool ok;
    double max = QInputDialog::getDouble(this, tr("Maximum Value"),
                                         tr("Please input a new maximum value:"),
                                         m_pUi->spinBox->maximum(), 0.01, 100000,
                                         2, &ok, Qt::WindowFlags(), 1);
    if(ok && max >= m_pUi->spinBox->minimum()){
        setRange(m_pUi->spinBox->minimum(), max);
    }
}
