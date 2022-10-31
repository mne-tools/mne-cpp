//=============================================================================================================
/**
 * @file     gpuinterpolationmaterial.h
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
 * @brief     GpuInterpolationMaterial class declaration.
 *
 */

#ifndef DISP3DLIB_GPUINTERPOLATIONMATERIAL_H
#define DISP3DLIB_GPUINTERPOLATIONMATERIAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"
#include "abstractphongalphamaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace QtCore {
        class QVector3D;
}

namespace Qt3DRender {
        class QParameter;
        class QShaderProgram;
        class QRenderPass;
        class QFilterKey;
        class QTechnique;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPDLIB
//=============================================================================================================

namespace DISP3DLIB {

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class stores the material used for interpolation with a compute shader.
 *
 * @brief Compute shader interpolation material.
 */

class DISP3DSHARED_EXPORT GpuInterpolationMaterial : public AbstractPhongAlphaMaterial
{
    Q_OBJECT

public:
    typedef QSharedPointer<GpuInterpolationMaterial> SPtr;            /**< Shared pointer type for GpuInterpolationMaterial. */
    typedef QSharedPointer<const GpuInterpolationMaterial> ConstSPtr; /**< Const shared pointer type for GpuInterpolationMaterial. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] bUseSortPolicy     Whether to use the sort policy in the framegraph.
     * @param[in] parent             The parent of this class.
     */
    explicit GpuInterpolationMaterial(bool bUseSortPolicy = false, Qt3DCore::QNode *parent = nullptr);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    virtual ~GpuInterpolationMaterial() = default;

protected:
    //=========================================================================================================
    /**
     * Init GpuInterpolationMaterial class.
     */
    void init() override;

    //=========================================================================================================
    /**
     * Add the shader code to the material.
     */
    void setShaderCode() override;

    //Compute Part
    QPointer<Qt3DRender::QShaderProgram>                m_pComputeShader;           /**< Stores the shader program of the compute shader. */
    QPointer<Qt3DRender::QRenderPass>                   m_pComputeRenderPass;       /**< The render pass for the compute run. */
    QPointer<Qt3DRender::QFilterKey>                    m_pComputeFilterKey;        /**< The compute filter key. */
    QPointer<Qt3DRender::QTechnique>                    m_pComputeTechnique;        /**< The technique of the compute shader. This should match with the frame graph. */

    //Draw Part
    QPointer<Qt3DRender::QShaderProgram>                m_pDrawShader;              /**< Stores the shader program of the draw shader. */
    QPointer<Qt3DRender::QRenderPass>                   m_pDrawRenderPass;          /**< The render pass for the draw run. */
    QPointer<Qt3DRender::QTechnique>                    m_pDrawTechnique;           /**< The technique of the draw shader. This should match with the frame graph. */

    //Measurement signal
    QPointer<Qt3DRender::QParameter>                    m_pSignalDataParameter;     /**< This parameter holds the signal data buffer. */

    //Interpolation matrix parameter
    QPointer<Qt3DRender::QParameter>                    m_pColsParameter;           /**< This parameter holds the number of columns in the Interpolation matrix. */
    QPointer<Qt3DRender::QParameter>                    m_pRowsParameter;           /**< This parameter holds the number of rows in the Interpolation matrix. */
    QPointer<Qt3DRender::QParameter>                    m_pInterpolationMatParameter;/**< This parameter holds the Interpolation matrix buffer. */

    //Output parameter
    QPointer<Qt3DRender::QParameter>                    m_pOutputColorParameter;    /**< This parameter holds the output color buffer. */

    //Lower and upper normalization threshold parameter
    QPointer<Qt3DRender::QParameter>                    m_pThresholdXParameter;     /**< This parameter holds the lower threshold value. */
    QPointer<Qt3DRender::QParameter>                    m_pThresholdZParameter;     /**< This parameter holds the upper threshold value. */

    //Colormap type
    QPointer<Qt3DRender::QParameter>                    m_pColormapParameter;       /**< This parameter stores the colormap type. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_GPUINTERPOLATIONMATERIAL_H
