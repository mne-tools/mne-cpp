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
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Renderable3DEntity::Renderable3DEntity(Qt3DCore::QEntity* parent)
: Qt3DCore::QEntity(parent)
, m_pTransform(new Qt3DCore::QTransform())
, m_fRotX(0.0f)
, m_fRotY(0.0f)
, m_fRotZ(0.0f)
{
    this->addComponent(m_pTransform);
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
    if(m_pTransform) {
        m_pTransform->setMatrix(transform.matrix());
    }
}


//*************************************************************************************************************

void Renderable3DEntity::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pTransform) {
        m_pTransform->setMatrix(m_pTransform->matrix() * transform.matrix());
    }
}


//*************************************************************************************************************

void Renderable3DEntity::setMaterialParameter(QVariant data, QString sParameterName)
{
    //Look for all materials and set the corresponding parameters
    QComponentVector vComponents;

    //Search in this item's components
    if(QEntity* pEntity = dynamic_cast<QEntity*>(this)) {
        vComponents = pEntity->components();

        for(int j = 0; j < vComponents.size(); ++j) {
            if(QMaterial* pMaterial = dynamic_cast<QMaterial*>(vComponents.at(j))) {
                for(int i = 0; i < pMaterial->effect()->parameters().size(); ++i) {
                    if(pMaterial->effect()->parameters().at(i)->name() == sParameterName) {
                        pMaterial->effect()->parameters().at(i)->setValue(data);
                    }
                }
            }
        }
    }

    //Search in all child item components
    for(int k = 0; k < this->childNodes().size(); ++k) {
        if(QEntity* pEntity = dynamic_cast<QEntity*>(this->childNodes().at(k))) {
            vComponents = pEntity->components();

            for(int j = 0; j < vComponents.size(); ++j) {
                if(QMaterial* pMaterial = dynamic_cast<QMaterial*>(vComponents.at(j))) {
                    for(int i = 0; i < pMaterial->effect()->parameters().size(); ++i) {
                        if(pMaterial->effect()->parameters().at(i)->name() == sParameterName) {
                            pMaterial->effect()->parameters().at(i)->setValue(data);
                        }
                    }
                }
            }
        }
    }
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
    if(m_pTransform) {
        m_pTransform->setScale(scale);
    }
}


//*************************************************************************************************************

void Renderable3DEntity::updateTransform()
{
    QMatrix4x4 m;

    //Do the translation after rotating, otherwise rotation around the x,y,z axis would be screwed up
    m.translate(m_position);
    m.rotate(m_fRotX, QVector3D(1.0f, 0.0f, 0.0f));
    m.rotate(m_fRotY, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(m_fRotZ, QVector3D(0.0f, 0.0f, 1.0f));

    if(m_pTransform) {
        m_pTransform->setMatrix(m);
    }
}
