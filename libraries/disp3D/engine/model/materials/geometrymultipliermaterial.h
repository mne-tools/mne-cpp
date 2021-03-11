//=============================================================================================================
/**
 * @file     geometrymultipliermaterial.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor. All rights reserved.
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
 * @brief     GeometryMultiplierMaterial class declaration.
 *
 */

#ifndef DISP3DLIB_GEOMETRYMULTIPLIERMATERIAL_H
#define DISP3DLIB_GEOMETRYMULTIPLIERMATERIAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"
#include "abstractphongalphamaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
    class QShaderProgram;
    class QFilterKey;
    class QTechnique;
    class QRenderPass;
}

namespace Qtcore {
    class QColor;
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
 * Custom phong alpha material for instanced rendering.
 *
 * @brief Custom phong alpha material for instanced rendering.
 */

class DISP3DSHARED_EXPORT GeometryMultiplierMaterial : public AbstractPhongAlphaMaterial
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] bUseSortPolicy     Whether to use the sort policy in the framegraph.
     * @param[in] parent             The parent of this object.
     */
    explicit GeometryMultiplierMaterial(bool bUseSortPolicy = false, Qt3DCore::QNode *parent = nullptr);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    virtual ~GeometryMultiplierMaterial() = default;

    //=========================================================================================================
    /**
     * Sets the color for the ambient color parameter.
     *
     * @param[in]       The new ambient color.
     */
    void setAmbient(const QColor &ambientColor);

private:

    //=========================================================================================================
    /**
     * Adds the shader code to the material.
     */
    virtual void setShaderCode() override;

    QPointer<Qt3DRender::QParameter>        m_pAmbientColor;            /**< Parameter that determines the ambient value. */

    QPointer<Qt3DRender::QShaderProgram>    m_pVertexES2Shader;         /**< Shader program for OpenGL version ES2.0. */
    QPointer<Qt3DRender::QShaderProgram>    m_pVertexGL3Shader;         /**< Shader program for OpenGL version 3. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_GEOMETRYMULTIPLIERMATERIAL_H
