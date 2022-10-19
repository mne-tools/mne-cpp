//=============================================================================================================
/**
 * @file     fwdsettingsview.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.1
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel, Gabriel B Motta. All rights reserved.
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
 * @brief    FwdSettingsView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwdsettingsview.h"
#include "ui_fwdsettingsview.h"

#include <fs/annotationset.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdSettingsView::FwdSettingsView(const QString& sSettingsPath,
                                 QWidget *parent,
                                 Qt::WindowFlags f)
: AbstractView(parent, f)
, m_bAnnotaionsLoaded(false)
, m_pUi(new Ui::FwdSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    // init
    m_pUi->m_checkBox_bDoRecomputation->setChecked(false);
    m_pUi->m_checkBox_bDoClustering->setChecked(false);
    m_pUi->m_lineEdit_iNChan->setText(QString::number(0));
    m_pUi->m_lineEdit_iNSourceSpace->setText(QString::number(0));
    m_pUi->m_lineEdit_iNDipole->setText(QString::number(0));
    m_pUi->m_lineEdit_sSourceOri->setText("fixed");
    m_pUi->m_lineEdit_sCoordFrame->setText("Head Space");
    m_pUi->m_lineEdit_iNDipoleClustered->setText("Not Clustered");

    // load init annotation set
    QString t_sAtlasDir = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/label";
    m_pUi->m_qLineEdit_AtlasDirName->setText(t_sAtlasDir);

    AnnotationSet::SPtr t_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(t_sAtlasDir+"/lh.aparc.a2009s.annot", t_sAtlasDir+"/rh.aparc.a2009s.annot"));

    if(!t_pAnnotationSet->isEmpty() && t_pAnnotationSet->size() == 2)
    {
        emit atlasDirChanged(t_sAtlasDir,t_pAnnotationSet);
        m_pUi->m_qLabel_atlasStat->setText("loaded");
        m_bAnnotaionsLoaded = true;
    }
    else
    {
        m_pUi->m_qLabel_atlasStat->setText("not loaded");
    }

    // connect
    connect(m_pUi->m_checkBox_bDoRecomputation, &QCheckBox::clicked,
            this, &FwdSettingsView::recompStatusChanged);
    connect(m_pUi->m_qPushButton_AtlasDirDialog, &QPushButton::released,
            this, &FwdSettingsView::showAtlasDirDialog);
    connect(m_pUi->m_checkBox_bDoClustering, &QCheckBox::clicked,
            this, &FwdSettingsView::onClusteringStatusChanged);
    connect(m_pUi->m_qPushButton_ComputeForward, &QPushButton::clicked,
            this, &FwdSettingsView::doForwardComputation);

    // load settings
    loadSettings();
}

//=============================================================================================================

FwdSettingsView::~FwdSettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void FwdSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    QVariant data;
}

//=============================================================================================================

void FwdSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    QVariant defaultData;
}

//=============================================================================================================

void FwdSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void FwdSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

bool FwdSettingsView::getRecomputationStatusChanged()
{
    return m_pUi->m_checkBox_bDoRecomputation->isChecked();
}

//=============================================================================================================

void FwdSettingsView::setRecomputationStatus(int iStatus)
{
    if(iStatus == 0) {
        m_pUi->m_label_recomputationFeedback->setText("Initializing");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else if(iStatus == 1) {
        m_pUi->m_label_recomputationFeedback->setText("Computing");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else if (iStatus == 2) {
        m_pUi->m_label_recomputationFeedback->setText("Recomputing");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else if (iStatus == 3) {
        m_pUi->m_label_recomputationFeedback->setText("Clustering");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else if (iStatus == 4) {
        m_pUi->m_label_recomputationFeedback->setText("Not Computed");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        m_pUi->m_label_recomputationFeedback->setText("Finished");
        m_pUi->m_label_recomputationFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}

//=============================================================================================================

void FwdSettingsView::setSolutionInformation(FIFFLIB::fiff_int_t iSourceOri,
                                             FIFFLIB::fiff_int_t iCoordFrame,
                                             int iNSource,
                                             int iNChan,
                                             int iNSpaces)
{
    // set source orientation
    if(iSourceOri == 0) {
        m_pUi->m_lineEdit_sSourceOri->setText("fixed");
    } else {
        m_pUi->m_lineEdit_sSourceOri->setText("free");
    }

    // set coordinate frame
    if(iCoordFrame == FIFFV_COORD_HEAD) {
        m_pUi->m_lineEdit_sCoordFrame->setText("Head Space");
    } else if (iCoordFrame == FIFFV_COORD_MRI){
        m_pUi->m_lineEdit_sCoordFrame->setText("MRI Space");
    } else {
        m_pUi->m_lineEdit_sCoordFrame->setText("Unknown");
    }

    // set number of sources
    m_pUi->m_lineEdit_iNDipole->setText(QString::number(iNSource));

    // set number of clustered sources
    m_pUi->m_lineEdit_iNDipoleClustered->setText("Not clustered");

    // set number of channels
    m_pUi->m_lineEdit_iNChan->setText(QString::number(iNChan));

    // set number of source spaces
    m_pUi->m_lineEdit_iNSourceSpace->setText(QString::number(iNSpaces));
}

//=============================================================================================================

bool FwdSettingsView::getClusteringStatusChanged()
{
    return m_pUi->m_checkBox_bDoClustering->isChecked();
}

//=============================================================================================================

void FwdSettingsView::onClusteringStatusChanged(bool bChecked)
{
    if(!m_bAnnotaionsLoaded) {
        QMessageBox msgBox;
        msgBox.setText("Please load an annotation set before clustering.");
        msgBox.exec();
        m_pUi->m_checkBox_bDoClustering->setChecked(false);
        return;
    } else {
        emit clusteringStatusChanged(bChecked);
    }
}

//=============================================================================================================

void FwdSettingsView::setClusteredInformation(int iNSources)
{
    // set number of clustered sources
    m_pUi->m_lineEdit_iNDipoleClustered->setText(QString::number(iNSources));
}

//=============================================================================================================

void FwdSettingsView::showAtlasDirDialog()
{
    QString t_sAtlasDir = QFileDialog::getExistingDirectory(this, tr("Open Atlas Directory"),
                                                            QCoreApplication::applicationDirPath(),
                                                            QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);

    m_pUi->m_qLineEdit_AtlasDirName->setText(t_sAtlasDir);

    AnnotationSet::SPtr t_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(t_sAtlasDir+"/lh.aparc.a2009s.annot", t_sAtlasDir+"/rh.aparc.a2009s.annot"));

    if(!t_pAnnotationSet->isEmpty() && t_pAnnotationSet->size() == 2)
    {
        emit atlasDirChanged(t_sAtlasDir,t_pAnnotationSet);
        m_pUi->m_qLabel_atlasStat->setText("loaded");
        m_bAnnotaionsLoaded = true;
    }
    else
    {
        m_pUi->m_qLabel_atlasStat->setText("not loaded");
    }
}

//=============================================================================================================

void FwdSettingsView::clearView()
{

}

//=============================================================================================================

void FwdSettingsView::setSettings(QSharedPointer<FWDLIB::ComputeFwdSettings> pSettings)
{
    m_pFwdSettings = pSettings;

    // init line edits
    m_pUi->m_qLineEdit_SolName->setText(m_pFwdSettings->solname);
    m_pUi->m_qLineEdit_MeasName->setText(m_pFwdSettings->measname);
    m_pUi->m_qLineEdit_BemName->setText(m_pFwdSettings->bemname);
    m_pUi->m_qLineEdit_SourceName->setText(m_pFwdSettings->srcname);
    m_pUi->m_qLineEdit_MriName->setText(m_pFwdSettings->mriname);
    m_pUi->m_qLineEdit_MinDistName->setText(m_pFwdSettings->mindistoutname);
    m_pUi->m_qLineEdit_EEGModelFile->setText(m_pFwdSettings->eeg_model_file);
    m_pUi->m_qLineEdit_EEGModelName->setText(m_pFwdSettings->eeg_model_name);

    // init checkboxes
    m_pUi->m_check_bDoAll->setChecked(m_pFwdSettings->do_all);
    m_pUi->m_check_bIncludeEEG->setChecked(m_pFwdSettings->include_eeg);
    m_pUi->m_check_bIncludeMeg->setChecked(m_pFwdSettings->include_meg);
    m_pUi->m_check_bComputeGrad->setChecked(m_pFwdSettings->compute_grad);

    if(m_pFwdSettings->coord_frame == FIFFV_COORD_MRI) {
        m_pUi->m_check_bCoordframe->setChecked(true);
    } else {
        m_pUi->m_check_bCoordframe->setChecked(false);
    }

    m_pUi->m_check_bAccurate->setChecked(m_pFwdSettings->accurate);
    m_pUi->m_check_bFixedOri->setChecked(m_pFwdSettings->fixed_ori);
    m_pUi->m_check_bFilterSpaces->setChecked(m_pFwdSettings->filter_spaces);
    m_pUi->m_check_bMriHeadIdent->setChecked(m_pFwdSettings->mri_head_ident);
    m_pUi->m_check_bUseThreads->setChecked(m_pFwdSettings->use_threads);
    m_pUi->m_check_bUseEquivEeg->setChecked(m_pFwdSettings->use_equiv_eeg);

    // init Spin Boxes
    m_pUi->m_doubleSpinBox_dMinDist->setValue(m_pFwdSettings->mindist*1000);
    m_pUi->m_doubleSpinBox_dEegSphereRad->setValue(m_pFwdSettings->eeg_sphere_rad*1000);
    m_pUi->m_doubleSpinBox_dVecR0x->setValue(m_pFwdSettings->r0.x()*1000);
    m_pUi->m_doubleSpinBox_dVecR0y->setValue(m_pFwdSettings->r0.y()*1000);
    m_pUi->m_doubleSpinBox_dVecR0z->setValue(m_pFwdSettings->r0.z()*1000);

    // connect line edits
    connect(m_pUi->m_qLineEdit_SolName, &QLineEdit::textChanged, this, &FwdSettingsView::onSolNameChanged);
    connect(m_pUi->m_qLineEdit_MinDistName, &QLineEdit::textChanged, this, &FwdSettingsView::onMinDistNameChanged);

    // connect checkboxes
    connect(m_pUi->m_check_bDoAll, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bAccurate, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bFixedOri, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bCoordframe, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bIncludeEEG, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bIncludeMeg, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bUseThreads, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bComputeGrad, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bScaleEegPos, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bUseEquivEeg, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bFilterSpaces, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);
    connect(m_pUi->m_check_bMriHeadIdent, &QCheckBox::stateChanged, this, &FwdSettingsView::onCheckStateChanged);

    // connect spin boxes
    connect(m_pUi->m_doubleSpinBox_dMinDist,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &FwdSettingsView::onMinDistChanged);
    connect(m_pUi->m_doubleSpinBox_dEegSphereRad, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &FwdSettingsView::onEEGSphereRadChanged);
    connect(m_pUi->m_doubleSpinBox_dVecR0x, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &FwdSettingsView::onEEGSphereOriginChanged);
    connect(m_pUi->m_doubleSpinBox_dVecR0y, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &FwdSettingsView::onEEGSphereOriginChanged);
    connect(m_pUi->m_doubleSpinBox_dVecR0z, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &FwdSettingsView::onEEGSphereOriginChanged);

    // connet push buttons
    connect(m_pUi->m_qPushButton_SolNameDialog, &QPushButton::released, this, &FwdSettingsView::showFwdDirDialog);
    connect(m_pUi->m_qPushButton_BemNameDialog, &QPushButton::released, this, &FwdSettingsView::showBemFileDialog);
    connect(m_pUi->m_qPushButton_MeasNameDialog, &QPushButton::released, this, &FwdSettingsView::showMeasFileDialog);
    connect(m_pUi->m_qPushButton_SourceNameDialog, &QPushButton::released, this, &FwdSettingsView::showSourceFileDialog);
    connect(m_pUi->m_qPushButton_MriNameDialog, &QPushButton::released, this, &FwdSettingsView::showMriFileDialog);
    connect(m_pUi->m_qPushButton_MinDistOutDialog, &QPushButton::released, this, &FwdSettingsView::showMinDistDirDialog);
    connect(m_pUi->m_qPushButton_EEGModelFileDialog, &QPushButton::released, this, &FwdSettingsView::showEEGModelFileDialog);

}

//=============================================================================================================

void FwdSettingsView::showFwdDirDialog()
{
    QString t_sSolDir = QFileDialog::getExistingDirectory(this,
                                                         tr("Select Directory to store the forward solution"),
                                                         QString(),
                                                         QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks);

    m_pUi->m_qLineEdit_SolName->setText(t_sSolDir);
}

//=============================================================================================================

void FwdSettingsView::onSolNameChanged()
{
    QString t_sFileName = m_pUi->m_qLineEdit_SolName->text();

    // check for file endings
    if(t_sFileName.contains("-fwd.fif")) {
        m_pFwdSettings->solname = t_sFileName;
    } else {
        qWarning() << "rtFwdSetup: make sure to name solution file correctly: -fwd.fif";
    }
}

//=============================================================================================================

void FwdSettingsView::showMeasFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Measurement File"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    m_pUi->m_qLineEdit_MeasName->setText(t_sFileName);

    QFile t_fSource(t_sFileName);
    if(t_fSource.open(QIODevice::ReadOnly)) {
        m_pFwdSettings->measname = t_sFileName;
        m_pUi->m_qLineEdit_MeasName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Measurement file cannot be opened";
    }
    t_fSource.close();

}

//=============================================================================================================

void FwdSettingsView::showSourceFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Source Space"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fSource(t_sFileName);
    if(t_fSource.open(QIODevice::ReadOnly)) {
        m_pFwdSettings->srcname = t_sFileName;
        m_pUi->m_qLineEdit_SourceName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Source file cannot be opened";
    }
    t_fSource.close();
}

//=============================================================================================================

void FwdSettingsView::showBemFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Bem Model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fBem(t_sFileName);
    if(t_fBem.open(QIODevice::ReadOnly)) {
        m_pFwdSettings->bemname = t_sFileName;
        m_pUi->m_qLineEdit_BemName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Bem file cannot be opened";
    }
    t_fBem.close();
}

//=============================================================================================================

void FwdSettingsView::showMriFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Mri-Head Transformation"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fMri(t_sFileName);
    if(t_fMri.open(QIODevice::ReadOnly)) {
        m_pFwdSettings->mriname = t_sFileName;
        m_pUi->m_qLineEdit_MriName->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Mri-Head transformation cannot be opened";
    }
    t_fMri.close();
}

//=============================================================================================================

void FwdSettingsView::showEEGModelFileDialog()
{
    QString t_sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select EEG model"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    QFile t_fEegModel(t_sFileName);
    if(t_fEegModel.open(QIODevice::ReadOnly)) {
        m_pFwdSettings->eeg_model_file = t_sFileName;
        m_pUi->m_qLineEdit_EEGModelFile->setText(t_sFileName);
    } else {
        qWarning() << "rtFwdSetup: Eeg model file cannot be opened";
    }
    t_fEegModel.close();
}

//=============================================================================================================

void FwdSettingsView::onEEGModelNameChanged()
{
    m_pFwdSettings->eeg_model_name = m_pUi->m_qLineEdit_EEGModelName->text();
}

//=============================================================================================================

void FwdSettingsView::showMinDistDirDialog()
{
    QString t_sMinDistDir = QFileDialog::getExistingDirectory(this,
                                                             tr("Select output for omitted source space"),
                                                             QString(),
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);

    m_pUi->m_qLineEdit_MinDistName->setText(t_sMinDistDir);
}

//=============================================================================================================

void FwdSettingsView::onMinDistNameChanged()
{
    QString t_sFileName = m_pUi->m_qLineEdit_MinDistName->text();
    m_pFwdSettings->mindistoutname = t_sFileName;
}

//=============================================================================================================

void FwdSettingsView::onMinDistChanged()
{
    m_pFwdSettings->mindist = m_pUi->m_doubleSpinBox_dMinDist->value()/1000;
}
//=============================================================================================================

void FwdSettingsView::onEEGSphereRadChanged()
{
    m_pFwdSettings->eeg_sphere_rad = m_pUi->m_doubleSpinBox_dEegSphereRad->value()/1000;
}

//=============================================================================================================

void FwdSettingsView::onEEGSphereOriginChanged()
{
    m_pFwdSettings->r0.x() = m_pUi->m_doubleSpinBox_dVecR0x->value()/1000;
    m_pFwdSettings->r0.y() = m_pUi->m_doubleSpinBox_dVecR0y->value()/1000;
    m_pFwdSettings->r0.z() = m_pUi->m_doubleSpinBox_dVecR0z->value()/1000;
}

//=============================================================================================================

void FwdSettingsView::onCheckStateChanged()
{
    m_pFwdSettings->do_all = m_pUi->m_check_bDoAll->isChecked();
    m_pFwdSettings->include_eeg = m_pUi->m_check_bIncludeEEG->isChecked();
    m_pFwdSettings->include_meg = m_pUi->m_check_bIncludeMeg->isChecked();
    m_pFwdSettings->compute_grad = m_pUi->m_check_bComputeGrad->isChecked();

    if( m_pUi->m_check_bCoordframe->isChecked()) {
        m_pFwdSettings->coord_frame = FIFFV_COORD_MRI;
    } else {
        m_pFwdSettings->coord_frame = FIFFV_COORD_HEAD;
    }

    m_pFwdSettings->accurate = m_pUi->m_check_bAccurate->isChecked();
    m_pFwdSettings->fixed_ori = m_pUi->m_check_bFixedOri->isChecked();
    m_pFwdSettings->filter_spaces = m_pUi->m_check_bFilterSpaces->isChecked();
    m_pFwdSettings->mri_head_ident = m_pUi->m_check_bMriHeadIdent->isChecked();
    m_pFwdSettings->use_threads = m_pUi->m_check_bUseThreads->isChecked();
    m_pFwdSettings->use_equiv_eeg = m_pUi->m_check_bUseEquivEeg->isChecked();
    m_pFwdSettings->scale_eeg_pos = m_pUi->m_check_bScaleEegPos->isChecked();
}
