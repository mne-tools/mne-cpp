//=============================================================================================================
/**
 * @file     gpuinterpolationmaterial.cpp
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
 * @brief    GpuInterpolationMaterial class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpuinterpolationmaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QGraphicsApiFilter>
#include <QUrl>

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

GpuInterpolationMaterial::GpuInterpolationMaterial(bool bUseSortPolicy, Qt3DCore::QNode *parent)
    : AbstractPhongAlphaMaterial(bUseSortPolicy, parent)
    , m_pComputeShader(new QShaderProgram)
    , m_pComputeRenderPass(new QRenderPass)
    , m_pComputeFilterKey(new QFilterKey)
    , m_pComputeTechnique(new QTechnique)
    , m_pDrawShader(new QShaderProgram)
    , m_pDrawRenderPass(new QRenderPass)
    , m_pDrawTechnique(new QTechnique)
    , m_pSignalDataParameter(new QParameter)
    , m_pColsParameter(new QParameter)
    , m_pRowsParameter(new QParameter)
    , m_pInterpolationMatParameter(new QParameter)
    , m_pOutputColorParameter(new QParameter)
    , m_pThresholdXParameter(new QParameter(QStringLiteral("fThresholdX"), 1e-10f))
    , m_pThresholdZParameter(new QParameter(QStringLiteral("fThresholdZ"), 6e-6f))
    , m_pColormapParameter(new QParameter(QStringLiteral("ColormapType"), 3))
{
    init();
    setShaderCode();
}

//=============================================================================================================

void GpuInterpolationMaterial::init()
{
    //Compute part
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

    //Set default Interpolation matrix parameters
    m_pColsParameter->setName(QStringLiteral("cols"));
    m_pColsParameter->setValue(1);
    m_pRowsParameter->setName(QStringLiteral("rows"));
    m_pRowsParameter->setValue(1);
    m_pInterpolationMatParameter->setName(QStringLiteral("InterpolationMat"));

    //Set default output
    m_pOutputColorParameter->setName(QStringLiteral("OutputColor"));

    //Set default input
    m_pSignalDataParameter->setName(QStringLiteral("InputVec"));

    //Add compute Parameter
    m_pComputeRenderPass->addParameter(m_pColsParameter);
    m_pComputeRenderPass->addParameter(m_pRowsParameter);
    m_pComputeRenderPass->addParameter(m_pOutputColorParameter);
    m_pComputeRenderPass->addParameter(m_pInterpolationMatParameter);
    m_pComputeRenderPass->addParameter(m_pSignalDataParameter);

    //Add Threshold parameter
    m_pComputeRenderPass->addParameter(m_pThresholdXParameter);
    m_pComputeRenderPass->addParameter(m_pThresholdZParameter);

    //Add ColormapType
    m_pComputeRenderPass->addParameter(m_pColormapParameter);

    //Draw part
    //Add Phongalpha parameter
    m_pDrawRenderPass->addParameter(m_pDiffuseParameter);
    m_pDrawRenderPass->addParameter(m_pSpecularParameter);
    m_pDrawRenderPass->addParameter(m_pShininessParameter);
    m_pDrawRenderPass->addParameter(m_pAlphaParameter);

    //Set OpenGL version
    m_pDrawTechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pDrawTechnique->graphicsApiFilter()->setMajorVersion(4);
    m_pDrawTechnique->graphicsApiFilter()->setMinorVersion(3);
    m_pDrawTechnique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

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

//=============================================================================================================

void GpuInterpolationMaterial::setShaderCode()
{
    m_pComputeShader->setComputeShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.csh"))));
    m_pComputeRenderPass->setShaderProgram(m_pComputeShader);

    m_pDrawShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.vert"))));
    m_pDrawShader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/compute/interpolation.frag"))));
    m_pDrawRenderPass->setShaderProgram(m_pDrawShader);
}

//=============================================================================================================

