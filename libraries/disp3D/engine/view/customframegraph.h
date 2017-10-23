//=============================================================================================================
/**
* @file     customframegraph.h
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
* @brief    CustomFrameGraph class declaration.
*
*/

#ifndef DISP3DLIB_CUSTOMFRAMEGRAPH_H
#define DISP3DLIB_CUSTOMFRAMEGRAPH_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D_global.h>

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
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {


//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* This class holds a custom framegraph that can be used for computations with OpenGL compute shadern.
*
* @brief Custom framegaph class.
*/

class DISP3DSHARED_EXPORT CustomFrameGraph : public Qt3DRender::QViewport
{
    Q_OBJECT
public:
    typedef QSharedPointer<CustomFrameGraph> SPtr;            /**< Shared pointer type for CustomFrameGraph. */
    typedef QSharedPointer<const CustomFrameGraph> ConstSPtr; /**< Const shared pointer type for CustomFrameGraph. */

    //=========================================================================================================
    /**
    * Constructs a CustomFrameGraph object.
    */
    explicit CustomFrameGraph(Qt3DCore::QNode *parent = 0);

    //=========================================================================================================
    /**
    * Copy constructor disabled.
    */
    CustomFrameGraph(const CustomFrameGraph &other) = delete;

    //=========================================================================================================
    /**
    * Copy operator disabled.
    */
    CustomFrameGraph& operator =(const CustomFrameGraph &other) = delete;

    //=========================================================================================================
    /**
    * Destructor.
    */
    ~CustomFrameGraph();

    //=========================================================================================================
    /**
     * Sets active camera for use in the framegraphs camera selector.
     *
     * @param tCamera               Pointer to QCamera object.
     */
    void setCamera(Qt3DRender::QCamera *tCamera);

    //=========================================================================================================
    /**
     * Sets the work group size for the computation in each dim.
     *
     * @param tX                     Size of X work group.
     * @param tY                     Size of Y work group.
     * @param tZ                     Size of Z work group.
     */
    void setWorkGroupSize(const uint tX, const uint tY , const uint tZ);

    //=========================================================================================================
    /**
     * Sets the clear color of the framegraph.
     *
     * @param tColor        New clear color.
     */
    void setClearColor(const QColor &tColor);

protected:

private:

    //=========================================================================================================
    /**
     * Init the  CustomFrameGraph class.
     */
    void init();


    QPointer<Qt3DRender::QRenderSurfaceSelector> m_pSurfaceSelector;

    QPointer<Qt3DRender::QClearBuffers> m_pClearBuffers;

    QPointer<Qt3DRender::QNoDraw> m_pNoDraw;

    QPointer<Qt3DRender::QDispatchCompute> m_pDispatchCompute;

    QPointer<Qt3DRender::QTechniqueFilter> m_pComputeFilter;

    QPointer<Qt3DRender::QCameraSelector> m_pCameraSelector;

    QPointer<Qt3DRender::QTechniqueFilter> m_pForwardFilter;

    QPointer<Qt3DRender::QMemoryBarrier> m_pMemoryBarrier;

    QPointer<Qt3DRender::QFilterKey> m_pForwardKey;

    QPointer<Qt3DRender::QFilterKey> m_pComputeKey;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CUSTOMFRAMEGRAPH_H
