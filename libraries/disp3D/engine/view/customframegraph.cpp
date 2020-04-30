//=============================================================================================================
/**
 * @file     customframegraph.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2017
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
 * @brief    CustomFrameGraph class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "customframegraph.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QNode>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/QDispatchCompute>
#include <Qt3DRender/QTechniqueFilter>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QMemoryBarrier>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QRenderCapture>
#include <QGLFormat>

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

CustomFrameGraph::CustomFrameGraph(Qt3DCore::QNode *parent)
: QViewport(parent)
, m_pForwardTranspKey(new QFilterKey)
, m_pForwardKey(new QFilterKey)
, m_pForwardSortedKey(new QFilterKey)
, m_pComputeKey(new QFilterKey)
, m_pDepthTest(new QDepthTest)
, m_pCullFace(new QCullFace)
, m_pBlendEquation(new QBlendEquation)
, m_pBlendArguments(new QBlendEquationArguments)
, m_pNoDepthMask( new QNoDepthMask)
, m_bUseOpenGl4_3(false)
{
    //Test for OpenGL version 4.3
    if(QGLFormat::openGLVersionFlags() >= QGLFormat::OpenGL_Version_4_3) {
        m_bUseOpenGl4_3 = true;
    }

    init();
}

//=============================================================================================================

CustomFrameGraph::~CustomFrameGraph()
{
    m_pSurfaceSelector->deleteLater();
    m_pClearBuffers->deleteLater();
    m_pNoDraw->deleteLater();
    m_pDispatchCompute->deleteLater();
    m_pComputeFilter->deleteLater();
    m_pCameraSelector->deleteLater();
    m_pForwardFilter->deleteLater();
    m_pSortPolicy->deleteLater();
    m_pForwardKey->deleteLater();
    m_pComputeKey->deleteLater();
    m_pForwardState->deleteLater();
    m_pTransparentState->deleteLater();
    m_pForwardSortedFilter->deleteLater();
    m_pForwardSortedKey->deleteLater();
    m_pForwardTranspFilter->deleteLater();
    m_pForwardTranspKey->deleteLater();
    m_pDepthTest->deleteLater();
    m_pCullFace->deleteLater();
    m_pBlendEquation->deleteLater();
    m_pBlendArguments->deleteLater();
    m_pNoDepthMask->deleteLater();
    if(m_bUseOpenGl4_3){
        m_pMemoryBarrier->deleteLater();
    }
}

//=============================================================================================================

void CustomFrameGraph::setCamera(QCamera *tCamera)
{
    m_pCameraSelector->setCamera(tCamera);
}

//=============================================================================================================

void CustomFrameGraph::setWorkGroupSize(const uint tX, const uint tY, const uint tZ)
{
    m_pDispatchCompute->setWorkGroupX(tX);
    m_pDispatchCompute->setWorkGroupY(tY);
    m_pDispatchCompute->setWorkGroupZ(tZ);
}

//=============================================================================================================

void CustomFrameGraph::setClearColor(const QColor &tColor)
{
    m_pClearBuffers->setClearColor(tColor);
}

//=============================================================================================================

QRenderCaptureReply* CustomFrameGraph::requestRenderCaptureReply()
{
    return m_pCapture->requestCapture();
}

//=============================================================================================================

void CustomFrameGraph::init()
{
    //Build the frame graph
    m_pSurfaceSelector = new QRenderSurfaceSelector(this);

    //Clear buffer branch
    m_pClearBuffers = new QClearBuffers(m_pSurfaceSelector);
    m_pNoDraw = new QNoDraw(m_pClearBuffers);

    //Compute branch
    m_pDispatchCompute = new QDispatchCompute(m_pSurfaceSelector);
    m_pComputeFilter = new QTechniqueFilter(m_pDispatchCompute);

    // Forward render branch
    m_pCameraSelector = new QCameraSelector(m_pSurfaceSelector);

    if(m_bUseOpenGl4_3) {
        m_pMemoryBarrier = new QMemoryBarrier(m_pCameraSelector);
        m_pForwardState = new QRenderStateSet(m_pMemoryBarrier);

        //Set Memory Barrier it ensures the finishing of the compute shader run before drawing the scene.
        m_pMemoryBarrier->setWaitOperations(QMemoryBarrier::VertexAttributeArray);
    } else {
        //don't use memory barrier
        m_pForwardState = new QRenderStateSet(m_pCameraSelector);
    }

    m_pForwardFilter = new QTechniqueFilter(m_pForwardState);

    //Transparent forward render branch
    m_pTransparentState = new QRenderStateSet(m_pForwardState);
    m_pForwardTranspFilter = new QTechniqueFilter(m_pTransparentState);

    //Transparent sorted forward render branch
    m_pForwardSortedFilter = new QTechniqueFilter(m_pTransparentState);
    m_pSortPolicy = new QSortPolicy(m_pForwardSortedFilter);
    m_pCapture = new QRenderCapture(m_pSortPolicy);

    //Init frame graph nodes
    this->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

    //Set ClearBuffer
    m_pClearBuffers->setBuffers(QClearBuffers::ColorDepthBuffer);
    m_pClearBuffers->setClearColor(Qt::black);

    //Set depth test
    m_pDepthTest->setDepthFunction(QDepthTest::Less);
    m_pForwardState->addRenderState(m_pDepthTest);
    m_pCullFace->setMode(QCullFace::Back);
    m_pForwardState->addRenderState(m_pCullFace);

    //Set Transparent states
    m_pBlendArguments->setSourceRgb(QBlendEquationArguments::SourceAlpha);
    m_pBlendArguments->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);
    m_pBlendEquation->setBlendFunction(QBlendEquation::Add);
    m_pTransparentState->addRenderState(m_pBlendArguments);
    m_pTransparentState->addRenderState(m_pBlendEquation);
    m_pTransparentState->addRenderState(m_pNoDepthMask);

    //Set filter keys these need to match with the material.
    m_pComputeKey->setName(QStringLiteral("renderingStyle"));
    m_pComputeKey->setValue(QStringLiteral("compute"));

    m_pForwardTranspKey->setName(QStringLiteral("renderingStyle"));
    m_pForwardTranspKey->setValue(QStringLiteral("forwardTransparent"));

    m_pForwardKey->setName(QStringLiteral("renderingStyle"));
    m_pForwardKey->setValue(QStringLiteral("forward"));

    m_pForwardSortedKey->setName(QStringLiteral("renderingStyle"));
    m_pForwardSortedKey->setValue(QStringLiteral("forwardSorted"));

    //Add Matches
    m_pComputeFilter->addMatch(m_pComputeKey);
    m_pForwardFilter->addMatch(m_pForwardKey);
    m_pForwardSortedFilter->addMatch(m_pForwardSortedKey);
    m_pForwardTranspFilter->addMatch(m_pForwardTranspKey);

    //Set draw policy
    QVector<QSortPolicy::SortType> sortTypes = {QSortPolicy::StateChangeCost, QSortPolicy::BackToFront};
    m_pSortPolicy->setSortTypes(sortTypes);
}

//=============================================================================================================
