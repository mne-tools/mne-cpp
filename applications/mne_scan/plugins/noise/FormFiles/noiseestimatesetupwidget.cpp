//=============================================================================================================
/**
 * @file     noiseestimatesetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the NoiseEstimateSetupWidget class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noiseestimatesetupwidget.h"
#include "../noiseestimate.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NOISEESTIMATEPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseEstimateSetupWidget::NoiseEstimateSetupWidget(NoiseEstimate* toolbox, QWidget *parent)
: QWidget(parent)
, m_pNoiseEstimate(toolbox)
{
    ui.setupUi(this);

    init();

    connect(ui.m_qComboBoxnFFT, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &NoiseEstimateSetupWidget::chgnFFT);
    connect(ui.m_qSpinDataLen, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NoiseEstimateSetupWidget::chgDataLen);
    connect(ui.m_cb_logscale, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked), this, &NoiseEstimateSetupWidget::chgXAxisType);

}


//*************************************************************************************************************

NoiseEstimateSetupWidget::~NoiseEstimateSetupWidget()
{

}


//*************************************************************************************************************

void NoiseEstimateSetupWidget::init()
{
    ui.m_qComboBoxnFFT->insertItem(0, "512");
    ui.m_qComboBoxnFFT->insertItem(1, "1024");
    ui.m_qComboBoxnFFT->insertItem(2, "2048");
    ui.m_qComboBoxnFFT->insertItem(3, "4096");
    ui.m_qComboBoxnFFT->insertItem(4, "8192");
    ui.m_qComboBoxnFFT->insertItem(5, "16384");

    qint32 i = 0;
    for(i = 0; i < ui.m_qComboBoxnFFT->count(); ++i)
        if(ui.m_qComboBoxnFFT->itemText(i).toInt() == m_pNoiseEstimate->m_iFFTlength)
            break;

    ui.m_qComboBoxnFFT->setCurrentIndex(i);


    //set up the data length for spectrum calculation
    ui.m_qSpinDataLen->setValue(6);

    //set up the default x-scale type of spectrum
    if ( m_pNoiseEstimate->m_x_scale_type == 0)
        ui.m_cb_logscale->setChecked(false);
    else
        ui.m_cb_logscale->setChecked(true);

}


//*************************************************************************************************************

void NoiseEstimateSetupWidget::chgnFFT(int idx)
{
    //qDebug() << "ui.m_qComboBoxnFFT->itemData(idx).toInt();" << ui.m_qComboBoxnFFT->itemText(idx).toInt();
    m_pNoiseEstimate->m_iFFTlength = ui.m_qComboBoxnFFT->itemText(idx).toInt();
}

//*************************************************************************************************************

void NoiseEstimateSetupWidget::chgDataLen(int idx)
{
    m_pNoiseEstimate->m_DataLen = idx;
    //qDebug() << "m_pNoiseEstimate->m_DataLen" <<m_pNoiseEstimate->m_DataLen;
}

//*************************************************************************************************************

void NoiseEstimateSetupWidget::chgXAxisType()
{

    //qDebug()<<"Check state1";
    bool checkstatus = ui.m_cb_logscale->isChecked();
    if ( checkstatus)
        m_pNoiseEstimate->m_x_scale_type = 1;
    else
        m_pNoiseEstimate->m_x_scale_type = 0;

    qDebug() << "setup widget scale type" << m_pNoiseEstimate->m_x_scale_type ;
}

