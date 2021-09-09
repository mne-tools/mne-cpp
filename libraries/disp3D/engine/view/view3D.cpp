//=============================================================================================================
/**
 * @file     view3D.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3D.h"
#include "../model/items/common/types.h"
#include "../model/data3Dtreemodel.h"
#include "../model/3dhelpers/renderable3Dentity.h"
#include "customframegraph.h"
#include "../model/3dhelpers/geometrymultiplier.h"
#include "../model/materials/geometrymultipliermaterial.h"
#include "orbitalcameracontroller.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QDate>
#include <QTime>
#include <QDir>

#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAspectEngine>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QRenderCaptureReply>
#include <Qt3DExtras/QCylinderGeometry>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QTechniqueFilter>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QNoDraw>

#include <QObjectPicker>
#include <QPickingSettings>
#include <QRenderSettings>
#include <QPickEvent>
#include <QPickTriangleEvent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace CONNECTIVITYLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

View3D::View3D()
: Qt3DExtras::Qt3DWindow()
, m_pRootEntity(new Qt3DCore::QEntity())
, m_p3DObjectsEntity(new Qt3DCore::QEntity(m_pRootEntity))
, m_pLightEntity(new Qt3DCore::QEntity(m_pRootEntity))
, m_pCamera(this->camera())
, m_pMultiCam1(new Qt3DRender::QCamera)
, m_pMultiCam2(new Qt3DRender::QCamera)
, m_pMultiCam3(new Qt3DRender::QCamera)
, m_pPicker(new Qt3DRender::QObjectPicker(m_pRootEntity))
, m_pCamController(new OrbitalCameraController(m_pRootEntity))
{
    //Root entity
    this->setRootEntity(m_pRootEntity);

    initSingleView();

    //Only render new frames when needed
    this->renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::OnDemand);

    initLight();

    createCoordSystem(m_pRootEntity);
    toggleCoordAxis(false);

    // initialize object picking and disable it by default to save resources
    initObjectPicking();
    activatePicker(true);

    initMultiView();

    this->setRootEntity(m_pRootEntity);

//    this->setActiveFrameGraph(m_pFrameGraph);
    this->setActiveFrameGraph(m_pMultiFrame);
}


//=============================================================================================================

void View3D::initObjectPicking()
{
    // add object picker to root entity
    m_pRootEntity->addComponent(m_pPicker);

    // emit signal whenever pick event occured
    connect(m_pPicker, &Qt3DRender::QObjectPicker::pressed,
            this, &View3D::handlePickerPress);

    // define renderSettings
    this->renderSettings()->setActiveFrameGraph(m_pFrameGraph);
    this->renderSettings()->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::PrimitivePicking);
    this->renderSettings()->pickingSettings()->setPickResultMode(Qt3DRender::QPickingSettings::NearestPick);
    this->renderSettings()->pickingSettings()->setWorldSpaceTolerance(0.00000001f);
}

//=============================================================================================================

void View3D::activatePicker(const bool bActivatePicker)
{
    m_pPicker->setEnabled(bActivatePicker);
}

//=============================================================================================================

void View3D::handlePickerPress(Qt3DRender::QPickEvent *qPickEvent)
{
    // only catch click events for left mouse button
    if(qPickEvent->button() == qPickEvent->LeftButton) {
        emit pickEventOccured(qPickEvent);
    }
}

//=============================================================================================================

void View3D::initLight()
{
    //Setup light positions, intensities and color
    QList<QVector3D> lLightPositions;
    const QColor lightColor(255,255,255);
    const float lightIntensity = 0.4f;

    lLightPositions << QVector3D(-0.5,0,0) << QVector3D(0.5,0,0); /*<< QVector3D(0,0,-0.5)
                    << QVector3D(0.5,0,0) << QVector3D(0,0,0.5)
                    << QVector3D(0,0.5,0) << QVector3D(0,-0.5,0);*/

    //Create all the lights - make it shine
    for(int i = 0; i < lLightPositions.size(); ++i) {
        //Light source
        Qt3DCore::QEntity* pLightEntity = new Qt3DCore::QEntity(m_pLightEntity);

        Qt3DCore::QTransform* pTransform = new Qt3DCore::QTransform();
        pTransform->setTranslation(lLightPositions.at(i));
        pLightEntity->addComponent(pTransform);

        Qt3DRender::QPointLight *pPointLight = new Qt3DRender::QPointLight(pLightEntity);
        pPointLight->setColor(lightColor);
        pPointLight->setIntensity(lightIntensity);
        pLightEntity->addComponent(pPointLight);

        m_lLightSources.append(pPointLight);

        Qt3DExtras::QSphereMesh* lightSphere = new Qt3DExtras::QSphereMesh(pLightEntity);
        lightSphere->setRadius(0.1f);
        pLightEntity->addComponent(lightSphere);

        //Uncomment the following to visualize the light sources for debugging:
//        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(pLightEntity);
//        material->setAmbient(lightColor);
//        pLightEntity->addComponent(material);
    }
}

