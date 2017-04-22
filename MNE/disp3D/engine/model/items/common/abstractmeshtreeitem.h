//=============================================================================================================
/**
* @file     AbstractMeshTreeItem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

#ifndef ABSTRACTMESHTREEITEM_H
#define ABSTRACTMESHTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"
#include "../common/types.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
    class QEntity;
}

namespace Qt3DRender {
    class QMaterial;
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

class MetaTreeItem;
class CustomMesh;


//=============================================================================================================
/**
* AbstractMeshTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic mesh tree item.
*/
class DISP3DNEWSHARED_EXPORT AbstractMeshTreeItem : public Abstract3DTreeItem
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
    explicit AbstractMeshTreeItem(Qt3DCore::QEntity* p3DEntityParent = 0,
                                  int iType = Data3DTreeModelItemTypes::AbstractMeshItem,
                                  const QString& text = "Abstract Mesh");

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    virtual void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Returns the custom mesh.
    *
    * @return The costum mesh.
    */
    virtual QPointer<CustomMesh> getCustomMesh();

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

    //=========================================================================================================
    /**
    * Call this function whenever surface normal visisbility state changed.
    *
    * @param[in] checkState        The current check state of teh corresponding item.
    */
    virtual void onSurfaceNormalsChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * Creates a QByteArray of colors for given color for the input vertices.
    *
    * @param[in] vertices       The vertices information.
    * @param[in] color          The vertex color information.
    *
    * @return The colors per vertex
    */
    virtual MatrixX3f createVertColor(const Eigen::MatrixXf& vertices, const QColor& color = QColor(100,100,100)) const;

    QPointer<Qt3DRender::QMaterial>     m_pMaterial;                        /**< The material. Ownership belongs to RenderableEntity. */
    QPointer<Qt3DRender::QMaterial>     m_pTessMaterial;                    /**< The tesselation material. Ownership belongs to RenderableEntity. */
    QPointer<Qt3DRender::QMaterial>     m_pNormalMaterial;                  /**< The normal material. Ownership belongs to RenderableEntity. */

    QPointer<CustomMesh>                m_pCustomMesh;                      /**< The actual mesh information (vertices, normals, colors). */
};

} //NAMESPACE DISP3DLIB

#endif // ABSTRACTMESHTREEITEM_H
