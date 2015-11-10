//=============================================================================================================
/**
* @file     view3D.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    View3D class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3D.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

View3D::View3D()
: Window()
, m_pRootEntity(new Qt3DCore::QEntity())
, m_pInputAspect(new Qt3DInput::QInputAspect())
, m_pCameraEntity(new Qt3DCore::QCamera(m_pRootEntity))
, m_pFrameGraph(new Qt3DRender::QFrameGraph())
, m_pForwardRenderer(new Qt3DRender::QForwardRenderer())
, m_pBrain(Brain::SPtr(new Brain(m_pRootEntity)))
, m_bRotationMode(false)
, m_bZoomMode(false)
, m_bTransMode(false)
, m_fCameraZoom(1.0f)
, m_fCameraZoomOld(1.0f)
, m_fCameraTransX(0.0f)
, m_fCameraTransXOld(0.0f)
, m_fCameraTransY(-0.5f)
, m_fCameraTransYOld(-0.5f)
, m_fCameraRotationX(0.0f)
, m_fCameraRotationXOld(0.0f)
, m_fCameraRotationY(0.0f)
, m_fCameraRotationYOld(0.0f)
, m_fRootEntityRotationX(-90.0f)
, m_fRootEntityRotationXOld(-90.0f)
, m_fRootEntityRotationY(130.0f)
, m_fRootEntityRotationYOld(130.0f)
{
    init();
}


//*************************************************************************************************************

View3D::~View3D()
{
    delete m_pRootEntity;
    delete m_pInputAspect;
    delete m_pCameraEntity;
    delete m_pFrameGraph;
    delete m_pForwardRenderer;

    delete m_pCameraTransform;
    delete m_pCameraScaleTransform;
    delete m_pCameraTranslateTransform;
    delete m_pCameraRotateTransformX;
    delete m_pCameraRotateTransformY;

    delete m_pRootEntityTransform;
    delete m_pRootEntityRotateTransformX;
    delete m_pRootEntityRotateTransformY;
}


//*************************************************************************************************************

void View3D::init()
{
    //Aspect engine
    m_aspectEngine.registerAspect(new Qt3DRender::QRenderAspect());

//    m_aspectEngine.registerAspect(m_pInputAspect);
    m_aspectEngine.initialize();

    //Data
    QVariantMap data;
    data.insert(QStringLiteral("surface"), QVariant::fromValue(static_cast<QSurface *>(this)));
    data.insert(QStringLiteral("eventSource"), QVariant::fromValue(this));
    m_aspectEngine.setData(data);

    // Light source
    Qt3DRender::QPointLight *light1 = new Qt3DRender::QPointLight();
    light1->setColor(Qt::white);
    light1->setIntensity(0.1f);
    m_pRootEntity->addComponent(light1);

    // Camera
    m_pCameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.001f, 100000.0f);
    m_pCameraEntity->setPosition(QVector3D(m_fCameraTransXOld, m_fCameraTransYOld, -2.0f));
    m_pCameraEntity->setUpVector(QVector3D(0, 1, 0));
    m_pCameraEntity->setViewCenter(QVector3D(1, 0, 0));
    //m_pInputAspect->setCamera(m_pCameraEntity);

    // FrameGraph
    m_pForwardRenderer = new Qt3DRender::QForwardRenderer();
    m_pForwardRenderer->setClearColor(QColor::fromRgbF(0.0, 0.5, 1.0, 1.0));
    m_pForwardRenderer->setCamera(m_pCameraEntity);
    m_pFrameGraph->setActiveFrameGraph(m_pForwardRenderer);

    // Initialize camera transform
    m_pCameraTransform = new Qt3DCore::QTransform;

    m_pCameraScaleTransform = new Qt3DCore::QScaleTransform;
    m_pCameraScaleTransform->setScale(m_fCameraZoomOld);
    m_pCameraTransform->addTransform(m_pCameraScaleTransform);

    m_pCameraTranslateTransform = new Qt3DCore::QTranslateTransform;
    m_pCameraTranslateTransform->setDx(m_fCameraTransXOld);
    m_pCameraTranslateTransform->setDy(m_fCameraTransYOld);
    m_pCameraTranslateTransform->setDz(-2);
    m_pCameraTransform->addTransform(m_pCameraTranslateTransform);

    m_pCameraRotateTransformX = new Qt3DCore::QRotateTransform;
    m_pCameraRotateTransformX->setAngleDeg(m_fCameraRotationXOld);
    m_pCameraRotateTransformX->setAxis(QVector3D(1,0,0));
    m_pCameraTransform->addTransform(m_pCameraRotateTransformX);

    m_pCameraRotateTransformY = new Qt3DCore::QRotateTransform;
    m_pCameraRotateTransformY->setAngleDeg(m_fCameraRotationYOld);
    m_pCameraRotateTransformY->setAxis(QVector3D(0,1,0));
    m_pCameraTransform->addTransform(m_pCameraRotateTransformY);

    m_pCameraEntity->addComponent(m_pCameraTransform);

    // Initialize root entity transform
    m_pRootEntityTransform = new Qt3DCore::QTransform;

    m_pRootEntityRotateTransformX = new Qt3DCore::QRotateTransform;
    m_pRootEntityRotateTransformX->setAngleDeg(m_fRootEntityRotationXOld);
    m_pRootEntityRotateTransformX->setAxis(QVector3D(1,0,0));
    m_pRootEntityTransform->addTransform(m_pRootEntityRotateTransformX);

    m_pRootEntityRotateTransformY = new Qt3DCore::QRotateTransform;
    m_pRootEntityRotateTransformY->setAngleDeg(m_fRootEntityRotationYOld);
    m_pRootEntityRotateTransformY->setAxis(QVector3D(0,1,0));
    m_pRootEntityTransform->addTransform(m_pRootEntityRotateTransformY);

    m_pBrain->addComponent(m_pRootEntityTransform);

    // Setting the FrameGraph
    m_pRootEntity->addComponent(m_pFrameGraph);

    // Set root object of the scene
    m_aspectEngine.setRootEntity(m_pRootEntity);

    createCoordSystem(m_pRootEntity);
}


//*************************************************************************************************************

bool View3D::addFsBrainData(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
{
    bool state = m_pBrain->addFsBrainData(subject_id, hemi, surf, subjects_dir);

    return state;
}


//*************************************************************************************************************

void View3D::keyPressEvent(QKeyEvent* e)
{
    qDebug()<<"key press";
    switch ( e->key() )
    {
        case Qt::Key_Space:
            //Translate
            break;

        default:
            Window::keyPressEvent(e);
    }
}


//*************************************************************************************************************

void View3D::mousePressEvent(QMouseEvent* e)
{
    m_mousePressPositon = e->pos();

    switch (e->button()) {
        case Qt::LeftButton:
            m_bRotationMode = true;
            break;
        case Qt::MidButton:
            m_bTransMode = true;
            break;
        case Qt::RightButton:
            m_bZoomMode = true;
            break;
    }

    Window::mousePressEvent(e);
}


//*************************************************************************************************************

void View3D::mouseReleaseEvent(QMouseEvent *e)
{
    m_bRotationMode = false;
    m_bZoomMode = false;
    m_bTransMode = false;
    m_fRootEntityRotationXOld = m_fRootEntityRotationX;
    m_fRootEntityRotationYOld = m_fRootEntityRotationY;
    m_fCameraTransXOld = m_fCameraTransX;
    m_fCameraTransYOld = m_fCameraTransY;
    m_fCameraZoomOld  = m_fCameraZoom;

    Window::mouseReleaseEvent(e);
}


//*************************************************************************************************************

void View3D::mouseMoveEvent(QMouseEvent* e)
{
    if(m_bRotationMode) {
        m_fRootEntityRotationX = ((e->pos().y() - m_mousePressPositon.y()) * -0.1f ) + m_fRootEntityRotationXOld;
        m_fRootEntityRotationY = ((e->pos().x() - m_mousePressPositon.x()) * 0.1f ) + m_fRootEntityRotationYOld;

        // Transform
        m_pRootEntityRotateTransformX->setAngleDeg(m_fRootEntityRotationX);
        m_pRootEntityRotateTransformY->setAngleDeg(m_fRootEntityRotationY);
    }

    if(m_bTransMode) {
        m_fCameraTransX = ((e->pos().x() - m_mousePressPositon.x()) * 0.001f) + m_fCameraTransXOld;
        m_fCameraTransY = ((e->pos().y() - m_mousePressPositon.y()) * -0.001f ) + m_fCameraTransYOld;

        // Transform
        m_pCameraTranslateTransform->setDx(m_fCameraTransX);
        m_pCameraTranslateTransform->setDy(m_fCameraTransY);
        //m_pCameraTranslateTransform->setDz(m_pCameraTranslateTransform->dz());
    }

    if(m_bZoomMode) {
        m_fCameraZoom = ((e->pos().x() - m_mousePressPositon.x()) * 0.001f) + m_fCameraZoomOld;

        qDebug()<<"m_fCameraZoom"<<m_fCameraZoom;

        // Transform
        if(m_fCameraZoom > 0)
            m_pCameraScaleTransform->setScale(m_fCameraZoom);
    }

    qDebug()<< m_pCameraTransform->matrix();

    qDebug()<< m_pRootEntityTransform->matrix();

    Window::mouseMoveEvent(e);
}


//*************************************************************************************************************

void View3D::createCoordSystem(Qt3DCore::QEntity *rootEntity)
{
    // Y - red
    Qt3DRender::QCylinderMesh *YAxis = new Qt3DRender::QCylinderMesh();
    YAxis->setRadius(0.1f);
    YAxis->setLength(3);
    YAxis->setRings(100);
    YAxis->setSlices(20);

    m_YAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(rootEntity));
    m_YAxisEntity->addComponent(YAxis);

    Qt3DRender::QPhongMaterial *phongMaterialY = new Qt3DRender::QPhongMaterial();
    phongMaterialY->setDiffuse(QColor(255, 0, 0));
    phongMaterialY->setAmbient(Qt::gray);
    phongMaterialY->setSpecular(Qt::white);
    phongMaterialY->setShininess(50.0f);
    m_YAxisEntity->addComponent(phongMaterialY);

    // Z - blue
    Qt3DRender::QCylinderMesh *ZAxis = new Qt3DRender::QCylinderMesh();
    ZAxis->setRadius(0.1f);
    ZAxis->setLength(3);
    ZAxis->setRings(100);
    ZAxis->setSlices(20);

    Qt3DCore::QRotateTransform *rotationZ = new Qt3DCore::QRotateTransform();
    Qt3DCore::QTransform *transformZ = new Qt3DCore::QTransform();

    rotationZ->setAngleDeg(90.0f);
    rotationZ->setAxis(QVector3D(1, 0, 0));
    transformZ->addTransform(rotationZ);

    m_ZAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(rootEntity));
    m_ZAxisEntity->addComponent(ZAxis);
    m_ZAxisEntity->addComponent(transformZ);

    Qt3DRender::QPhongMaterial *phongMaterialZ = new Qt3DRender::QPhongMaterial();
    phongMaterialZ->setDiffuse(QColor(0, 0, 255));
    phongMaterialZ->setAmbient(Qt::gray);
    phongMaterialZ->setSpecular(Qt::white);
    phongMaterialZ->setShininess(50.0f);
    m_ZAxisEntity->addComponent(phongMaterialZ);

    // X - green
    Qt3DRender::QCylinderMesh *XAxis = new Qt3DRender::QCylinderMesh();
    XAxis->setRadius(0.1f);
    XAxis->setLength(3);
    XAxis->setRings(100);
    XAxis->setSlices(20);

    Qt3DCore::QRotateTransform *rotationX = new Qt3DCore::QRotateTransform();
    Qt3DCore::QTransform *transformX = new Qt3DCore::QTransform();

    rotationX->setAngleDeg(90.0f);
    rotationX->setAxis(QVector3D(0, 0, 1));
    transformX->addTransform(rotationX);

    m_XAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(rootEntity));
    m_XAxisEntity->addComponent(XAxis);
    m_XAxisEntity->addComponent(transformX);

    Qt3DRender::QPhongMaterial *phongMaterialX = new Qt3DRender::QPhongMaterial();
    phongMaterialX->setDiffuse(QColor(0, 255, 0));
    phongMaterialX->setAmbient(Qt::gray);
    phongMaterialX->setSpecular(Qt::white);
    phongMaterialX->setShininess(50.0f);
    m_XAxisEntity->addComponent(phongMaterialX);
}


