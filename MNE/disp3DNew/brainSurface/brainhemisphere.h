//=============================================================================================================
/**
* @file     brainhemisphere.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of brain BrainHemisphere which holds the data of the right or left brain hemisphere in form of a mesh.
*
*/

#ifndef BRAINHEMISPHERE_H
#define BRAINHEMISPHERE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3DNew_global.h"

#include "brainsurfacemesh.h"
#include "../helpers/renderableentity.h"

#include <fs/surfaceset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRenderer/qt3drenderer_global.h>
#include <Qt3DRenderer/QPhongMaterial>
#include <Qt3DRenderer/QDiffuseMapMaterial>
#include <Qt3DRenderer/QPerVertexColorMaterial>

#include <QRgb>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Qt3D;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Holds the data of one hemisphere in form of a mesh.
*
* @brief Holds the data of one hemisphere in form of a mesh.
*/
class DISP3DNEWSHARED_EXPORT BrainHemisphere : public RenderableEntity
{
    Q_OBJECT
public:
    typedef QSharedPointer<BrainHemisphere> SPtr;             /**< Shared pointer type for Hemisphere class. */
    typedef QSharedPointer<const BrainHemisphere> ConstSPtr;  /**< Const shared pointer type for Hemisphere class. */

    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] parent         The parent node
    */
    explicit BrainHemisphere(QNode *parent = 0);

    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] surf           The surface data
    * @param[in] qmVertexColors The surface color data
    * @param[in] parent         The parent node
    */
    explicit BrainHemisphere(const Surface &surf, const QMap<int, QColor> &qmVertexColors, QNode *parent = 0);

    void updateActivation(const QMap<int, QColor> &vertexColor);

protected:
    void init();

    BrainSurfaceMesh* m_pSurfaceMesh;

    Surface m_surface;
    QMap<int, QColor> m_qmVertexColors;

private:
};

} // NAMESPACE

#endif // HEMISPHERE_H
