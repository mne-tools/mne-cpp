//=============================================================================================================
/**
* @file     gpuinterpolationitem.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
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
* @brief    GpuInterpolationItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpuinterpolationitem.h"
#include "../../materials/gpuinterpolationmaterial.h"
#include "../../3dhelpers/custommesh.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QComputeCommand>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometryRenderer>
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
using namespace Qt3DRender;
using namespace Qt3DCore;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GpuInterpolationItem::GpuInterpolationItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: AbstractMeshTreeItem(p3DEntityParent, iType, text)
, m_bIsDataInit(false)
, m_pGPUMaterial(new GpuInterpolationMaterial())
, m_pInterpolationMatBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
, m_pOutputColorBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
, m_pSignalDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
{
}


//*************************************************************************************************************

GpuInterpolationItem::~GpuInterpolationItem()
{
    m_pInterpolationMatBuffer->deleteLater();
    m_pOutputColorBuffer->deleteLater();
    m_pSignalDataBuffer->deleteLater();
}


//*************************************************************************************************************

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
    Qt3DRender::QAttribute* pInterpolatedSignalAttrib = new QAttribute;
    pInterpolatedSignalAttrib->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    pInterpolatedSignalAttrib->setDataType(Qt3DRender::QAttribute::Float);
    pInterpolatedSignalAttrib->setVertexSize(4);
    pInterpolatedSignalAttrib->setByteOffset(0);
    pInterpolatedSignalAttrib->setByteStride(4 * sizeof(float));
    pInterpolatedSignalAttrib->setName(QStringLiteral("OutputColor"));
    pInterpolatedSignalAttrib->setBuffer(m_pOutputColorBuffer);

    m_pCustomMesh->addAttribute(pInterpolatedSignalAttrib);

    //Create material, init and connect all necessary buffers
    this->setMaterial(m_pGPUMaterial);

    m_pInterpolationMatBuffer->setData(buildZeroBuffer(1));
    this->setMaterialParameter(QVariant::fromValue(m_pSignalDataBuffer.data()), "InterpolationMat");
    m_pOutputColorBuffer->setData(buildZeroBuffer(4));
    this->setMaterialParameter(QVariant::fromValue(m_pOutputColorBuffer.data()), "OutputColor");
    m_pSignalDataBuffer->setData(buildZeroBuffer(1));
    this->setMaterialParameter(QVariant::fromValue(m_pSignalDataBuffer.data()), "InputVec");

    //Create and add compute shader
    QPointer<Qt3DRender::QComputeCommand> pComputeCommand = new QComputeCommand();
    this->addComponent(pComputeCommand);

    const uint iInterpolationMatRows = matVertices.rows();
    const uint iWorkGroupsSize = static_cast<uint>(std::ceil(std::sqrt(iInterpolationMatRows)));
    pComputeCommand->setWorkGroupX(iWorkGroupsSize);
    pComputeCommand->setWorkGroupY(iWorkGroupsSize);
    pComputeCommand->setWorkGroupZ(1);

    //Set custom mesh data
    //generate mesh base color
    MatrixX3f matVertColor = createVertColor(matVertices.rows(), QColor(0,0,0));

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(matVertices,
                               matNormals,
                               matTriangles,
                               matVertColor,
                               Qt3DRender::QGeometryRenderer::Triangles);

    m_bIsDataInit = true;
}


//*************************************************************************************************************

void GpuInterpolationItem::setInterpolationMatrix(const Eigen::SparseMatrix<float> &matInterpolationMatrix)
{
    if(m_bIsDataInit == false)
    {
        qDebug("GpuInterpolationItem::setInterpolationMatrix - item data is not initialized!");
        return;
    }

    //Init and set interpolation buffer
    QByteArray interpolationBufferData = buildInterpolationMatrixBuffer(matInterpolationMatrix);

    if(m_pInterpolationMatBuffer->data().size() != interpolationBufferData.size()) {
        //Set Rows and Cols
        this->setMaterialParameter(QVariant::fromValue(matInterpolationMatrix.cols()), "cols");
        this->setMaterialParameter(QVariant::fromValue(matInterpolationMatrix.rows()), "rows");

        m_pInterpolationMatBuffer->setData(interpolationBufferData);
        this->setMaterialParameter(QVariant::fromValue(m_pInterpolationMatBuffer.data()), "InterpolationMat");

        m_pOutputColorBuffer->setData(buildZeroBuffer(4 * matInterpolationMatrix.rows()));
        this->setMaterialParameter(QVariant::fromValue(m_pOutputColorBuffer.data()), "OutputColor");
    } else {
        m_pInterpolationMatBuffer->updateData(0, interpolationBufferData);
    }

}


//*************************************************************************************************************

void GpuInterpolationItem::addNewRtData(const VectorXf &tSignalVec)
{
    if(m_bIsDataInit == false)
    {
        qDebug("GpuInterpolationItem::addNewRtData - item data is not initialized!");
        return;
    }

    const uint iBufferSize = tSignalVec.rows();

    QByteArray bufferData;
    bufferData.resize(iBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    for(uint i = 0; i < iBufferSize; ++i)
    {
        rawVertexArray[i] = static_cast<float>(tSignalVec[i]);
    }

    //Init and set signal data buffer
    if(m_pSignalDataBuffer->data().size() != bufferData.size()) {
        m_pSignalDataBuffer->setData(bufferData);
        this->setMaterialParameter(QVariant::fromValue(m_pSignalDataBuffer.data()), "InputVec");
    } else {
        m_pSignalDataBuffer->updateData(0, bufferData);
    }
}


//*************************************************************************************************************

void GpuInterpolationItem::setThresholds(const QVector3D &tVecThresholds)
{   
    this->setMaterialParameter(QVariant::fromValue(tVecThresholds.x()), "fThresholdX");
    this->setMaterialParameter(QVariant::fromValue(tVecThresholds.z()), "fThresholdZ");
}


//*************************************************************************************************************

void GpuInterpolationItem::setColormapType(const QString &tColormapType)
{
    int colorMapId = 0;
    if(tColormapType == "Hot") {
        colorMapId = 0;
    } else if(tColormapType == "Hot Negative 1") {
        colorMapId = 1;
    } else if(tColormapType == "Hot Negative 2") {
        colorMapId = 2;
    } else if(tColormapType == "Jet") {
        colorMapId = 3;
    }

    this->setMaterialParameter(QVariant::fromValue(colorMapId), "ColormapType");
}


//*************************************************************************************************************

QByteArray GpuInterpolationItem::buildInterpolationMatrixBuffer(SparseMatrix<float> matInterpolationMatrix)
{
    QByteArray bufferData;

    const uint iRows = matInterpolationMatrix.rows();
    const uint iCols = matInterpolationMatrix.cols();

    //bufferData.resize(iRows * iCols * (int)sizeof(float));
    bufferData = buildZeroBuffer(iRows * iCols);

    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Iterate over non zero entries only and transform from col major to row major (shader works with row major)
    for (int k=0; k<matInterpolationMatrix.outerSize(); ++k) {
        for (SparseMatrix<float>::InnerIterator it(matInterpolationMatrix,k); it; ++it)
        {
            //rawVertexArray[(it.col()*iRows)+it.row()] = static_cast<float>(it.value()); //Col major as result
            rawVertexArray[(it.row()*iCols)+it.col()] = static_cast<float>(it.value()); //Row major as result
        }
    }

    return bufferData;
}


//*************************************************************************************************************

QByteArray GpuInterpolationItem::buildZeroBuffer(const uint tSize)
{
    QByteArray bufferData;
    bufferData.resize(tSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(uint i = 0; i < tSize; ++i)
    {
        rawVertexArray[i] = static_cast<float>(0.0);
    }

    return bufferData;
}
