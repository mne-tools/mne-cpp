//=============================================================================================================
/**
 * @file     rtcmnesetupwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the RtcMneSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmnesetupwidget.h"

#include "../rtcmne.h"

#include <fs/annotationset.h>
#include <fs/surfaceset.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne_forwardsolution.h>

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

    AnnotationSet::SPtr t_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(t_sAtlasDir+"/lh.aparc.a2009s.annot", t_sAtlasDir+"/rh.aparc.a2009s.annot"));

    if(!t_pAnnotationSet->isEmpty() && t_pAnnotationSet->size() == 2)
    {
        m_pMNE->m_pAnnotationSet = t_pAnnotationSet;

        m_pMNE->m_sAtlasDir = t_sAtlasDir;

        m_pMNE->m_pRTSEOutput->measurementData()->setAnnotSet(t_pAnnotationSet);

        ui.m_qLabel_atlasStat->setText("loaded");
    }
    else
    {
        m_pMNE->m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet());
        ui.m_qLabel_atlasStat->setText("not loaded");
    }
}

//=============================================================================================================

void RtcMneSetupWidget::showSurfaceDirDialog()
{
    QString t_sSurfaceDir = QFileDialog::getExistingDirectory(  this, tr("Open Surface Directory"),
                                                                QString(),
                                                                QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);

    SurfaceSet::SPtr t_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet(t_sSurfaceDir+"/lh.orig", t_sSurfaceDir+"/rh.orig"));

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
        m_pMNE->m_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet());
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
