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
, m_pStcDataModel(QSharedPointer<StcDataModel>(new StcDataModel(this)))
{
    init(QString(), QString(), -1, QString(), QString(), QString());
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
: Qt3D::Window()
, m_pStcDataModel(QSharedPointer<StcDataModel>(new StcDataModel(this)))
{
    init(QString(), subject_id, hemi, surf, QString(), subjects_dir);
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir)
: Qt3D::Window()
, m_pStcDataModel(QSharedPointer<StcDataModel>(new StcDataModel(this)))
{
    init(QString(), subject_id, hemi, surf, atlas, subjects_dir);
}


//*************************************************************************************************************

BrainView::BrainView(const QString& p_sFile)
: Qt3D::Window()
, m_pStcDataModel(QSharedPointer<StcDataModel>(new StcDataModel(this)))
{
    init(p_sFile, QString(), -1, QString(), QString(), QString());
}


//*************************************************************************************************************

BrainView::~BrainView()
{
}


//*************************************************************************************************************

void BrainView::addSourceEstimate(MNESourceEstimate &p_sourceEstimate)
{
    std::cout<<"BrainView::addSourceEstimate()"<<std::endl;

    m_pStcDataModel->addData(p_sourceEstimate);
}


//*************************************************************************************************************

void BrainView::initStcDataModel(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, const QString &atlas, const MNEForwardSolution &forwardSolution)
{
    // Init stc data model
    m_pStcDataModel->init(subject_id, hemi, surf, subjects_dir, atlas, forwardSolution);

    //Set stc models to views
    m_pBrainSurfaceEntity->setModel(m_pStcDataModel);
}


//*************************************************************************************************************

void BrainView::init(const QString& p_sFile, const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir)
{
    Q_UNUSED(atlas)
    Q_UNUSED(p_sFile)

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
    m_pBrainSurfaceEntity = QSharedPointer<BrainSurface>(new BrainSurface(subject_id, hemi, surf, subjects_dir, m_pRootEntity));
    m_pBrainSurfaceEntity->setObjectName(QStringLiteral("m_pBrainSurfaceEntity"));

    // Light source
    Qt3D::QPointLight *light1 = new Qt3D::QPointLight();
    light1->setColor(Qt::white);
    light1->setIntensity(0.1f);
    m_pRootEntity->addComponent(light1);

    // Build Coordinate System
    //createCoordSystem(m_pRootEntity);

    // Camera
    Qt3D::QCamera *cameraEntity = new Qt3D::QCamera(m_pRootEntity);
    cameraEntity->setObjectName(QStringLiteral("cameraEntity"));

    cameraEntity->lens()->setPerspectiveProjection(60.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D(-0, 0, -1.0f));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));
    cameraEntity->setUpVector(QVector3D(0, 1, 0));
    m_pAspectInput->setCamera(cameraEntity);

    // FrameGraph
    Qt3D::QFrameGraph *frameGraph = new Qt3D::QFrameGraph();
    Qt3D::QForwardRenderer *forwardRenderer = new Qt3D::QForwardRenderer();
    forwardRenderer->setClearColor(QColor::fromRgbF(1.0, 1.0, 1.0, 1.0));
    forwardRenderer->setCamera(cameraEntity);
    frameGraph->setActiveFrameGraph(forwardRenderer);

    // Setting the FrameGraph
    m_pRootEntity->addComponent(frameGraph);

    // Set root object of the scene
    m_Engine.setRootEntity(m_pRootEntity);
}


//*************************************************************************************************************

void BrainView::createCoordSystem(QEntity *rootEntity)
{
    // X
    Qt3D::QCylinderMesh *XAxis = new Qt3D::QCylinderMesh();
    XAxis->setRadius(0.1f);
    XAxis->setLength(3);
    XAxis->setRings(100);
    XAxis->setSlices(20);

    m_XAxisEntity = QSharedPointer<Qt3D::QEntity>(new Qt3D::QEntity(rootEntity));
    m_XAxisEntity->addComponent(XAxis);

    QPhongMaterial *phongMaterialX = new QPhongMaterial();
    phongMaterialX->setDiffuse(QColor(255, 0, 0));
    phongMaterialX->setAmbient(Qt::gray);
    phongMaterialX->setSpecular(Qt::white);
    phongMaterialX->setShininess(50.0f);
    m_XAxisEntity->addComponent(phongMaterialX);

    // Y
    Qt3D::QCylinderMesh *YAxis = new Qt3D::QCylinderMesh();
    YAxis->setRadius(0.1f);
    YAxis->setLength(3);
    YAxis->setRings(100);
    YAxis->setSlices(20);

    Qt3D::QRotateTransform *rotationY = new Qt3D::QRotateTransform();
    Qt3D::QTransform *transformY = new Qt3D::QTransform();

    rotationY->setAngleDeg(90.0f);
    rotationY->setAxis(QVector3D(1, 0, 0));
    transformY->addTransform(rotationY);

    m_YAxisEntity = QSharedPointer<Qt3D::QEntity>(new Qt3D::QEntity(rootEntity));
    m_YAxisEntity->addComponent(YAxis);
    m_YAxisEntity->addComponent(transformY);

    QPhongMaterial *phongMaterialY = new QPhongMaterial();
    phongMaterialY->setDiffuse(QColor(0, 0, 255));
    phongMaterialY->setAmbient(Qt::gray);
    phongMaterialY->setSpecular(Qt::white);
    phongMaterialY->setShininess(50.0f);
    m_YAxisEntity->addComponent(phongMaterialY);

    // Z
    Qt3D::QCylinderMesh *ZAxis = new Qt3D::QCylinderMesh();
    ZAxis->setRadius(0.1f);
    ZAxis->setLength(3);
    ZAxis->setRings(100);
    ZAxis->setSlices(20);

    Qt3D::QRotateTransform *rotationZ = new Qt3D::QRotateTransform();
    Qt3D::QTransform *transformZ = new Qt3D::QTransform();

    rotationZ->setAngleDeg(90.0f);
    rotationZ->setAxis(QVector3D(0, 0, 1));
    transformZ->addTransform(rotationZ);

    m_ZAxisEntity = QSharedPointer<Qt3D::QEntity>(new Qt3D::QEntity(rootEntity));
    m_ZAxisEntity->addComponent(ZAxis);
    m_ZAxisEntity->addComponent(transformZ);

    QPhongMaterial *phongMaterialZ = new QPhongMaterial();
    phongMaterialZ->setDiffuse(QColor(0, 255, 0));
    phongMaterialZ->setAmbient(Qt::gray);
    phongMaterialZ->setSpecular(Qt::white);
    phongMaterialZ->setShininess(50.0f);
    m_ZAxisEntity->addComponent(phongMaterialZ);
}

void BrainView::mousePressEvent(QMouseEvent *e)
{
    std::cout<<"mouse click"<<std::endl;
    if(e->buttons() & Qt::RightButton)
    {
        std::cout<<"mouse click"<<std::endl;
    }
}
