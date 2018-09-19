//=============================================================================================================
/**
* @file     averagingsettingsview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the AveragingSettingsView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsettingsview.h"
#include "ui_averagingsettingsview.h"

#include <fiff/fiff_info.h>


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

AveragingSettingsView::AveragingSettingsView(QWidget *parent,
                                             FiffInfo::SPtr pFiffInfo,
                                             const QList<qint32>& qListStimChs,
                                             int iStimChan,
                                             int iNumAverages,
                                             int iAverageMode,
                                             int iPreStimSeconds,
                                             int iPostStimSeconds,
                                             bool bDoArtifactThresholdReduction,
                                             bool bDoArtifactVarianceReduction,
                                             double dArtifactThresholdFirst,
                                             int iArtifactThresholdSecond,
                                             double dArtifactVariance,
                                             bool bDoBaselineCorrection,
                                             int iBaselineFromSeconds,
                                             int iBaselineToSeconds)
: QWidget(parent)
, ui(new Ui::AverageSettingsViewWidget)
{
    ui->setupUi(this);

    this->setWindowTitle("Averaging Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    if(pFiffInfo && qListStimChs.size() > 0) {
        for(qint32 i = 0; i < qListStimChs.size(); ++i) {
            ui->m_pComboBoxChSelection->insertItem(i,pFiffInfo->ch_names[qListStimChs[i]],QVariant(i));
        }

        ui->m_pComboBoxChSelection->setCurrentIndex(iStimChan);

        connect(ui->m_pComboBoxChSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &AveragingSettingsView::changeStimChannel);
    }

    ui->m_pSpinBoxNumAverages->setValue(iNumAverages);
    connect(ui->m_pSpinBoxNumAverages, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeNumAverages);

    ui->m_comboBox_runningAvr->setCurrentIndex(iAverageMode);
    connect(ui->m_comboBox_runningAvr, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AveragingSettingsView::changeAverageMode);

    //Pre Post stimulus
    ui->m_pSpinBoxPreStimSamples->setValue(iPreStimSeconds);
    connect(ui->m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePreStim);

    ui->m_pSpinBoxPostStimSamples->setValue(iPostStimSeconds);
    connect(ui->m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePostStim);

    //Artifact rejection
    ui->m_pcheckBox_artifactReduction->setChecked(bDoArtifactThresholdReduction);
    connect(ui->m_pcheckBox_artifactReduction, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeArtifactThresholdReductionActive);

    ui->m_pcheckBox_varianceReduction->setChecked(bDoArtifactVarianceReduction);
    connect(ui->m_pcheckBox_varianceReduction, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeArtifactVarianceReductionActive);

    ui->m_pSpinBox_artifactThresholdFirst->setValue(dArtifactThresholdFirst);
    connect(ui->m_pSpinBox_artifactThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &AveragingSettingsView::onChangeArtifactThreshold);

    ui->m_pSpinBox_artifactThresholdSecond->setValue(iArtifactThresholdSecond);
    connect(ui->m_pSpinBox_artifactThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &AveragingSettingsView::onChangeArtifactThreshold);

    ui->m_spinBox_variance->setValue(dArtifactVariance);
    connect(ui->m_spinBox_variance, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &AveragingSettingsView::changeArtifactVariance);

    //Baseline Correction
    ui->m_pcheckBoxBaselineCorrection->setChecked(bDoBaselineCorrection);
    connect(ui->m_pcheckBoxBaselineCorrection, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeBaselineActive);

    ui->m_pSpinBoxBaselineFrom->setMinimum(ui->m_pSpinBoxPreStimSamples->value()*-1);
    ui->m_pSpinBoxBaselineFrom->setMaximum(ui->m_pSpinBoxPostStimSamples->value());
    ui->m_pSpinBoxBaselineFrom->setValue(iBaselineFromSeconds);
    connect(ui->m_pSpinBoxBaselineFrom, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineFrom);

    ui->m_pSpinBoxBaselineTo->setMinimum(ui->m_pSpinBoxPreStimSamples->value()*-1);
    ui->m_pSpinBoxBaselineTo->setMaximum(ui->m_pSpinBoxPostStimSamples->value());
    ui->m_pSpinBoxBaselineTo->setValue(iBaselineToSeconds);
    connect(ui->m_pSpinBoxBaselineTo, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineTo);

    connect(ui->m_pushButton_reset, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &AveragingSettingsView::resetAverage);

    setWindowFlags(Qt::WindowStaysOnTopHint);

    // Disable and hide variance rejection
    ui->m_pcheckBox_varianceReduction->setChecked(false);
    ui->m_line_adrtifactRejection->hide();
    ui->m_pcheckBox_varianceReduction->hide();
    ui->m_label_varianceValue->hide();
    ui->m_spinBox_variance->hide();
}


//*************************************************************************************************************

void AveragingSettingsView::setStimChannels(FiffInfo::SPtr pFiffInfo,
                                               QList<qint32> qListStimChs,
                                               int iStimChan)
{
    if(pFiffInfo && qListStimChs.size() > 0) {
        ui->m_pComboBoxChSelection->clear();

        for(qint32 i = 0; i < qListStimChs.size(); ++i) {
            ui->m_pComboBoxChSelection->insertItem(i,pFiffInfo->ch_names[ qListStimChs[i] ],QVariant(i));
        }

        ui->m_pComboBoxChSelection->setCurrentIndex(iStimChan);

        connect(ui->m_pComboBoxChSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &AveragingSettingsView::changeStimChannel);
    }
}


//*************************************************************************************************************

int AveragingSettingsView::getStimChannelIdx()
{
    return ui->m_pComboBoxChSelection->currentData().toInt();
}


//*************************************************************************************************************

void AveragingSettingsView::onChangePreStim()
{
    qint32 mSeconds = ui->m_pSpinBoxPreStimSamples->value();
    ui->m_pSpinBoxBaselineTo->setMinimum(ui->m_pSpinBoxPreStimSamples->value()*-1);
    ui->m_pSpinBoxBaselineFrom->setMinimum(ui->m_pSpinBoxPreStimSamples->value()*-1);

    emit changePreStim(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangePostStim()
{
    qint32 mSeconds = ui->m_pSpinBoxPostStimSamples->value();
    ui->m_pSpinBoxBaselineTo->setMaximum(ui->m_pSpinBoxPostStimSamples->value());
    ui->m_pSpinBoxBaselineFrom->setMaximum(ui->m_pSpinBoxPostStimSamples->value());

    emit changePostStim(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineFrom()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineFrom->value();
    ui->m_pSpinBoxBaselineTo->setMinimum(mSeconds);

    emit changeBaselineFrom(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineTo()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineTo->value();
    ui->m_pSpinBoxBaselineFrom->setMaximum(mSeconds);

    emit changeBaselineTo(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeArtifactThreshold()
{
    emit changeArtifactThreshold(ui->m_pSpinBox_artifactThresholdFirst->value(),
                                 ui->m_pSpinBox_artifactThresholdSecond->value());
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeNumAverages()
{
    emit changeNumAverages(ui->m_pSpinBoxNumAverages->value());
}

