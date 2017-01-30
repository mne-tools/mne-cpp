//=============================================================================================================
/**
* @file     shadermaterial.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
, m_pVertexGL3Technique(new QTechnique())
, m_pVertexGL3RenderPass(new QRenderPass())
, m_pVertexGL3Shader(new QShaderProgram())
, m_pFilterKey(new QFilterKey)
, m_pNoDepthMask(new QNoDepthMask())
, m_pBlendState(new QBlendEquationArguments())
, m_pBlendEquation(new QBlendEquation())
, m_bShaderInit(false)
{
    this->init();
}


//*************************************************************************************************************

ShaderMaterial::~ShaderMaterial()
{
}


//*************************************************************************************************************

void ShaderMaterial::init()
{
    //Set OpenGL version
    m_pVertexGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_pVertexGL3Technique->graphicsApiFilter()->setMajorVersion(4);
    m_pVertexGL3Technique->graphicsApiFilter()->setMinorVersion(0);
    m_pVertexGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    //Setup transparency
    m_pBlendState->setSourceRgb(QBlendEquationArguments::SourceAlpha);
    m_pBlendState->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);
    m_pBlendEquation->setBlendFunction(QBlendEquation::Add);

    m_pVertexGL3RenderPass->addRenderState(m_pBlendEquation);
    m_pVertexGL3RenderPass->addRenderState(m_pNoDepthMask);
    m_pVertexGL3RenderPass->addRenderState(m_pBlendState);

    m_pFilterKey->setName(QStringLiteral("renderingStyle"));
    m_pFilterKey->setValue(QStringLiteral("forward"));
    m_pVertexGL3Technique->addFilterKey(m_pFilterKey);

    m_pVertexGL3Technique->addRenderPass(m_pVertexGL3RenderPass);

    m_pVertexEffect->addTechnique(m_pVertexGL3Technique);

    this->setEffect(m_pVertexEffect);
}


//*************************************************************************************************************

void ShaderMaterial::setShader(const QUrl& sShader)
{
    QString fileName = sShader.fileName();

    if(fileName.contains(".vert")) {
        m_pVertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(sShader));
    }

    if(fileName.contains(".tcs")) {
        m_pVertexGL3Shader->setTessellationControlShaderCode(QShaderProgram::loadSource(sShader));
    }

    if(fileName.contains(".tes")) {
        m_pVertexGL3Shader->setTessellationEvaluationShaderCode(QShaderProgram::loadSource(sShader));
    }

    if(fileName.contains(".geom")) {
        m_pVertexGL3Shader->setGeometryShaderCode(QShaderProgram::loadSource(sShader));
    }

    if(fileName.contains(".frag")) {
        m_pVertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(sShader));
    }    

    if(!m_bShaderInit) {
        m_pVertexGL3RenderPass->setShaderProgram(m_pVertexGL3Shader);
        m_bShaderInit = true;
    }
}
