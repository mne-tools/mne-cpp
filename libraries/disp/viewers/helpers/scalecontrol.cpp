#include "scalecontrol.h"
#include "ui_scalecontrol.h"

#include <QDebug>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGridLayout>

using namespace DISPLIB;

//=============================================================================================================

ScaleControl::ScaleControl(const char* label, QWidget* parent)
: QWidget(parent)
, m_pUi(new Ui::ScaleControlWidget)
, m_bManagingSpinBoxChange(false)
, m_bManagingSliderChange(false)
, m_fSensitivity(3.0)
, m_fMaxSensitivityPoint(0.0)
, m_fMapYconstant(0.0)
, m_fMapKconstant(0.0)
, m_bSliderInverted(false)
{
    initLabel(label);
    initSpinBox();
    initSlider();
    updateNLMapConstants();
}

//=============================================================================================================

ScaleControl::ScaleControl(const char* label, double min, double max)
: ScaleControl(label)
{
    m_pSpinBox->setRange(min, max);
}

//=============================================================================================================

void ScaleControl::setValue(double value)
{
    m_pSpinBox->setValue(value);
}

//=============================================================================================================

void ScaleControl::setMaxSensitivityValue(double s)
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
    m_pSpinBox->setRange(min, max);
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::setInverted(bool inverted)
{
    if(inverted)
    {
        setSliderRange(-1000,-1);
        m_bSliderInverted = true;
    } else
    {
        setSliderRange(1,1000);
        m_bSliderInverted = false;
    }
}

//=============================================================================================================

void ScaleControl::initLabel(const char* label)
{
    m_pLabel = new QLabel(label);
}

//=============================================================================================================

void ScaleControl::initSpinBox()
{
    m_pSpinBox = new QDoubleSpinBox;
    m_pSpinBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_pSpinBox->setKeyboardTracking(false);
    m_pSpinBox->setMaximumWidth(100);
    m_pSpinBox->setSingleStep(0.01);
    m_pSpinBox->setDecimals(2);
    m_pSpinBox->setPrefix("+/- ");

    connect(m_pSpinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleControl::spinBoxChanged);
}

//=============================================================================================================

void ScaleControl::initSlider()
{
    m_pSlider = new QSlider(Qt::Horizontal);
    m_pSlider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_pSlider->setSingleStep(1);
    m_pSlider->setPageStep(1);
    setSliderRange(1,1000);

    connect(m_pSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
            this,&ScaleControl::sliderChanged);
}

//=============================================================================================================

void ScaleControl::setSliderRange(int min, int max)
{
    if( (min >= max)            ||
         min == 0               ||
         max == 0               ||
        (min < 0 && max > 0)    )
    {
        qDebug() << "Error. Invalid slider range";
        return;
    }
    m_pSlider->setRange(min, max);
    updateNLMapConstants();
}

//=============================================================================================================

void ScaleControl::updateNLMapConstants()
{
    m_fMapYconstant = atanf(m_fSensitivity * (m_pSpinBox->minimum() - m_fMaxSensitivityPoint));
    m_fMapKconstant = (m_pSlider->maximum() - m_pSlider->minimum()) / (atanf(m_fSensitivity * (m_pSpinBox->maximum() - m_fMaxSensitivityPoint)) - m_fMapYconstant);
}

//=============================================================================================================

void ScaleControl::spinBoxChanged(double value)
{
    m_bManagingSpinBoxChange = true;
    if(!m_bManagingSliderChange)
    {
        m_pSlider->setValue(mapSpinBoxToSlider(value));
    }
    m_bManagingSpinBoxChange = false;
    emit valueChanged(value);
}

void ScaleControl::sliderChanged(int value)
{
    m_bManagingSliderChange = true;
    if(!m_bManagingSpinBoxChange)
    {
        m_pSpinBox->setValue(mapSliderToSpinBox(value));
    }
    m_bManagingSliderChange = false;
}

//=============================================================================================================

int ScaleControl::mapSpinBoxToSlider(double value)
{
    double map = m_fMapKconstant * (atanf(m_fSensitivity * (static_cast<float>(value) - m_fMaxSensitivityPoint)) - m_fMapYconstant);
    int out;
    if(m_bSliderInverted)
    {
        out = static_cast<int> (m_pSlider->maximum() - map);
    } else {
        out = static_cast<int> (map);
    }
    return out;

}

//=============================================================================================================

double ScaleControl::mapSliderToSpinBox(int value)
{
    float map = (1/m_fSensitivity) * tanf((static_cast<float>(value) / m_fMapKconstant) + m_fMapYconstant) + m_fMaxSensitivityPoint;
    double out;
    if(m_bSliderInverted)
    {
        out = static_cast<double>(m_pSpinBox->maximum() - map);
    } else {
        out = static_cast<double>(map);
    }
    return out;
}

//=============================================================================================================

//void ScaleControl::setToolTip(const QString &s)
//{
//    m_pLabel->setToolTip(s);
//    m_pSlider->setToolTip(s);
//    m_pSpinBox->setToolTip(s);
//}
