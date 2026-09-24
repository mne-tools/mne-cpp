//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtcmnesetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RtcMneSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmnesetupwidget.h"

#include "../rtcmne.h"

#include <fs/fs_annotationset.h>
#include <fs/fs_surfaceset.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne_forward_solution.h>

#include <scMeas/realtimesourceestimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFileDialog>
#include <QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCMNEPLUGIN;
using namespace MNELIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtcMneSetupWidget::RtcMneSetupWidget(RtcMne* toolbox, QWidget *parent)
: QWidget(parent)
, m_pMNE(toolbox)
{
    ui.setupUi(this);

    ui.m_qLineEdit_AtlasDirName->setText(m_pMNE->m_sAtlasDir);
    if(m_pMNE->m_pAnnotationSet->isEmpty())
        ui.m_qLabel_atlasStat->setText("not loaded");
    else
        ui.m_qLabel_atlasStat->setText("loaded");

    ui.m_qLineEdit_SurfaceDirName->setText(m_pMNE->m_sSurfaceDir);
    if(m_pMNE->m_pSurfaceSet->isEmpty())
        ui.m_qLabel_surfaceStat->setText("not loaded");
    else
        ui.m_qLabel_surfaceStat->setText("loaded");

    ui.m_qLineEdit_MriHeadTrans->setText(m_pMNE->m_fMriHeadTrans.fileName());

    connect(ui.m_qPushButton_AtlasDirDialog, &QPushButton::released, this, &RtcMneSetupWidget::showAtlasDirDialog);
    connect(ui.m_qPushButton_SurfaceDirDialog, &QPushButton::released, this, &RtcMneSetupWidget::showSurfaceDirDialog);
    connect(ui.m_qPushButton_MriHeadTrans, &QPushButton::released, this, &RtcMneSetupWidget::showMriHeadFileDialog);
}

//=============================================================================================================

RtcMneSetupWidget::~RtcMneSetupWidget()
{
}

//=============================================================================================================

void RtcMneSetupWidget::showAtlasDirDialog()
{
    QString t_sAtlasDir = QFileDialog::getExistingDirectory(this, tr("Open Atlas Directory"),
                                                            QString(),
                                                            QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);

    m_pMNE->m_sAtlasDir = t_sAtlasDir;

    ui.m_qLineEdit_AtlasDirName->setText(m_pMNE->m_sAtlasDir);

    FsAnnotationSet::SPtr t_pAnnotationSet = FsAnnotationSet::SPtr(new FsAnnotationSet(t_sAtlasDir+"/lh.aparc.a2009s.annot", t_sAtlasDir+"/rh.aparc.a2009s.annot"));

    if(!t_pAnnotationSet->isEmpty() && t_pAnnotationSet->size() == 2)
    {
        m_pMNE->m_pAnnotationSet = t_pAnnotationSet;

        m_pMNE->m_sAtlasDir = t_sAtlasDir;

        m_pMNE->m_pRTSEOutput->measurementData()->setAnnotSet(t_pAnnotationSet);

        ui.m_qLabel_atlasStat->setText("loaded");
    }
    else
    {
        m_pMNE->m_pAnnotationSet = FsAnnotationSet::SPtr(new FsAnnotationSet());
        ui.m_qLabel_atlasStat->setText("not loaded");
    }
}

//=============================================================================================================

void RtcMneSetupWidget::showSurfaceDirDialog()
{
    QString t_sSurfaceDir = QFileDialog::getExistingDirectory(  this, tr("Open FsSurface Directory"),
                                                                QString(),
                                                                QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);

    FsSurfaceSet::SPtr t_pSurfaceSet = FsSurfaceSet::SPtr(new FsSurfaceSet(t_sSurfaceDir+"/lh.orig", t_sSurfaceDir+"/rh.orig"));

    ui.m_qLineEdit_SurfaceDirName->setText(t_sSurfaceDir);

    if(!t_pSurfaceSet->isEmpty() && t_pSurfaceSet->size() == 2)
    {
        m_pMNE->m_pSurfaceSet = t_pSurfaceSet;

        m_pMNE->m_sSurfaceDir = t_sSurfaceDir;

        m_pMNE->m_pRTSEOutput->measurementData()->setSurfSet(t_pSurfaceSet);

        ui.m_qLabel_surfaceStat->setText("loaded");
    }
    else
    {
        m_pMNE->m_pSurfaceSet = FsSurfaceSet::SPtr(new FsSurfaceSet());
        ui.m_qLabel_surfaceStat->setText("not loaded");
    }
}

//=============================================================================================================

void RtcMneSetupWidget::showMriHeadFileDialog()
{
    QString t_sMriHeadFile = QFileDialog::getOpenFileName(this,
                                                          tr("Select Mri-Head transformation"),
                                                          QString(),
                                                          tr("Fif Files (*.fif)"));

    QFile file(t_sMriHeadFile);

    FIFFLIB::FiffCoordTrans mriHeadTrans = FIFFLIB::FiffCoordTrans(file);

    if(!mriHeadTrans.isEmpty()) {
        m_pMNE->m_mriHeadTrans = mriHeadTrans;
        ui.m_qLineEdit_MriHeadTrans->setText(t_sMriHeadFile);
    }
}
