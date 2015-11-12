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
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Renderable3DEntity::Renderable3DEntity()
{

}


//*************************************************************************************************************

Renderable3DEntity::Renderable3DEntity(const MatrixX3f &tMatVert, const MatrixX3f &tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset, Qt3DCore::QEntity *parent)
: Qt3DCore::QEntity(parent)
, m_pCustomMesh(new CustomMesh(tMatVert, tMatNorm, tMatTris, tVecOffset))
, m_pTransform(QSharedPointer<Qt3DCore::QTransform>(new Qt3DCore::QTransform()))
, m_pScaleTransform(QSharedPointer<Qt3DCore::QScaleTransform>(new Qt3DCore::QScaleTransform()))
, m_pTranslateTransform(QSharedPointer<Qt3DCore::QTranslateTransform>(new Qt3DCore::QTranslateTransform()))
, m_pRotateTransform(QSharedPointer<Qt3DCore::QRotateTransform>(new Qt3DCore::QRotateTransform()))
, m_pRotateTransformX(QSharedPointer<Qt3DCore::QRotateTransform>(new Qt3DCore::QRotateTransform()))
, m_pRotateTransformY(QSharedPointer<Qt3DCore::QRotateTransform>(new Qt3DCore::QRotateTransform()))
, m_pRotateTransformZ(QSharedPointer<Qt3DCore::QRotateTransform>(new Qt3DCore::QRotateTransform()))
, m_pMaterial(QSharedPointer<Qt3DRender::QMaterial>(new Qt3DRender::QPerVertexColorMaterial(this)))
//, m_pMaterial(QSharedPointer<Qt3DRender::QMaterial>(new Qt3DRender::QNormalDiffuseMapMaterial(this)))
{
    m_pRotateTransformX->setAxis(QVector3D(1,0,0));
    m_pRotateTransformY->setAxis(QVector3D(0,1,0));
    m_pRotateTransformZ->setAxis(QVector3D(0,0,1));

    m_pTransform->addTransform(m_pScaleTransform.data());
    m_pTransform->addTransform(m_pTranslateTransform.data());
    m_pTransform->addTransform(m_pRotateTransform.data());
    m_pTransform->addTransform(m_pRotateTransformX.data());
    m_pTransform->addTransform(m_pRotateTransformY.data());
    m_pTransform->addTransform(m_pRotateTransformZ.data());

    this->addComponent(m_pCustomMesh.data());
    this->addComponent(m_pTransform.data());
    this->addComponent(m_pMaterial.data());
}


//*************************************************************************************************************

Renderable3DEntity::~Renderable3DEntity()
{
}


//*************************************************************************************************************

bool Renderable3DEntity::updateVertColors(const MatrixX3f &tMatColors)
{
    return m_pCustomMesh->updateVertColors(tMatColors);
}


//*************************************************************************************************************

void Renderable3DEntity::setScale(float scaleFactor)
{
    m_pScaleTransform->setScale(scaleFactor);
}


//*************************************************************************************************************

void Renderable3DEntity::setRotationX(float degree)
{
    m_pRotateTransformX->setAngleDeg(degree);
}


//*************************************************************************************************************

void Renderable3DEntity::setRotationY(float degree)
{
    m_pRotateTransformY->setAngleDeg(degree);
}


//*************************************************************************************************************

void Renderable3DEntity::setRotationZ(float degree)
{
    m_pRotateTransformZ->setAngleDeg(degree);
}


//*************************************************************************************************************

void Renderable3DEntity::setRotation(float degree, const QVector3D &rotAxis)
{
    m_pRotateTransform->setAxis(rotAxis);
    m_pRotateTransform->setAngleDeg(degree);
}


//*************************************************************************************************************

void Renderable3DEntity::addRotation(float degree, const QVector3D &rotAxis)
{
    Qt3DCore::QRotateTransform* rotateTransform = new Qt3DCore::QRotateTransform();
    rotateTransform->setAxis(rotAxis);
    rotateTransform->setAngleDeg(degree);

    m_pTransform->addTransform(rotateTransform);

    //qDebug()<<"Adding rotation transform. Total:"<<m_pTransform->transforms().size();
}



//*************************************************************************************************************

void Renderable3DEntity::setTranslation(const QVector3D &trans)
{
    m_pTranslateTransform->setTranslation(trans);
}



