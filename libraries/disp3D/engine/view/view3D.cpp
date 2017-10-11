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
#include "../model/3dhelpers/renderable3Dentity.h"
#include "../model/items/common/abstract3Dtreeitem.h"
#include "../model/items/common/types.h"
#include "../model/data3Dtreemodel.h"

#include <mne/mne_sourceestimate.h>
#include <fiff/fiff_dig_point_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QPropertyAnimation>
#include <QKeyEvent>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DRender/QCamera>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QSphereMesh>


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
, m_pCameraEntity(this->camera())
, m_bCameraRotationMode(false)
, m_bCameraTransMode(false)
, m_bModelRotationMode(false)
, m_vecViewTrans(QVector3D(0.0f,-0.025f,-0.25f))
, m_vecViewTransOld(QVector3D(0.0,0.0,-0.5))
, m_vecViewRotation(QVector3D(-90.0,130.0,0.0))
, m_vecViewRotationOld(QVector3D(-90.0,130.0,0.0))
, m_pCameraTransform(new Qt3DCore::QTransform())
{
    init();
}


//*************************************************************************************************************

View3D::~View3D()
{
}


//*************************************************************************************************************

void View3D::init()
{
    //Create the lights
    initLight();

    // Camera
    m_pCameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pCameraEntity->setUpVector(QVector3D(0, 1, 0));
    m_pCameraEntity->setViewCenter(QVector3D(0, 0, 0));

    Qt3DExtras::QFirstPersonCameraController *camController = new Qt3DExtras::QFirstPersonCameraController(m_pRootEntity);
    camController->setCamera(m_pCameraEntity);

    this->defaultFrameGraph()->setClearColor(QColor::fromRgbF(0.0, 0.0, 0.0, 0.5));

    //Init the transforms
    initTransformations();

    // Set root object of the scene
    this->setRootEntity(m_pRootEntity);

    //Create coordinate system and hide as default
    createCoordSystem(m_p3DObjectsEntity);
    toggleCoordAxis(false);
}


//*************************************************************************************************************

void View3D::initLight()
{
    //Setup light positions, intensities and color
    QList<QVector3D> lLightPositions;
    QList<QVector3D> lLightDirections;
    QList<float> lLightIntensities;
    QList<QColor> lLightColor;

    QColor lightColor(255,255,255);
    float lightIntensity = 0.2f;

    lLightPositions << QVector3D(-0.5,0,0);// << QVector3D(0,0,-0.5) << QVector3D(0.5,0,0) << QVector3D(-0.5,0,0) << QVector3D(0,0.5,0) << QVector3D(0,-0.5,0);
    lLightDirections << QVector3D(0.5,0,0) << QVector3D(0,0,0.5) << QVector3D(-0.5,0,0) << QVector3D(0.5,0,0) << QVector3D(0,-0.5,0) << QVector3D(0,0.5,0);
    lLightIntensities << lightIntensity << lightIntensity << lightIntensity << lightIntensity << lightIntensity << lightIntensity;
    lLightColor << lightColor << lightColor << lightColor << lightColor << lightColor << lightColor;

    //Create all the lights - make it shine
    for(int i = 0; i < lLightPositions.size(); ++i) {
        //Light source
        Qt3DCore::QEntity* enitityLight = new Qt3DCore::QEntity(m_pLightEntity);
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
        QMatrix4x4 m;
        m.translate(lLightPositions.at(i));
        transform->setMatrix(m);

        enitityLight->addComponent(transform);

        Qt3DRender::QPointLight *light1 = new Qt3DRender::QPointLight(enitityLight);
        light1->setColor(lLightColor.at(i));
        //light1->setWorldDirection(lLightDirections.at(i));
        light1->setIntensity(lLightIntensities.at(i));
        enitityLight->addComponent(light1);

//        Qt3DExtras::QSphereMesh* lightSphere = new Qt3DExtras::QSphereMesh(enitityLight);
//        lightSphere->setRadius(0.1f);
//        enitityLight->addComponent(lightSphere);

        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(enitityLight);
        material->setAmbient(lLightColor.at(i));
        enitityLight->addComponent(material);

        QPair<Qt3DRender::QPointLight*, Qt3DExtras::QPhongMaterial*> pair;
        pair.first = light1;
        pair.second = material;
        m_lLightSources.append(pair);
    }
}


//*************************************************************************************************************

void View3D::initTransformations()
{
    // Initialize camera transforms
    m_pCameraTransform->setTranslation(m_vecViewTrans);
    m_pCameraTransform->setRotationX(m_vecViewRotation.x());
    m_pCameraTransform->setRotationY(m_vecViewRotation.y());
    m_pCameraTransform->setRotationZ(m_vecViewRotation.z());

    m_pCameraEntity->addComponent(m_pCameraTransform);
}


//*************************************************************************************************************

void View3D::setModel(Data3DTreeModel::SPtr pModel)
{
    pModel->getRootEntity()->setParent(m_p3DObjectsEntity);
}


