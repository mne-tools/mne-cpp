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

using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FSLIB;


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
, m_bCameraRotationMode(false)
, m_bCameraTransMode(false)
, m_vecCameraTrans(QVector3D(0.0,0.0,-0.5))
, m_vecCameraTransOld(QVector3D(0.0,0.0,-0.5))
, m_vecCameraRotation(QVector3D(0.0,0.0,0.0))
, m_vecCameraRotationOld(QVector3D(0.0,0.0,0.0))
{
    initMetatypes();
    init();
}


//*************************************************************************************************************

View3D::~View3D()
{
}


//*************************************************************************************************************

void View3D::initMetatypes()
{
    qRegisterMetaType<QByteArray>();

    qRegisterMetaType<Eigen::MatrixX3i>();
    qRegisterMetaType<Eigen::MatrixXd>();
    qRegisterMetaType<Eigen::MatrixX3f>();
    qRegisterMetaType<Eigen::VectorXf>();
    qRegisterMetaType<Eigen::VectorXi>();
    qRegisterMetaType<Eigen::VectorXd>();
    qRegisterMetaType<Eigen::RowVectorXf>();
    qRegisterMetaType<Eigen::Vector3f>();

    qRegisterMetaType<MatrixX3i>();
    qRegisterMetaType<MatrixXd>();
    qRegisterMetaType<MatrixX3f>();
    qRegisterMetaType<VectorXf>();
    qRegisterMetaType<VectorXi>();
    qRegisterMetaType<VectorXd>();
    qRegisterMetaType<RowVectorXf>();
    qRegisterMetaType<Vector3f>();
}


//*************************************************************************************************************

void View3D::init()
{
    //Aspect engine
    m_aspectEngine.registerAspect(new Qt3DRender::QRenderAspect());

    //m_aspectEngine.registerAspect(m_pInputAspect);

    //Data
    QVariantMap data;
    data.insert(QStringLiteral("surface"), QVariant::fromValue(static_cast<QSurface *>(this)));
    data.insert(QStringLiteral("eventSource"), QVariant::fromValue(this));
    m_aspectEngine.setData(data);

    // Light source
//    Qt3DRender::QPointLight *light1 = new Qt3DRender::QPointLight();
//    light1->setColor(Qt::white);
//    light1->setIntensity(0.1f);
//    m_pRootEntity->addComponent(light1);

    // Camera
    m_pCameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pCameraEntity->setPosition(m_vecCameraTrans);
    m_pCameraEntity->setUpVector(QVector3D(0, 1, 0));
    m_pCameraEntity->setViewCenter(QVector3D(0, 0, 0));
    //m_pInputAspect->setCamera(m_pCameraEntity);

    // FrameGraph
    m_pForwardRenderer = new Qt3DRender::QForwardRenderer();
    m_pForwardRenderer->setClearColor(QColor::fromRgbF(0.0, 0.0, 0.0, 1.0));
    m_pForwardRenderer->setCamera(m_pCameraEntity);
    m_pFrameGraph->setActiveFrameGraph(m_pForwardRenderer);

    //Init the transforms
    initTransformations();

    // Setting the FrameGraph
    m_pRootEntity->addComponent(m_pFrameGraph);

    // Set root object of the scene
    m_aspectEngine.setRootEntity(m_pRootEntity);

    //createCoordSystem(m_pRootEntity);
}


//*************************************************************************************************************

void View3D::initTransformations()
{
    // Initialize camera transforms
    m_pCameraTransform = new Qt3DCore::QTransform;
    m_pCameraTransform->setTranslation(m_vecCameraTrans);
    m_pCameraEntity->addComponent(m_pCameraTransform);
}


//*************************************************************************************************************

bool View3D::addBrainData(const QString& text, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet)
{
    return m_pBrain->addData(text, tSurfaceSet, tAnnotationSet);
}


//*************************************************************************************************************

bool View3D::addBrainData(const QString& text, const Surface& tSurface, const Annotation& tAnnotation)
{
    return m_pBrain->addData(text, tSurface, tAnnotation);
}


//*************************************************************************************************************

bool View3D::addBrainData(const QString& text, const MNESourceSpace& tSourceSpace)
{
    return m_pBrain->addData(text, tSourceSpace);
}


