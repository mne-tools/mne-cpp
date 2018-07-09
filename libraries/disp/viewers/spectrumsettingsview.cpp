//=============================================================================================================
/**
* @file     spectrumsettingsview.cpp
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
* @brief    Definition of the SpectrumSettingsView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrumsettingsview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QSlider>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpectrumSettingsView::SpectrumSettingsView(QWidget *parent)
: QWidget(parent, Qt::Window)
{
    this->setWindowTitle("Spectrum Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    QLabel *t_pLabelLower = new QLabel;
    t_pLabelLower->setText("Lower Frequency");
    m_pSliderLowerBound = new QSlider(Qt::Horizontal);
    QLabel *t_pLabelUpper = new QLabel;
    t_pLabelUpper->setText("Upper Frequency");
    m_pSliderUpperBound = new QSlider(Qt::Horizontal);

    m_pSliderUpperBound->setMinimum(0);
    m_pSliderUpperBound->setMaximum(100);

    connect(m_pSliderLowerBound, &QSlider::valueChanged,
            this, &SpectrumSettingsView::updateValue);
    connect(m_pSliderUpperBound, &QSlider::valueChanged,
            this, &SpectrumSettingsView::updateValue);

    t_pGridLayout->addWidget(t_pLabelLower,0,0);
    t_pGridLayout->addWidget(m_pSliderLowerBound,0,1);
    t_pGridLayout->addWidget(t_pLabelUpper,1,0);
    t_pGridLayout->addWidget(m_pSliderUpperBound,1,1);

    this->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void SpectrumSettingsView::updateValue(qint32 value)
{
    Q_UNUSED(value)

    if(m_pSliderLowerBound->value() > m_pSliderUpperBound->value())
        m_pSliderLowerBound->setValue(m_pSliderUpperBound->value());
    else if(m_pSliderUpperBound->value() < m_pSliderLowerBound->value())
        m_pSliderUpperBound->setValue(m_pSliderLowerBound->value());

    emit settingsChanged();
}
