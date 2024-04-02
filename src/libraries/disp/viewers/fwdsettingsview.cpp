//=============================================================================================================
/**
 * @file     fwdsettingsview.cpp
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
 * @brief    FwdSettingsView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwdsettingsview.h"
#include "QtWidgets/qspinbox.h"
#include "ui_fwdsettingsview.h"

#include <fs/annotationset.h>

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
    m_pUi->m_spinBox_iNDipoleClustered->setValue(200);

    // load init annotation set
    QString t_sAtlasDir = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/label";
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
    connect(m_pUi->m_spinBox_iNDipoleClustered, &QSpinBox::valueChanged,
            this, &FwdSettingsView::clusterNumberChanged);

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

int FwdSettingsView::getClusterNumber()
{
    return m_pUi->m_spinBox_iNDipoleClustered->value();
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
