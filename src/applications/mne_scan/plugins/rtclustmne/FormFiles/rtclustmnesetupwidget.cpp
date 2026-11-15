//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtclustmnesetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RtClustMneSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtclustmnesetupwidget.h"
#include <memory>

#include "../rtclustmne.h"

#include <fs/fs_annotationset.h>
#include <fs/fs_surfaceset.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne_forward_solution.h>
#include <base/string_utils.h>

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

using namespace RTCLUSTMNEPLUGIN;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtClustMneSetupWidget::RtClustMneSetupWidget(RtClustMne* toolbox, QWidget* parent)
: QWidget(parent)
, m_pMNE(toolbox)
{
    ui.setupUi(this);

    ui.m_qLineEdit_AtlasDirName->setText(m_pMNE->m_sAtlasDir);
    if (m_pMNE->m_pAnnotationSet->empty())
        ui.m_qLabel_atlasStat->setText("not loaded");
    else
        ui.m_qLabel_atlasStat->setText("loaded");

    ui.m_qLineEdit_SurfaceDirName->setText(m_pMNE->m_sSurfaceDir);
    if (m_pMNE->m_pSurfaceSet->empty())
        ui.m_qLabel_surfaceStat->setText("not loaded");
    else
        ui.m_qLabel_surfaceStat->setText("loaded");

    ui.m_qLineEdit_MriHeadTrans->setText(m_pMNE->m_fMriHeadTrans.fileName());

    connect(ui.m_qPushButton_AtlasDirDialog, &QPushButton::released, this, &RtClustMneSetupWidget::showAtlasDirDialog);
    connect(ui.m_qPushButton_SurfaceDirDialog, &QPushButton::released, this, &RtClustMneSetupWidget::showSurfaceDirDialog);
    connect(ui.m_qPushButton_MriHeadTrans, &QPushButton::released, this, &RtClustMneSetupWidget::showMriHeadFileDialog);
}

//=============================================================================================================

RtClustMneSetupWidget::~RtClustMneSetupWidget()
{
}

//=============================================================================================================

void RtClustMneSetupWidget::showAtlasDirDialog()
{
    QString t_sAtlasDir = QFileDialog::getExistingDirectory(this, tr("Open Atlas Directory"),
                                                            QString(),
                                                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    m_pMNE->m_sAtlasDir = t_sAtlasDir;

    ui.m_qLineEdit_AtlasDirName->setText(m_pMNE->m_sAtlasDir);

    std::shared_ptr<FsAnnotationSet> t_pAnnotationSet = std::make_shared<FsAnnotationSet>((t_sAtlasDir + "/lh.aparc.a2009s.annot").toStdU16String(), (t_sAtlasDir + "/rh.aparc.a2009s.annot").toStdU16String());

    if (!t_pAnnotationSet->empty() && t_pAnnotationSet->size() == 2) {
        m_pMNE->m_pAnnotationSet = t_pAnnotationSet;

        m_pMNE->m_sAtlasDir = t_sAtlasDir;

        m_pMNE->m_pRTSEOutput->measurementData()->setAnnotSet(t_pAnnotationSet);

        ui.m_qLabel_atlasStat->setText("loaded");
    } else {
        m_pMNE->m_pAnnotationSet = std::make_shared<FsAnnotationSet>();
        ui.m_qLabel_atlasStat->setText("not loaded");
    }
}

//=============================================================================================================

void RtClustMneSetupWidget::showSurfaceDirDialog()
{
    QString t_sSurfaceDir = QFileDialog::getExistingDirectory(this, tr("Open FsSurface Directory"),
                                                              QString(),
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    std::shared_ptr<FsSurfaceSet> t_pSurfaceSet = std::make_shared<FsSurfaceSet>((t_sSurfaceDir + "/lh.orig").toStdU16String(), (t_sSurfaceDir + "/rh.orig").toStdU16String());

    ui.m_qLineEdit_SurfaceDirName->setText(t_sSurfaceDir);

    if (!t_pSurfaceSet->empty() && t_pSurfaceSet->size() == 2) {
        m_pMNE->m_pSurfaceSet = t_pSurfaceSet;

        m_pMNE->m_sSurfaceDir = t_sSurfaceDir;

        m_pMNE->m_pRTSEOutput->measurementData()->setSurfSet(t_pSurfaceSet);

        ui.m_qLabel_surfaceStat->setText("loaded");
    } else {
        m_pMNE->m_pSurfaceSet = std::make_shared<FsSurfaceSet>();
        ui.m_qLabel_surfaceStat->setText("not loaded");
    }
}

//=============================================================================================================

void RtClustMneSetupWidget::showMriHeadFileDialog()
{
    QString t_sMriHeadFile = QFileDialog::getOpenFileName(this,
                                                          tr("Select Mri-Head transformation"),
                                                          QString(),
                                                          tr("Fif Files (*.fif)"));


    const auto mriHeadTrans = mne::FiffCoordTrans::read(t_sMriHeadFile.toStdU16String());
    if (mriHeadTrans) {
        m_pMNE->m_mriHeadTrans = *mriHeadTrans;
        ui.m_qLineEdit_MriHeadTrans->setText(t_sMriHeadFile);
    }
}
