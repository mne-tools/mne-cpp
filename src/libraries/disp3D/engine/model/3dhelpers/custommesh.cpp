//=============================================================================================================
/**
 * @file     custommesh.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "custommesh.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CustomMesh::CustomMesh()
: Qt3DRender::QGeometryRenderer()
, m_iNumVert(0)
{
    init();
}

//=============================================================================================================

CustomMesh::CustomMesh(const MatrixX3f& tMatVert,
                       const MatrixX3f& tMatNorm,
                       const MatrixXi& tMatTris,
                       const MatrixX4f& tMatColors,
                       Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType)
: Qt3DRender::QGeometryRenderer()
, m_iNumVert(tMatVert.rows())
{
    init();

    setMeshData(tMatVert,
                tMatNorm,
                tMatTris,
                tMatColors,
                primitiveType);
}

//=============================================================================================================

void CustomMesh::init()
{
    m_pCustomGeometry = new QT_NAMESPACE_3D::QGeometry(this);

    this->setGeometry(m_pCustomGeometry);

    m_pVertexDataBuffer = new QT_NAMESPACE_3D::QBuffer();
    m_pNormalDataBuffer = new QT_NAMESPACE_3D::QBuffer();
    m_pColorDataBuffer = new QT_NAMESPACE_3D::QBuffer();
    m_pIndexDataBuffer = new QT_NAMESPACE_3D::QBuffer();

    m_pIndexAttribute = new QT_NAMESPACE_3D::QAttribute();
    m_pIndexAttribute->setAttributeType(QT_NAMESPACE_3D::QAttribute::IndexAttribute);
    m_pIndexAttribute->setVertexBaseType(QT_NAMESPACE_3D::QAttribute::UnsignedInt);
    m_pIndexAttribute->setByteOffset(0);
    m_pIndexAttribute->setBuffer(m_pIndexDataBuffer);

    m_pVertexAttribute = new QT_NAMESPACE_3D::QAttribute();
    m_pVertexAttribute->setAttributeType(QT_NAMESPACE_3D::QAttribute::VertexAttribute);
    m_pVertexAttribute->setVertexBaseType(QT_NAMESPACE_3D::QAttribute::Float);
    m_pVertexAttribute->setVertexSize(3);
    m_pVertexAttribute->setByteOffset(0);
    m_pVertexAttribute->setByteStride(3 * sizeof(float));
    m_pVertexAttribute->setName(QT_NAMESPACE_3D::QAttribute::defaultPositionAttributeName());
    m_pVertexAttribute->setBuffer(m_pVertexDataBuffer);

    m_pNormalAttribute = new QT_NAMESPACE_3D::QAttribute();
    m_pNormalAttribute->setAttributeType(QT_NAMESPACE_3D::QAttribute::VertexAttribute);
    m_pNormalAttribute->setVertexBaseType(QT_NAMESPACE_3D::QAttribute::Float);
    m_pNormalAttribute->setVertexSize(3);
    m_pNormalAttribute->setByteOffset(0);
    m_pNormalAttribute->setByteStride(3 * sizeof(float));
    m_pNormalAttribute->setName(QT_NAMESPACE_3D::QAttribute::defaultNormalAttributeName());
    m_pNormalAttribute->setBuffer(m_pNormalDataBuffer);

    m_pColorAttribute = new QT_NAMESPACE_3D::QAttribute();
    m_pColorAttribute->setAttributeType(QT_NAMESPACE_3D::QAttribute::VertexAttribute);
    m_pColorAttribute->setVertexBaseType(QT_NAMESPACE_3D::QAttribute::Float);
    m_pColorAttribute->setVertexSize(4);
    m_pColorAttribute->setByteOffset(0);
    m_pColorAttribute->setByteStride(4 * sizeof(float));
    m_pColorAttribute->setName(QT_NAMESPACE_3D::QAttribute::defaultColorAttributeName());
    m_pColorAttribute->setBuffer(m_pColorDataBuffer);

    m_pCustomGeometry->addAttribute(m_pVertexAttribute);
    m_pCustomGeometry->addAttribute(m_pNormalAttribute);
    m_pCustomGeometry->addAttribute(m_pColorAttribute);
    m_pCustomGeometry->addAttribute(m_pIndexAttribute);
}

//=============================================================================================================

CustomMesh::~CustomMesh()
{
    m_pVertexDataBuffer->deleteLater();
    m_pNormalDataBuffer->deleteLater();
    m_pColorDataBuffer->deleteLater();
    m_pIndexDataBuffer->deleteLater();
    m_pCustomGeometry->deleteLater();
    m_pIndexAttribute->deleteLater();
    m_pVertexAttribute->deleteLater();
    m_pNormalAttribute->deleteLater();
    m_pColorAttribute->deleteLater();
}

//=============================================================================================================

void CustomMesh::setColor(const Eigen::MatrixX4f& tMatColors)
{
    QByteArray colorBufferData;
    colorBufferData.resize(tMatColors.rows() * 4 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(colorBufferData.data());

    int idxColor = 0;

    for(int i = 0; i < tMatColors.rows(); ++i) {
        rawColorArray[idxColor++] = tMatColors(i,0);
        rawColorArray[idxColor++] = tMatColors(i,1);
        rawColorArray[idxColor++] = tMatColors(i,2);
        rawColorArray[idxColor++] = tMatColors(i,3);
    }

    //Update color
    m_pColorDataBuffer->setData(colorBufferData);

    //m_pColorAttribute->setBuffer(m_pColorDataBuffer);
    m_pColorAttribute->setCount(tMatColors.rows());
}

//=============================================================================================================

void CustomMesh::setNormals(const Eigen::MatrixX3f& tMatNorm)
{
    QByteArray normalBufferData;
    normalBufferData.resize(tMatNorm.rows() * 3 * (int)sizeof(float));
    float *rawNormalArray = reinterpret_cast<float *>(normalBufferData.data());

    int idxNorm = 0;
    for(int i = 0; i < tMatNorm.rows(); ++i) {
        //Normal
        rawNormalArray[idxNorm++] = tMatNorm(i,0);
        rawNormalArray[idxNorm++] = tMatNorm(i,1);
        rawNormalArray[idxNorm++] = tMatNorm(i,2);
    }

    m_pNormalDataBuffer->setData(normalBufferData);

    //m_pNormalAttribute->setBuffer(m_pNormalDataBuffer);
    m_pNormalAttribute->setCount(tMatNorm.rows());
}

//=============================================================================================================

void CustomMesh::setVertex(const Eigen::MatrixX3f& tMatVert)
{
    QByteArray vertexBufferData;
    vertexBufferData.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());

    int idxVert = 0;
    for(int i = 0; i < tMatVert.rows(); ++i) {
        rawVertexArray[idxVert++] = (tMatVert(i,0));
        rawVertexArray[idxVert++] = (tMatVert(i,1));
        rawVertexArray[idxVert++] = (tMatVert(i,2));
    }

    m_pVertexDataBuffer->setData(vertexBufferData);

    //m_pVertexAttribute->setBuffer(m_pVertexDataBuffer);
    m_pVertexAttribute->setCount(tMatVert.rows());
}

//=============================================================================================================

void CustomMesh::setIndex(const Eigen::MatrixXi& tMatTris)
{
    QByteArray indexBufferData;

    const uint iIndicesCount = tMatTris.rows() * tMatTris.cols();
    indexBufferData.resize(iIndicesCount * (int)sizeof(uint));

    uint *rawIndexArray = reinterpret_cast<uint *>(indexBufferData.data());
    int idxTris = 0;

    for(int i = 0; i < tMatTris.rows(); ++i) {
        //patches/tris/lines
        for(int f = 0; f < tMatTris.cols(); ++f) {
            rawIndexArray[idxTris++] = tMatTris(i,f);
        }
    }

    m_pIndexDataBuffer->setData(indexBufferData);

    m_pIndexAttribute->setCount(iIndicesCount);

    //Set the final geometry and primitive type
    this->setVerticesPerPatch(tMatTris.cols());
    this->setVertexCount(tMatTris.rows()*3);
}

//=============================================================================================================

void CustomMesh::setMeshData(const MatrixX3f& tMatVert,
                             const MatrixX3f& tMatNorm,
                             const MatrixXi& tMatTris,
                             const MatrixX4f& tMatColors,
                             Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType)
{
    m_iNumVert = tMatVert.rows();

    setVertex(tMatVert);
    setNormals(tMatNorm);
    setIndex(tMatTris);
    setColor(tMatColors);

    this->setPrimitiveType(primitiveType);

    ////    this->setInstanceCount(1);
    ////    this->setIndexOffset(0);
    ////    //this->setFirstVertex(0);
    ////    this->setFirstInstance(0);
}

//=============================================================================================================

void CustomMesh::addAttribute(QT_NAMESPACE_3D::QAttribute *pAttribute)
{
    m_pCustomGeometry->addAttribute(pAttribute);
}
