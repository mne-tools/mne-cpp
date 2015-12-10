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

Renderable3DEntity::Renderable3DEntity(Qt3DCore::QEntity *parent)
: Qt3DCore::QEntity(parent)
, m_pCustomMesh(CustomMesh::SPtr(new CustomMesh()))
, m_pTransform(QSharedPointer<Qt3DCore::QTransform>(new Qt3DCore::QTransform()))
, m_pMaterial(QSharedPointer<Qt3DRender::QMaterial>(new Qt3DRender::QPerVertexColorMaterial))
{
    this->addComponent(m_pCustomMesh.data());
    this->addComponent(m_pTransform.data());
    this->addComponent(m_pMaterial.data());
}


//*************************************************************************************************************

Renderable3DEntity::Renderable3DEntity(const MatrixX3f &tMatVert, const MatrixX3f &tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset, Qt3DCore::QEntity *parent)
: Qt3DCore::QEntity(parent)
, m_pCustomMesh(new CustomMesh(tMatVert, tMatNorm, tMatTris, tVecOffset))
, m_pTransform(QSharedPointer<Qt3DCore::QTransform>(new Qt3DCore::QTransform()))
, m_pMaterial(QSharedPointer<Qt3DRender::QMaterial>(new Qt3DRender::QPerVertexColorMaterial(this)))
//, m_pMaterial(QSharedPointer<Qt3DRender::QMaterial>(new Qt3DRender::QNormalDiffuseMapMaterial(this)))
{
    this->addComponent(m_pCustomMesh.data());
    this->addComponent(m_pTransform.data());
    this->addComponent(m_pMaterial.data());
}


//*************************************************************************************************************

Renderable3DEntity::~Renderable3DEntity()
{
}


//*************************************************************************************************************

bool Renderable3DEntity::setVertColor(const MatrixX3f &tMatColors)
{
    return m_pCustomMesh->setVertColor(tMatColors);
}


//*************************************************************************************************************

bool Renderable3DEntity::setMeshData(const MatrixX3f &tMatVert, const MatrixX3f tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset)
{
    return m_pCustomMesh->setMeshData(tMatVert, tMatNorm, tMatTris, tVecOffset);
}


