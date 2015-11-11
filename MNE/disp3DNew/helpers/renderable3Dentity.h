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
#include <QVector3D>

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
    * Default constructor for freesurfer mesh.
    */
    Renderable3DEntity();

    //=========================================================================================================
    /**
    * Default constructor for freesurfer mesh.
    *
    * @param[in] tMatVert       Vertices in form of a matrix.
    * @param[in] tMatNorm       Normals in form of a matrix.
    * @param[in] tMatTris       Tris/Faces in form of a matrix.
    * @param[in] tVecOffset     The offset which is to be used on all the vertices.
    * @param[in] parent         The parent of this class.
    */
    Renderable3DEntity(const MatrixX3f &tMatVert, const MatrixX3f &tMatNorm, const MatrixX3i &tMatTris, const Vector3f &tVecOffset, Qt3DCore::QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~Renderable3DEntity();

    //=========================================================================================================
    /**
    * Refresh the vertices colors of the mesh.
    *
    * @param[in] tMatColors     New color information for the vertices.
    */
    bool updateVertColors(const MatrixX3f &tMatColors);

    //=========================================================================================================
    /**
    * Scale the entity.
    *
    * @param[in] scaleFactor    The factor which is to be used for scaling.
    */
    void setScale(float scaleFactor);

    //=========================================================================================================
    /**
    * Set the X-axis rotation of the entity.
    *
    * @param[in] degree         The degrees which are to be used to rotate the entity.
    */
    void setRotationX(float degree);

    //=========================================================================================================
    /**
    * Set the Y-axis rotation of the entity.
    *
    * @param[in] degree         The degrees which are to be used to rotate the entity.
    */
    void setRotationY(float degree);

    //=========================================================================================================
    /**
    * Set the Z-axis rotation of the entity.
    *
    * @param[in] degree         The degrees which are to be used to rotate the entity.
    */
    void setRotationZ(float degree);

    //=========================================================================================================
    /**
    * Set the rotation of the entity. Please note that this function will overrwrite all other rotation transformations.
    *
    * @param[in] degree         The degrees which are to be used to rotate the entity.
    * @param[in] rotAxis        The rotation axis.
    */
    void setRotation(float degree, const QVector3D &rotAxis);

    //=========================================================================================================
    /**
    * Add a rotation to the exiting rotation of the entity. Please note that this funtion will add a rotation transformation,
    * not overwriting already presented transformations. However, this will increase the number of stored transformations very quickly if not used carefully
    *
    * @param[in] degree         The degrees which are to be used to rotate the entity.
    * @param[in] rotAxis        The rotation axis.
    */
    void addRotation(float degree, const QVector3D &rotAxis);

    //=========================================================================================================
    /**
    * Translate the entity.
    *
    * @param[in] trans          The vector for the translation.
    */
    void setTranslation(const QVector3D &trans);

protected:
    CustomMesh::SPtr                                m_pCustomMesh;          /**< The actual mesh information (vertices, normals, colors). */
    QSharedPointer<Qt3DCore::QTransform>            m_pTransform;           /**< The main transformation. */
    QSharedPointer<Qt3DCore::QScaleTransform>       m_pScaleTransform;      /**< The scaling transformation. */
    QSharedPointer<Qt3DCore::QTranslateTransform>   m_pTranslateTransform;  /**< The translation transformation. */
    QSharedPointer<Qt3DCore::QRotateTransform>      m_pRotateTransform;     /**< The rotation transformation which is used when setting a rotation. */
    QSharedPointer<Qt3DCore::QRotateTransform>      m_pRotateTransformX;    /**< The X-Axis transformation. */
    QSharedPointer<Qt3DCore::QRotateTransform>      m_pRotateTransformY;    /**< The Y-Axis transformation. */
    QSharedPointer<Qt3DCore::QRotateTransform>      m_pRotateTransformZ;    /**< The Z-Axis transformation. */

    QSharedPointer<Qt3DRender::QMaterial>           m_pMaterial;            /**< The material to be used for this entity. */
};

} // NAMESPACE

#endif // RENDERABLE3DENTITY_H
