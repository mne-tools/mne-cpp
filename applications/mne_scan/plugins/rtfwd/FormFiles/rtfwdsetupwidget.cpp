//=============================================================================================================
/**
 * @file     rtfwdsetupwidget.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
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

#include <fiff/fiff_stream.h>

#include <fwd/computeFwd/compute_fwd_settings.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;
using namespace FIFFLIB;

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
    m_ui.m_qLineEdit_MeasName->setText(m_pRtFwd->m_pFwdSettings->measname);
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

    if(m_pRtFwd->m_pFwdSettings->coord_frame == FIFFV_COORD_MRI) {
        m_ui.m_check_bCoordframe->setChecked(true);
    } else {
        m_ui.m_check_bCoordframe->setChecked(false);
    }

    m_ui.m_check_bAccurate->setChecked(m_pRtFwd->m_pFwdSettings->accurate);
    m_ui.m_check_bFixedOri->setChecked(m_pRtFwd->m_pFwdSettings->fixed_ori);
    m_ui.m_check_bFilterSpaces->setChecked(m_pRtFwd->m_pFwdSettings->filter_spaces);
    m_ui.m_check_bMriHeadIdent->setChecked(m_pRtFwd->m_pFwdSettings->mri_head_ident);
    m_ui.m_check_bUseThreads->setChecked(m_pRtFwd->m_pFwdSettings->use_threads);
    m_ui.m_check_bUseEquivEeg->setChecked(m_pRtFwd->m_pFwdSettings->use_equiv_eeg);

    // init Spin Boxes
    m_ui.m_doubleSpinBox_dMinDist->setValue(m_pRtFwd->m_pFwdSettings->mindist*1000);
    m_ui.m_doubleSpinBox_dEegSphereRad->setValue(m_pRtFwd->m_pFwdSettings->eeg_sphere_rad*1000);
    m_ui.m_doubleSpinBox_dVecR0x->setValue(m_pRtFwd->m_pFwdSettings->r0.x()*1000);
    m_ui.m_doubleSpinBox_dVecR0y->setValue(m_pRtFwd->m_pFwdSettings->r0.y()*1000);
    m_ui.m_doubleSpinBox_dVecR0z->setValue(m_pRtFwd->m_pFwdSettings->r0.z()*1000);

    // connect line edits
    connect(m_ui.m_qLineEdit_SolName, &QLineEdit::textChanged, this, &RtFwdSetupWidget::onSolNameChanged);
    connect(m_ui.m_qLineEdit_MinDistName, &QLineEdit::textChanged, this, &RtFwdSetupWidget::onMinDistNameChanged);

    // connect checkboxes
    connect(m_ui.m_check_bDoAll, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bAccurate, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bFixedOri, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bCoordframe, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bIncludeEEG, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bIncludeMeg, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bUseThreads, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bComputeGrad, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bScaleEegPos, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bUseEquivEeg, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bFilterSpaces, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);
    connect(m_ui.m_check_bMriHeadIdent, &QCheckBox::stateChanged, this, &RtFwdSetupWidget::onCheckStateChanged);

    // connect spin boxes
    connect(m_ui.m_doubleSpinBox_dMinDist,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::onMinDistChanged);
    connect(m_ui.m_doubleSpinBox_dEegSphereRad, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::onEEGSphereRadChanged);
    connect(m_ui.m_doubleSpinBox_dVecR0x, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::onEEGSphereOriginChanged);
    connect(m_ui.m_doubleSpinBox_dVecR0y, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::onEEGSphereOriginChanged);
    connect(m_ui.m_doubleSpinBox_dVecR0z, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RtFwdSetupWidget::onEEGSphereOriginChanged);

    // connet push buttons
    connect(m_ui.m_qPushButton_SolNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showFwdDirDialog);
    connect(m_ui.m_qPushButton_BemNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showBemFileDialog);
    connect(m_ui.m_qPushButton_MeasNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showMeasFileDialog);
    connect(m_ui.m_qPushButton_SourceNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showSourceFileDialog);
    connect(m_ui.m_qPushButton_MriNameDialog, &QPushButton::released, this, &RtFwdSetupWidget::showMriFileDialog);
    connect(m_ui.m_qPushButton_MinDistOutDialog, &QPushButton::released, this, &RtFwdSetupWidget::showMinDistDirDialog);
    connect(m_ui.m_qPushButton_EEGModelFileDialog, &QPushButton::released, this, &RtFwdSetupWidget::showEEGModelFileDialog);
}

//=============================================================================================================

void RtFwdSetupWidget::showFwdDirDialog()
{
    QString t_sSolDir = QFileDialog::getExistingDirectory(this,
                                                         tr("Select Directory to store the forward solution"),
                                                         QString(),
                                                         QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks);

    m_ui.m_qLineEdit_SolName->setText(t_sSolDir);
}

//=============================================================================================================

void RtFwdSetupWidget::onSolNameChanged()
{
    QString t_sFileName = m_ui.m_qLineEdit_SolName->text();

    // check for file endings
    if(t_sFileName.contains("-fwd.fif")) {
        m_pRtFwd->m_pFwdSettings->solname = t_sFileName;
    } else {
        qWarning() << "rtFwdSetup: make sure to name solution file correctly: -fwd.fif";
    }
}

//=============================================================================================================

void RtFwdSetupWidget::showMeasFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Measurement File"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_ui.m_qLineEdit_MeasName->setText(t_sFileName);

    QFile t_fSource(t_sFileName);
    if(t_fSource.open(QIODevice::ReadOnly)) {
        m_pRtFwd->m_pFwdSettings->measname = t_sFileName;
        m_ui.m_qLineEdit_MeasName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Measurement file cannot be opened";
    }
    t_fSource.close();

}

//=============================================================================================================

void RtFwdSetupWidget::showSourceFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Source Space"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fSource(t_sFileName);
    if(t_fSource.open(QIODevice::ReadOnly)) {
        m_pRtFwd->m_pFwdSettings->srcname = t_sFileName;
        m_ui.m_qLineEdit_SourceName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Source file cannot be opened";
    }
    t_fSource.close();
}

//=============================================================================================================

void RtFwdSetupWidget::showBemFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Bem Model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fBem(t_sFileName);
    if(t_fBem.open(QIODevice::ReadOnly)) {
        m_pRtFwd->m_pFwdSettings->bemname = t_sFileName;
        m_ui.m_qLineEdit_BemName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Bem file cannot be opened";
    }
    t_fBem.close();
}

//=============================================================================================================

void RtFwdSetupWidget::showMriFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Mri-Head Transformation"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fMri(t_sFileName);
    if(t_fMri.open(QIODevice::ReadOnly)) {
        m_pRtFwd->m_pFwdSettings->mriname = t_sFileName;
        m_ui.m_qLineEdit_MriName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Mri-Head transformation cannot be opened";
    }
    t_fMri.close();
}

//=============================================================================================================

void RtFwdSetupWidget::showEEGModelFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select EEG model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fEegModel(t_sFileName);
    if(t_fEegModel.open(QIODevice::ReadOnly)) {
        m_pRtFwd->m_pFwdSettings->eeg_model_file = t_sFileName;
        m_ui.m_qLineEdit_EEGModelFile->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Eeg model file cannot be opened";
    }
    t_fEegModel.close();
}

//=============================================================================================================

void RtFwdSetupWidget::onEEGModelNameChanged()
{
    m_pRtFwd->m_pFwdSettings->eeg_model_name = m_ui.m_qLineEdit_EEGModelName->text();
}

//=============================================================================================================

void RtFwdSetupWidget::showMinDistDirDialog()
{
    QString t_sMinDistDir = QFileDialog::getExistingDirectory(this,
                                                             tr("Select output for omitted source space"),
                                                             QString(),
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);

    m_ui.m_qLineEdit_MinDistName->setText(t_sMinDistDir);
}

//=============================================================================================================

void RtFwdSetupWidget::onMinDistNameChanged()
{
    QString t_sFileName = m_ui.m_qLineEdit_MinDistName->text();
    m_pRtFwd->m_pFwdSettings->mindistoutname = t_sFileName;
}

//=============================================================================================================

void RtFwdSetupWidget::onMinDistChanged()
{
    m_pRtFwd->m_pFwdSettings->mindist = m_ui.m_doubleSpinBox_dMinDist->value()/1000;
}
//=============================================================================================================

void RtFwdSetupWidget::onEEGSphereRadChanged()
{
    m_pRtFwd->m_pFwdSettings->eeg_sphere_rad = m_ui.m_doubleSpinBox_dEegSphereRad->value()/1000;
}

//=============================================================================================================

void RtFwdSetupWidget::onEEGSphereOriginChanged()
{
    m_pRtFwd->m_pFwdSettings->r0.x() = m_ui.m_doubleSpinBox_dVecR0x->value()/1000;
    m_pRtFwd->m_pFwdSettings->r0.y() = m_ui.m_doubleSpinBox_dVecR0y->value()/1000;
    m_pRtFwd->m_pFwdSettings->r0.z() = m_ui.m_doubleSpinBox_dVecR0z->value()/1000;
}

//=============================================================================================================

void RtFwdSetupWidget::onCheckStateChanged()
{
    m_pRtFwd->m_pFwdSettings->do_all = m_ui.m_check_bDoAll->isChecked();
    m_pRtFwd->m_pFwdSettings->include_eeg = m_ui.m_check_bIncludeEEG->isChecked();
    m_pRtFwd->m_pFwdSettings->include_meg = m_ui.m_check_bIncludeMeg->isChecked();
    m_pRtFwd->m_pFwdSettings->compute_grad = m_ui.m_check_bComputeGrad->isChecked();

    if( m_ui.m_check_bCoordframe->isChecked()) {
        m_pRtFwd->m_pFwdSettings->coord_frame = FIFFV_COORD_MRI;
    } else {
        m_pRtFwd->m_pFwdSettings->coord_frame = FIFFV_COORD_HEAD;
    }

    m_pRtFwd->m_pFwdSettings->accurate = m_ui.m_check_bAccurate->isChecked();
    m_pRtFwd->m_pFwdSettings->fixed_ori = m_ui.m_check_bFixedOri->isChecked();
    m_pRtFwd->m_pFwdSettings->filter_spaces = m_ui.m_check_bFilterSpaces->isChecked();
    m_pRtFwd->m_pFwdSettings->mri_head_ident = m_ui.m_check_bMriHeadIdent->isChecked();
    m_pRtFwd->m_pFwdSettings->use_threads = m_ui.m_check_bUseThreads->isChecked();
    m_pRtFwd->m_pFwdSettings->use_equiv_eeg = m_ui.m_check_bUseEquivEeg->isChecked();
    m_pRtFwd->m_pFwdSettings->scale_eeg_pos = m_ui.m_check_bScaleEegPos->isChecked();
}

//=============================================================================================================

RtFwdSetupWidget::~RtFwdSetupWidget()
{
}
