//=============================================================================================================
/**
* @file     custominstancedmesh.cpp
* @author   Lars Debor <lars.debor@gmx.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    CustomInstancedMesh class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "custominstancedmesh.h"
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>

#include <Qt3DCore/QNode>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CustomInstancedMesh::CustomInstancedMesh(QSharedPointer<Qt3DRender::QGeometry> tGeometry,
                                         Qt3DCore::QNode *tParent)
    : QGeometryRenderer(tParent)
    , m_pGeometry(tGeometry)
    , m_pPositionBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
    , m_pPositionAttribute(new QAttribute())
{
    init();
}


//*************************************************************************************************************

CustomInstancedMesh::~CustomInstancedMesh()
{
    m_pGeometry->deleteLater();
    m_pPositionBuffer->deleteLater();
    m_pPositionAttribute->deleteLater();
}


//*************************************************************************************************************

void CustomInstancedMesh::setPositions(const Eigen::MatrixX3f &tVertPositions)
{
    if(tVertPositions.rows() == 0)
    {
        qDebug ("ERROR!: CustomInstancedMesh::setPositions: Matrix is empty!");
        return;
    }

    //init buffer
    m_pPositionBuffer->setData(buildPositionBuffer(tVertPositions));
    m_pPositionAttribute->setBuffer(m_pPositionBuffer);

    //set number of instances to draw
    this->setInstanceCount(tVertPositions.rows());
}


//*************************************************************************************************************

void CustomInstancedMesh::init()
{
    //Set Attribute parameters
    m_pPositionAttribute->setName(QStringLiteral("geometryPosition"));
    m_pPositionAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_pPositionAttribute->setVertexBaseType(QAttribute::Float);
    m_pPositionAttribute->setVertexSize(3);
    m_pPositionAttribute->setDivisor(1);
    m_pPositionAttribute->setByteOffset(0);
    m_pPositionAttribute->setByteStride(3 * (int)sizeof(float));

    //Set default position
    Eigen::MatrixX3f tempPos = Eigen::MatrixX3f::Zero(1, 3);
    setPositions(tempPos);

    //Add Attibute to Geometry
    m_pGeometry->addAttribute(m_pPositionAttribute);

    //configure geometry renderer
    this->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    this->setIndexOffset(0);
    this->setFirstInstance(0);
    this->setGeometry(m_pGeometry.data());

}


//*************************************************************************************************************

QByteArray CustomInstancedMesh::buildPositionBuffer(const Eigen::MatrixX3f& tVertPositions)
{
    const uint iVertNum = tVertPositions.rows();
    const uint iVertSize = tVertPositions.cols();

    //create byre array
    QByteArray bufferData;
    bufferData.resize(iVertNum* iVertSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //copy positions into buffer
    for(uint i = 0 ; i < iVertNum; i++)
    {
        for(uint j = 0; j < iVertSize; j++)
        {
            rawVertexArray[3 * i + j] = tVertPositions(i, j);
        }
    }

    return bufferData;
}


//*************************************************************************************************************
