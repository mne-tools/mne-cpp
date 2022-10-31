//=============================================================================================================
/**
 * @file     spectrumsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrumsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpectrumSettingsView::SpectrumSettingsView(const QString& sSettingsPath,
                                           QWidget *parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Spectrum Settings");

    QGridLayout* t_pGridLayout = new QGridLayout;

    QLabel *t_pLabelLower = new QLabel;
    t_pLabelLower->setText("Lower Frequency");
    m_pSliderLowerBound = new QSlider(Qt::Horizontal);
    QLabel *t_pLabelUpper = new QLabel;
    t_pLabelUpper->setText("Upper Frequency");
    m_pSliderUpperBound = new QSlider(Qt::Horizontal);

    m_pSliderUpperBound->setMinimum(0);
    m_pSliderUpperBound->setMaximum(100);

    connect(m_pSliderLowerBound.data(), &QSlider::valueChanged,
            this, &SpectrumSettingsView::updateValue);
    connect(m_pSliderUpperBound.data(), &QSlider::valueChanged,
            this, &SpectrumSettingsView::updateValue);

    t_pGridLayout->addWidget(t_pLabelLower,0,0);
    t_pGridLayout->addWidget(m_pSliderLowerBound,0,1);
    t_pGridLayout->addWidget(t_pLabelUpper,1,0);
    t_pGridLayout->addWidget(m_pSliderUpperBound,1,1);

    this->setLayout(t_pGridLayout);

    loadSettings();
}

//=============================================================================================================

SpectrumSettingsView::~SpectrumSettingsView()
{
    saveSettings();
}

//=============================================================================================================

void SpectrumSettingsView::updateValue(qint32 value)
{
    Q_UNUSED(value)

    if(m_pSliderLowerBound->value() > m_pSliderUpperBound->value())
        m_pSliderLowerBound->setValue(m_pSliderUpperBound->value());
    else if(m_pSliderUpperBound->value() < m_pSliderLowerBound->value())
        m_pSliderUpperBound->setValue(m_pSliderLowerBound->value());

    emit settingsChanged();
}

//=============================================================================================================

void SpectrumSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpectrumSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpectrumSettingsView::setBoundaries(float fSFreq,
                                         float fLowerBound,
                                         float fUpperBound)
{
    m_pSliderLowerBound->setMinimum(0);
    m_pSliderLowerBound->setMaximum((qint32)(fSFreq/2)*1000);
    m_pSliderLowerBound->setValue((qint32)(fLowerBound*1000));

    m_pSliderUpperBound->setMinimum(0);
    m_pSliderUpperBound->setMaximum((qint32)(fSFreq/2)*1000);
    m_pSliderUpperBound->setValue((qint32)(fUpperBound*1000));
}

//=============================================================================================================

float SpectrumSettingsView::getLowerBound()
{
    return m_pSliderLowerBound->value()/1000.0f;
}

//=============================================================================================================

float SpectrumSettingsView::getUpperBound()
{
    return m_pSliderUpperBound->value()/1000.0f;
}

//=============================================================================================================

void SpectrumSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void SpectrumSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void SpectrumSettingsView::clearView()
{

}