//=============================================================================================================

void View3D::setModel(QSharedPointer<Data3DTreeModel> pModel)
{
    pModel->getRootEntity()->setParent(m_p3DObjectsEntity);
}

//=============================================================================================================

void View3D::setSceneColor(const QColor& colSceneColor)
{
    m_pFrameGraph->setClearColor(colSceneColor);
}

//=============================================================================================================

void View3D::toggleCoordAxis(bool bChecked)
{
    m_pCoordSysEntity->setEnabled(bChecked);
}

//=============================================================================================================

void View3D::showFullScreen(bool bChecked)
{
    if(bChecked) {
        this->Qt3DWindow::showFullScreen();
    }
    else {
        this->showNormal();
    }
}

//=============================================================================================================

void View3D::setLightColor(const QColor &color)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i)->setColor(color);
    }
}

//=============================================================================================================

void View3D::setLightIntensity(double value)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i)->setIntensity(value);
    }
}

//=============================================================================================================

void View3D::takeScreenshot()
{
    if(m_pScreenCaptureReply) {
        if(!m_pScreenCaptureReply->isComplete()) {
            return;
        }
    }

    m_pScreenCaptureReply = m_pFrameGraph->requestRenderCaptureReply();

    if(!m_pScreenCaptureReply) {
        return;
    }

    QObject::connect(m_pScreenCaptureReply.data(), &Qt3DRender::QRenderCaptureReply::completed,
                     this, &View3D::saveScreenshot);
}

//=============================================================================================================

void View3D::saveScreenshot()
{
    if(!m_pScreenCaptureReply) {
        return;
    }

    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName = QString("./Screenshots/%1-%2-View3D.bmp").arg(sDate).arg(sTime);
    m_pScreenCaptureReply->saveImage(fileName);

    delete m_pScreenCaptureReply;
}

//=============================================================================================================

void View3D::keyPressEvent(QKeyEvent* e)
{
    if(e->key() == Qt::Key_Escape) {
        this->showNormal();
    }

    Qt3DWindow::keyPressEvent(e);
}

//=============================================================================================================

void View3D::createCoordSystem(Qt3DCore::QEntity* parent)
{
    m_pCoordSysEntity = QSharedPointer<Qt3DCore::QEntity>::create(parent);

    //create geometry
    QSharedPointer<Qt3DExtras::QCylinderGeometry> pAxis =  QSharedPointer<Qt3DExtras::QCylinderGeometry>::create();
    pAxis->setRadius(0.001f);
    pAxis->setLength(30);
    pAxis->setRings(100);
    pAxis->setSlices(20);

    //create mesh
    GeometryMultiplier *pCoordSysMesh = new GeometryMultiplier(pAxis);
    QVector<QColor> vColors;
    vColors.reserve(3);
    QVector<QMatrix4x4> vTransforms;
    vTransforms.reserve(3);
    QMatrix4x4 transformMat;

    // Y - red
    transformMat.setToIdentity();
    vTransforms.push_back(transformMat);
    vColors.push_back(QColor(255, 0, 0));

    // X - blue
    transformMat.setToIdentity();
    transformMat.rotate(90.0f, QVector3D(0,0,1));
    vTransforms.push_back(transformMat);
    vColors.push_back(QColor(0, 0, 255));

    // Z - green
    transformMat.setToIdentity();
    transformMat.rotate(90.0f, QVector3D(1,0,0));
    vTransforms.push_back(transformMat);
    vColors.push_back(QColor(0, 255, 0));

    //Set transforms and colors
    pCoordSysMesh->setTransforms(vTransforms);
    pCoordSysMesh->setColors(vColors);

    //Add material
    GeometryMultiplierMaterial* pCoordSysMaterial = new GeometryMultiplierMaterial;
    pCoordSysMaterial->setAmbient(QColor(0,0,0));
    pCoordSysMaterial->setAlpha(1.0f);

    m_pCoordSysEntity->addComponent(pCoordSysMesh);
    m_pCoordSysEntity->addComponent(pCoordSysMaterial);
}

//=============================================================================================================

void View3D::setCameraRotation(float fAngle)
{
    m_pCamera->setPosition(QVector3D(0.0f, -0.4f, -0.25f));
    m_pCamera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    m_pCamera->tiltAboutViewCenter(180);
    QQuaternion quat = QQuaternion::QQuaternion::fromEulerAngles(0,0,fAngle);
    m_pCamera->rotateAboutViewCenter(quat);
}

//=============================================================================================================

//Qt3DCore::QTransform View3D::getCameraTransform() {
//    Qt3DCore::QTransform trans = *new Qt3DCore::QTransform(m_pCamera->transform());
//    return trans;
//}

