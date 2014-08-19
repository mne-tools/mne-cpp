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
#include <QDoubleValidator>

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
        t_pLabelModality->setText("MAG");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFF_UNIT_T]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }

    //GRAD
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFF_UNIT_T_M))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFF_UNIT_T_M]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }

    //EEG
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_EEG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFFV_EEG_CH]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }

    //EOG
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_EOG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EOG");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFFV_EOG_CH]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }

    //STIM
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_STIM_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFFV_STIM_CH]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }

    //MISC
    if(m_pRTMSAW->m_qMapChScaling.contains(FIFFV_MISC_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleValidator* t_pDoubleValidator = new QDoubleValidator(10e-11,1,16,this);
        QLineEdit* t_pLineEditScale = new QLineEdit;
        t_pLineEditScale->setMaximumWidth(100);
        t_pLineEditScale->setValidator(t_pDoubleValidator);
        t_pLineEditScale->setText(QString("%1").arg(m_pRTMSAW->m_qMapChScaling[FIFFV_MISC_CH]));
        m_qListModalityLineEdit << t_pLineEditScale;
        connect(t_pLineEditScale,&QLineEdit::textEdited,this,&RealTimeMultiSampleArrayScalingWidget::updateLineEdit);
        t_pGridLayout->addWidget(t_pLineEditScale,i,2,1,1);
        ++i;
    }


    this->setLayout(t_pGridLayout);
}



//*************************************************************************************************************

void RealTimeMultiSampleArrayScalingWidget::updateLineEdit(const QString & text)
{
    Q_UNUSED(text)

//    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
//        m_pRealTimeEvokedWidget->m_qListModalities[i].m_fNorm = (float)m_qListModalityLineEdit[i]->text().toDouble();

    emit scalingChanged();
}
