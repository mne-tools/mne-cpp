//=============================================================================================================
/**
 * @file     geometrymultiplier.h
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
 * @brief     GeometryMultiplier class declaration.
 *
 */

#ifndef DISP3DLIB_GEOMETRYMULTIPLIER_H
#define DISP3DLIB_GEOMETRYMULTIPLIER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QAttribute>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
        class QNode;
}

namespace QtCore {
        class QVector3D;
        class QMatrix4x4;
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
 * This classes uses instanced rendering to draw the same Gemometry multiple at multiple positions.
 * For example it can be used with QSphereGeometry
 *
 * @brief Instaced based renderer.
 */

class DISP3DSHARED_EXPORT GeometryMultiplier : public Qt3DRender::QGeometryRenderer
{
    Q_OBJECT

public:
    typedef QSharedPointer<GeometryMultiplier> SPtr;            /**< Shared pointer type for GeometryMultiplier. */
    typedef QSharedPointer<const GeometryMultiplier> ConstSPtr; /**< Const shared pointer type for GeometryMultiplier. */

    //=========================================================================================================
    /**
     * Constructs a GeometryMultiplier object.
     */
    explicit GeometryMultiplier(QSharedPointer<Qt3DCore::QGeometry> tGeometry,
                                                      Qt3DCore::QNode *tParent = nullptr);

    //=========================================================================================================
    /**
     * Copy Constructor disabled
     */
    GeometryMultiplier(const GeometryMultiplier& other) = delete;

    //=========================================================================================================
    /**
     * Copy operator disabled
     */
    GeometryMultiplier& operator =(const GeometryMultiplier& other) = delete;

    //=========================================================================================================
    /**
     * Destructor
     */
    ~GeometryMultiplier();

    //=========================================================================================================
    /**
     * Sets the transformation matrix for each instance of the geometry.
     * It can be used to translate, scale and rotate each instance individually.
     *
     * @param[in] tInstanceTansform         Transformation matrix.
     */
    void setTransforms(const QVector<QMatrix4x4> &tInstanceTansform);

    //=========================================================================================================
    /**
     * Sets the color for each instance of the geometry.
     *
     * @param[in] tInstanceColors           Color of the geometry;.
     */
    void setColors(const QVector<QColor> &tInstanceColors);

private:
    //=========================================================================================================
    /**
     * Initialize GeometryMultiplier object.
     */
    void init();

    //=========================================================================================================
    /**
     * Builds the transform matrix buffer content.
     *
     * @param[in] tInstanceTransform        Transformation matrix for each instance.
     * @return                          buffer content.
     */
    QByteArray buildTransformBuffer(const QVector<QMatrix4x4> &tInstanceTransform);

    //=========================================================================================================
    /**
     * Builds color buffer content.
     *
     * @param[in] tInstanceColor            Color for each instance.
     * @return                          buffer content.
     */
    QByteArray buildColorBuffer(const QVector<QColor> &tInstanceColor);

    QSharedPointer<Qt3DCore::QGeometry>             m_pGeometry;

    QPointer<Qt3DCore::QBuffer>                   m_pTransformBuffer;

    QPointer<Qt3DCore::QBuffer>                   m_pColorBuffer;

    QPointer<Qt3DCore::QAttribute>                m_pTransformAttribute;

    QPointer<Qt3DCore::QAttribute>                m_pColorAttribute;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_GEOMETRYMULTIPLIER_H
