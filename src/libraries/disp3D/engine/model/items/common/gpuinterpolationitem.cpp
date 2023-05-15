//=============================================================================================================
/**
 * @file     gpuinterpolationitem.cpp
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
 * @brief    GpuInterpolationItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpuinterpolationitem.h"
#include "../../materials/gpuinterpolationmaterial.h"
#include "../../3dhelpers/custommesh.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QComputeCommand>
#include <Qt3DRender/QGeometryRenderer>
#include <QBuffer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Qt3DRender;
using namespace Qt3DCore;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GpuInterpolationItem::GpuInterpolationItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
, m_bIsDataInit(false)
, m_pGPUMaterial(new GpuInterpolationMaterial())
, m_pCustomMesh(new CustomMesh)
, m_pInterpolationMatBuffer(new QT_COMPATIBILITY_3D::QBuffer())
, m_pOutputColorBuffer(new QT_COMPATIBILITY_3D::QBuffer())
, m_pSignalDataBuffer(new QT_COMPATIBILITY_3D::QBuffer())
{
}

//=============================================================================================================

GpuInterpolationItem::~GpuInterpolationItem()
{
    delete m_pInterpolationMatBuffer;
    delete m_pOutputColorBuffer;
    delete m_pSignalDataBuffer;
}

//=============================================================================================================

void GpuInterpolationItem::initData(const MatrixX3f &matVertices,
                                    const MatrixX3f &matNormals,
                                    const MatrixX3i &matTriangles)
{
    if(m_bIsDataInit == true)
    {
       qDebug("GpuInterpolationItem::initData data already initialized");
       return;
    }

    //Create and add interpolated color signal attribute    
    QT_COMPATIBILITY_3D::QAttribute* pInterpolatedSignalAttrib = new QAttribute;
    pInterpolatedSignalAttrib->setAttributeType(QT_COMPATIBILITY_3D::QAttribute::VertexAttribute);
    pInterpolatedSignalAttrib->setVertexBaseType(QT_COMPATIBILITY_3D::QAttribute::Float);
    pInterpolatedSignalAttrib->setVertexSize(4);
    pInterpolatedSignalAttrib->setByteOffset(0);
    pInterpolatedSignalAttrib->setByteStride(4 * sizeof(float));
    pInterpolatedSignalAttrib->setName(QStringLiteral("OutputColor"));
    pInterpolatedSignalAttrib->setBuffer(m_pOutputColorBuffer);

    m_pCustomMesh->addAttribute(pInterpolatedSignalAttrib);

    //Create material, init and connect all necessary buffers
    this->addComponent(m_pGPUMaterial);
    this->addComponent(m_pCustomMesh);

    m_pInterpolationMatBuffer->setData(buildZeroBuffer(1));
    this->setMaterialParameter(QVariant::fromValue(m_pInterpolationMatBuffer.data()), QStringLiteral("InterpolationMat"));
    m_pOutputColorBuffer->setData(buildZeroBuffer(4));
    this->setMaterialParameter(QVariant::fromValue(m_pOutputColorBuffer.data()), QStringLiteral("OutputColor"));
    m_pSignalDataBuffer->setData(buildZeroBuffer(1));
    this->setMaterialParameter(QVariant::fromValue(m_pSignalDataBuffer.data()), QStringLiteral("InputVec"));

    //Set custom mesh data
    //generate mesh base color
    MatrixX4f matVertColor = createVertColor(matVertices.rows(), QColor(0,0,0));

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(matVertices,
                               matNormals,
                               matTriangles,
                               matVertColor,
                               Qt3DRender::QGeometryRenderer::Triangles);

    m_bIsDataInit = true;
}

//=============================================================================================================

void GpuInterpolationItem::setInterpolationMatrix(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix)
{
    if(m_bIsDataInit == false)
    {
        qDebug("GpuInterpolationItem::setInterpolationMatrix - item data is not initialized!");
        return;
    }

    //qDebug("GpuInterpolationItem::setInterpolationMatrix - buildInterpolationMatrixBuffer");
    QByteArray interpolationBufferData = buildInterpolationMatrixBuffer(pMatInterpolationMatrix);

    //Init and set interpolation buffer
    if(m_pInterpolationMatBuffer->data().size() != interpolationBufferData.size()) {

        //Set Rows and Cols
        this->setMaterialParameter(QVariant::fromValue(pMatInterpolationMatrix->cols()), QStringLiteral("cols"));
        this->setMaterialParameter(QVariant::fromValue(pMatInterpolationMatrix->rows()), QStringLiteral("rows"));

        m_pOutputColorBuffer->setData(buildZeroBuffer(4 * pMatInterpolationMatrix->rows()));
        m_pSignalDataBuffer->setData(buildZeroBuffer(pMatInterpolationMatrix->cols()));

        //Set work group size
        if(!m_pComputeCommand) {
            m_pComputeCommand = new QComputeCommand();
            this->addComponent(m_pComputeCommand);
        }
        const uint iWorkGroupsSize = static_cast<uint>(std::ceil(std::sqrt(pMatInterpolationMatrix->rows())));
        m_pComputeCommand->setWorkGroupX(iWorkGroupsSize);
        m_pComputeCommand->setWorkGroupY(iWorkGroupsSize);
        m_pComputeCommand->setWorkGroupZ(1);

        //qDebug() << "4 * pMatInterpolationMatrix->rows()"<<4 * pMatInterpolationMatrix->rows();
        //qDebug() << "pMatInterpolationMatrix->rows()*pMatInterpolationMatrix->cols()"<<pMatInterpolationMatrix->rows()*pMatInterpolationMatrix->cols();
    }

    m_pInterpolationMatBuffer->setData(interpolationBufferData);

//        QByteArray updateData;
//        updateData.resize(pMatInterpolationMatrix->cols() * sizeof(float));
//        float *rawVertexArray = reinterpret_cast<float *>(updateData.data());

//        int pos = 0; //matrix element offset

//        for(uint i = 0; i < pMatInterpolationMatrix->rows(); ++i) {
//            //qDebug()<<"row "<<i;

//            //Extract row
//            int itr = 0;
//            for(uint j = 0; j < pMatInterpolationMatrix->cols(); ++j) {
//                rawVertexArray[itr] = pMatInterpolationMatrix->coeff(i,j);
//                itr++;
//            }

//            m_pInterpolationMatBuffer->updateData(pos, updateData);
//            pos += pMatInterpolationMatrix->cols() * sizeof(float); //stride
//        }

    qDebug("GpuInterpolationItem::setInterpolationMatrix - finished");
}

//=============================================================================================================

void GpuInterpolationItem::addNewRtData(const VectorXf &tSignalVec)
{
    if(m_bIsDataInit == false)
    {
        qDebug("GpuInterpolationItem::addNewRtData - item data is not initialized!");
        return;
    }

    const uint iSignalSize = tSignalVec.rows();

    QByteArray bufferData (iSignalSize * (int)sizeof(float), '0');
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    for(uint i = 0; i < iSignalSize; ++i)
    {
        rawVertexArray[i] = static_cast<float>(tSignalVec[i]);
    }

    m_pSignalDataBuffer->setData(bufferData);
}

//=============================================================================================================

void GpuInterpolationItem::setThresholds(const QVector3D &tVecThresholds)
{
    this->setMaterialParameter(QVariant::fromValue(tVecThresholds.x()), QStringLiteral("fThresholdX"));
    this->setMaterialParameter(QVariant::fromValue(tVecThresholds.z()), QStringLiteral("fThresholdZ"));
}

//=============================================================================================================

void GpuInterpolationItem::setColormapType(const QString &tColormapType)
{
    uint colorMapId = 0;
    if(tColormapType == QStringLiteral("Hot")) {
        colorMapId = 0;
    } else if(tColormapType == QStringLiteral("Hot Negative 1")) {
        colorMapId = 1;
    } else if(tColormapType == QStringLiteral("Hot Negative 2")) {
        colorMapId = 2;
    } else if(tColormapType == QStringLiteral("Jet")) {
        colorMapId = 3;
    }

    this->setMaterialParameter(QVariant::fromValue(colorMapId), QStringLiteral("ColormapType"));
}

//=============================================================================================================

QByteArray GpuInterpolationItem::buildInterpolationMatrixBuffer(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix)
{
    const uint iRows = pMatInterpolationMatrix->rows();
    const uint iCols = pMatInterpolationMatrix->cols();

//    qDebug() << "(int)sizeof(float)" << (int)sizeof(float);
//    qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - iRows" << iRows ;
//    qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - iCols" << iCols;
//    qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - iRows * iCols " << iRows * iCols;
//    qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - iRows * iCols * (int)sizeof(float) " << iRows * iCols * (int)sizeof(float);

    QByteArray bufferData (iRows * iCols * (int)sizeof(float), '0');

    // We do not need to use the buildZeroBuffer because we need to go through the non-zero entries again
    //qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - Fill Byte Array";
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    unsigned int iCtr = 0;
    for(uint i = 0; i < iRows; ++i) {
        for(uint j = 0; j < iCols; ++j) {
            rawVertexArray[iCtr] = static_cast<float>(pMatInterpolationMatrix->coeff(i, j));
            iCtr++;
        }
    }

    //qDebug() << "GpuInterpolationItem::buildInterpolationMatrixBuffer - Finished";

    return bufferData;
}

//=============================================================================================================

QByteArray GpuInterpolationItem::buildZeroBuffer(const uint tSize)
{
    //qDebug() << "GpuInterpolationItem::buildZeroBuffer 0 "<<tSize;
    QByteArray bufferData (tSize * (int)sizeof(float), '0');
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //qDebug() << "GpuInterpolationItem::buildZeroBuffer 1 ";

    //Set default values
    for(uint i = 0; i < tSize; ++i) {
        rawVertexArray[i] = 0.0f;
    }
    //qDebug() << "GpuInterpolationItem::buildZeroBuffer 2 ";

    return bufferData;
}
