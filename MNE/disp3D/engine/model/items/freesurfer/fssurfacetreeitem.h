//=============================================================================================================
/**
* @file     fssurfacetreeitem.h
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
* @brief     FsSurfaceTreeItem class declaration.
*
*/

#ifndef FSSURFACETREEITEM_H
#define FSSURFACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstractsurfacetreeitem.h"
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
* FsSurfaceTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT FsSurfaceTreeItem : public AbstractSurfaceTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<FsSurfaceTreeItem> SPtr;             /**< Shared pointer type for FsSurfaceTreeItem class. */
    typedef QSharedPointer<const FsSurfaceTreeItem> ConstSPtr;  /**< Const shared pointer type for FsSurfaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit FsSurfaceTreeItem(int iType = Data3DTreeModelItemTypes::SurfaceItem, const QString& text = "Surface");

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surface and annotation data to this item.
    *
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] parent             The Qt3D entity parent of the new item.
    */
    void addData(const FSLIB::Surface& tSurface, Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
    * Call this function whenever new colors for the activation data plotting are available.
    *
    * @param[in] sourceColorSamples     The color values for each estimated source.
    */
    void setRtVertColor(const MatrixX3f &sourceColorSamples);

    //=========================================================================================================
    /**
    * Call this function whenever visibilty of teh annoation has changed.
    *
    * @param[in] isVisible     The visibility flag.
    */
    void onAnnotationVisibilityChanged(bool isVisible);

protected:
    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();

    //=========================================================================================================
    /**
    * Call this function whenever the curvature color or origin of color information (curvature or annotation) changed.
    */
    void onColorInfoOriginOrCurvColorChanged();

    //=========================================================================================================
    /**
    * Creates a QByteArray of colors for given curvature and color data.
    *
    * @param[in] curvature      The curvature information.
    * @param[in] colSulci       The sulci color information.
    * @param[in] colGyri        The gyri color information.
    *
    * @return The final colors per vertex (RGB).
    */
    MatrixX3f createCurvatureVertColor(const Eigen::VectorXf& curvature, const QColor& colSulci = QColor(50,50,50), const QColor& colGyri = QColor(125,125,125));

    QString                         m_sColorInfoOrigin;                         /**< The surface color origin. */

    //These are stored as member variables because we do not wat to look for them everytime we call functions, especially not when we perform rt source loc
    MetaTreeItem*                   m_pItemSurfColSulci;                        /**< The item which holds the sulci color information. */
    MetaTreeItem*                   m_pItemSurfColGyri;                         /**< The item which holds the gyri color information. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the origin of the vertex color (from curvature, from annotation) changed.
    */
    void colorOriginChanged();
};

} //NAMESPACE DISP3DLIB

#endif // FSSURFACETREEITEM_H
