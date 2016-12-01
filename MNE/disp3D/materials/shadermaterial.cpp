//=============================================================================================================
/**
* @file     shadermaterial.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Februaray, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    ShaderMaterial class definition
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "shadermaterial.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <QFilterKey>
#include <Qt3DRender/qdepthtest.h>
#include <Qt3DRender/qblendequation.h>
#include <Qt3DRender/qblendequationarguments.h>
#include <Qt3DRender/qnodepthmask.h>
#include <Qt3DRender/qgraphicsapifilter.h>

#include <QUrl>
#include <QVector3D>
#include <QVector4D>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ShaderMaterial::ShaderMaterial(QNode *parent)
: QMaterial(parent)
, m_pVertexEffect(new QEffect())
, m_pAmbientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
, m_pDiffuseParameter(new QParameter(QStringLiteral("kd"), QColor::fromRgbF(0.7f, 0.7f, 0.7f, 1.0f)))
, m_pSpecularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.1f, 0.1f, 0.1f, 1.0f)))
, m_pShininessParameter(new QParameter(QStringLiteral("shininess"), 10.0f))
, m_pAlphaParameter(new QParameter("alpha", 0.5f))
, m_pVertexGL3Technique(new QTechnique())
, m_pVertexGL3RenderPass(new QRenderPass())
, m_pVertexGL3Shader(new QShaderProgram())
, m_pFilterKey(new QFilterKey)
, m_pNoDepthMask(new QNoDepthMask())
, m_pBlendState(new QBlendEquationArguments())
, m_pBlendEquation(new QBlendEquation())
{
    this->init();
}


//*************************************************************************************************************

ShaderMaterial::~ShaderMaterial()
{
    //Not sure if Qt3d module implemented internal garbage handling, so I do it manually here
//    delete m_pAmbientParameter;
//    delete m_pDiffuseParameter;
//    delete m_pSpecularParameter;
//    delete m_pShininessParameter;
//    delete m_pAlphaParameter;
//    delete m_pFilterKey;

//    delete m_pVertexGL3Shader;
//    delete m_pVertexGL3RenderPass;

//    delete m_pVertexGL3Technique;

//    delete m_pNoDepthMask;
//    delete m_pBlendState;
//    delete m_pBlendEquation;

//    delete m_pVertexEffect;
}


//*************************************************************************************************************

void ShaderMaterial::init()
{
    //Set shaders
    //m_pVertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/brain.vert"))));
    //m_pVertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/brain.frag"))));

    //Old with no tesselation
//    m_pVertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/pervertexphongalpha_old.vert"))));
//    m_pVertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/pervertexphongalpha_old.frag"))));

    //New with tesselation
    m_pVertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/pervertexphongalpha.vert"))));

    m_pVertexGL3Shader->setTessellationControlShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/triangles.tcs"))));
    m_pVertexGL3Shader->setTessellationEvaluationShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/triangles_bezier_interpolation.tes"))));
    //m_pVertexGL3Shader->setTessellationEvaluationShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/triangles_simple_interpolation.tes"))));

    m_pVertexGL3Shader->setGeometryShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/pervertexphongalpha.geom"))));

    m_pVertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/pervertexphongalpha.frag"))));

    //Set OpenGL version
    m_pVertexGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pVertexGL3Technique->graphicsApiFilter()->setMajorVersion(4);
    m_pVertexGL3Technique->graphicsApiFilter()->setMinorVersion(0);
    m_pVertexGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_pVertexGL3RenderPass->setShaderProgram(m_pVertexGL3Shader);

//    //Setup transparency
//    m_pBlendState->setSourceRgb(QBlendEquationArguments::SourceAlpha);
//    m_pBlendState->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);
//    m_pBlendEquation->setBlendFunction(QBlendEquation::Add);

//    m_pVertexGL3RenderPass->addRenderState(m_pBlendEquation);
//    m_pVertexGL3RenderPass->addRenderState(m_pNoDepthMask);
//    m_pVertexGL3RenderPass->addRenderState(m_pBlendState);

    m_pFilterKey->setName(QStringLiteral("renderingStyle"));
    m_pFilterKey->setValue(QStringLiteral("forward"));
    m_pVertexGL3Technique->addFilterKey(m_pFilterKey);

    m_pVertexGL3Technique->addRenderPass(m_pVertexGL3RenderPass);

    m_pVertexEffect->addTechnique(m_pVertexGL3Technique);

    m_pVertexEffect->addParameter(m_pAmbientParameter);
    m_pVertexEffect->addParameter(m_pDiffuseParameter);
    m_pVertexEffect->addParameter(m_pSpecularParameter);
    m_pVertexEffect->addParameter(m_pShininessParameter);
    m_pVertexEffect->addParameter(m_pAlphaParameter);

    this->setEffect(m_pVertexEffect);
}


//*************************************************************************************************************

float ShaderMaterial::alpha()
{
    return m_pAlphaParameter->value().toFloat();
}


//*************************************************************************************************************

void ShaderMaterial::setAlpha(float alpha)
{
    m_pAlphaParameter->setValue(alpha);
}

