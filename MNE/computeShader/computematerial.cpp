//=============================================================================================================
/**
* @file     computematerial.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2017
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
* @brief    ComputeMaterial class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computematerial.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <cmath>

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
#include <QUrl>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CSH;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeMaterial::ComputeMaterial(Qt3DCore::QNode *parent)
    : QMaterial(parent)
    , m_iSignalCtr(0)
    , m_pEffect(new QEffect)
    , m_pComputeShader(new QShaderProgram)
    , m_pComputeRenderPass(new QRenderPass)
    , m_pComputeFilterKey(new QFilterKey)
    , m_pComputeTechnique(new QTechnique)
    , m_pDrawShader(new QShaderProgram)
    , m_pDrawRenderPass(new QRenderPass)
    , m_pDrawFilterKey(new QFilterKey)
    , m_pDrawTechnique(new QTechnique)
    , m_pColorParameter(new QParameter)
    , m_pSignalDataBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
    , m_pSignalDataParameter(new QParameter)
    , m_pTimer(new QTimer(this))
{
    init();
}


//*************************************************************************************************************

void ComputeMaterial::setYOutBuffer(Qt3DRender::QBuffer *yOutBuffer)
{
    m_pColorParameter->setName(QStringLiteral("YOutVec"));

    //Set the buffer as parameter data
    m_pColorParameter->setValue(QVariant::fromValue(yOutBuffer));
    m_pComputeRenderPass->addParameter(m_pColorParameter);
}


//*************************************************************************************************************

void ComputeMaterial::addComputePassParameter(QPointer<QParameter> tParameter)
{
    if(tParameter.isNull())
    {
        std::cerr << "setComputePassParameter: QParameter = nullptr. Returning!" << std::endl;
        return;
    }

    if(m_CustomParameters.contains(tParameter->name()))
    {
        std::cerr << "setComputePassParameter: A Parameter with this Name: " << tParameter->name().toStdString()
                  << " already exists. Returning!" << std::endl;
        return;
    }

    m_pComputeRenderPass->addParameter(tParameter);
    //add the parameter to the hash table for later usage.
    m_CustomParameters.insert(tParameter->name(), tParameter);
}


//*************************************************************************************************************

void ComputeMaterial::addDrawPassParameter(QPointer<QParameter> tParameter)
{
    if(tParameter.isNull())
    {
        std::cerr << "setRenderPassParameter: QParameter = nullptr. Returning!" << std::endl;
        return;
    }

    if(m_CustomParameters.contains(tParameter->name()))
    {
        std::cerr << "setRenderPassParameter: A Parameter with this Name: " << tParameter->name().toStdString()
                  << " already exists. Returning!" << std::endl;
        return;
    }

    m_pDrawRenderPass->addParameter(tParameter);
    //add the parameter to the hash table for later usage.
    m_CustomParameters.insert(tParameter->name(), tParameter);
}


//*************************************************************************************************************

void ComputeMaterial::addSignalData(const Eigen::MatrixXf &tSignalMat)
{
    m_signalMatrix = tSignalMat;
    updateSignalBuffer();
}


//*************************************************************************************************************

void ComputeMaterial::updateSignalBuffer()
{
    const uint iBufferSize = m_signalMatrix.rows();
    QByteArray bufferData;
    bufferData.resize(iBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    for(uint i = 0; i < iBufferSize; ++i)
    {
        rawVertexArray[i] = m_signalMatrix(i, m_iSignalCtr);
    }

    m_iSignalCtr++;

    if(m_iSignalCtr >= m_signalMatrix.cols())
    {
        m_iSignalCtr = 0;
    }

    //Set buffer and parameter
    m_pSignalDataBuffer->setData(bufferData);
    m_pSignalDataParameter->setValue(QVariant::fromValue(m_pSignalDataBuffer.data()));
}


//*************************************************************************************************************

void ComputeMaterial::init()
{
    //Compute part
    //Set shader
    m_pComputeShader->setComputeShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/interpolation.csh"))));

    m_pComputeRenderPass->setShaderProgram(m_pComputeShader);

    //Set OpenGL version
    m_pComputeTechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pComputeTechnique->graphicsApiFilter()->setMajorVersion(4);
    m_pComputeTechnique->graphicsApiFilter()->setMinorVersion(3);
    m_pComputeTechnique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    //Set filter Keys
    m_pComputeFilterKey->setName(QStringLiteral("type"));
    m_pComputeFilterKey->setValue(QStringLiteral("compute"));

    //Add to technique
    m_pComputeTechnique->addFilterKey(m_pComputeFilterKey);
    m_pComputeTechnique->addRenderPass(m_pComputeRenderPass);

    //Draw part
    //Set shader
    m_pDrawShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/vertexshader.vert"))));
    m_pDrawShader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/fragmentshader.frag"))));

    m_pDrawRenderPass->setShaderProgram(m_pDrawShader);

    //Set OpenGL version
    m_pDrawTechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pDrawTechnique->graphicsApiFilter()->setMajorVersion(4);
    m_pDrawTechnique->graphicsApiFilter()->setMinorVersion(3);
    m_pDrawTechnique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    //Set filter Keys
    m_pDrawFilterKey->setName(QStringLiteral("type"));
    m_pDrawFilterKey->setValue(QStringLiteral("draw"));

    //Add to technique
    m_pDrawTechnique->addFilterKey(m_pDrawFilterKey);
    m_pDrawTechnique->addRenderPass(m_pDrawRenderPass);

    //init signal processing
    m_pSignalDataParameter->setName(QStringLiteral("MeasurementVec"));
    m_pComputeRenderPass->addParameter(m_pSignalDataParameter);

    //Effect
    //Link shader and uniforms
    m_pEffect->addTechnique(m_pComputeTechnique);
    m_pEffect->addTechnique(m_pDrawTechnique);

    //Add to material
    this->setEffect(m_pEffect);

    //init timer and connect with updateSignalBuffer
    connect(m_pTimer, &QTimer::timeout,this, &ComputeMaterial::updateSignalBuffer);
    m_pTimer->start(16); //60Hz
}


//*************************************************************************************************************
