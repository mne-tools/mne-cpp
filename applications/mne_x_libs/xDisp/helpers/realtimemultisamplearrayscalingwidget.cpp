//=============================================================================================================
/**
* @file     realtimemultisamplearrayscalingwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the RealTimeMultiSampleArrayScalingWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearrayscalingwidget.h"
#include "../realtimemultisamplearraywidget.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QGridLayout>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayScalingWidget::RealTimeMultiSampleArrayScalingWidget(RealTimeMultiSampleArrayWidget *toolbox)
: m_pRTMSAW(toolbox)
{
    this->setWindowTitle("Covariance Modality Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    qint32 i = 0;
    //MAG
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFF_UNIT_T))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MAG (fT/cm)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(2000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFF_UNIT_T]/(1e-12));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }

    //GRAD
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFF_UNIT_T_M))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD (pt)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(2000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(20.0);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFF_UNIT_T_M]/(1e-15 * 100));   //*100 because data in fiff files is stored as fT/m not fT/cm
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }

    //EEG
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_EEG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(2000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1.0);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFFV_EEG_CH]/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }

    //EOG
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_EOG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EOG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(10000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(10.0);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFFV_EOG_CH]/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }

    //STIM
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_STIM_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1.0);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFFV_STIM_CH]);
        m_qMapScalingDoubleSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }

    //MISC
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_MISC_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(10e-11);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1.0);
        t_pDoubleSpinBoxScale->setValue(m_pRTMSAW->m_qMapChScaling[FIFFV_MISC_CH]);
        m_qMapScalingDoubleSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i,2,1,1);
        ++i;
    }


    this->setLayout(t_pGridLayout);
}



//*************************************************************************************************************

void RealTimeMultiSampleArrayScalingWidget::updateDoubleSpinBox(const double val)
{
    Q_UNUSED(val)

    QMap<qint32, QDoubleSpinBox*>::iterator it;
    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
    {
        double scaleValue = 0;

        switch(it.key())
        {
            case FIFF_UNIT_T:
                //MAG
                scaleValue = 1e-12;
                break;
            case FIFF_UNIT_T_M:
                //GRAD
                scaleValue = 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
                break;
            case FIFFV_EEG_CH:
                //EEG
                scaleValue = 1e-06;
                break;
            case FIFFV_EOG_CH:
                //EOG
                scaleValue = 1e-06;
                break;
            case FIFFV_EMG_CH:
                //EMG
                scaleValue = 1e-03;
                break;
            case FIFFV_ECG_CH:
                //ECG
                scaleValue = 1e-03;
                break;
            case FIFFV_MISC_CH:
                //MISC
                scaleValue = 1;
                break;
            case FIFFV_STIM_CH:
                //STIM
                scaleValue = 1;
                break;
            default:
                scaleValue = 1.0;
        }

        m_pRTMSAW->m_qMapChScaling[it.key()] = it.value()->value() * scaleValue;

//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }

    emit scalingChanged();
}
