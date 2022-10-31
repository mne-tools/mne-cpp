//=============================================================================================================
/**
 * @file     abstractphongalphamaterial.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lars Debor. All rights reserved.
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

#ifndef DISP3DLIB_ABSTRACTPHONGALPHAMATERIAL_H
#define DISP3DLIB_ABSTRACTPHONGALPHAMATERIAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <Qt3DRender/QMaterial>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DRender {
    class QEffect;
    class QParameter;
    class QFilterKey;
    class QShaderProgram;
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
 * This abstract class is used as a base class for all materials that are using the phong alpha lightining model in their shaders.
 *
 * @brief This abstract class is used as a base class for all materials that are using the phong alpha lightining model in their shaders.
 */

class DISP3DSHARED_EXPORT AbstractPhongAlphaMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT

public:
    typedef QSharedPointer<AbstractPhongAlphaMaterial> SPtr;            /**< Shared pointer type for AbstractPhongAlphaMaterial. */
    typedef QSharedPointer<const AbstractPhongAlphaMaterial> ConstSPtr; /**< Const shared pointer type for AbstractPhongAlphaMaterial. */

    //=========================================================================================================
    /**
     * Default constructs a AbstractPhongAlphaMaterial object.
     *
     * @param[in] bUseSortPolicy     Whether to use the sort policy in the framegraph.
     * @param[in] parent             The parent of this object.
     */
    explicit AbstractPhongAlphaMaterial(bool bUseSortPolicy, QNode *parent);

    //=========================================================================================================
    /**
     * The virtual default destructor.
     */
    virtual ~AbstractPhongAlphaMaterial() = default;

    //=========================================================================================================
    /**
     * Get the current alpha value.
     *
     * @return   The current alpha value.
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

    //=========================================================================================================
    /**
     * Inits the OpenGL 3.3, 2.0, ES2.0 techniques and add phong alpha parameters.
     * This functions needs to be overridden for other techniques.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * This abstract function should be used by the derived class to set the appropriate shader code.
     * The implemented function has to be called by the derived class.
     */
    virtual void setShaderCode() = 0;

    //=========================================================================================================
    /**
     * This function searches the children of this item for a QRenderPass with matching name
     * and sets the given shader program.
     *
     * @param[in] sObjectName         The object name of the render pass.
     * @param[in] pShaderProgramm     The shader programm. Passing a nullptr is not allowed.
     */
    virtual void addShaderToRenderPass(const QString &sObjectName, Qt3DRender::QShaderProgram *pShaderProgramm);

    //=========================================================================================================
    /**
     * This function gets called whenever the alpha value is changed.
     * It handles the change between opaque and transparent depending on the new alpha.
     *
     * @param[in] fAlpha         The new alpha value.
     */
    virtual void onAlphaChanged(const QVariant &fAlpha);

    QPointer<Qt3DRender::QEffect>           m_pEffect;                  /**< Material Effect. */

    QPointer<Qt3DRender::QParameter>        m_pDiffuseParameter;        /**< Parameter that determines the diffuse value. */
    QPointer<Qt3DRender::QParameter>        m_pSpecularParameter;       /**< Parameter that determines the specular value. */
    QPointer<Qt3DRender::QParameter>        m_pShininessParameter;      /**< Parameter that determines the shininess value. */
    QPointer<Qt3DRender::QParameter>        m_pAlphaParameter;          /**< Parameter that determines the alpha value. */

    QPointer<Qt3DRender::QFilterKey>        m_pDrawFilterKey;           /**< Filter key for navigating in the frame graph. */

    bool                                    m_bUseSortPolicy;           /**< Flag that indicated hether to use the sort policy in the frame graph. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_ABSTRACTPHONGALPHAMATERIAL_H
