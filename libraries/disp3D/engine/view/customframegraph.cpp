//=============================================================================================================
/**
* @file     customframegraph.cpp
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
* @brief    CustomFrameGraph class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "customframegraph.h"


//*************************************************************************************************************
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

CustomFrameGraph::CustomFrameGraph(Qt3DCore::QNode *parent)
    : QViewport(parent)
    , m_pSurfaceSelector(new QRenderSurfaceSelector(this))
    , m_pClearBuffers(new QClearBuffers(m_pSurfaceSelector))
    , m_pNoDraw(new QNoDraw(m_pClearBuffers))
    , m_pDispatchCompute(new QDispatchCompute(m_pSurfaceSelector))
    , m_pComputeFilter(new QTechniqueFilter(m_pDispatchCompute))
    , m_pCameraSelector(new QCameraSelector(m_pSurfaceSelector))
    , m_pForwardFilter(new QTechniqueFilter(m_pCameraSelector))
    , m_pMemoryBarrier(new QMemoryBarrier(m_pForwardFilter))
    , m_pForwardKey(new QFilterKey)
    , m_pComputeKey(new QFilterKey)
{
    init();
}


//*************************************************************************************************************

CustomFrameGraph::~CustomFrameGraph()
{
    m_pSurfaceSelector->deleteLater();
    m_pClearBuffers->deleteLater();
    m_pNoDraw->deleteLater();
    m_pDispatchCompute->deleteLater();
    m_pComputeFilter->deleteLater();
    m_pCameraSelector->deleteLater();
    m_pForwardFilter->deleteLater();
    m_pMemoryBarrier->deleteLater();
    m_pForwardKey->deleteLater();
    m_pComputeKey->deleteLater();
}


//*************************************************************************************************************

void CustomFrameGraph::setCamera(QCamera *tCamera)
{
    m_pCameraSelector->setCamera(tCamera);
}


//*************************************************************************************************************

void CustomFrameGraph::setWorkGroupSize(const uint tX, const uint tY, const uint tZ)
{
    m_pDispatchCompute->setWorkGroupX(tX);
    m_pDispatchCompute->setWorkGroupY(tY);
    m_pDispatchCompute->setWorkGroupZ(tZ);
}


//*************************************************************************************************************

void CustomFrameGraph::setClearColor(const QColor &tColor)
{
    m_pClearBuffers->setClearColor(tColor);
}


//*************************************************************************************************************

void CustomFrameGraph::init()
{
    this->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

    //Set ClearBuffer
    m_pClearBuffers->setBuffers(QClearBuffers::ColorDepthBuffer);
    m_pClearBuffers->setClearColor(Qt::black);

    //Set filter keys these need to match with the material.
    m_pComputeKey->setName(QStringLiteral("renderingStyle"));
    m_pComputeKey->setValue(QStringLiteral("compute"));

    m_pForwardKey->setName(QStringLiteral("renderingStyle"));
    m_pForwardKey->setValue(QStringLiteral("forward"));

    //Add Matches
    m_pComputeFilter->addMatch(m_pComputeKey);
    m_pForwardFilter->addMatch(m_pForwardKey);

    //Set Memory Barrier it ensures the finishing of the compute shader run before drawing the scene.
    m_pMemoryBarrier->setWaitOperations(QMemoryBarrier::VertexAttributeArray);
}


//*************************************************************************************************************
