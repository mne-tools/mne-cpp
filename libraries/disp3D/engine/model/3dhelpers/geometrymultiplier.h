//=============================================================================================================
/**
* @file     geometrymultiplier.h
* @author   Lars Debor <lars.debor@gmx.de>;
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
* @brief     GeometryMultiplier class declaration.
*
*/

#ifndef DISP3DLIB_CUSTOMINSTANCEDRENDERER_H
#define DISP3DLIB_CUSTOMINSTANCEDRENDERER_H


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
#include <Qt3DRender/QGeometryRenderer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DRender {
        class QGeometry;
        class QBuffer;
        class QAttribute;
}

namespace Qt3DCore {
        class QNode;
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
* This classes uses instanced rendering to draw the same Gemometry multiple at multiple positions.
* For example it can be used with QSphereGeometry
*
* @brief Instaced based renderer.
*/

class DISP3DSHARED_EXPORT GeometryMultiplier : public Qt3DRender::QGeometryRenderer
{
    Q_OBJECT

public:
    typedef QSharedPointer<GeometryMultiplier> SPtr;            /**< Shared pointer type for CustomInstancedMesh. */
    typedef QSharedPointer<const GeometryMultiplier> ConstSPtr; /**< Const shared pointer type for CustomInstancedMesh. */

    //=========================================================================================================
    /**
    * Constructs a GeometryMultiplier object.
    */
    explicit GeometryMultiplier(QSharedPointer<Qt3DRender::QGeometry> tGeometry,
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
     * Sets the positions for each instance of the geometry.
     *
     * @param tVertPositions            Matrix with x, y and z coordinates for each instance.
     */
    void setPositions(const Eigen::MatrixX3f& tVertPositions);

    //=========================================================================================================
    /**
     * Overloaded functions to set the positions for each instance of the geometry.
     *
     * @param tVertPositions            Array of row vectors with x, y and z coordinates for each instance.
     */
    void setPositions(const QVector<QVector3D> &tVertPositions);

protected:

private:

    //=========================================================================================================
    /**
     * Initialize GeometryMultiplier object.
     */
    void init();

    //=========================================================================================================
    /**
     * Builds the position buffer content.
     *
     * @param tVertPositions            Matrix with x, y and z coordinates for each instance.
     * @return                          buffer content.
     */
    QByteArray buildPositionBuffer(const Eigen::MatrixX3f& tVertPositions);


    QSharedPointer<Qt3DRender::QGeometry>           m_pGeometry;

    QPointer<Qt3DRender::QBuffer>                   m_pPositionBuffer;

    QPointer<Qt3DRender::QAttribute>                m_pPositionAttribute;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CUSTOMINSTANCEDRENDERER_H
