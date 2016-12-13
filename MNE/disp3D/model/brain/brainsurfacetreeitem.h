//=============================================================================================================
/**
* @file     brainsurfacetreeitem.h
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
* @brief     BrainSurfaceTreeItem class declaration.
*
*/

#ifndef BRAINSURFACETREEITEM_H
#define BRAINSURFACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"
#include "../common/abstracttreeitem.h"
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
class Renderable3DEntity;


//=============================================================================================================
/**
* BrainSurfaceTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT BrainSurfaceTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<BrainSurfaceTreeItem> SPtr;             /**< Shared pointer type for BrainSurfaceTreeItem class. */
    typedef QSharedPointer<const BrainSurfaceTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainSurfaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit BrainSurfaceTreeItem(int iType = Data3DTreeModelItemTypes::SurfaceItem, const QString& text = "Surface");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainSurfaceTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surface and annotation data to this item.
    *
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] parent             The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const FSLIB::Surface& tSurface, Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
    * Call this function whenever new colors for the activation data plotting are available.
    *
    * @param[in] sourceColorSamples     The color values for each estimated source.
    */
    void onRtVertColorChanged(const QByteArray& sourceColorSamples);

    //=========================================================================================================
    /**
    * Call this function whenever visibilty of teh annoation has changed.
    *
    * @param[in] isVisible     The visibility flag.
    */
    void onAnnotationVisibilityChanged(bool isVisible);

    //=========================================================================================================
    /**
    * Call this function whenever you want to change the visibilty of the 3D rendered content.
    *
    * @param[in] state     The visiblity flag.
    */
    void setVisible(bool state);

private:
    //=========================================================================================================
    /**
    * Call this function whenever the curvature color or origin of color information (curvature or annotation) changed.
    */
    void onColorInfoOriginOrCurvColorChanged();

    //=========================================================================================================
    /**
    * Call this function whenever the alpha value changed.
    *
    * @param[in] fAlpha     The new alpha value.
    */
    void onSurfaceAlphaChanged(float fAlpha);

    //=========================================================================================================
    /**
    * Call this function whenever the inner tesselation value changed.
    *
    * @param[in] fTessInner     The new inner tesselation value.
    */
    void onSurfaceTessInnerChanged(float fTessInner);

    //=========================================================================================================
    /**
    * Call this function whenever the outer tesselation value changed.
    *
    * @param[in] fTessOuter     The new outer tesselation value.
    */
    void onSurfaceTessOuterChanged(float fTessOuter);

    //=========================================================================================================
    /**
    * Call this function whenever the triangle scale value changed.
    *
    * @param[in] fTriangleScale     The triangle scale value.
    */
    void onSurfaceTriangleScaleChanged(float fTriangleScale);

    //=========================================================================================================
    /**
    * Call this function whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * Call this function whenever the the translation x of this item changed.
    *
    * @param[in] fTransX        The current x translation.
    */
    void onSurfaceTranslationXChanged(float fTransX);

    //=========================================================================================================
    /**
    * Call this function whenever the the translation y of this item changed.
    *
    * @param[in] fTransY        The current y translation.
    */
    void onSurfaceTranslationYChanged(float fTransY);

    //=========================================================================================================
    /**
    * Call this function whenever the the translation z of this item changed.
    *
    * @param[in] fTransZ        The current z translation.
    */
    void onSurfaceTranslationZChanged(float fTransZ);

    //=========================================================================================================
    /**
    * Creates a QByteArray of colors for given curvature and color data.
    *
    * @param[in] curvature      The curvature information.
    * @param[in] colSulci       The sulci color information.
    * @param[in] colGyri        The gyri color information.
    */
    QByteArray createCurvatureVertColor(const Eigen::VectorXf& curvature, const QColor& colSulci = QColor(50,50,50), const QColor& colGyri = QColor(125,125,125));

    QString                         m_sColorInfoOrigin;                         /**< The surface color origin. */
    QPointer<Qt3DCore::QEntity>     m_pParentEntity;                            /**< The parent 3D entity. */
    Renderable3DEntity*    m_pRenderable3DEntity;                      /**< The surface renderable 3D entity. */
    Renderable3DEntity*    m_pRenderable3DEntityNormals;               /**< The normals renderable 3D entity. */

    //These are stored as member variables because we do not wat to look for them everytime we call functions, especially not when we perform rt source loc
    MetaTreeItem*                   m_pItemSurfColSulci;                        /**< The item which holds the sulci color information. */
    MetaTreeItem*                   m_pItemSurfColGyri;                         /**< The item which holds the gyri color information. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the origin of the vertex color (from curvature, from annotation) changed.
    */
    void colorInfoOriginChanged();
};

} //NAMESPACE DISP3DLIB

#endif // BRAINSURFACETREEITEM_H
