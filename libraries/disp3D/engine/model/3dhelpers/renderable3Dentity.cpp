//=============================================================================================================
/**
* @file     renderable3Dentity.cpp
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
* @brief    Renderable3DEntity class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "renderable3Dentity.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QTransform>

#include <fiff/fiff_coord_trans.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QComponent>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QParameter>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace Qt3DCore;
using namespace FIFFLIB;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Renderable3DEntity::Renderable3DEntity(Qt3DCore::QEntity* parent)
: Qt3DCore::QEntity(parent)
, m_fScale(1.0f)
, m_fRotX(0.0f)
, m_fRotY(0.0f)
, m_fRotZ(0.0f)
{
}


//*************************************************************************************************************

Renderable3DEntity::~Renderable3DEntity()
{
    //releaseNode(this);
}


////*************************************************************************************************************

//void Renderable3DEntity::releaseNode(Qt3DCore::QNode* node)
//{
//    if (QEntity* entity = dynamic_cast<QEntity*>(node)) {
//        QComponentVector components = entity->components();

//        foreach (QComponent* component, components) {
//            entity->removeComponent(component);
//            delete component;
//        }
//    }

//    QNodeVector nodes = node->childNodes();

//    foreach (QNode* nodeIt, nodes) {
//        releaseNode(nodeIt);
//        delete nodeIt;
//    }
//}


//*************************************************************************************************************

void Renderable3DEntity::setTransform(const Qt3DCore::QTransform& transform)
{
    if(!m_pTransform) {
        m_pTransform = new Qt3DCore::QTransform();
        this->addComponent(m_pTransform);
    }

    m_pTransform->setMatrix(transform.matrix());
}


//*************************************************************************************************************

void Renderable3DEntity::setTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(!m_pTransform) {
        m_pTransform = new Qt3DCore::QTransform();
        this->addComponent(m_pTransform);
    }

    Qt3DCore::QTransform transform3d;

    if(bApplyInverse) {
        QMatrix4x4 matrix(transform.invtrans(0,0),
                            transform.invtrans(0,1),
                            transform.invtrans(0,2),
                            transform.invtrans(0,3),
                            transform.invtrans(1,0),
                            transform.invtrans(1,1),
                            transform.invtrans(1,2),
                            transform.invtrans(1,3),
                            transform.invtrans(2,0),
                            transform.invtrans(2,1),
                            transform.invtrans(2,2),
                            transform.invtrans(2,3),
                            transform.invtrans(3,0),
                            transform.invtrans(3,1),
                            transform.invtrans(3,2),
                            transform.invtrans(3,3));
        transform3d.setMatrix(matrix);
    } else {
        QMatrix4x4 matrix(transform.trans(0,0),
                            transform.trans(0,1),
                            transform.trans(0,2),
                            transform.trans(0,3),
                            transform.trans(1,0),
                            transform.trans(1,1),
                            transform.trans(1,2),
                            transform.trans(1,3),
                            transform.trans(2,0),
                            transform.trans(2,1),
                            transform.trans(2,2),
                            transform.trans(2,3),
                            transform.trans(3,0),
                            transform.trans(3,1),
                            transform.trans(3,2),
                            transform.trans(3,3));
        transform3d.setMatrix(matrix);
    }

    m_pTransform->setMatrix(transform3d.matrix());
}


//*************************************************************************************************************

void Renderable3DEntity::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(!m_pTransform) {
        m_pTransform = new Qt3DCore::QTransform();
        this->addComponent(m_pTransform);
    }

    m_pTransform->setMatrix(transform.matrix() * m_pTransform->matrix());
}




//*************************************************************************************************************

void Renderable3DEntity::applyTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(!m_pTransform) {
        m_pTransform = new Qt3DCore::QTransform();
        this->addComponent(m_pTransform);
    }

    Qt3DCore::QTransform transform3d;

    if(bApplyInverse) {
        QMatrix4x4 matrix(transform.invtrans(0,0),
                          transform.invtrans(0,1),
                          transform.invtrans(0,2),
                          transform.invtrans(0,3),
                          transform.invtrans(1,0),
                          transform.invtrans(1,1),
                          transform.invtrans(1,2),
                          transform.invtrans(1,3),
                          transform.invtrans(2,0),
                          transform.invtrans(2,1),
                          transform.invtrans(2,2),
                          transform.invtrans(2,3),
                          transform.invtrans(3,0),
                          transform.invtrans(3,1),
                          transform.invtrans(3,2),
                          transform.invtrans(3,3));
        transform3d.setMatrix(matrix);
    } else {
        QMatrix4x4 matrix(transform.trans(0,0),
                          transform.trans(0,1),
                          transform.trans(0,2),
                          transform.trans(0,3),
                          transform.trans(1,0),
                          transform.trans(1,1),
                          transform.trans(1,2),
                          transform.trans(1,3),
                          transform.trans(2,0),
                          transform.trans(2,1),
                          transform.trans(2,2),
                          transform.trans(2,3),
                          transform.trans(3,0),
                          transform.trans(3,1),
                          transform.trans(3,2),
                          transform.trans(3,3));
        transform3d.setMatrix(matrix);
    }

    m_pTransform->setMatrix(transform3d.matrix() * m_pTransform->matrix());
}


//*************************************************************************************************************

float Renderable3DEntity::scaleValue() const
{
    return m_fScale;
}


//*************************************************************************************************************

float Renderable3DEntity::rotX() const
{
    return m_fRotX;
}


//*************************************************************************************************************

float Renderable3DEntity::rotY() const
{
    return m_fRotY;
}


//*************************************************************************************************************

float Renderable3DEntity::rotZ() const
{
    return m_fRotZ;
}


//*************************************************************************************************************

QVector3D Renderable3DEntity::position() const
{
    return m_position;
}


//*************************************************************************************************************

void Renderable3DEntity::setRotX(float rotX)
{
    if(m_fRotX == rotX) {
        return;
    }

    m_fRotX = rotX;
    emit rotXChanged(rotX);
    updateTransform();
}


//*************************************************************************************************************

void Renderable3DEntity::setRotY(float rotY)
{
    if(m_fRotY == rotY) {
        return;
    }

    m_fRotY = rotY;
    emit rotYChanged(rotY);
    updateTransform();
}


//*************************************************************************************************************

void Renderable3DEntity::setRotZ(float rotZ)
{
    if(m_fRotZ == rotZ) {
        return;
    }

    m_fRotZ = rotZ;
    emit rotZChanged(rotZ);
    updateTransform();
}


//*************************************************************************************************************

void Renderable3DEntity::setPosition(QVector3D position)
{
    if(m_position == position) {
        return;
    }

    m_position = position;
    emit positionChanged(position);
    updateTransform();
}


//*************************************************************************************************************

void Renderable3DEntity::setVisible(bool state)
{
    for(int i = 0; i < this->childNodes().size(); ++i) {
        this->childNodes()[i]->setEnabled(state);
    }
    this->setEnabled(state);
}


//*************************************************************************************************************

void Renderable3DEntity::setScale(float scale)
{
    if(m_fScale == scale) {
        return;
    }

    m_fScale = scale;
    emit scaleChanged(scale);
    updateTransform();
}


//*************************************************************************************************************

void Renderable3DEntity::setMaterialParameter(const QVariant &data,
                                              const QString &sParameterName)
{
    setMaterialParameterRecursive(this, data, sParameterName);
}


//*************************************************************************************************************

QVariant Renderable3DEntity::getMaterialParameter(const QString &sParameterName)
{
    QPair<bool, QVariant> resultPair = getMaterialParameterRecursive(this, sParameterName);

    return resultPair.second;
}


//*************************************************************************************************************

void Renderable3DEntity::setMaterialParameterRecursive(QObject * pObject,
                                                       const QVariant &data,
                                                       const QString &sParameterName)
{
    if(QParameter* pParameter = dynamic_cast<QParameter*>(pObject)) {
        if(pParameter->name() == sParameterName) {
            pParameter->setValue(data);
        }
    }

    for(int i = 0; i < pObject->children().size(); ++i) {
        setMaterialParameterRecursive(pObject->children().at(i), data, sParameterName);
    }
}


//*************************************************************************************************************

QPair<bool, QVariant> Renderable3DEntity::getMaterialParameterRecursive(QObject * pObject,
                                                           const QString &sParameterName)
{
    if(QParameter* pParameter = dynamic_cast<QParameter*>(pObject)) {
        if(pParameter->name() == sParameterName) {
            return QPair<bool, QVariant>(true, pParameter->value());
        }
    }

    for(int i = 0; i < pObject->children().size(); ++i) {
        QPair<bool, QVariant> pair = getMaterialParameterRecursive(pObject->children().at(i), sParameterName);

        if(pair.first) {
            return pair;
        }
    }

    return QPair<bool, QVariant>(false, QVariant());
}


//*************************************************************************************************************

void Renderable3DEntity::updateTransform()
{
    QMatrix4x4 m;

    //Do the translation after rotating, otherwise rotation around the x,y,z axis would be screwed up
    m.scale(m_fScale);
    m.rotate(m_fRotX, QVector3D(1.0f, 0.0f, 0.0f));
    m.rotate(m_fRotY, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(m_fRotZ, QVector3D(0.0f, 0.0f, 1.0f));
    m.translate(m_position);

    if(!m_pTransform) {
        m_pTransform = new Qt3DCore::QTransform();
        this->addComponent(m_pTransform);
    }

    m_pTransform->setMatrix(m);
}
