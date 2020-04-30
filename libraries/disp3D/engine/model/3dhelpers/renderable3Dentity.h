//=============================================================================================================
/**
 * @file     renderable3Dentity.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    Renderable3DEntity class declaration
 *
 */

#ifndef DISP3DLIB_RENDERABLE3DENTITY_H
#define DISP3DLIB_RENDERABLE3DENTITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>
#include <QVector3D>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
    class QTransform;
}

namespace FIFFLIB {
    class FiffCoordTrans;
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
 * Base class for renederable 3D QEntities.
 *
 * @brief Base class for renederable 3D QEntities.
 */
class DISP3DSHARED_EXPORT Renderable3DEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(float scale READ scaleValue WRITE applyScale NOTIFY scaleChanged)
    Q_PROPERTY(float rotX READ rotX WRITE setRotX NOTIFY rotXChanged)
    Q_PROPERTY(float rotY READ rotY WRITE setRotY NOTIFY rotYChanged)
    Q_PROPERTY(float rotZ READ rotZ WRITE setRotZ NOTIFY rotZChanged)
    Q_PROPERTY(QVector3D position READ position WRITE applyPosition NOTIFY positionChanged)

public:
    typedef QSharedPointer<Renderable3DEntity> SPtr;             /**< Shared pointer type for Renderable3DEntity class. */
    typedef QSharedPointer<const Renderable3DEntity> ConstSPtr;  /**< Const shared pointer type for Renderable3DEntity class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] parent         The parent of this entity.
     */
    explicit Renderable3DEntity(Qt3DCore::QEntity* parent = 0);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    virtual ~Renderable3DEntity();

    //=========================================================================================================
    /**
     * Manual garbage collection, since Qt3D is still a bit buggy when it come to memory handling.
     */
    //void releaseNode(Qt3DCore::QNode *node);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void setTransform(const Qt3DCore::QTransform &transform);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void setTransform(const FIFFLIB::FiffCoordTrans& transform,
                              bool bApplyInverse = false);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void applyTransform(const Qt3DCore::QTransform& transform);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void applyTransform(const FIFFLIB::FiffCoordTrans& transform,
                                bool bApplyInverse = false);

    //=========================================================================================================
    /**
     * Returns the current scaling value.
     *
     * @return The scaling value.
     */
    virtual float scaleValue() const;

    //=========================================================================================================
    /**
     * Returns the current rotation around the x-axis.
     *
     * @return The x-axis rotation value.
     */
    virtual float rotX() const;

    //=========================================================================================================
    /**
     * Returns the current rotation around the y-axis.
     *
     * @return The y-axis rotation value.
     */
    virtual float rotY() const;

    //=========================================================================================================
    /**
     * Returns the current rotation around the z-axis.
     *
     * @return The z-axis rotation value.
     */
    virtual float rotZ() const;

    //=========================================================================================================
    /**
     * Returns the current position/translation.
     *
     * @return The position/translation value.
     */
    virtual QVector3D position() const;

    //=========================================================================================================
    /**
     * Applies the current rotation around the x-axis.
     *
     * @param[in] rotX     The x-axis rotation value.
     */
    virtual void applyRotX(float rotX);

    //=========================================================================================================
    /**
     * Sets the current rotation around the x-axis.
     *
     * @param[in] rotX     The x-axis rotation value.
     */
    virtual void setRotX(float rotX);

    //=========================================================================================================
    /**
     * Applies the current rotation around the y-axis.
     *
     * @param[in] rotY     The y-axis rotation value.
     */
    virtual void applyRotY(float rotY);

    //=========================================================================================================
    /**
     * Sets the current rotation around the x-axis.
     *
     * @param[in] rotY     The y-axis rotation value.
     */
    virtual void setRotY(float rotY);

    //=========================================================================================================
    /**
     * Applies the current rotation around the z-axis.
     *
     * @param[in] rotZ     The z-axis rotation value.
     */
    virtual void applyRotZ(float rotZ);

    //=========================================================================================================
    /**
     * Sets the current rotation around the x-axis.
     *
     * @param[in] rotZ     The z-axis rotation value.
     */
    virtual void setRotZ(float rotZ);

    //=========================================================================================================
    /**
     * Applies the current position/translation.
     *
     * @param[in] position     The position/translation value.
     */
    virtual void applyPosition(const QVector3D& position);

    //=========================================================================================================
    /**
     * Sets the current position/translation.
     *
     * @param[in] position     The position/translation value.
     */
    virtual void setPosition(const QVector3D& position);

    //=========================================================================================================
    /**
     * Applies the current scale.
     *
     * @param[in] scale     The new scaling value.
     */
    virtual void applyScale(float scale);

    //=========================================================================================================
    /**
     * Sets the current scale.
     *
     * @param[in] scale     The new scaling value.
     */
    virtual void setScale(float scale);

    //=========================================================================================================
    /**
     * Call this function whenever you want to change the visibilty of the 3D rendered content.
     *
     * @param[in] state     The visiblity flag.
     */
    virtual void setVisible(bool state);

    //=========================================================================================================
    /**
     * Sets the value of a specific parameter of the materials for this entity.
     *
     * @param[in] data             The value to be set.
     * @param[in] sParameterName   The parameters name.
     */
    virtual void setMaterialParameter(const QVariant &data,
                                      const QString &sParameterName);

    //=========================================================================================================
    /**
     * Gets the value of a specific parameter of the materials for this entity.
     *
     * @param[in] sParameterName             The parameters name.
     *
     * @return   The data of the parameter.
     */
    virtual QVariant getMaterialParameter(const QString &sParameterName);

