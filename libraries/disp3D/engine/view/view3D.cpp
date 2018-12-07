//=============================================================================================================
/**
* @file     view3D.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch, Lars Debor and Matti Hamalainen. All rights reserved.
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
#include "../model/items/common/types.h"
#include "../model/data3Dtreemodel.h"
#include "../model/3dhelpers/renderable3Dentity.h"
#include "customframegraph.h"
#include "../model/3dhelpers/geometrymultiplier.h"
#include "../model/materials/geometrymultipliermaterial.h"
#include "orbitalcameracontroller.h"


//*************************************************************************************************************
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
#include <Qt3DRender/QRenderCapture>
#include <Qt3DExtras/QCylinderGeometry>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QRenderSettings>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

View3D::View3D()
: Qt3DExtras::Qt3DWindow()
, m_pRootEntity(new Qt3DCore::QEntity())
, m_p3DObjectsEntity(new Qt3DCore::QEntity(m_pRootEntity))
, m_pLightEntity(new Qt3DCore::QEntity(m_pRootEntity))
, m_pCamera(this->camera())
, m_pCapture(new Qt3DRender::QRenderCapture)
{
    //Root entity
    this->setRootEntity(m_pRootEntity);

    //FrameGraph
    m_pFrameGraph = new CustomFrameGraph();
    m_pFrameGraph->setParent(m_pCapture);
    m_pFrameGraph->setClearColor(QColor::fromRgbF(0.0, 0.0, 0.0, 1.0));
    this->setActiveFrameGraph(m_pCapture);

    //Only render new frames when needed
    this->renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::OnDemand);

    initLight();

    m_pCamera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pCamera->setPosition(QVector3D(0.0f, -0.4f, -0.25f));
    m_pCamera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    m_pCamera->tiltAboutViewCenter(180);
    m_pFrameGraph->setCamera(m_pCamera);

    OrbitalCameraController *pCamController = new OrbitalCameraController(m_pRootEntity);
    pCamController->setCamera(m_pCamera);

    createCoordSystem(m_pRootEntity);
    toggleCoordAxis(false);
}


//*************************************************************************************************************

void View3D::initLight()
{
    //Setup light positions, intensities and color
    QList<QVector3D> lLightPositions;
    const QColor lightColor(255,255,255);
    const float lightIntensity = 0.2f;

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

        //Uncomment the following to visualize the light sources for debugging:
//        Qt3DExtras::QSphereMesh* lightSphere = new Qt3DExtras::QSphereMesh(pLightEntity);
//        lightSphere->setRadius(0.1f);
//        pLightEntity->addComponent(lightSphere);
//        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(pLightEntity);
//        material->setAmbient(lightColor);
//        pLightEntity->addComponent(material);
    }
}


//*************************************************************************************************************

void View3D::setModel(QSharedPointer<Data3DTreeModel> pModel)
{
    pModel->getRootEntity()->setParent(m_p3DObjectsEntity);
}


//*************************************************************************************************************

void View3D::setSceneColor(const QColor& colSceneColor)
{
    m_pFrameGraph->setClearColor(colSceneColor);
}


//*************************************************************************************************************

void View3D::toggleCoordAxis(bool checked)
{
    m_pCoordSysEntity->setEnabled(checked);
}


//*************************************************************************************************************

void View3D::showFullScreen(bool checked)
{
    if(checked) {
        this->Qt3DWindow::showFullScreen();
    }
    else {
        this->showNormal();
    }
}


//*************************************************************************************************************

void View3D::setLightColor(const QColor &color)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i)->setColor(color);
    }
}


//*************************************************************************************************************

void View3D::setLightIntensity(double value)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i)->setIntensity(value);
    }
}


//*************************************************************************************************************

void View3D::takeScreenshot()
{
    if(!m_pCapture) {
        return;
    }

    m_pReply = m_pCapture->requestCapture();

    QObject::connect(m_pReply.data(), &Qt3DRender::QRenderCaptureReply::completed,
                     this, &View3D::saveScreenshot);
}


//*************************************************************************************************************

void View3D::saveScreenshot()
{
    if(!m_pReply) {
        return;
    }

    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName = QString("./Screenshots/%1-%2-View3D.bmp").arg(sDate).arg(sTime);
    QImage image = m_pReply->image();

    bool flag = image.save(fileName);
    qDebug() << "image.rect()" << image.rect();
    qDebug() << "flag" << flag;

    delete m_pReply;
}


//*************************************************************************************************************

void View3D::keyPressEvent(QKeyEvent* e)
{
    if(e->key() == Qt::Key_Escape) {
        this->showNormal();
    }

    Qt3DWindow::keyPressEvent(e);
}


//*************************************************************************************************************

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


//*************************************************************************************************************

void View3D::startModelRotationRecursive(QObject* pObject)
{
    //TODO this won't work with QEntities
    if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(pObject)) {
        QPropertyAnimation *anim = new QPropertyAnimation(pItem, QByteArrayLiteral("rotZ"));
        anim->setDuration(30000);
        anim->setStartValue(QVariant::fromValue(pItem->rotZ()));
        anim->setEndValue(QVariant::fromValue(pItem->rotZ() + 360.0f));
        anim->setLoopCount(-1);
        anim->start();
        m_lPropertyAnimations << anim;
    }

    for(int i = 0; i < pObject->children().size(); ++i) {
        startModelRotationRecursive(pObject->children().at(i));
    }
}


//*************************************************************************************************************

void View3D::startStopModelRotation(bool checked)
{
    if(checked) {
        //Start animation
        m_lPropertyAnimations.clear();

        for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
            startModelRotationRecursive(m_p3DObjectsEntity->children().at(i));
        }
    }
    else {
        for(int i = 0; i < m_lPropertyAnimations.size(); ++i) {
            m_lPropertyAnimations.at(i)->stop();
        }
    }
}