//*************************************************************************************************************

void View3D::setSceneColor(const QColor& colSceneColor)
{
    this->defaultFrameGraph()->setClearColor(colSceneColor);
}


//*************************************************************************************************************

void View3D::startModelRotationRecursive(QObject* pObject)
{
    //TODO this won't work with QEntities
    if(Abstract3DTreeItem* pItem = dynamic_cast<Abstract3DTreeItem*>(pObject)) {
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
    } else {
        for(int i = 0; i < m_lPropertyAnimations.size(); ++i) {
            m_lPropertyAnimations.at(i)->stop();
        }
    }
}


//*************************************************************************************************************

void View3D::toggleCoordAxis(bool checked)
{
    m_XAxisEntity->setEnabled(checked);
    m_YAxisEntity->setEnabled(checked);
    m_ZAxisEntity->setEnabled(checked);
}


//*************************************************************************************************************

void View3D::showFullScreen(bool checked)
{
    if(checked) {
        this->Qt3DWindow::showFullScreen();
    } else {
        this->showNormal();
    }
}


//*************************************************************************************************************

void View3D::setLightColor(QColor color)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i).first->setColor(color);
        m_lLightSources.at(i).second->setAmbient(color);
    }
}


//*************************************************************************************************************

void View3D::setLightIntensity(double value)
{
    for(int i = 0; i < m_lLightSources.size(); ++i) {
        m_lLightSources.at(i).first->setIntensity(value);
    }
}


//*************************************************************************************************************

void View3D::keyPressEvent(QKeyEvent* e)
{
    switch ( e->key() )
    {
        case Qt::Key_Space:
            m_bModelRotationMode = true;
            break;

        case Qt::Key_Left:
                m_vecViewRotation.setY(-0.75 + m_vecViewRotationOld.y());

                //Rotate all surface objects
                if(!m_bModelRotationMode) {
                    //Rotate camera
                    m_pCameraTransform->setRotationY(m_vecViewRotation.y());
                } else {
                    //Rotate all surface objects
                    for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
                        if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(m_p3DObjectsEntity->children().at(i))) {
                            pItem->setRotY(m_vecViewRotation.y());
                        }
                    }
                }
            break;

        case Qt::Key_Right:
                m_vecViewRotation.setY(0.75 + m_vecViewRotationOld.y());

                //Rotate all surface objects
                if(!m_bModelRotationMode) {
                    //Rotate camera
                    m_pCameraTransform->setRotationY(m_vecViewRotation.y());
                } else {
                    //Rotate all surface objects
                    for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
                        if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(m_p3DObjectsEntity->children().at(i))) {
                            pItem->setRotY(m_vecViewRotation.y());
                        }
                    }
                }
            break;

        case Qt::Key_Up:
                m_vecViewRotation.setX(0.75 + m_vecViewRotationOld.x());

                //Rotate all surface objects
                if(!m_bModelRotationMode) {
                    //Rotate camera
                    m_pCameraTransform->setRotationX(m_vecViewRotation.x());
                } else {
                    //Rotate all surface objects
                    for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
                        if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(m_p3DObjectsEntity->children().at(i))) {
                            pItem->setRotX(m_vecViewRotation.x());
                        }
                    }
                }
            break;

        case Qt::Key_Down:
                m_vecViewRotation.setX(-0.75 + m_vecViewRotationOld.x());

                //Rotate all surface objects
                if(!m_bModelRotationMode) {
                    //Rotate camera
                    m_pCameraTransform->setRotationX(m_vecViewRotation.x());
                } else {
                    //Rotate all surface objects
                    for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
                        if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(m_p3DObjectsEntity->children().at(i))) {
                            pItem->setRotX(m_vecViewRotation.x());
                        }
                    }
                }
            break;

        default:
            Qt3DWindow::keyPressEvent(e);
    }
}


//*************************************************************************************************************

void View3D::keyReleaseEvent(QKeyEvent* e)
{
    switch ( e->key() )
    {
        case Qt::Key_Space:
            m_bModelRotationMode = false;
            break;

        case Qt::Key_Left:
            m_vecViewRotationOld = m_vecViewRotation;
            break;

        case Qt::Key_Right:
            m_vecViewRotationOld = m_vecViewRotation;
            break;

        case Qt::Key_Up:
            m_vecViewRotationOld = m_vecViewRotation;
            break;

        case Qt::Key_Down:
            m_vecViewRotationOld = m_vecViewRotation;
            break;

        default:
            Qt3DWindow::keyPressEvent(e);
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

        default:
            Qt3DWindow::mousePressEvent(e);
    }    
}


//*************************************************************************************************************

void View3D::wheelEvent(QWheelEvent* e)
{
    if(e->angleDelta().y() > 0)
        m_vecViewTrans.setZ(m_vecViewTrans.z() + 0.005f);
    else
        m_vecViewTrans.setZ(m_vecViewTrans.z() - 0.005f);

    // Transform
    m_pCameraTransform->setTranslation(m_vecViewTrans);

    Qt3DWindow::wheelEvent(e);
}


