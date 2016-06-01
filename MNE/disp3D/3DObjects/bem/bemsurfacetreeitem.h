//=============================================================================================================
/**
* @file     bemsurfacetreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     BemSurfaceTreeItem class declaration.
*
*/

#ifndef BEMSURFACETREEITEM_H
#define BEMSURFACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "../../helpers/abstracttreeitem.h"
#include "../common/metatreeitem.h"

#include "../../helpers/types.h"
#include "../../helpers/renderable3Dentity.h"

#include "mne/mne_bem.h"
#include "fiff/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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
* BemSurfaceTreeItem provides a generic brain tree item to hold BEM surfaces.
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT BemSurfaceTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<BemSurfaceTreeItem> SPtr;             /**< Shared pointer type for BemSurfaceTreeItem class. */
    typedef QSharedPointer<const BemSurfaceTreeItem> ConstSPtr;  /**< Const shared pointer type for BemSurfaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit BemSurfaceTreeItem(int iType = Data3DTreeModelItemTypes::BemSurfaceItem, const QString& text = "BEM Surface");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BemSurfaceTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds BEM model data.
    *
    * @param[in] tBemSurface        The bem data.
    * @param[in] parent             The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const MNELIB::MNEBemSurface &tBemSurface, Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
    * Call this slot whenever you want to change the visibilty of the 3D rendered content.
    *
    * @param[in] state     The visiblity flag.
    */
    void setVisible(bool state);

private slots:
    //=========================================================================================================
    /**
    * Call this slot whenever the curvature color or origin of color information (curvature or annotation) changed.
    *
    * @param[in] fAlpha     The new alpha value.
    */
    void onSurfaceAlphaChanged(float fAlpha);

    //=========================================================================================================
    /**
    * Call this slot whenever the surface color was changed.
    *
    * @param[in] color        The new surface color.
    */
    void onSurfaceColorChanged(const QColor &color);

    //=========================================================================================================
    /**
    * Call this slot whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

private:
    //=========================================================================================================
    /**
    * Creates a QByteArray of colors for given color for the input vertices.
    *
    * @param[in] vertices       The vertices information.
    * @param[in] color          The vertex color information.
    */
    QByteArray createVertColor(const Eigen::MatrixXf& vertices, const QColor& color = QColor(100,100,100)) const;

    Qt3DCore::QEntity*      m_pParentEntity;                            /**< The parent 3D entity. */
    Renderable3DEntity*     m_pRenderable3DEntity;                      /**< The renderable 3D entity. */

    QObjectList             m_lChildren;

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the origin of the vertex color (from curvature, from annotation) changed.
    *
    * @param[in] arrayVertColor      The new vertex colors.
    */
    void colorInfoOriginChanged(const QByteArray& arrayVertColor);
};

} //NAMESPACE DISP3DLIB

#endif // BEMSURFACETREEITEM_H
