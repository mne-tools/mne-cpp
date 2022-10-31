//=============================================================================================================
/**
 * @file     geometrymultiplier.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    GeometryMultiplier class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometrymultiplier.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>

#include <Qt3DCore/QNode>

#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Qt3DRender;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GeometryMultiplier::GeometryMultiplier(QSharedPointer<Qt3DRender::QGeometry> tGeometry,
                                         Qt3DCore::QNode *tParent)
: QGeometryRenderer(tParent)
, m_pGeometry(tGeometry)
, m_pTransformBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pColorBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pTransformAttribute(new QAttribute())
, m_pColorAttribute(new QAttribute())
{
    init();
}

//=============================================================================================================

GeometryMultiplier::~GeometryMultiplier()
{
    m_pGeometry->deleteLater();
    m_pTransformBuffer->deleteLater();
    m_pColorBuffer->deleteLater();
    m_pTransformAttribute->deleteLater();
    m_pColorAttribute->deleteLater();
}

//=============================================================================================================#

void GeometryMultiplier::setTransforms(const QVector<QMatrix4x4> &tInstanceTansform)
{
    //Update buffer content
    m_pTransformBuffer->setData(buildTransformBuffer(tInstanceTansform));

    this->setInstanceCount(tInstanceTansform.size());
}

//=============================================================================================================#

void GeometryMultiplier::setColors(const QVector<QColor> &tInstanceColors)
{
    //Update buffer content
    m_pColorBuffer->setData(buildColorBuffer(tInstanceColors));

    if(tInstanceColors.size() > 1) {
        m_pColorAttribute->setDivisor(1);
    } else {
        //enable 1 color for x transforms
        m_pColorAttribute->setDivisor(0);
    }

    this->setInstanceCount(tInstanceColors.size());
}

//=============================================================================================================

void GeometryMultiplier::init()
{
    //Set transform attribute parameter
    m_pTransformAttribute->setName(QStringLiteral("instanceModelMatrix"));
    m_pTransformAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_pTransformAttribute->setVertexBaseType(QAttribute::Float);
    m_pTransformAttribute->setVertexSize(16);
    m_pTransformAttribute->setDivisor(1);
    m_pTransformAttribute->setByteOffset(0);
    m_pTransformAttribute->setBuffer(m_pTransformBuffer);

    //Set color attribute parameters
    m_pColorAttribute->setName(QStringLiteral("instanceColor"));
    m_pColorAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_pColorAttribute->setVertexBaseType(QAttribute::Float);
    m_pColorAttribute->setVertexSize(4);

    //Set divisor 0 to enable empty color buffer
    m_pColorAttribute->setDivisor(0);
    m_pColorAttribute->setByteOffset(0);
    m_pColorAttribute->setBuffer(m_pColorBuffer);

    //Set default instance color
    QVector<QColor> tempColors;
    tempColors.push_back(QColor(0, 0, 255));
    setColors(tempColors);

    //set default transforms
    QVector<QMatrix4x4> tempTrans;
    tempTrans.push_back(QMatrix4x4());
    setTransforms(tempTrans);

    //Add Attibute to Geometry
    m_pGeometry->addAttribute(m_pTransformAttribute);
    m_pGeometry->addAttribute(m_pColorAttribute);

    //configure geometry renderer
    this->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    this->setIndexOffset(0);
    this->setFirstInstance(0);
    this->setGeometry(m_pGeometry.data());
}

//=============================================================================================================

QByteArray GeometryMultiplier::buildTransformBuffer(const QVector<QMatrix4x4> &tInstanceTransform)
{
    const uint iVertNum = tInstanceTransform.size();
    const uint iMatrixDim = 4;
    const uint iMatrixSize = iMatrixDim * iMatrixDim;
    //create byte array
    QByteArray bufferData;
    bufferData.resize(iVertNum * iMatrixSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //copy transforms into buffer
    for(uint i = 0 ; i < iVertNum; i++)
    {
        const float *rawMatrix = tInstanceTransform.at(i).data();
        for(uint idx = 0; idx < iMatrixSize; idx++)
        {
            rawVertexArray[iMatrixSize * i + idx] = rawMatrix[idx];
        }
    }

    return bufferData;
}

//=============================================================================================================

QByteArray GeometryMultiplier::buildColorBuffer(const QVector<QColor> &tInstanceColor)
{
    const uint iVertSize = 4;
    //create byte array
    QByteArray bufferData;
    bufferData.resize(tInstanceColor.size() * iVertSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //copy colors into buffer
    for(int i = 0; i < tInstanceColor.size(); i++)
    {
        rawVertexArray[iVertSize * i] = tInstanceColor[i].redF();
        rawVertexArray[iVertSize * i + 1] = tInstanceColor[i].greenF();
        rawVertexArray[iVertSize * i + 2] = tInstanceColor[i].blueF();
        rawVertexArray[iVertSize * i + 3] = tInstanceColor[i].alphaF();
    }

    return bufferData;
}
