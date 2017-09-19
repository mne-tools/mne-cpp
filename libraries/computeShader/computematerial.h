//=============================================================================================================
/**
* @file     computematerial.h
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
* @brief     ComputeMaterial class declaration.
*
*/

#ifndef CSH_COMPUTEMATERIAL_H
#define CSH_COMPUTEMATERIAL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include"computeShader_global.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QHash>
#include <Qt3DRender/QMaterial>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
        class QNode;
}

namespace Qt3DRender {
        class QEffect;
        class QParameter;
        class QShaderProgram;
        class QRenderPass;
        class QFilterKey;
        class QTechnique;
        class QBuffer;
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
* This class stores the Material used for compute shader usage.
*
* @brief Compute material storage.
*/

class COMPUTE_SHADERSHARED_EXPORT ComputeMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT
public:
    typedef QSharedPointer<ComputeMaterial> SPtr;            /**< Shared pointer type for ComputeMaterial. */
    typedef QSharedPointer<const ComputeMaterial> ConstSPtr; /**< Const shared pointer type for ComputeMaterial. */

    //=========================================================================================================
    /**
    * Constructs a ComputeMaterial object.
    */
    explicit ComputeMaterial(Qt3DCore::QNode *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~ComputeMaterial() = default;

    //=========================================================================================================
    /**
     * Set buffer to save the result of the interpolation.
     * @param tBuffer                   Pointer to the buffer.
     * @param tBufferName               Name of the buffer. The name is used in compute shader code.
     */
    void setInterpolatedSignalBuffer(Qt3DRender::QBuffer *tBuffer, const QString &tBufferName);

    //=========================================================================================================
    /**
     * Add one parameter to compute-renderpass and save the pointer to it.
     * @param tParameter                New QParameter from outside the class.
     */
    void addComputePassParameter(QPointer<Qt3DRender::QParameter> tParameter);

    //=========================================================================================================
    /**
     * Add one parameter to draw-renderpass and save the pointer to it.
     * @param tParameter                New QParameter from outside the class.
     */
    void addDrawPassParameter(QPointer<Qt3DRender::QParameter> tParameter);

    //=========================================================================================================
    /**
     * Add a new vector with signal data form the sensors and push them into the m_pSignalDataBuffer.
     *
     * @param tSignalVec                Vector with one float value for each sensor.
     */
    void addSignalData(const Eigen::VectorXf &tSignalVec);

protected:



private:
    //=========================================================================================================
    /**
     * Init ComputeMaterial class.
     */
    void init();

    //=========================================================================================================
    /**
     * This function is temporary used to init m_pSignalDataBuffer
     * @param tSize
     * @return
     */
    QByteArray buildZeroBuffer(const uint tSize);

    //=========================================================================================================
    QPointer<Qt3DRender::QEffect> m_pEffect;

    //Compute Part
    QPointer<Qt3DRender::QShaderProgram> m_pComputeShader;
    QPointer<Qt3DRender::QRenderPass> m_pComputeRenderPass;
    QPointer<Qt3DRender::QFilterKey> m_pComputeFilterKey;
    QPointer<Qt3DRender::QTechnique> m_pComputeTechnique;

    //Draw Part
    QPointer<Qt3DRender::QShaderProgram> m_pDrawShader;
    QPointer<Qt3DRender::QRenderPass> m_pDrawRenderPass;
    QPointer<Qt3DRender::QFilterKey> m_pDrawFilterKey;
    QPointer<Qt3DRender::QTechnique> m_pDrawTechnique;

    //Phongalpha parameter
    QPointer<Qt3DRender::QParameter>        m_pDiffuseParameter;
    QPointer<Qt3DRender::QParameter>        m_pSpecularParameter;
    QPointer<Qt3DRender::QParameter>        m_pShininessParameter;
    QPointer<Qt3DRender::QParameter>        m_pAlphaParameter;

    //Storage for custom parameters
    QHash<QString, QPointer<Qt3DRender::QParameter>> m_CustomParameters;

    //Measurement signal
    QPointer<Qt3DRender::QBuffer> m_pSignalDataBuffer;
    QPointer<Qt3DRender::QParameter> m_pSignalDataParameter;

    //Output parameter
    QPointer<Qt3DRender::QParameter> m_pInterpolatedSignalParameter;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#endif // CSH_COMPUTEMATERIAL_H
