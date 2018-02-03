//=============================================================================================================
/**
* @file     abstractphongalphamaterial.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     AbstractPhongAlphaMaterial class declaration.
*
*/

#ifndef DISP3DLIB_ABSTRACT_PHONG_ALPHA_MATERIAL_H
#define DISP3DLIB_ABSTRACT_PHONG_ALPHA_MATERIAL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DRender {
    class QMaterial;
    class QEffect;
    class QParameter;
    class QShaderProgram;
    class QMaterial;
    class QFilterKey;
    class QTechnique;
    class QRenderPass;
    class QGraphicsApiFilter;
}

namespace Qtcore {
    class QColor;
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
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class AbstractPhongAlphaMaterial : public Qt3DRender::QMaterial
{

public:
    typedef QSharedPointer<AbstractPhongAlphaMaterial> SPtr;            /**< Shared pointer type for AbstractPhongAlphaMaterial. */
    typedef QSharedPointer<const AbstractPhongAlphaMaterial> ConstSPtr; /**< Const shared pointer type for AbstractPhongAlphaMaterial. */

    //=========================================================================================================
    /**
    * Constructs a AbstractPhongAlphaMaterial object.
    */
    AbstractPhongAlphaMaterial(bool bUseSortPolicy = false, QNode *parent = nullptr);

    virtual ~AbstractPhongAlphaMaterial();


    //=========================================================================================================
    /**
    * Get the current alpha value.
    *
    * @return The current alpha value.
    */
    virtual float alpha() const;

    //=========================================================================================================
    /**
    * Set the current alpha value.
    *
    * @param[in] fAlpha      The new alpha value.
    */
    virtual void setAlpha(float fAlpha);

protected:

    virtual void initData();

    virtual void setShaderCode() = 0;

    //=========================================================================================================
    /**
    * This function gets called whenever the alpha value is changed.
    * It handles the change between opaque and transparent depending on the new alpha.
    *
    * @param[in] fAlpha         The new alpha value.
    */
    virtual void onAlphaChanged(const QVariant &fAlpha);

    QPointer<Qt3DRender::QEffect>           m_pEffect;

    QPointer<Qt3DRender::QParameter>        m_pDiffuseParameter;        /**< Parameter that determines the diffuse value. */
    QPointer<Qt3DRender::QParameter>        m_pSpecularParameter;       /**< Parameter that determines the specular value. */
    QPointer<Qt3DRender::QParameter>        m_pShininessParameter;      /**< Parameter that determines the shininess value. */
    QPointer<Qt3DRender::QParameter>        m_pAlphaParameter;          /**< Parameter that determines the alpha value. */

    QPointer<Qt3DRender::QFilterKey>        m_pDrawFilterKey;

//    QPointer<Qt3DRender::QTechnique>        m_pVertexGL3Technique;
//    QPointer<Qt3DRender::QRenderPass>       m_pVertexGL3RenderPass;
//    QPointer<Qt3DRender::QShaderProgram>    m_pVertexGL3Shader;

//    QPointer<Qt3DRender::QTechnique>        m_pVertexGL2Technique;
//    QPointer<Qt3DRender::QRenderPass>       m_pVertexGL2RenderPass;

//    QPointer<Qt3DRender::QTechnique>        m_pVertexES2Technique;
//    QPointer<Qt3DRender::QRenderPass>       m_pVertexES2RenderPass;
//    QPointer<Qt3DRender::QShaderProgram>    m_pVertexES2Shader;

    bool                                    m_bUseSortPolicy;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_ABSTRACT_PHONG_ALPHA_MATERIAL_H
