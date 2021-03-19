//=============================================================================================================
/**
 * @file     orbitalcameracontroller.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2018
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
 * @brief     OrbitalCameraController class declaration.
 *
 */

#ifndef DISP3DLIB_ORBITALCAMERACONTROLLER_H
#define DISP3DLIB_ORBITALCAMERACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QAbstractCameraController>
#include "../../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector3D>
#include <QObject>
#include <QMatrix4x4>

namespace Qt3DCore {
class QTransform;
}


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class allows controlling the scene camera along an orbital path.
 *
 * The controls are:
 * - use arrow key or hold down the right mouse button to orbit the camera around the view center
 * - press page up and down or the mouse wheel to zoom the camera in and out
 * - hold down to the middle mouse button or press the ALT-key + right mouse button
 *   to translate the camera and the view center in x/y direction
 *
 * @brief This class allows controlling the scene camera along an orbital path.
 */
class DISP3DSHARED_EXPORT OrbitalCameraController : public Qt3DExtras::QAbstractCameraController
{
   Q_OBJECT
   Q_PROPERTY(int rotating READ rotating WRITE setRotating)                 /**< Access for QPropertyAnimation. */

public:
    typedef QSharedPointer<OrbitalCameraController> SPtr;            /**< Shared pointer type for OrbitalCameraController. */
    typedef QSharedPointer<const OrbitalCameraController> ConstSPtr; /**< Const shared pointer type for OrbitalCameraController. */

    //=========================================================================================================
    /**
     * Constructs a OrbitalCameraController object.
     */
    OrbitalCameraController(Qt3DCore::QNode *pParent = nullptr);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~OrbitalCameraController() = default;

    //=========================================================================================================
    /**
     * Turns inverse rotation of the camera on and off.
     *
     * @param[in] newStatusFlag      The new status of the inversion.
     */
    void invertCameraRotation(bool newStatusFlag);

    //=========================================================================================================
    /**
     * Sets the angle of the camera for rotating around
     *
     * @param[in] count      The counter for how long the rotation has been happening.
     */
    void setRotating(int count);

    //=========================================================================================================
    /**
     * Queries the camera rotating counter
     *
     * @return The rotation counter.
     */
    int rotating() const;

private:
    //=========================================================================================================
    /**
     * QAbstractCameraController function:
     *
     * This method is called whenever a frame action is triggered.
     * It does implement the camera movement specific to this controller.
     */
    void moveCamera(const QAbstractCameraController::InputState &state, float dt) override;

    //=========================================================================================================
    /**
     * Initialzes OrbitalCameraController object.
     */
    void initController();

    //=========================================================================================================
    /**
     * Calcultes the distance between 2 points.
     *
     * @param[in] firstPoint     The first point.
     * @param[in] secondPoint    The second point.
     *
     * @returns the distance between the 2 points.
     */
    inline float distance(const QVector3D &firstPoint, const QVector3D &secondPoint) const;

    float m_fRotationInverseFactor = 1.0f;      /**< The factor used to invers the camera rotation. */
    const float m_fZoomInLimit = 0.04f;         /**< The minimum distance of the camera to the view center. */
    const float m_fAutoRotationSpeed = 1.0f;    /**< The speed that automatic rotation rotates. */
    int m_iRotating;                            /**< How long the camera has been rotating with regards
                                                     to the view center. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

float OrbitalCameraController::distance(const QVector3D &firstPoint, const QVector3D &secondPoint) const
{
    return (secondPoint - firstPoint).length();
}
} // namespace DISP3DLIB

#endif // DISP3DLIB_ORBITALCAMERACONTROLLER_H
