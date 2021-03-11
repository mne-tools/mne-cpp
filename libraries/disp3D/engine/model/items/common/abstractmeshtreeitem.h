//=============================================================================================================
/**
 * @file     abstractmeshtreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
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
 * @brief     AbstractMeshTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_ABSTRACTMESHTREEITEM_H
#define DISP3DLIB_ABSTRACTMESHTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"
#include "../common/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QGeometryRenderer>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
    class QEntity;
}

namespace Qt3DRender {
    class QMaterial;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class MetaTreeItem;
class CustomMesh;

//=============================================================================================================
/**
 * AbstractMeshTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
 *
 * @brief Provides a generic mesh tree item.
 */
class DISP3DSHARED_EXPORT AbstractMeshTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<AbstractMeshTreeItem> SPtr;             /**< Shared pointer type for AbstractMeshTreeItem class. */
    typedef QSharedPointer<const AbstractMeshTreeItem> ConstSPtr;  /**< Const shared pointer type for AbstractMeshTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType              The type of the item. See types.h for declaration and definition.
     * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit AbstractMeshTreeItem(Qt3DCore::QEntity* p3DEntityParent = Q_NULLPTR,
                                  int iType = Data3DTreeModelItemTypes::AbstractMeshItem,
                                  const QString& text = "Abstract Mesh Item");

    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    virtual void setData(const QVariant& value, int role = Qt::UserRole + 1);

    virtual void setMaterial(Qt3DRender::QMaterial *pMaterial);

    //=========================================================================================================
    /**
     * Set the needed information to create the mesh and then creates a new mesh.
     *
     * @param[in] tMatVert       Vertices in form of a matrix.
     * @param[in] tMatNorm       Normals in form of a matrix.
     * @param[in] tMatTris       Tris/Faces in form of a matrix.
     * @param[in] tMatColors     The color info of all the vertices.
     * @param[in] primitiveType  The primitive type of the mesh lines, triangles, etc.
     */
    void setMeshData(const Eigen::MatrixX3f& tMatVert,
                     const Eigen::MatrixX3f& tMatNorm,
                     const Eigen::MatrixXi& tMatTris,
                     const Eigen::MatrixX4f &tMatColors,
                     Qt3DRender::QGeometryRenderer::PrimitiveType primitiveType = Qt3DRender::QGeometryRenderer::Triangles);

    //=========================================================================================================
    /**
     * Returns the custom mesh.
     *
     * @return The costum mesh.
     */
    virtual QPointer<CustomMesh> getCustomMesh();

    //=========================================================================================================
    /**
     * Set new vertices colors to the mesh.
     *
     * @param[in] vertColor  New color matrix Eigen::MatrixX3f.
     *
     * @return The costum mesh.
     */
    virtual void setVertColor(const Eigen::MatrixX4f &vertColor);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    virtual void initItem();

    //=========================================================================================================
    /**
     * Call this function whenever the inner tesselation value changed.
     *
     * @param[in] fTessInner     The new inner tesselation value.
     */
    virtual void onSurfaceTessInnerChanged(const QVariant& fTessInner);

    //=========================================================================================================
    /**
     * Call this function whenever the outer tesselation value changed.
     *
     * @param[in] fTessOuter     The new outer tesselation value.
     */
    virtual void onSurfaceTessOuterChanged(const QVariant& fTessOuter);

    //=========================================================================================================
    /**
     * Call this function whenever the triangle scale value changed.
     *
     * @param[in] fTriangleScale     The triangle scale value.
     */
    virtual void onSurfaceTriangleScaleChanged(const QVariant& fTriangleScale);

    //=========================================================================================================
    /**
     * Call this function whenever the surface color was changed.
     *
     * @param[in] color        The new surface color.
     */
    virtual void onColorChanged(const QVariant& color);

    //=========================================================================================================
    /**
     * Call this function whenever the surface material was changed.
     *
     * @param[in] material        The new surface material.
     */
    virtual void onSurfaceMaterialChanged(const QVariant& sMaterial);

    QPointer<Qt3DRender::QMaterial>     m_pMaterial;                        /**< The material. Ownership belongs to RenderableEntity. */
    QPointer<CustomMesh>                m_pCustomMesh;                      /**< The actual mesh information (vertices, normals, colors). */

signals:
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_ABSTRACTMESHTREEITEM_H
