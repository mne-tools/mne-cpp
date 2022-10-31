//=============================================================================================================
/**
 * @file     customframegraph.h
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
 * @brief    CustomFrameGraph class declaration.
 *
 */

#ifndef DISP3DLIB_CUSTOMFRAMEGRAPH_H
#define DISP3DLIB_CUSTOMFRAMEGRAPH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <Qt3DRender/QViewport>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

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
        class QSortPolicy;
        class QRenderStateSet;
        class QDepthTest;
        class QNoDepthMask;
        class QBlendEquationArguments;
        class QBlendEquation;
        class QCullFace;
        class QRenderCapture;
        class QRenderCaptureReply;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class holds a custom framegraph that can be used for computations with OpenGL compute shader.
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
     *
     * @param[in] parent                 Pointer to parent node.
     */
    explicit CustomFrameGraph(Qt3DCore::QNode *parent = 0);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~CustomFrameGraph();

    //=========================================================================================================
    /**
     * This function sets active camera for use in the framegraphs camera selector.
     *
     * @param[in] tCamera               Pointer to QCamera object.
     */
    void setCamera(Qt3DRender::QCamera *tCamera);

    //=========================================================================================================
    /**
     * This function sets the work group size for the computation in each dimension.
     *
     * @param[in] tX                     Size of X work group.
     * @param[in] tY                     Size of Y work group.
     * @param[in] tZ                     Size of Z work group.
     */
    void setWorkGroupSize(const uint tX, const uint tY , const uint tZ);

    //=========================================================================================================
    /**
     * Sets the clear color of the framegraph.
     *
     * @param[in] tColor        New clear color.
     */
    void setClearColor(const QColor &tColor);

    //=========================================================================================================
    /**
     * Used to request render capture. Only one render capture result is produced per requestCapture call.
     * The function returns a QRenderCaptureReply object, which receives the captured image when it is done.
     * The user is responsible for deallocating the returned object.
     */
    Qt3DRender::QRenderCaptureReply* requestRenderCaptureReply();

protected:

private:

    //=========================================================================================================
    /**
     * Init the  CustomFrameGraph object.
     */
    void init();

    QPointer<Qt3DRender::QRenderSurfaceSelector>    m_pSurfaceSelector;     /**< Frame graph node that declares the render surface. */

    QPointer<Qt3DRender::QClearBuffers>             m_pClearBuffers;        /**< Frame graph node that clears buffers in the branch. */

    QPointer<Qt3DRender::QNoDraw>                   m_pNoDraw;              /**< Frame graph node that prevents rendering in the branch. */

    QPointer<Qt3DRender::QDispatchCompute>          m_pDispatchCompute;     /**< Frame graph node that issues work to the compute shader. */

    QPointer<Qt3DRender::QTechniqueFilter>          m_pComputeFilter;       /**< Frame graph node selects the compute technique. */

    QPointer<Qt3DRender::QCameraSelector>           m_pCameraSelector;      /**< Frame graph node that selects the camera. */

    QPointer<Qt3DRender::QMemoryBarrier>            m_pMemoryBarrier;       /**< Frame graph node that emplaces a memory barrier to synchronize computing and rendering. */

    QPointer<Qt3DRender::QRenderStateSet>           m_pForwardState;        /**< Frame graph node that holds the depth test render state. */

    QPointer<Qt3DRender::QTechniqueFilter>          m_pForwardTranspFilter; /**< Frame graph node that selects the forward rendering technique for transparent objects. */

    QPointer<Qt3DRender::QRenderStateSet>           m_pTransparentState;    /**< Frame graph node that holds the render states for transparency. */

    QPointer<Qt3DRender::QTechniqueFilter>          m_pForwardFilter;       /**< Frame graph node that selects the forward rendering technique for opaque objects. */

    QPointer<Qt3DRender::QTechniqueFilter>          m_pForwardSortedFilter; /**< Frame graph node that selects the forward sorted rendering technique. */

    QPointer<Qt3DRender::QSortPolicy>               m_pSortPolicy;          /**< Frame graph node that defines the drawing order. */

    QPointer<Qt3DRender::QFilterKey>                m_pForwardTranspKey;    /**< Filter key for the transparent forward filter. */

    QPointer<Qt3DRender::QFilterKey>                m_pForwardKey;          /**< Filter key for the forward filter. */

    QPointer<Qt3DRender::QFilterKey>                m_pForwardSortedKey;    /**< Filter key for the sorted forward filter. */

    QPointer<Qt3DRender::QFilterKey>                m_pComputeKey;          /**< Filter key for the compute filter. */

    QPointer<Qt3DRender::QDepthTest>                m_pDepthTest;           /**< Depth test render state. */

    QPointer<Qt3DRender::QCullFace>                 m_pCullFace;            /**< Render state for face culling. */

    QPointer<Qt3DRender::QBlendEquation>            m_pBlendEquation;       /**< Blend equation render state. */

    QPointer<Qt3DRender::QBlendEquationArguments>   m_pBlendArguments;      /**< Blend equation arguments render state. */

    QPointer<Qt3DRender::QNoDepthMask>              m_pNoDepthMask;         /**< Render state for disableling writing to depth buffer. */

    QPointer<Qt3DRender::QRenderCapture>            m_pCapture;             /**< Used to take a screenshot of the render result. */

    bool                                            m_bUseOpenGl4_3;        /**< OpenGL version 4.3 status flag. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_CUSTOMFRAMEGRAPH_H
