//=============================================================================================================
/**
* @file     custommesh.cpp
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
* @brief    CustomMesh class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "custommesh.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CustomMesh::CustomMesh(const MatrixX3f &tMatVert, const MatrixX3f tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset)
: Qt3DRender::QGeometryRenderer()
, m_iNumVert(tMatVert.rows())
{
    customGeometry = new Qt3DRender::QGeometry(this);

    m_pVertexDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
    m_pIndexDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer, customGeometry));

    //Fill vertexBuffer with data which hold the vertices, normals and colors
    QByteArray vertexBufferData;
    vertexBufferData.resize(tMatVert.rows() * (3 + 3 + 3) * sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());
    int idx = 0;

    for(int i = 0; i<tMatVert.rows(); i++) {
        //Vertex
        rawVertexArray[idx++] = tMatVert(i,0) + tVecOffset(0);
        rawVertexArray[idx++] = tMatVert(i,1) + tVecOffset(1);
        rawVertexArray[idx++] = tMatVert(i,2) + tVecOffset(2);

        //Normal
        rawVertexArray[idx++] = tMatNorm(i,0);
        rawVertexArray[idx++] = tMatNorm(i,1);
        rawVertexArray[idx++] = tMatNorm(i,2);

        //Color (this is the default color and will be used until the updateVertColor function was called)
        rawVertexArray[idx++] = 1.0f;//m_matColor(i,0);
        rawVertexArray[idx++] = 0.0f;//m_matColor(i,1);
        rawVertexArray[idx++] = 0.0f;//m_matColor(i,2);
    }

    //Fill indexBufferData with data which holds the triangulation information (faces/tris)
    QByteArray indexBufferData;
    indexBufferData.resize(tMatTris.rows() * 3 * sizeof(uint));
    uint *rawIndexArray = reinterpret_cast<uint *>(indexBufferData.data());
    idx = 0;

    for(int i = 0; i<tMatTris.rows(); i++) {
        //Faces/Tris
        rawIndexArray[idx++] = tMatTris(i,0);
        rawIndexArray[idx++] = tMatTris(i,1);
        rawIndexArray[idx++] = tMatTris(i,2);
    }

    m_pVertexDataBuffer->setData(vertexBufferData);
    m_pIndexDataBuffer->setData(indexBufferData);

    // Attributes
    Qt3DRender::QAttribute *positionAttribute = new Qt3DRender::QAttribute();
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(m_pVertexDataBuffer.data());
    positionAttribute->setDataType(Qt3DRender::QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(9 * sizeof(float));
    positionAttribute->setCount(tMatVert.rows());
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());

    Qt3DRender::QAttribute *normalAttribute = new Qt3DRender::QAttribute();
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(m_pVertexDataBuffer.data());
    normalAttribute->setDataType(Qt3DRender::QAttribute::Float);
    normalAttribute->setDataSize(3);
    normalAttribute->setByteOffset(3 * sizeof(float));
    normalAttribute->setByteStride(9 * sizeof(float));
    normalAttribute->setCount(tMatVert.rows());
    normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());

    Qt3DRender::QAttribute *colorAttribute = new Qt3DRender::QAttribute();
    colorAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(m_pVertexDataBuffer.data());
    colorAttribute->setDataType(Qt3DRender::QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(6 * sizeof(float));
    colorAttribute->setByteStride(9 * sizeof(float));
    colorAttribute->setCount(tMatVert.rows());
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());

    Qt3DRender::QAttribute *indexAttribute = new Qt3DRender::QAttribute();
    indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(m_pIndexDataBuffer.data());
    indexAttribute->setDataType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttribute->setDataSize(1);
    indexAttribute->setByteOffset(0);
    indexAttribute->setByteStride(0);
    indexAttribute->setCount(tMatTris.rows());

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    customGeometry->addAttribute(colorAttribute);
    customGeometry->addAttribute(indexAttribute);

    this->setInstanceCount(1);
    this->setBaseVertex(0);
    this->setBaseInstance(0);
    this->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    this->setGeometry(customGeometry);

    this->setPrimitiveCount(tMatTris.rows()*3);
}

//*************************************************************************************************************

bool CustomMesh::updateVertColors(const MatrixX3f &tMatColors)
{
    //Check dimensions
    if(tMatColors.rows() != m_iNumVert) {
        qDebug()<<"CustomMesh::updateVertColors - new color and vertices dimensions do not match!";
        return false;
    }

        qDebug()<<"Start - Updated color";

    //Fill vertexBuffer with data which hold the vertices, normals and colors
//    Qt3DRender::QGeometry::QAttributeList attributeList = this->geometry()->attributes();

//    for(int i = 0; attributeList.size(); i++)
//        if(atributteList.at(i).name == "ColorAttribute")

    float *rawVertexArray = reinterpret_cast<float *>(m_pVertexDataBuffer->data().data());
    int idx = 5;

    qDebug()<<"Size of buffer"<<m_pVertexDataBuffer->data().size();
    qDebug()<<"buffer at [0]"<<m_pVertexDataBuffer->data().data()[0];
    for(int i = 0; i<tMatColors.rows(); i++) {
        qDebug()<<rawVertexArray[idx++];
        rawVertexArray[idx++] = tMatColors(i,0);
        rawVertexArray[idx++] = tMatColors(i,1);
        rawVertexArray[idx++] = tMatColors(i,2);
        idx += 6;
    }

    qDebug()<<"Done - Updated color";

    return true;
}


//*************************************************************************************************************

CustomMesh::~CustomMesh()
{
}
