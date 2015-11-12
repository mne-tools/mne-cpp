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

    m_pVertexDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);//QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
    m_pNormalDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);//QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
    m_pColorDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);//QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
    m_pIndexDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer, customGeometry);//QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer, customGeometry));

    //Fill vertexBuffer with data which hold the vertices, normals and colors
    QByteArray vertexBufferData;
    vertexBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());

    QByteArray normalBufferData;
    normalBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawNormalArray = reinterpret_cast<float *>(normalBufferData.data());

    QByteArray colorBufferData;
    colorBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(colorBufferData.data());

    int idxVert = 0;
    int idxNorm = 0;
    int idxColor = 0;

    for(int i = 0; i<tMatVert.rows(); i++) {
        //Vertex
        rawVertexArray[idxVert++] = tMatVert(i,0) + tVecOffset(0);
        rawVertexArray[idxVert++] = tMatVert(i,1) + tVecOffset(1);
        rawVertexArray[idxVert++] = tMatVert(i,2) + tVecOffset(2);

        //Normal
        rawNormalArray[idxNorm++] = tMatNorm(i,0);
        rawNormalArray[idxNorm++] = tMatNorm(i,1);
        rawNormalArray[idxNorm++] = tMatNorm(i,2);

        //Color (this is the default color and will be used until the updateVertColor function was called)
        rawColorArray[idxColor++] = 0.5f;
        rawColorArray[idxColor++] = 0.2f;
        rawColorArray[idxColor++] = 0.2f;
    }

    //Fill indexBufferData with data which holds the triangulation information (faces/tris)
    QByteArray indexBufferData;
    indexBufferData.resize(tMatTris.rows() * 3 * (int)sizeof(uint));
    uint *rawIndexArray = reinterpret_cast<uint *>(indexBufferData.data());
    int idxTris = 0;

    for(int i = 0; i<tMatTris.rows(); i++) {
        //Faces/Tris
        rawIndexArray[idxTris++] = tMatTris(i,0);
        rawIndexArray[idxTris++] = tMatTris(i,1);
        rawIndexArray[idxTris++] = tMatTris(i,2);
    }

    //Set data to buffers
    m_pVertexDataBuffer->setData(vertexBufferData);
    m_pNormalDataBuffer->setData(normalBufferData);
    m_pColorDataBuffer->setData(colorBufferData);
    m_pIndexDataBuffer->setData(indexBufferData);

    // Attributes
    Qt3DRender::QAttribute *positionAttribute = new Qt3DRender::QAttribute();
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(m_pVertexDataBuffer);
    positionAttribute->setDataType(Qt3DRender::QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(tMatVert.rows());
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());

    Qt3DRender::QAttribute *normalAttribute = new Qt3DRender::QAttribute();
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(m_pNormalDataBuffer);
    normalAttribute->setDataType(Qt3DRender::QAttribute::Float);
    normalAttribute->setDataSize(3);
    normalAttribute->setByteOffset(0);
    normalAttribute->setByteStride(3 * sizeof(float));
    normalAttribute->setCount(tMatVert.rows());
    normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());

    Qt3DRender::QAttribute *colorAttribute = new Qt3DRender::QAttribute();
    colorAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(m_pColorDataBuffer);
    colorAttribute->setDataType(Qt3DRender::QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(tMatVert.rows());
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());

    Qt3DRender::QAttribute *indexAttribute = new Qt3DRender::QAttribute();
    indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(m_pIndexDataBuffer);
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

CustomMesh::~CustomMesh()
{
}


//*************************************************************************************************************

bool CustomMesh::updateVertColors(const MatrixX3f &tMatColors)
{
    //Check dimensions
    if(tMatColors.rows() != m_iNumVert) {
        qDebug()<<"CustomMesh::updateVertColors - new color and vertices dimensions do not match!";
        return false;
    }

    Qt3DRender::QAttributeList list = this->geometry()->attributes();

    Qt3DRender::QAttribute* colorAttribute;
    for(int i = 0; i<list.size(); i++)
        if(list.at(i)->name() == Qt3DRender::QAttribute::defaultColorAttributeName())
            colorAttribute = dynamic_cast<Qt3DRender::QAttribute*>(list.at(i));

    //Update color
    QByteArray newColorData;
    newColorData.resize(tMatColors.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(newColorData.data());

    int idx = 0;

    for(int i = 0; i<tMatColors.rows(); i++) {
        rawColorArray[idx++] = tMatColors(i,0);
        rawColorArray[idx++] = tMatColors(i,1);
        rawColorArray[idx++] = tMatColors(i,2);
    }

    colorAttribute->buffer()->setData(newColorData);

    return true;
}

