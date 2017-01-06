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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector3D>

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CustomMesh::CustomMesh()
: Qt3DRender::QGeometryRenderer()
, m_pVertexDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pNormalDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pColorDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pIndexDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer))
//, m_pVertexDataBuffer(QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer)))
//, m_pNormalDataBuffer(QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer)))
//, m_pColorDataBuffer(QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer)))
//, m_pIndexDataBuffer(QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer)))
, m_iNumVert(0)
{

}


//*************************************************************************************************************

CustomMesh::CustomMesh(const MatrixX3f& tMatVert,
                       const MatrixX3f& tMatNorm,
                       const MatrixX3i& tMatTris,
                       const QByteArray& tArrayColors,
                       Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType)
: Qt3DRender::QGeometryRenderer()
, m_iNumVert(tMatVert.rows())
{
    this->createCustomMesh(tMatVert, tMatNorm, tMatTris, tArrayColors, primitiveType);
}


//*************************************************************************************************************

CustomMesh::~CustomMesh()
{
    delete m_pVertexDataBuffer;
    delete m_pNormalDataBuffer;
    delete m_pColorDataBuffer;
    delete m_pIndexDataBuffer;
}


//*************************************************************************************************************

void CustomMesh::setVertColor(const QByteArray& tArrayColors)
{
    //Check dimensions
    if(tArrayColors.size() != m_iNumVert * 3 * (int)sizeof(float)) {
        qDebug() << "CustomMesh::updateVertColors - new color and vertices dimensions do not match or mesh data was not set yet!";
        return;
    }

    //Update color
    m_pColorDataBuffer->setData(tArrayColors);
}


//*************************************************************************************************************

void CustomMesh::setMeshData(const MatrixX3f& tMatVert,
                             const MatrixX3f& tMatNorm,
                             const MatrixXi& tMatTris,
                             const QByteArray& tArrayColors,
                             Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType)
{
    m_iNumVert = tMatVert.rows();

    this->createCustomMesh(tMatVert, tMatNorm, tMatTris, tArrayColors, primitiveType);
}

//*************************************************************************************************************

void CustomMesh::createCustomMesh(const MatrixX3f& tMatVert,
                                  const MatrixX3f& tMatNorm,
                                  const MatrixXi& tMatTris,
                                  const QByteArray& tArrayColors,
                                  Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType)
{
    Qt3DRender::QGeometry* customGeometry = new Qt3DRender::QGeometry(this);

    //Delete old buffers first
    delete m_pVertexDataBuffer;
    delete m_pNormalDataBuffer;
    delete m_pColorDataBuffer;
    delete m_pIndexDataBuffer;

    m_pVertexDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);
    m_pNormalDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);
    m_pColorDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry);
    m_pIndexDataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer, customGeometry);

//    m_pVertexDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
//    m_pNormalDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
//    m_pColorDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, customGeometry));
//    m_pIndexDataBuffer = QSharedPointer<Qt3DRender::QBuffer>(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer, customGeometry));

    //Fill vertexBuffer with data which hold the vertices, normals and colors
    QByteArray vertexBufferData;
    vertexBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());

    QByteArray normalBufferData;
    normalBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawNormalArray = reinterpret_cast<float *>(normalBufferData.data());

    QByteArray colorBufferData = tArrayColors;
    colorBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(colorBufferData.data());

    int idxVert = 0;
    int idxNorm = 0;
    int idxColor = 0;

    for(int i = 0; i < tMatVert.rows(); ++i) {
        //Vertex
        rawVertexArray[idxVert++] = (tMatVert(i,0));
        rawVertexArray[idxVert++] = (tMatVert(i,1));
        rawVertexArray[idxVert++] = (tMatVert(i,2));

        //Normal
        rawNormalArray[idxNorm++] = tMatNorm(i,0);
        rawNormalArray[idxNorm++] = tMatNorm(i,1);
        rawNormalArray[idxNorm++] = tMatNorm(i,2);

        //Color (this is the default color and will be used until the updateVertColor function was called)
        if(tArrayColors.size() == 0 || tArrayColors.size() != colorBufferData.size()) {
//            rawColorArray[idxColor++] = 0.5f;
//            rawColorArray[idxColor++] = 0.2f;
//            rawColorArray[idxColor++] = 0.2f;
            rawColorArray[idxColor++] = 1.0f;
            rawColorArray[idxColor++] = 1.0f;
            rawColorArray[idxColor++] = 1.0f;
        }
    }

    //Fill indexBufferData with data which holds the triangulation information (patches/tris/lines)
    QByteArray indexBufferData;
    indexBufferData.resize(tMatTris.rows() * tMatTris.cols() * (int)sizeof(uint));
    uint *rawIndexArray = reinterpret_cast<uint *>(indexBufferData.data());
    int idxTris = 0;

    for(int i = 0; i < tMatTris.rows(); ++i) {
        //patches/tris/lines
        for(int f = 0; f < tMatTris.cols(); ++f) {
            rawIndexArray[idxTris++] = tMatTris(i,f);
        }
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
   // positionAttribute->setBuffer(m_pVertexDataBuffer.data());
    positionAttribute->setDataType(Qt3DRender::QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(tMatVert.rows());
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());

    Qt3DRender::QAttribute *normalAttribute = new Qt3DRender::QAttribute();
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(m_pNormalDataBuffer);
    //normalAttribute->setBuffer(m_pNormalDataBuffer.data());
    normalAttribute->setDataType(Qt3DRender::QAttribute::Float);
    normalAttribute->setDataSize(3);
    normalAttribute->setByteOffset(0);
    normalAttribute->setByteStride(3 * sizeof(float));
    normalAttribute->setCount(tMatVert.rows());
    normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());

    Qt3DRender::QAttribute* colorAttribute = new Qt3DRender::QAttribute();
    colorAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(m_pColorDataBuffer);
    //colorAttribute->setBuffer(m_pColorDataBuffer.data());
    colorAttribute->setDataType(Qt3DRender::QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(tMatVert.rows());
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());

    Qt3DRender::QAttribute *indexAttribute = new Qt3DRender::QAttribute();
    indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(m_pIndexDataBuffer);
    //indexAttribute->setBuffer(m_pIndexDataBuffer.data());
    indexAttribute->setDataType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttribute->setDataSize(tMatTris.cols());
    indexAttribute->setByteOffset(0);
    indexAttribute->setByteStride(tMatTris.cols() * sizeof(uint));
    indexAttribute->setCount(tMatTris.rows());

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    customGeometry->addAttribute(colorAttribute);
    customGeometry->addAttribute(indexAttribute);

    //Set the final geometry and primitive type
    this->setPrimitiveType(primitiveType);
    this->setVerticesPerPatch(tMatTris.cols());
    this->setGeometry(customGeometry);

//    this->setInstanceCount(1);
//    this->setIndexOffset(0);
//    //this->setFirstVertex(0);
//    this->setFirstInstance(0);

    this->setVertexCount(tMatTris.rows()*3);
}
