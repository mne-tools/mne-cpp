//=============================================================================================================
/**
* @file     renderable3Dentity.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    Renderable3DEntity class declaration
*
*/

#ifndef RENDERABLE3DENTITY_H
#define RENDERABLE3DENTITY_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3dnew_global.h"
#include "custommesh.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QScaleTransform>
#include <Qt3DCore/QRotateTransform>
#include <Qt3DCore/QTranslateTransform>

#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QPerVertexColorMaterial>
#include <Qt3DRender/QPhongMaterial>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Base class for renederable 3D and 2D QEntities.
*
* @brief Base class for renederable 3D QEntities.
*/
class DISP3DNEWSHARED_EXPORT Renderable3DEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

public:
    typedef QSharedPointer<Renderable3DEntity> SPtr;             /**< Shared pointer type for Renderable3DEntity class. */
    typedef QSharedPointer<const Renderable3DEntity> ConstSPtr;  /**< Const shared pointer type for Renderable3DEntity class. */

    //=========================================================================================================
    /**
    * Default constructor for freesurfer mesh
    */
    Renderable3DEntity();

    //=========================================================================================================
    /**
    * Default constructor for freesurfer mesh
    */
    Renderable3DEntity(const MatrixX3f &tMatVert, const MatrixX3f &tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset, float tInitScale, Qt3DCore::QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~Renderable3DEntity();

    //=========================================================================================================
    /**
    * Refresh the vertices colors of the mesh
    *
    */
    bool updateVertColors(const MatrixX3f &tMatColors);

protected:
    CustomMesh::SPtr                            m_pCustomMesh;
    QSharedPointer<Qt3DCore::QTransform>        m_pTransform;
    QSharedPointer<Qt3DRender::QMaterial>       m_pMaterial;
};

} // NAMESPACE

#endif // RENDERABLE3DENTITY_H
