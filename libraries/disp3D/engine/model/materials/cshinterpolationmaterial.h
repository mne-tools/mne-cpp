//=============================================================================================================
/**
* @file     cshinterpolationmaterial.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
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
* @brief     CshInterpolationMaterial class declaration.
*
*/

#ifndef DISP3DLIB_CSHINTERPOLATIONMATERIAL_H
#define DISP3DLIB_CSHINTERPOLATIONMATERIAL_H


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
        class QCullFace;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPDLIB
//=============================================================================================================

namespace DISP3DLIB {


//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* This class stores the material used for interpolation with a compute shader.
*
* @brief Compute shader interpolation material.
*/

class DISP3DSHARED_EXPORT CshInterpolationMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT

public:
    typedef QSharedPointer<CshInterpolationMaterial> SPtr;            /**< Shared pointer type for CshInterpolationMaterial. */
    typedef QSharedPointer<const CshInterpolationMaterial> ConstSPtr; /**< Const shared pointer type for CshInterpolationMaterial. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] bUseAlpha      Whether to use alpha/transparency.
    * @param[in] parent         The parent of this class.
    */
    explicit CshInterpolationMaterial(bool bUseAlpha = false, Qt3DCore::QNode *parent = 0);


    //=========================================================================================================
    /**
    * Copy constructor disabled.
    */
    CshInterpolationMaterial(const CshInterpolationMaterial &other) = delete;

    //=========================================================================================================
    /**
    * Copy operator disabled.
    */
    CshInterpolationMaterial& operator =(const CshInterpolationMaterial &other) = delete;

    //=========================================================================================================
    /**
    * default destructor.
    */
    ~CshInterpolationMaterial();

    //=========================================================================================================
    /**
    * Add a new vector with signal data form the sensors and push them into m_pSignalDataBuffer.
    *
    * @param tSignalVec                Vector with one float value for each sensor.
    */
    void addSignalData(const Eigen::VectorXf &tSignalVec);

    //=========================================================================================================
    /**
    * Get the current alpha value.
    *
    * @return The current alpha value.
    */
    float alpha();

    //=========================================================================================================
    /**
    * Set the current alpha value.
    *
    * @param[in] alpha  The new alpha value.
    */
    void setAlpha(float alpha);

protected:

private:
    //=========================================================================================================
    /**
     * Init CshInterpolationMaterial class.
     */
    void init();

    //=========================================================================================================
    /**
     * Build buffer of 0.0f .
     *
     * @param tSize         Number of zeros.
     * @return              Buffer content.
     */
    QByteArray buildZeroBuffer(const uint tSize);

    bool                                    m_bUseAlpha;

    QPointer<Qt3DRender::QEffect>           m_pEffect;

    //Phongalpha parameter
    QPointer<Qt3DRender::QParameter>        m_pDiffuseParameter;
    QPointer<Qt3DRender::QParameter>        m_pSpecularParameter;
    QPointer<Qt3DRender::QParameter>        m_pShininessParameter;
    QPointer<Qt3DRender::QParameter>        m_pAlphaParameter;

    //Compute Part
    QPointer<Qt3DRender::QShaderProgram>    m_pComputeShader;
    QPointer<Qt3DRender::QRenderPass>       m_pComputeRenderPass;
    QPointer<Qt3DRender::QFilterKey>        m_pComputeFilterKey;
    QPointer<Qt3DRender::QTechnique>        m_pComputeTechnique;

    //Draw Part
    QPointer<Qt3DRender::QShaderProgram>    m_pDrawShader;
    QPointer<Qt3DRender::QRenderPass>       m_pDrawRenderPass;
    QPointer<Qt3DRender::QFilterKey>        m_pDrawFilterKey;
    QPointer<Qt3DRender::QTechnique>        m_pDrawTechnique;

    //Measurement signal
    QPointer<Qt3DRender::QBuffer>           m_pSignalDataBuffer;
    QPointer<Qt3DRender::QParameter>        m_pSignalDataParameter;

    //Output parameter
    QPointer<Qt3DRender::QParameter>        m_pInterpolatedSignalParameter;

    QPointer<Qt3DRender::QCullFace>         m_pCullFace;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CSHINTERPOLATIONMATERIAL_H
