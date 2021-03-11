//=============================================================================================================
/**
 * @file     networkmaterial.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
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
 * @brief    NetworkMaterial class declaration
 */

#ifndef DISP3DLIB_NETWORKMATERIAL_H
#define DISP3DLIB_NETWORKMATERIAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"
#include "abstractphongalphamaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DRender {
    class QShaderProgram;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * NetworkMaterial is provides a Qt3D material with own shader support.
 *
 * @brief NetworkMaterial is provides a Qt3D material with own shader support.
 */
class DISP3DSHARED_EXPORT NetworkMaterial : public AbstractPhongAlphaMaterial
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
    explicit NetworkMaterial(bool bUseSortPolicy = false, Qt3DCore::QNode *parent = nullptr);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~NetworkMaterial() = default;

private:

    //=========================================================================================================
    /**
     * Adds the shader code to the material.
     */
    virtual void setShaderCode() override;

    QPointer<Qt3DRender::QShaderProgram>    m_pVertexES2Shader;         /**< Shader program for OpenGL version ES2.0. */
    QPointer<Qt3DRender::QShaderProgram>    m_pVertexGL3Shader;         /**< Shader program for OpenGL version 3. */
};
} // namespace DISP3DLIB

#endif // DISP3DLIB_NETWORKMATERIAL_H