protected:
    //=========================================================================================================
    /**
     * Sets the value of a specific parameter of the materials for this entity.
     *
     * @param[in] pObject            The QObject to be scanned for parameters.
     * @param[in] data               The new data.
     * @param[in] sParameterName     The parameters name.
     */
    virtual void setMaterialParameterRecursive(QObject * pObject,
                                               const QVariant &data,
                                               const QString &sParameterName);

    //=========================================================================================================
    /**
     * Gets the value of a specific parameter of the materials for this entity.
     *
     * @param[in] pObject            The QObject to be scanned for parameters.
     * @param[in] sParameterName     The parameters name.
     *
     * @return   The data of the parameter.
     */
    virtual QPair<bool, QVariant> getMaterialParameterRecursive(QObject * pObject,
                                                                const QString &sParameterName);

    QPointer<Qt3DCore::QTransform>              m_pTransform;            /**< The main transformation. */

    float                                       m_fScale;                /**< The scaling value. */
    float                                       m_fRotX;                 /**< The x axis rotation value. */
    float                                       m_fRotY;                 /**< The y axis rotation value. */
    float                                       m_fRotZ;                 /**< The z axis rotation value. */
    QVector3D                                   m_position;              /**< The position/translation value. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the scaling changed.
     *
     * @param[in] scale     The scaling value.
     */
    void scaleChanged(float scale);

    //=========================================================================================================
    /**
     * Emit this signal whenever the x-axis rotation changed.
     *
     * @param[in] rotX     The x-axis rotation value.
     */
    void rotXChanged(float rotX);

    //=========================================================================================================
    /**
     * Emit this signal whenever the y-axis rotation changed.
     *
     * @param[in] rotY     The y-axis rotation value.
     */
    void rotYChanged(float rotY);

    //=========================================================================================================
    /**
     * Emit this signal whenever the z-axis rotation changed.
     *
     * @param[in] rotZ     The z-axis rotation value.
     */
    void rotZChanged(float rotZ);

    //=========================================================================================================
    /**
     * Emit this signal whenever the position/translation changed.
     *
     * @param[in] position     The position/translation value.
     */
    void positionChanged(QVector3D position);
};
} // NAMESPACE

#endif // DISP3DLIB_RENDERABLE3DENTITY_H
