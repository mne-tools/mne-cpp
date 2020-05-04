//=============================================================================================================
/**
 * @file     rtfwdsetupwidget.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RtFwdSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwdsetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwdSetupWidget::RtFwdSetupWidget(RtFwd* toolbox, QWidget *parent)
: QWidget(parent)
, m_pRtFwd(toolbox)
{
    m_ui.setupUi(this);

    // init line edits
    m_ui.m_qLineEdit_SolName->setText(m_pRtFwd->m_pFwdSettings->solname);
    m_ui.m_qLineEdit_BemName->setText(m_pRtFwd->m_pFwdSettings->bemname);
    m_ui.m_qLineEdit_SourceName->setText(m_pRtFwd->m_pFwdSettings->srcname);
    m_ui.m_qLineEdit_MriName->setText(m_pRtFwd->m_pFwdSettings->mriname);
    m_ui.m_qLineEdit_MinDistName->setText(m_pRtFwd->m_pFwdSettings->mindistoutname);
    m_ui.m_qLineEdit_EEGModelFile->setText(m_pRtFwd->m_pFwdSettings->eeg_model_file);
    m_ui.m_qLineEdit_EEGModelName->setText(m_pRtFwd->m_pFwdSettings->eeg_model_name);

    // init checkboxes
    m_ui.m_check_bDoAll->setChecked(m_pRtFwd->m_pFwdSettings->do_all);
    m_ui.m_check_bIncludeEEG->setChecked(m_pRtFwd->m_pFwdSettings->include_eeg);
    m_ui.m_check_bIncludeMeg->setChecked(m_pRtFwd->m_pFwdSettings->include_meg);
    m_ui.m_check_bComputeGrad->setChecked(m_pRtFwd->m_pFwdSettings->compute_grad);
    m_ui.m_check_bAccurate->setChecked(m_pRtFwd->m_pFwdSettings->accurate);
    m_ui.m_check_bFixedOri->setChecked(m_pRtFwd->m_pFwdSettings->fixed_ori);
    m_ui.m_check_bFilterSpaces->setChecked(m_pRtFwd->m_pFwdSettings->filter_spaces);
    m_ui.m_check_bMriHeadIdent->setChecked(m_pRtFwd->m_pFwdSettings->mri_head_ident);
    m_ui.m_check_bUseThreads->setChecked(m_pRtFwd->m_pFwdSettings->use_threads);
    m_ui.m_check_bUseEquivEeg->setChecked(m_pRtFwd->m_pFwdSettings->use_equiv_eeg);

    // init Spin Boxes
    m_ui.m_doubleSpinBox_dMinDist->setValue(m_pRtFwd->m_pFwdSettings->mindist);
    m_ui.m_doubleSpinBox_dEegSphereRad->setValue(m_pRtFwd->m_pFwdSettings->eeg_sphere_rad);
    m_ui.m_doubleSpinBox_dVecR0x->setValue(m_pRtFwd->m_pFwdSettings->r0.x());
    m_ui.m_doubleSpinBox_dVecR0y->setValue(m_pRtFwd->m_pFwdSettings->r0.y());
    m_ui.m_doubleSpinBox_dVecR0z->setValue(m_pRtFwd->m_pFwdSettings->r0.z());

    // connec line edits
    connect(m_ui.m_qLineEdit_SolName, &QLineEdit::textChanged, this, &RtFwdSetupWidget::changeSolName);
    connect(m_ui.m_qLineEdit_MinDistName, &QLineEdit::textChanged, this, &RtFwdSetupWidget::changeMinDistName);

    // connec spin boxes
    connect(m_ui.m_doubleSpinBox_dMinDist,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::changeMinDist);
    connect(m_ui.m_doubleSpinBox_dEegSphereRad, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::changeEEGSphereRad);
    connect(m_ui.m_doubleSpinBox_dVecR0x, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::changeEEGSphereOrigin);
    connect(m_ui.m_doubleSpinBox_dVecR0y, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::changeEEGSphereOrigin);
    connect(m_ui.m_doubleSpinBox_dVecR0z, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::changeEEGSphereOrigin);

    // connet Bushbuttons
    connect(m_ui.m_qPushButton_SolNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showFwdDirDialog);
    connect(m_ui.m_qPushButton_BemNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showBemFileDialog);
    connect(m_ui.m_qPushButton_SourceNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showSourceFileDialog);
    connect(m_ui.m_qPushButton_MriNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showMriFileDialog);
    connect(m_ui.m_qPushButton_MinDistOutDialog, &QPushButton::released, this, &RtFwdSetupWidget::showMinDistDirDialog);
    connect(m_ui.m_qPushButton_EEGModelFileDialog, &QPushButton::released, this, &RtFwdSetupWidget::showEEGModelFileDialog);
    connect(m_ui.m_qPushButton_EEGModelNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showEEGModelNameDialog);
}

//=============================================================================================================

void RtFwdSetupWidget::showFwdDirDialog()
{
    m_sSolDir = QFileDialog::getExistingDirectory(this,
                                                  tr("Select Directory to store the forward solution"),
                                                  QString(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);

    QString t_sFileName = m_ui.m_qLineEdit_SolName->text();
    m_pRtFwd->m_pFwdSettings->solname = m_sSolDir + "/" + t_sFileName;
}

//=============================================================================================================

void RtFwdSetupWidget::changeSolName()
{
    QString t_sFileName = m_ui.m_qLineEdit_SolName->text();
    m_pRtFwd->m_pFwdSettings->solname = m_sSolDir + "/" + t_sFileName;
}

//=============================================================================================================

void RtFwdSetupWidget::showSourceFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Source Space"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pRtFwd->m_pFwdSettings->srcname = t_sFileName;
    m_ui.m_qLineEdit_SourceName->setText(t_sFileName);
}

//=============================================================================================================

void RtFwdSetupWidget::showBemFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Bem Model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pRtFwd->m_pFwdSettings->bemname = t_sFileName;
    m_ui.m_qLineEdit_BemName->setText(t_sFileName);
}

//=============================================================================================================

void RtFwdSetupWidget::showMriFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Mri-Head Transformation"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pRtFwd->m_pFwdSettings->mriname = t_sFileName;
    m_ui.m_qLineEdit_MriName->setText(t_sFileName);
}

//=============================================================================================================

void RtFwdSetupWidget::showEEGModelFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select EEG model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pRtFwd->m_pFwdSettings->eeg_model_file = t_sFileName;
    m_ui.m_qLineEdit_EEGModelFile->setText(t_sFileName);
}

//=============================================================================================================

void RtFwdSetupWidget::showEEGModelNameDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select EEG model name"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pRtFwd->m_pFwdSettings->eeg_model_name = t_sFileName;
    m_ui.m_qLineEdit_EEGModelName->setText(t_sFileName);
}

//=============================================================================================================

void RtFwdSetupWidget::showMinDistDirDialog()
{
    m_sMinDistDir = QFileDialog::getExistingDirectory(this,
                                                      tr("Select output for omitted source space"),
                                                      QString(),
                                                      QFileDialog::ShowDirsOnly
                                                      | QFileDialog::DontResolveSymlinks);

    QString t_sFileName = m_ui.m_qLineEdit_MinDistName->text();
    m_pRtFwd->m_pFwdSettings->mindistoutname = m_sMinDistDir + "/" + t_sFileName;
}

//=============================================================================================================

void RtFwdSetupWidget::changeMinDistName()
{
    QString t_sFileName = m_ui.m_qLineEdit_MinDistName->text();
    m_pRtFwd->m_pFwdSettings->mindistoutname = m_sMinDistDir + "/" + t_sFileName;
}

//=============================================================================================================

void RtFwdSetupWidget::changeMinDist()
{
    m_pRtFwd->m_pFwdSettings->mindist = m_ui.m_doubleSpinBox_dMinDist->value();
}
//=============================================================================================================

void RtFwdSetupWidget::changeEEGSphereRad()
{
    m_pRtFwd->m_pFwdSettings->eeg_sphere_rad = m_ui.m_doubleSpinBox_dEegSphereRad->value();
}

//=============================================================================================================

void RtFwdSetupWidget::changeEEGSphereOrigin()
{
    m_pRtFwd->m_pFwdSettings->r0.x() = m_ui.m_doubleSpinBox_dVecR0x->value();
    m_pRtFwd->m_pFwdSettings->r0.y() = m_ui.m_doubleSpinBox_dVecR0y->value();
    m_pRtFwd->m_pFwdSettings->r0.z() = m_ui.m_doubleSpinBox_dVecR0z->value();
    std::cout << m_pRtFwd->m_vecR0 << std::endl;
}
//=============================================================================================================

RtFwdSetupWidget::~RtFwdSetupWidget()
{
}
