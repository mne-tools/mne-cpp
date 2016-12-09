//=============================================================================================================
/**
* @file     custommesh.h
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
* @brief    CustomMesh class declaration
*
*/

#ifndef CUSTOMMESH_H
#define CUSTOMMESH_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QGeometryRenderer>
#include <QPointer>


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
    class QBuffer;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Custom mesh functionality.
*
* @brief Custom mesh functionality.
*/
class DISP3DNEWSHARED_EXPORT CustomMesh : public Qt3DRender::QGeometryRenderer
{
    Q_OBJECT

public:
    typedef QSharedPointer<CustomMesh> SPtr;             /**< Shared pointer type for CustomMesh class. */
    typedef QSharedPointer<const CustomMesh> ConstSPtr;  /**< Const shared pointer type for CustomMesh class. */

    //=========================================================================================================
    /**
    * Default constructor.
    */
    CustomMesh();

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] tMatVert       Vertices in form of a matrix.
    * @param[in] tMatNorm       Normals in form of a matrix.
    * @param[in] tMatTris       Tris/Faces in form of a matrix.
    * @param[in] tVecOffset     The offset which is to be used on all the vertices.
    * @param[in] tArrayColors   The vertex colors. If empty a default value will be used.
    * @param[in] primitiveType  The primitive type of the mesh lines, triangles, etc.
    */
    CustomMesh(const Eigen::MatrixX3f& tMatVert,
                const Eigen::MatrixX3f& tMatNorm,
                const Eigen::MatrixX3i& tMatTris,
                const QByteArray& tArrayColors = QByteArray(),
                Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType = Qt3DRender::QGeometryRenderer::Triangles);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~CustomMesh();

    //=========================================================================================================
    /**
    * Update the vertices colors of the mesh.
    *
    * @param[in] tArrayColors     New color information for the vertices.
    */
    bool setVertColor(const QByteArray& tArrayColors);

    //=========================================================================================================
    /**
    * Set the needed information to create the mesh and then creates a new mesh.
    *
    * @param[in] tMatVert       Vertices in form of a matrix.
    * @param[in] tMatNorm       Normals in form of a matrix.
    * @param[in] tMatTris       Tris/Faces in form of a matrix.
    * @param[in] tVecOffset     The offset which is to be used on all the vertices.
    * @param[in] tArrayColors   The color info of all the vertices.
    * @param[in] primitiveType  The primitive type of the mesh lines, triangles, etc.
    *
    * @return If successful returns true, false otherwise.
    */
    bool setMeshData(const Eigen::MatrixX3f& tMatVert,
                     const Eigen::MatrixX3f& tMatNorm,
                     const Eigen::MatrixXi &tMatTris,
                     const QByteArray &tArrayColors = QByteArray(),
                     Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType = Qt3DRender::QGeometryRenderer::Triangles);

protected:
    //=========================================================================================================
    /**
    * Creates the actual mesh from the set vertex, normals, tris and offset members.
    *
    * @param[in] tMatVert       Vertices in form of a matrix.
    * @param[in] tMatNorm       Normals in form of a matrix.
    * @param[in] tMatTris       Tris/Faces in form of a matrix.
    * @param[in] tVecOffset     The offset which is to be used on all the vertices.
    * @param[in] tArrayColors   The color info of all the vertices.
    * @param[in] primitiveType  The primitive type of the mesh lines, triangles, etc.
    *
    * @return If successful returns true, false otherwise.
    */
    bool createCustomMesh(const Eigen::MatrixX3f& tMatVert,
                          const Eigen::MatrixX3f& tMatNorm,
                          const Eigen::MatrixXi &tMatTris,
                          const QByteArray& tArrayColors,
                          Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType);

    QPointer<Qt3DRender::QBuffer>    m_pVertexDataBuffer;    /**< The vertex buffer. */
    QPointer<Qt3DRender::QBuffer>    m_pNormalDataBuffer;    /**< The normal buffer. */
    QPointer<Qt3DRender::QBuffer>    m_pColorDataBuffer;     /**< The color buffer. */
    QPointer<Qt3DRender::QBuffer>    m_pIndexDataBuffer;     /**< The index buffer. */

//    QSharedPointer<Qt3DRender::QBuffer>    m_pVertexDataBuffer;    /**< The vertex buffer. */
//    QSharedPointer<Qt3DRender::QBuffer>    m_pNormalDataBuffer;    /**< The normal buffer. */
//    QSharedPointer<Qt3DRender::QBuffer>    m_pColorDataBuffer;     /**< The color buffer. */
//    QSharedPointer<Qt3DRender::QBuffer>    m_pIndexDataBuffer;     /**< The index buffer. */

    int                     m_iNumVert;             /**< The total number of set vertices. */
};

} // NAMESPACE

#endif // CUSTOMMESH_H
