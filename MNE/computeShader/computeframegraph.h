//=============================================================================================================
/**
* @file     computeframegraph.h
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
* @brief     ComputeFramegraph class declaration.
*
*/

#ifndef CSH_COMPUTEFRAMEGRAPH_H
#define CSH_COMPUTEFRAMEGRAPH_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computeShader_global.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <Qt3DRender/QViewport>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
namespace Qt3DRender {
        class QRenderSurfaceSelector;
        class QClearBuffers;
        class QNoDraw;
        class QDispatchCompute;
        class QTechniqueFilter;
        class QCameraSelector;
        class QCamera;
        class QFilterKey;
        class QMemoryBarrier;
        class QCamera;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSH
//=============================================================================================================

namespace CSH {


//*************************************************************************************************************
//=============================================================================================================
// CSH FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class COMPUTE_SHADERSHARED_EXPORT ComputeFramegraph : public Qt3DRender::QViewport
{

public:
    typedef QSharedPointer<ComputeFramegraph> SPtr;            /**< Shared pointer type for ComputeFramegraph. */
    typedef QSharedPointer<const ComputeFramegraph> ConstSPtr; /**< Const shared pointer type for ComputeFramegraph. */

    //=========================================================================================================
    /**
    * Constructs a ComputeFramegraph object.
    */
    explicit ComputeFramegraph(Qt3DCore::QNode *parent = 0);

    //void setWorkGroups(const int x, const int y, const int z);

    void setCamera(Qt3DRender::QCamera *pCamera);

    void setWorkGroupSize(const unsigned int x, const unsigned int y , const unsigned int z);

    //Qt3DRender::QCamera *getCamera();

protected:

private:
    void init();

//    QPointer<Qt3DRender::QCamera> m_pCamera;

    QPointer<Qt3DRender::QRenderSurfaceSelector> m_pSurfaceSelector;

    QPointer<Qt3DRender::QClearBuffers> m_pClearBuffers;

    QPointer<Qt3DRender::QNoDraw> m_pNoDraw;

    QPointer<Qt3DRender::QDispatchCompute> m_pDispatchCompute;

    QPointer<Qt3DRender::QFilterKey> m_pComputeKey;
    QPointer<Qt3DRender::QFilterKey> m_pDrawKey;

    QPointer<Qt3DRender::QCameraSelector> m_pCameraSelector;

    QPointer<Qt3DRender::QTechniqueFilter> m_pComputeFilter;
    QPointer<Qt3DRender::QTechniqueFilter> m_pDrawFilter;



    QPointer<Qt3DRender::QMemoryBarrier> m_pMemoryBarrier;


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#endif // CSH_COMPUTEFRAMEGRAPH_H
