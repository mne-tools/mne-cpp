//=============================================================================================================
/**
* @file     computeframegraph.cpp
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
* @brief    ComputeFramegraph class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computeframegraph.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


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

ComputeFramegraph::ComputeFramegraph(Qt3DCore::QNode *parent)
    : QViewport(parent)
    , m_pSurfaceSelector(new QRenderSurfaceSelector(this))
    , m_pClearBuffers(new QClearBuffers(m_pSurfaceSelector))
    , m_pNoDraw(new QNoDraw(m_pClearBuffers))
    , m_pDispatchCompute(new QDispatchCompute(m_pSurfaceSelector))
    , m_pComputeFilter(new QTechniqueFilter(m_pDispatchCompute))
    , m_pCameraSelector(new QCameraSelector(m_pSurfaceSelector))
    , m_pDrawFilter(new QTechniqueFilter(m_pCameraSelector))
    , m_pMemoryBarrier(new QMemoryBarrier(m_pDrawFilter))
    , m_pDrawKey(new QFilterKey)
    , m_pComputeKey(new QFilterKey)
   // , m_pCamera(new QCamera)
{
    init();
}

void ComputeFramegraph::setCamera(QCamera *pCamera)
{

    m_pCameraSelector->setCamera(pCamera);
}

void ComputeFramegraph::setWorkGroupSize(const unsigned int x, const unsigned int y, const unsigned int z)
{
    m_pDispatchCompute->setWorkGroupX(x);
    m_pDispatchCompute->setWorkGroupY(y);
    m_pDispatchCompute->setWorkGroupZ(z);

}

//QCamera *ComputeFramegraph::getCamera()
//{
//    return m_pCamera.data();
//}

void ComputeFramegraph::init()
{
    this->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

    //Set ClearBuffers
    m_pClearBuffers->setBuffers(QClearBuffers::ColorDepthBuffer);
    m_pClearBuffers->setClearColor(Qt::black);

    //Set Workgroup size
    //@TO-DO set correct workgroup size
    m_pDispatchCompute->setWorkGroupX(50);
    m_pDispatchCompute->setWorkGroupY(1);
    m_pDispatchCompute->setWorkGroupZ(1);

    //Set FilterKeys
    m_pComputeKey->setName(QStringLiteral("type"));
    m_pComputeKey->setValue(QStringLiteral("compute"));

    m_pDrawKey->setName(QStringLiteral("type"));
    m_pDrawKey->setValue(QStringLiteral("draw"));

    //Add Matches
    m_pComputeFilter->addMatch(m_pComputeKey);
    m_pDrawFilter->addMatch(m_pDrawKey);


    //Set Memory Barrier
    //@TO-DO change this if needed
    m_pMemoryBarrier->setWaitOperations(QMemoryBarrier::VertexAttributeArray);

    //Set camera
//    m_pCamera->setProjectionType(QCameraLens::PerspectiveProjection);
//    m_pCamera->setViewCenter(QVector3D(0, 0, 0));
//    m_pCamera->setPosition(QVector3D(0, 0, -40.0f));
//    m_pCamera->setNearPlane(0.1f);
//    m_pCamera->setFarPlane(1000.0f);
//    m_pCamera->setFieldOfView(25.0f);
//    m_pCamera->setAspectRatio(1.33f);
//    m_pCameraSelector->setCamera(m_pCamera);
    //camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);


}


//*************************************************************************************************************