//*************************************************************************************************************

void View3D::mouseReleaseEvent(QMouseEvent* e)
{
    m_bCameraRotationMode = false;
    m_bCameraTransMode = false;
    m_vecViewTransOld = m_vecViewTrans;
    m_vecViewRotationOld = m_vecViewRotation;

    Qt3DWindow::mouseReleaseEvent(e);
}


//*************************************************************************************************************

void View3D::mouseMoveEvent(QMouseEvent* e)
{
    if(m_bCameraRotationMode) {
        m_vecViewRotation.setX(((e->pos().y() - m_mousePressPositon.y()) * -0.1f) + m_vecViewRotationOld.x());
        m_vecViewRotation.setY(((e->pos().x() - m_mousePressPositon.x()) * 0.1f) + m_vecViewRotationOld.y());

        //Rotate all surface objects
        if(!m_bModelRotationMode) {
            //Rotate camera
            m_pCameraTransform->setRotationX(m_vecViewRotation.x());
            m_pCameraTransform->setRotationY(m_vecViewRotation.y());
        } else {
            //Rotate all surface objects
            for(int i = 0; i < m_p3DObjectsEntity->children().size(); ++i) {
                if(Renderable3DEntity* pItem = dynamic_cast<Renderable3DEntity*>(m_p3DObjectsEntity->children().at(i))) {
                    pItem->setRotZ(m_vecViewRotation.y());
                    pItem->setRotX(m_vecViewRotation.x());
                }
            }
        }
    }

    if(m_bCameraTransMode) {
        m_vecViewTrans.setX(((e->pos().x() - m_mousePressPositon.x()) * 0.0001f) + m_vecViewTransOld.x());
        m_vecViewTrans.setY(((e->pos().y() - m_mousePressPositon.y()) * -0.0001f) + m_vecViewTransOld.y());

        // Camera translation transform
        m_pCameraTransform->setTranslation(m_vecViewTrans);
    }

    Qt3DWindow::mouseMoveEvent(e);
}


//*************************************************************************************************************

void View3D::createCoordSystem(Qt3DCore::QEntity* parent)
{
    // Y - red
    Qt3DExtras::QCylinderMesh *YAxis = new Qt3DExtras::QCylinderMesh();
    YAxis->setRadius(0.001f);
    YAxis->setLength(30);
    YAxis->setRings(100);
    YAxis->setSlices(20);

    m_YAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
    m_YAxisEntity->addComponent(YAxis); //will take ownership of YAxis if no parent was declared!

    Qt3DExtras::QPhongMaterial *phongMaterialY = new Qt3DExtras::QPhongMaterial();
    phongMaterialY->setDiffuse(QColor(255, 0, 0));
    phongMaterialY->setAmbient(Qt::gray);
    phongMaterialY->setSpecular(Qt::white);
    phongMaterialY->setShininess(50.0f);
    m_YAxisEntity->addComponent(phongMaterialY);

    // Z - blue
    Qt3DExtras::QCylinderMesh *ZAxis = new Qt3DExtras::QCylinderMesh();
    ZAxis->setRadius(0.001f);
    ZAxis->setLength(30);
    ZAxis->setRings(100);
    ZAxis->setSlices(20);

    Qt3DCore::QTransform *transformZ = new Qt3DCore::QTransform();
    transformZ->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0,0,1), 90));

    m_ZAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
    m_ZAxisEntity->addComponent(ZAxis);
    m_ZAxisEntity->addComponent(transformZ);

    Qt3DExtras::QPhongMaterial *phongMaterialZ = new Qt3DExtras::QPhongMaterial();
    phongMaterialZ->setDiffuse(QColor(0, 0, 255));
    phongMaterialZ->setAmbient(Qt::gray);
    phongMaterialZ->setSpecular(Qt::white);
    phongMaterialZ->setShininess(50.0f);
    m_ZAxisEntity->addComponent(phongMaterialZ);

    // X - green
    Qt3DExtras::QCylinderMesh *XAxis = new Qt3DExtras::QCylinderMesh();
    XAxis->setRadius(0.001f);
    XAxis->setLength(30);
    XAxis->setRings(100);
    XAxis->setSlices(20);

    Qt3DCore::QTransform *transformX = new Qt3DCore::QTransform();
    transformX->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 90));

    m_XAxisEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(parent));
    m_XAxisEntity->addComponent(XAxis);
    m_XAxisEntity->addComponent(transformX);

    Qt3DExtras::QPhongMaterial *phongMaterialX = new Qt3DExtras::QPhongMaterial();
    phongMaterialX->setDiffuse(QColor(0, 255, 0));
    phongMaterialX->setAmbient(Qt::gray);
    phongMaterialX->setSpecular(Qt::white);
    phongMaterialX->setShininess(50.0f);
    m_XAxisEntity->addComponent(phongMaterialX);
}
