//=============================================================================================================
/**
 * @file     timefrequencysettingsview.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the TimeFrequencySettingsWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencysettingsview.h"

#include "ui_timefrequencysettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencySettingsView::TimeFrequencySettingsView(const QString& sSettingsPath,
                                                     QWidget *parent)
: AbstractView(parent)
, m_pUi(new Ui::TimeFrequencySettingsWidget)
{

    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Time-Frequency Settings");
    this->setMinimumWidth(330);

    initGUI();
}

//=============================================================================================================

void TimeFrequencySettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TimeFrequencySettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:

            break;
        default: // default is realtime mode

            break;
    }
}

//=============================================================================================================

void TimeFrequencySettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

}

//=============================================================================================================

void TimeFrequencySettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }
}

//=============================================================================================================

void TimeFrequencySettingsView::clearView()
{

}

//=============================================================================================================

void TimeFrequencySettingsView::initGUI()
{
    //Freq
    connect(m_pUi->spinBox_minFreq, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TimeFrequencySettingsView::minFreqChanged, Qt::UniqueConnection);
    connect(m_pUi->spinBox_maxFreq, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TimeFrequencySettingsView::maxFreqChanged, Qt::UniqueConnection);

    //Color map

    connect(m_pUi->comboBox_colorMap, &QComboBox::currentTextChanged,
            this, &TimeFrequencySettingsView::colorMapChanged, Qt::UniqueConnection);

    //Compute

    connect(m_pUi->pushButton_ciompute, &QPushButton::released,
            this, &TimeFrequencySettingsView::computePushed, Qt::UniqueConnection);
}
