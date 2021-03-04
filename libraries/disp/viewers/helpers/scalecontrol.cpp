#include "scalecontrol.h"
#include "ui_scalecontrol.h"

#include <QDebug>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGridLayout>

using namespace DISPLIB;


const static double m_dDefaultMin(0.0);
const static double m_dDefaultMax(1.0);
const static double m_dDefaultMaxSensitivityPoint(0.5);
const static double m_dDefaultSensitivity(1.0);
const static int    m_iDefaultSliderMin(1);
const static int    m_iDefaultSliderMax(1000);

//=============================================================================================================

ScaleControl::ScaleControl(const char* label)
: ScaleControl(label, nullptr, m_dDefaultMin, m_dDefaultMax)
{ }

//=============================================================================================================

ScaleControl::ScaleControl(const char* label, QWidget* parent)
: ScaleControl(label, parent, m_dDefaultMin, m_dDefaultMax)
{ }

//=============================================================================================================

ScaleControl::ScaleControl(const char* label, QWidget* parent, double min, double max)
: QWidget(parent)
, m_pUi(new Ui::ScaleControlWidget)
, m_bManagingSpinBoxChange(false)
, m_bManagingSliderChange(false)
, m_fSensitivity(m_dDefaultSensitivity)
, m_fMaxSensitivityPoint(m_dDefaultMaxSensitivityPoint)
, m_fMapYconstant(0.0)
, m_fMapKconstant(0.0)
, m_bSliderInverted(false)
{
    m_pUi->setupUi(this);
    initLabel(label);
    initSpinBox();
    initSlider();
    updateNLMapConstants();
    setRange(min, max);
}

//=============================================================================================================

void ScaleControl::initLabel(const char* label)
{
    m_pUi->label->setText(label);
}

//=============================================================================================================

void ScaleControl::initSpinBox()
{
    m_pUi->spinBox->setKeyboardTracking(false);
    m_pUi->spinBox->setSingleStep(0.01);
    m_pUi->spinBox->setDecimals(2);
    m_pUi->spinBox->setPrefix("+/- ");

    connect(m_pUi->spinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleControl::spinBoxChanged);
}

//=============================================================================================================

void ScaleControl::initSlider()
{
    m_pUi->slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_pUi->slider->setSingleStep(1);
    m_pUi->slider->setPageStep(1);
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

void ScaleControl::setValue(double value)
{
    m_pUi->spinBox->setValue(value);
}

//=============================================================================================================

void ScaleControl::setMaxSensitivityPoint(double s)
{
    m_fMaxSensitivityPoint = s;
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::setSensitivity(double s)
{
    m_fSensitivity = s;
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::setRange(double min, double max)
{
    m_pUi->spinBox->setRange(min, max);
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::invertSlider(bool inverted)
{
    m_bSliderInverted = inverted;
}

//=============================================================================================================

void ScaleControl::setSliderRange(int min, int max)
{
    if( (min < 0)   ||
        (max < 0)   ||
        (min > max) )
    {
        qDebug() << "Error. Invalid slider range";
        return;
    }
    m_pUi->slider->setRange(min, max);
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::updateNLMapConstants()
{
    m_fMapYconstant = atanf(m_fSensitivity * (m_pUi->spinBox->minimum() - m_fMaxSensitivityPoint));
    m_fMapKconstant = (m_pUi->slider->maximum() - m_pUi->slider->minimum()) / (atanf(m_fSensitivity * (m_pUi->spinBox->maximum() - m_fMaxSensitivityPoint)) - m_fMapYconstant);
}

//=============================================================================================================

void ScaleControl::spinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        qDebug() << "spinBox changed - Value: " << value << " - setting slider to: " << mapSpinBoxToSlider(value);
        m_pUi->slider->setValue(mapSpinBoxToSlider(value));
    }
    m_bManagingSpinBoxChange = false;
    emit valueChanged(value);
}

//=============================================================================================================

void ScaleControl::sliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        qDebug() << "slider Changed - Value: " << value << " - setting spinbox to: " << mapSliderToSpinBox(value);
        m_pUi->spinBox->setValue(mapSliderToSpinBox(value));
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

int ScaleControl::mapSpinBoxToSlider(double value)
{
    float map = m_fMapKconstant * (atanf(m_fSensitivity * (static_cast<float>(value) - m_fMaxSensitivityPoint)) - m_fMapYconstant);
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

double ScaleControl::mapSliderToSpinBox(int value)
{
    int valueCorrected = m_bSliderInverted? m_pUi->slider->maximum()- value : value;
    float map = (1/m_fSensitivity) * tanf((static_cast<float>(valueCorrected) / m_fMapKconstant) + m_fMapYconstant) + m_fMaxSensitivityPoint;
    return static_cast<double>(map);
}

//=============================================================================================================