//*************************************************************************************************************

bool View3D::addBrainData(const QString& text, const MNEForwardSolution& tForwardSolution)
{
    return m_pBrain->addData(text, tForwardSolution.src);
}


//*************************************************************************************************************

QList<BrainRTSourceLocDataTreeItem*> View3D::addRtBrainData(const QString& text, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    return m_pBrain->addData(text, tSourceEstimate, tForwardSolution);
}


//*************************************************************************************************************

BrainTreeModel* View3D::getBrainTreeModel()
{
    return m_pBrain->getBrainTreeModel();
}


//*************************************************************************************************************

void View3D::setSceneColor(const QColor& colSceneColor)
{
    m_pForwardRenderer->setClearColor(colSceneColor);
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
            //TODO: SelectionMode
            break;
        case Qt::MidButton:
            m_bCameraRotationMode = true;
            break;
        case Qt::RightButton:
            m_bCameraTransMode = true;
            break;
    }

    Window::mousePressEvent(e);
}


//*************************************************************************************************************

void View3D::wheelEvent(QWheelEvent* e)
{
    if(e->angleDelta().y() > 0)
        m_vecCameraTrans.setZ(m_vecCameraTrans.z() + 0.05f);
    else
        m_vecCameraTrans.setZ(m_vecCameraTrans.z() - 0.05f);

    // Transform
    m_pCameraTransform->setTranslation(m_vecCameraTrans);

    Window::wheelEvent(e);
}


//*************************************************************************************************************

void View3D::mouseReleaseEvent(QMouseEvent* e)
{
    m_bCameraRotationMode = false;
    m_bCameraTransMode = false;
    m_vecCameraTransOld = m_vecCameraTrans;
    m_vecCameraRotationOld = m_vecCameraRotation;

    Window::mouseReleaseEvent(e);
}


//*************************************************************************************************************

void View3D::mouseMoveEvent(QMouseEvent* e)
{
    if(m_bCameraRotationMode) {
        m_vecCameraRotation.setX(((e->pos().y() - m_mousePressPositon.y()) * 0.1f) + m_vecCameraRotationOld.x());
        m_vecCameraRotation.setY(((e->pos().x() - m_mousePressPositon.x()) * 0.1f) + m_vecCameraRotationOld.y());

        m_pCameraTransform->setRotationX(m_vecCameraRotation.x());
        m_pCameraTransform->setRotationY(m_vecCameraRotation.y());
    }

    if(m_bCameraTransMode) {
        m_vecCameraTrans.setX(((e->pos().x() - m_mousePressPositon.x()) * 0.0001f) + m_vecCameraTransOld.x());
        m_vecCameraTrans.setY(((e->pos().y() - m_mousePressPositon.y()) * -0.0001f) + m_vecCameraTransOld.y());

        // Camera translation transform
        m_pCameraTransform->setTranslation(m_vecCameraTrans);
    }

    Window::mouseMoveEvent(e);
}


//*************************************************************************************************************

void View3D::createCoordSystem(Qt3DCore::QEntity* parent)
{
    // Y - red
    Qt3DRender::QCylinderMesh *YAxis = new Qt3DRender::QCylinderMesh();
    YAxis->setRadius(0.1f);
    YAxis->setLength(3);
    YAxis->setRings(100);
    YAxis->setSlices(20);

    m_YAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
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

    Qt3DCore::QTransform *transformZ = new Qt3DCore::QTransform();
    transformZ->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 90));

    m_ZAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
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

    Qt3DCore::QTransform *transformX = new Qt3DCore::QTransform();
    transformX->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0,0,1), 90));

    m_XAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
    m_XAxisEntity->addComponent(XAxis);
    m_XAxisEntity->addComponent(transformX);

    Qt3DRender::QPhongMaterial *phongMaterialX = new Qt3DRender::QPhongMaterial();
    phongMaterialX->setDiffuse(QColor(0, 255, 0));
    phongMaterialX->setAmbient(Qt::gray);
    phongMaterialX->setSpecular(Qt::white);
    phongMaterialX->setShininess(50.0f);
    m_XAxisEntity->addComponent(phongMaterialX);
}


