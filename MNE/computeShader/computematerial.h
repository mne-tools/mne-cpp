//=============================================================================================================
/**
* @file     computematerial.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Lars Debor and Matti Hamalainen. All rights reserved.
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
#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QHash>
#include <QTimer>
#include <Qt3DRender/QMaterial>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


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
     * Set a Colorbuffer for testing.
     * @param colorbuffer
     */
    void setYOutBuffer(Qt3DRender::QBuffer *yOutBuffer);

    //=========================================================================================================
    /**
     * @brief setComputePassParameter
     * @param tParameter
     */
    void addComputePassParameter(QPointer<Qt3DRender::QParameter> tParameter);

    //=========================================================================================================
    /**
     * generate a random signal matrix for testing.
     */
    void createSignalMatrix(uint tRows, uint tCols);



protected:

    //=========================================================================================================
    /**
     * update the signal buffer with new measurement data.
     */
    void updateSignalBuffer();

private:
    void init();

    QPointer<Qt3DRender::QEffect> m_pEffect;

    //Compute Part
    QPointer<Qt3DRender::QShaderProgram> m_pComputeShader;
    QPointer<Qt3DRender::QRenderPass> m_pComputeRenderPass;
    QPointer<Qt3DRender::QFilterKey> m_pComputeFilterKey;
    QPointer<Qt3DRender::QTechnique> m_pComputeTechnique;

    //test
//    QPointer<Qt3DRender::QBuffer> m_pShaderStorage;
//    QPointer<Qt3DRender::QParameter> m_pStorageParameter;
//    QPointer<Qt3DRender::QParameter> m_pSinParameter;
//    QPointer<QTimer> m_pTimer;
    //

    //Draw Part
    QPointer<Qt3DRender::QShaderProgram> m_pDrawShader;
    QPointer<Qt3DRender::QRenderPass> m_pDrawRenderPass;
    QPointer<Qt3DRender::QFilterKey> m_pDrawFilterKey;
    QPointer<Qt3DRender::QTechnique> m_pDrawTechnique;

    QPointer<Qt3DRender::QParameter> m_pColorParameter;

    QHash<QString, QPointer<Qt3DRender::QParameter>> m_CustomParameters;

    //Measurement signal
    QPointer<Qt3DRender::QBuffer> m_pSignalDataBuffer;
    QPointer<Qt3DRender::QParameter> m_pSignalDataParameter;
    Eigen::MatrixXf m_signalMatrix;
    uint m_iSignalCtr;
    QPointer<QTimer> m_pTimer;


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#endif // CSH_COMPUTEMATERIAL_H
