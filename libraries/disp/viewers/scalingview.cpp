//=============================================================================================================
/**
* @file     scalingview.cpp
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
* @brief    Definition of the ScalingView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scalingview.h"

#include <fiff/fiff_constants.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QSlider>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScalingView::ScalingView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
{
    this->setWindowTitle("Scaling");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void ScalingView::init(const QMap<qint32,float>& qMapChScaling)
{
    m_qMapChScaling = qMapChScaling;

    QGridLayout* t_pGridLayout = new QGridLayout;

    qint32 i = 0;

    //MAG
    if(m_qMapChScaling.contains(FIFF_UNIT_T))
    {
        QLabel* t_pLabelModality = new QLabel("MAG (pT)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(50000);
        t_pDoubleSpinBoxScale->setMaximumWidth(500);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(1e-12));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(1e-12)*10);
        m_qMapScalingSlider.insert(FIFF_UNIT_T,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //GRAD
    if(m_qMapChScaling.contains(FIFF_UNIT_T_M))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD (fT/cm)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(1);
        t_pDoubleSpinBoxScale->setMaximum(500000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(1e-15 * 100));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(10);
        t_pHorizontalSlider->setPageStep(10);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(1e-15*100));
        m_qMapScalingSlider.insert(FIFF_UNIT_T_M,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EEG
    if(m_qMapChScaling.contains(FIFFV_EEG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(25000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EEG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EOG
    if(m_qMapChScaling.contains(FIFFV_EOG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EOG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(102500e14);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EOG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //STIM
    if(m_qMapChScaling.contains(FIFFV_STIM_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.001);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.001);
        t_pDoubleSpinBoxScale->setDecimals(3);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_STIM_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_STIM_CH)*10);
        m_qMapScalingSlider.insert(FIFFV_STIM_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);


        i+=2;
    }

    //MISC
    if(m_qMapChScaling.contains(FIFFV_MISC_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(10000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_MISC_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&ScalingView::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(10000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_MISC_CH)/10);
        m_qMapScalingSlider.insert(FIFFV_MISC_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&ScalingView::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    this->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void ScalingView::onUpdateSpinBoxScaling(double value)
{
    Q_UNUSED(value)

    QMap<qint32, QDoubleSpinBox*>::iterator it;
    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
    {
        double scaleValue = 0;

        switch(it.key())
        {
            case FIFF_UNIT_T:
                //MAG
                scaleValue = 1e-12;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFF_UNIT_T_M:
                //GRAD
                scaleValue = 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*1);
                break;
            case FIFFV_EEG_CH:
                //EEG
                scaleValue = 1e-06;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_EOG_CH:
                //EOG
                scaleValue = 1e-06;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_EMG_CH:
                //EMG
                scaleValue = 1e-03;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_ECG_CH:
                //ECG
                scaleValue = 1e-03;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_MISC_CH:
                //MISC
                scaleValue = 1;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_STIM_CH:
                //STIM
                scaleValue = 1;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            default:
                scaleValue = 1.0;
        }

        //if(m_qMapScalingSlider[it.key()]->maximum()<it.value()->value()*10)
            m_qMapChScaling.insert(it.key(), it.value()->value() * scaleValue);
//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }
}


//*************************************************************************************************************

void ScalingView::onUpdateSliderScaling(int value)
{
    Q_UNUSED(value)

    QMap<qint32, QDoubleSpinBox*>::iterator it;
    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
    {
        double scaleValue = 0;

        switch(it.key())
        {
            case FIFF_UNIT_T:
                //MAG
                scaleValue = 1e-12;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFF_UNIT_T_M:
                //GRAD
                scaleValue = 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/1);
                break;
            case FIFFV_EEG_CH:
                //EEG
                scaleValue = 1e-06;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_EOG_CH:
                //EOG
                scaleValue = 1e-06;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_EMG_CH:
                //EMG
                scaleValue = 1e-03;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_ECG_CH:
                //ECG
                scaleValue = 1e-03;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_MISC_CH:
                //MISC
                scaleValue = 1;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_STIM_CH:
                //STIM
                scaleValue = 1;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            default:
                scaleValue = 1.0;
        }

//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }

    emit scalingChanged(m_qMapChScaling);
}
