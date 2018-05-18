//=============================================================================================================
/**
* @file     orbitalcameracontroller.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2018
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
* @brief     OrbitalCameraController class declaration.
*
*/

#ifndef DISP3DLIB_ORBITALCAMERACONTROLLER_H
#define DISP3DLIB_ORBITALCAMERACONTROLLER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QAbstractCameraController>
#include "../../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector3D>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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
class DISP3DSHARED_EXPORT OrbitalCameraController : public Qt3DExtras::QAbstractCameraController
{
   Q_OBJECT

public:
    typedef QSharedPointer<OrbitalCameraController> SPtr;            /**< Shared pointer type for OrbitalCameraController. */
    typedef QSharedPointer<const OrbitalCameraController> ConstSPtr; /**< Const shared pointer type for OrbitalCameraController. */

    //=========================================================================================================
    /**
    * Constructs a OrbitalCameraController object.
    */
    OrbitalCameraController(Qt3DCore::QNode *pParent = nullptr);

    ~OrbitalCameraController() = default;



protected:

private:


    void moveCamera(const QAbstractCameraController::InputState &state, float dt) override;

    void initController();

    inline float distance(const QVector3D &firstPoint, const QVector3D &secondPoint);

    const float m_fZoomInLimit = 0.04f;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


float OrbitalCameraController::distance(const QVector3D &firstPoint, const QVector3D &secondPoint)
{
    return (secondPoint - firstPoint).length();
}

} // namespace DISP3DLIB

#endif // DISP3DLIB_ORBITALCAMERACONTROLLER_H
