//=============================================================================================================
/**
* @file     Gpuinterpolationmaterial.cpp
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
* @brief    GpuInterpolationMaterial class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpuinterpolationmaterial.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QNode>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QNoDepthMask>
#include <QUrl>
#include <QColor>
#include <QVector3D>


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

GpuInterpolationMaterial::GpuInterpolationMaterial(bool bUseAlpha, Qt3DCore::QNode *parent)
    : QMaterial(parent)
    , m_bUseAlpha(bUseAlpha)
    , m_pEffect(new QEffect)
    , m_pDiffuseParameter(new QParameter(QStringLiteral("kd"), QColor::fromRgbF(0.7f, 0.7f, 0.7f, 1.0f)))
    , m_pSpecularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.1f, 0.1f, 0.1f, 1.0f)))
    , m_pShininessParameter(new QParameter(QStringLiteral("shininess"), 4.5f))
    , m_pAlphaParameter(new QParameter(QStringLiteral("alpha"), 0.5f))
    , m_pComputeShader(new QShaderProgram)
    , m_pComputeRenderPass(new QRenderPass)
    , m_pComputeFilterKey(new QFilterKey)
    , m_pComputeTechnique(new QTechnique)
    , m_pDrawShader(new QShaderProgram)
    , m_pDrawRenderPass(new QRenderPass)
    , m_pDrawFilterKey(new QFilterKey)
    , m_pDrawTechnique(new QTechnique)
    , m_pSignalDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
    , m_pSignalDataParameter(new QParameter)
    , m_pColsParameter(new QParameter)
    , m_pRowsParameter(new QParameter)
    , m_pWeightMatParameter(new QParameter)
    , m_pWeightMatBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
    , m_pOutputColorParameter(new QParameter)
    , m_pOutputColorBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
    , m_pThresholdXParameter(new QParameter(QStringLiteral("fThresholdX"), 1e-10f))
    , m_pThresholdZParameter(new QParameter(QStringLiteral("fThresholdZ"), 6e-6f))
    , m_pColormapParameter(new QParameter(QStringLiteral("ColormapType"), 3))
    , m_pCullFace(new QCullFace)
    , m_pNoDepthMask(new QNoDepthMask())
    , m_pBlendState(new QBlendEquationArguments())
    , m_pBlendEquation(new QBlendEquation())
{
    init();
}


//*************************************************************************************************************

GpuInterpolationMaterial::~GpuInterpolationMaterial()
{

}


//*************************************************************************************************************

void GpuInterpolationMaterial::setWeightMatrix(QSharedPointer<Eigen::SparseMatrix<double> > tInterpolationMatrix)
{
    //Set Rows and Cols
    m_pColsParameter->setValue(static_cast<uint>(tInterpolationMatrix->cols()));
    m_pRowsParameter->setValue(static_cast<uint>(tInterpolationMatrix->rows()));

    //Set buffer
    m_pWeightMatBuffer->setData(buildWeightMatrixBuffer(tInterpolationMatrix));
    //Set weight matrix parameter
    m_pWeightMatParameter->setValue(QVariant::fromValue(m_pWeightMatBuffer.data()));

    //Set output buffer
    m_pOutputColorBuffer->setData(buildZeroBuffer(4 * tInterpolationMatrix->rows()));
    m_pOutputColorParameter->setValue(QVariant::fromValue(m_pOutputColorBuffer.data()));
}


//*************************************************************************************************************

void GpuInterpolationMaterial::addSignalData(const Eigen::VectorXf &tSignalVec)
{
    const uint iBufferSize = tSignalVec.rows();

    if(iBufferSize != m_pColsParameter->value())
    {
        qDebug("GpuInterpolationMaterial::addSignalData input vector dimension mismatch!");
        return;
    }

    QByteArray bufferData;
    bufferData.resize(iBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    for(uint i = 0; i < iBufferSize; ++i)
    {
        rawVertexArray[i] = tSignalVec[i];
    }

    //Set buffer and parameter
    m_pSignalDataBuffer->setData(bufferData);
    m_pSignalDataParameter->setValue(QVariant::fromValue(m_pSignalDataBuffer.data()));
}


//*************************************************************************************************************

float GpuInterpolationMaterial::alpha()
{
    return m_pAlphaParameter->value().toFloat();
}


//*************************************************************************************************************

void GpuInterpolationMaterial::setAlpha(const float tAlpha)
{
    m_pAlphaParameter->setValue(tAlpha);
}


//*************************************************************************************************************

void GpuInterpolationMaterial::setNormalization(const QVector3D &tVecThresholds)
{
    m_pThresholdXParameter->setValue(QVariant::fromValue(tVecThresholds.x()));
    m_pThresholdZParameter->setValue(QVariant::fromValue(tVecThresholds.z()));
}


//*************************************************************************************************************

void GpuInterpolationMaterial::setColormapType(const QString &tColormapType)
{
    if(tColormapType == "Hot") {
        m_pColormapParameter->setValue(0);
    } else if(tColormapType == "Hot Negative 1") {
        m_pColormapParameter->setValue(1);
    } else if(tColormapType == "Hot Negative 2") {
        m_pColormapParameter->setValue(2);
    } else if(tColormapType == "Jet") {
        m_pColormapParameter->setValue(3);
    }
}


//*************************************************************************************************************

Qt3DRender::QBuffer * GpuInterpolationMaterial::getOutputColorBuffer()
{
    return m_pOutputColorBuffer.data();
}


//*************************************************************************************************************

void GpuInterpolationMaterial::init()
{
    //Compute part
    //Set shader
    m_pComputeShader->setComputeShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.csh"))));

    m_pComputeRenderPass->setShaderProgram(m_pComputeShader);

    //Set OpenGL version
    m_pComputeTechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pComputeTechnique->graphicsApiFilter()->setMajorVersion(4);
    m_pComputeTechnique->graphicsApiFilter()->setMinorVersion(3);
    m_pComputeTechnique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    //Set filter Keys
    m_pComputeFilterKey->setName(QStringLiteral("renderingStyle"));
    m_pComputeFilterKey->setValue(QStringLiteral("compute"));

    //Add to technique
    m_pComputeTechnique->addFilterKey(m_pComputeFilterKey);
    m_pComputeTechnique->addRenderPass(m_pComputeRenderPass);

    //Set default weight matrix parameters
    m_pColsParameter->setName(QStringLiteral("cols"));
    m_pColsParameter->setValue(1);
    m_pRowsParameter->setName(QStringLiteral("rows"));
    m_pRowsParameter->setValue(1);

    m_pWeightMatBuffer->setData(buildZeroBuffer(1));
    m_pWeightMatParameter->setName(QStringLiteral("WeightMat"));
    m_pWeightMatParameter->setValue(QVariant::fromValue(m_pWeightMatBuffer.data()));

    //Set default output
    m_pOutputColorBuffer->setData(buildZeroBuffer(4));
    m_pOutputColorParameter->setName(QStringLiteral("OutputColor"));
    m_pOutputColorParameter->setValue(QVariant::fromValue(m_pOutputColorBuffer.data()));

    //Set default input
    m_pSignalDataBuffer->setData(buildZeroBuffer(1));
    m_pSignalDataParameter->setName(QStringLiteral("InputVec"));
    m_pSignalDataParameter->setValue(QVariant::fromValue(m_pSignalDataBuffer.data()));

    //Add compute Parameter
    m_pComputeRenderPass->addParameter(m_pColsParameter);
    m_pComputeRenderPass->addParameter(m_pRowsParameter);
    m_pComputeRenderPass->addParameter(m_pOutputColorParameter);
    m_pComputeRenderPass->addParameter(m_pWeightMatParameter);
    m_pComputeRenderPass->addParameter(m_pSignalDataParameter);

    //Add Threshold parameter
    m_pComputeRenderPass->addParameter(m_pThresholdXParameter);
    m_pComputeRenderPass->addParameter(m_pThresholdZParameter);

    //Add ColormapType
    m_pComputeRenderPass->addParameter(m_pColormapParameter);

    //Draw part
    //Set shader
    m_pDrawShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.vert"))));
    m_pDrawShader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.frag"))));

    m_pDrawRenderPass->setShaderProgram(m_pDrawShader);

    //Add Phongalpha parameter
    m_pDrawRenderPass->addParameter(m_pDiffuseParameter);
    m_pDrawRenderPass->addParameter(m_pSpecularParameter);
    m_pDrawRenderPass->addParameter(m_pShininessParameter);
    m_pDrawRenderPass->addParameter(m_pAlphaParameter);

    if(m_bUseAlpha)
    {
        m_pBlendState->setSourceRgb(QBlendEquationArguments::SourceAlpha);
        m_pBlendState->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);
        m_pBlendEquation->setBlendFunction(QBlendEquation::Add);

        m_pDrawRenderPass->addRenderState(m_pBlendEquation);

        m_pDrawRenderPass->addRenderState(m_pNoDepthMask);

        m_pDrawRenderPass->addRenderState(m_pBlendState);
    }

    //Add Face Culling
    m_pCullFace->setMode(QCullFace::Back);
    m_pDrawRenderPass->addRenderState(m_pCullFace);

    //Set OpenGL version
    m_pDrawTechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pDrawTechnique->graphicsApiFilter()->setMajorVersion(4);
    m_pDrawTechnique->graphicsApiFilter()->setMinorVersion(3);
    m_pDrawTechnique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    //Set filter Keys
    m_pDrawFilterKey->setName(QStringLiteral("renderingStyle"));
    m_pDrawFilterKey->setValue(QStringLiteral("forward"));

    //Add to technique
    m_pDrawTechnique->addFilterKey(m_pDrawFilterKey);
    m_pDrawTechnique->addRenderPass(m_pDrawRenderPass);

    //Effect
    //Link shader and uniforms
    m_pEffect->addTechnique(m_pComputeTechnique);
    m_pEffect->addTechnique(m_pDrawTechnique);

    //Add to material
    this->setEffect(m_pEffect);
}


//*************************************************************************************************************

QByteArray GpuInterpolationMaterial::buildWeightMatrixBuffer(QSharedPointer<Eigen::SparseMatrix<double> > tInterpolationMatrix)
{
    QByteArray bufferData;

    const uint iRows = tInterpolationMatrix->rows();
    const uint iCols = tInterpolationMatrix->cols();

    bufferData.resize(iRows * iCols * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    unsigned int iCtr = 0;
    for(uint i = 0; i < iRows; ++i)
    {
        for(uint j = 0; j < iCols; ++j)
        {
            //@TODO this is probably not the best way to extract the weight matrix components
            rawVertexArray[iCtr] = static_cast<float>(tInterpolationMatrix->coeff(i, j));
            iCtr++;
        }
    }

    return bufferData;
}


//*************************************************************************************************************

QByteArray GpuInterpolationMaterial::buildZeroBuffer(const uint tSize)
{
    QByteArray bufferData;
    bufferData.resize(tSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(uint i = 0; i < tSize; ++i)
    {
        rawVertexArray[i] = 0.0f;
    }
    return bufferData;
}


//*************************************************************************************************************
