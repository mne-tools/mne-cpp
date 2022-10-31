//=============================================================================================================
/**
 * @file     geometrymultipliermaterial.cpp
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
 * @brief    GeometryMultiplierMaterial class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometrymultipliermaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QUrl>
#include <QColor>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qparameter.h>
#include <QFilterKey>

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

GeometryMultiplierMaterial::GeometryMultiplierMaterial(bool bUseSortPolicy, Qt3DCore::QNode *parent)
: AbstractPhongAlphaMaterial(bUseSortPolicy, parent)
, m_pAmbientColor(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f)))
, m_pVertexGL3Shader(new QShaderProgram())
, m_pVertexES2Shader(new QShaderProgram())
{
    init();
    setShaderCode();
    m_pEffect->addParameter(m_pAmbientColor);
}

//=============================================================================================================

void GeometryMultiplierMaterial::setShaderCode()
{
    m_pVertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/gl3/instancedposition.vert"))));
    m_pVertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/gl3/instancedposition.frag"))));

    m_pVertexES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/es2/instancedposition.vert"))));
    m_pVertexES2Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/engine/model/materials/shaders/es2/instancedposition.frag"))));

    addShaderToRenderPass(QStringLiteral("pVertexGL3RenderPass"), m_pVertexGL3Shader);
    addShaderToRenderPass(QStringLiteral("pVertexGL2RenderPass"), m_pVertexES2Shader);
    addShaderToRenderPass(QStringLiteral("pVertexES2RenderPass"), m_pVertexES2Shader);
}

//=============================================================================================================

void GeometryMultiplierMaterial::setAmbient(const QColor &tColor)
{
    m_pAmbientColor->setValue(tColor);
}

//=============================================================================================================

