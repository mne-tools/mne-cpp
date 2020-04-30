//=============================================================================================================
/**
 * @file     abstractphongalphamaterial.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    AbstractPhongAlphaMaterial class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractphongalphamaterial.h"

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
#include <Qt3DRender/qgraphicsapifilter.h>

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

AbstractPhongAlphaMaterial::AbstractPhongAlphaMaterial(bool bUseSortPolicy, QNode *parent)
    : QMaterial(parent)
    , m_pEffect(new QEffect())
    , m_pDiffuseParameter(new QParameter(QStringLiteral("kd"), QColor::fromRgbF(0.7f, 0.7f, 0.7f, 1.0f)))
    , m_pSpecularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.1f, 0.1f, 0.1f, 1.0f)))
    , m_pShininessParameter(new QParameter(QStringLiteral("shininess"), 2.0f))
    , m_pAlphaParameter(new QParameter("alpha", 0.75f))
    , m_pDrawFilterKey(new QFilterKey)
    , m_bUseSortPolicy(bUseSortPolicy)
{
    connect(m_pAlphaParameter.data(), &QParameter::valueChanged,
            this, & AbstractPhongAlphaMaterial::onAlphaChanged);

    //Init filter keys
    m_pDrawFilterKey->setName(QStringLiteral("renderingStyle"));
    onAlphaChanged(m_pAlphaParameter->value());
}

//=============================================================================================================

float AbstractPhongAlphaMaterial::alpha() const
{
    return m_pAlphaParameter->value().toFloat();
}

//=============================================================================================================

void AbstractPhongAlphaMaterial::setAlpha(float fAlpha)
{
    m_pAlphaParameter->setValue(fAlpha);
}

//=============================================================================================================

void AbstractPhongAlphaMaterial::init()
{
    //Set OpenGL version
    QTechnique *pVertexGL3Technique = new QTechnique;
    QTechnique *pVertexGL2Technique = new QTechnique;
    QTechnique *pVertexES2Technique = new QTechnique;

    pVertexGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    pVertexGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    pVertexGL3Technique->graphicsApiFilter()->setMinorVersion(2);
    pVertexGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    pVertexGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    pVertexGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    pVertexGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    pVertexGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    pVertexES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    pVertexES2Technique->graphicsApiFilter()->setMajorVersion(2);
    pVertexES2Technique->graphicsApiFilter()->setMinorVersion(0);
    pVertexES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    pVertexGL3Technique->addFilterKey(m_pDrawFilterKey);
    pVertexGL2Technique->addFilterKey(m_pDrawFilterKey);
    pVertexES2Technique->addFilterKey(m_pDrawFilterKey);

    QRenderPass *pVertexGL3RenderPass = new QRenderPass;
    QRenderPass *pVertexGL2RenderPass = new QRenderPass;
    QRenderPass *pVertexES2RenderPass = new QRenderPass;

    //Set Names to find them in child materials
    pVertexGL3RenderPass->setObjectName(QStringLiteral("pVertexGL3RenderPass"));
    pVertexGL2RenderPass->setObjectName(QStringLiteral("pVertexGL2RenderPass"));
    pVertexES2RenderPass->setObjectName(QStringLiteral("pVertexES2RenderPass"));

    pVertexGL3Technique->addRenderPass(pVertexGL3RenderPass);
    pVertexGL2Technique->addRenderPass(pVertexGL2RenderPass);
    pVertexES2Technique->addRenderPass(pVertexES2RenderPass);

    m_pEffect->addTechnique(pVertexGL3Technique);
    m_pEffect->addTechnique(pVertexGL2Technique);
    m_pEffect->addTechnique(pVertexES2Technique);

    m_pEffect->addParameter(m_pDiffuseParameter);
    m_pEffect->addParameter(m_pSpecularParameter);
    m_pEffect->addParameter(m_pShininessParameter);
    m_pEffect->addParameter(m_pAlphaParameter);

    this->setEffect(m_pEffect);
}

//=============================================================================================================

void AbstractPhongAlphaMaterial::addShaderToRenderPass(const QString &sObjectName, QShaderProgram *pShaderProgramm)
{
    if(QRenderPass *pRenderPass = this->findChild<QRenderPass*>(sObjectName)) {
        pRenderPass->setShaderProgram(pShaderProgramm);
    }
    else {
        qDebug() << "AbstractPhongAlphaMaterial::addShaderToRenderPass: Renderpass " << sObjectName <<  " not found!";
    }
}

//=============================================================================================================

void AbstractPhongAlphaMaterial::onAlphaChanged(const QVariant &fAlpha)
{
    if(fAlpha.canConvert<float>())
    {
        float tempAlpha = fAlpha.toFloat();

        if(tempAlpha >= 1.0f) {
            m_pDrawFilterKey->setValue(QStringLiteral("forward"));
        } else {
            if(m_bUseSortPolicy) {
                m_pDrawFilterKey->setValue(QStringLiteral("forwardSorted"));
            } else {
                m_pDrawFilterKey->setValue(QStringLiteral("forwardTransparent"));
            }
        }
    }
}

//=============================================================================================================
