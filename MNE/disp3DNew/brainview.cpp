//=============================================================================================================
/**
* @file     brainview.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the BrainView class with qt3d 2.0 support.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainview.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMouseEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainView::BrainView()
: Qt3D::Window()
{
    init(QString(), QString(), -1, QString(), QString(), QString());
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
: Qt3D::Window()
{
    init(QString(), subject_id, hemi, surf, QString(), subjects_dir);
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir)
: Qt3D::Window()
{
    init(QString(), subject_id, hemi, surf, atlas, subjects_dir);
}


//*************************************************************************************************************

BrainView::BrainView(const QString& p_sFile)
: Qt3D::Window()
{
    init(p_sFile, QString(), -1, QString(), QString(), QString());
}


//*************************************************************************************************************

BrainView::~BrainView()
{
}


//*************************************************************************************************************

void BrainView::init(const QString& p_sFile, const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir)
{
    m_Engine.registerAspect(new Qt3D::QRenderAspect());
    Qt3D::QInputAspect *m_pAspectInput = new Qt3D::QInputAspect;
    m_Engine.registerAspect(m_pAspectInput);
    m_Engine.initialize();

    m_data.insert(QStringLiteral("surface"), QVariant::fromValue(static_cast<QSurface *>(this)));
    m_data.insert(QStringLiteral("eventSource"), QVariant::fromValue(this));
    m_Engine.setData(m_data);

    // Root entity
    m_pRootEntity = new Qt3D::QEntity();
    m_pRootEntity->setObjectName(QStringLiteral("m_pRootEntity"));

    // Surface
    m_pBrainSurfaceEntity = new BrainSurface(subject_id, hemi, surf, subjects_dir, m_pRootEntity);

    // Camera
    Qt3D::QCamera *cameraEntity = new Qt3D::QCamera(m_pRootEntity);
    cameraEntity->setObjectName(QStringLiteral("cameraEntity"));

    cameraEntity->lens()->setPerspectiveProjection(60.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D(-5, 0, -20.0f));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));
    cameraEntity->setUpVector(QVector3D(0, 1, 0));
    m_pAspectInput->setCamera(cameraEntity);

    // FrameGraph
    Qt3D::QFrameGraph *frameGraph = new Qt3D::QFrameGraph();
    Qt3D::QTechniqueFilter *techniqueFilter = new Qt3D::QTechniqueFilter();
    Qt3D::QViewport *viewport = new Qt3D::QViewport(techniqueFilter);
    Qt3D::QClearBuffer *clearBuffer = new Qt3D::QClearBuffer(viewport);
    Qt3D::QCameraSelector *cameraSelector = new Qt3D::QCameraSelector(clearBuffer);
    (void) new Qt3D::QRenderPassFilter(cameraSelector);

    // TechiqueFilter and renderPassFilter are not implement yet
    viewport->setRect(QRectF(0, 0, 1, 1));
    clearBuffer->setBuffers(Qt3D::QClearBuffer::ColorDepthBuffer);
    cameraSelector->setCamera(cameraEntity);
    frameGraph->setActiveFrameGraph(techniqueFilter);

    // Setting the FrameGraph
    m_pRootEntity->addComponent(frameGraph);

    // Set root object of the scene
    m_Engine.setRootEntity(m_pRootEntity);
}