//=============================================================================================================

void View3D::startStopCameraRotation(bool bChecked)
{
    if(!m_pCameraAnimation) {
        m_pCameraAnimation = new QPropertyAnimation(m_pCamController, "rotating");
        m_pCameraAnimation->setStartValue(QVariant::fromValue(1));
        m_pCameraAnimation->setEndValue(QVariant::fromValue(10000));
        m_pCameraAnimation->setDuration(10000);
        m_pCameraAnimation->setLoopCount(-1);
    }

    if(bChecked) {
        //Start animation
        m_pCameraAnimation->start();
    }
    else {
        m_pCameraAnimation->stop();
    }
}

//=============================================================================================================

void View3D::initSingleCam()
{
    m_pCamera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pCamera->setPosition(QVector3D(0.0f, -0.4f, -0.25f));
    m_pCamera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    m_pCamera->tiltAboutViewCenter(180);
    m_pCamera->lens()->setPerspectiveProjection(45.0f, this->width()/this->height(), 0.01f, 5000.0f);
    m_pFrameGraph->setCamera(m_pCamera);

    m_pCamController->setCamera(m_pCamera);
}

//=============================================================================================================

void View3D::initMultiCams()
{
    m_pMultiCam1->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pMultiCam1->setPosition(QVector3D(0.0f, 0.4f, 0.0f));
    m_pMultiCam1->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pMultiCam1->setUpVector(QVector3D(0.0f, 0.0f, 1.0f));
//    m_pMultiCam1->setParent(m_pRootEntity);

    m_pMultiCam2->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pMultiCam2->setPosition(QVector3D(-0.4f, 0.0f, 0.0f));
    m_pMultiCam2->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pMultiCam2->setUpVector(QVector3D(0.0f, 0.0f, 1.0f));
//    m_pMultiCam2->setParent(m_pRootEntity);

    m_pMultiCam3->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pMultiCam3->setPosition(QVector3D(0.0f, 0.0f, 0.4f));
    m_pMultiCam3->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pMultiCam3->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
//    m_pMultiCam3->setParent(m_pRootEntity);

}

//=============================================================================================================

void View3D::initSingleView()
{
    //FrameGraph
    m_pFrameGraph = new CustomFrameGraph();
    m_pFrameGraph->setClearColor(QColor::fromRgbF(0.0, 0.0, 0.0, 1.0));

    initSingleCam();
}

//=============================================================================================================

void View3D::initMultiView()
{
    initMultiCams();

    // Framegraph root node
    m_pMultiFrame = new Qt3DRender::QRenderSurfaceSelector();
    auto mainViewPort = new Qt3DRender::QViewport(m_pMultiFrame);

    // First RenderView: clear buffers
    auto clearBuffers = new Qt3DRender::QClearBuffers(mainViewPort);
    clearBuffers->setBuffers(Qt3DRender::QClearBuffers::ColorDepthBuffer);
    clearBuffers->setClearColor(QColor::fromRgbF(0.0, 0.0, 0.0, 1.0));

    auto viewPort1 = new Qt3DRender::QViewport(mainViewPort);
    viewPort1->setNormalizedRect(QRectF(0.0f, 0.0f, 0.333f, 1.0f));
    auto cameraSelector1 = new Qt3DRender::QCameraSelector(viewPort1);
    cameraSelector1->setCamera(m_pMultiCam1);

    auto viewPort2 = new Qt3DRender::QViewport(mainViewPort);
    viewPort2->setNormalizedRect(QRectF(0.333f, 0.0f, 0.333f, 1.0f));
    auto cameraSelector2= new Qt3DRender::QCameraSelector(viewPort2);
    cameraSelector2->setCamera(m_pMultiCam2);

    auto viewPort3 = new Qt3DRender::QViewport(mainViewPort);
    viewPort3->setNormalizedRect(QRectF(0.666f, 0.0f, 0.333f, 1.0f));
    auto cameraSelector3= new Qt3DRender::QCameraSelector(viewPort3);
    cameraSelector3->setCamera(m_pMultiCam3);

    auto noDraw = new Qt3DRender::QNoDraw(clearBuffers);
}

//=============================================================================================================

void View3D::resizeEvent(QResizeEvent *event)
{
    //updateMultiViewAspectRatio();
    //single view updates implicitly due to use of default camera from Qt3DWindow.

    Qt3DWindow::resizeEvent(event);
}

//=============================================================================================================

void View3D::updateMultiViewAspectRatio()
{
    m_pMultiCam1->setAspectRatio(this->width()/(3 * this->height()));

    m_pMultiCam2->setAspectRatio(this->width()/(3 * this->height()));

    m_pMultiCam3->setAspectRatio(this->width()/(3 * this->height()));
}
