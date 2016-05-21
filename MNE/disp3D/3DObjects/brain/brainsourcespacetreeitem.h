//=============================================================================================================
/**
* @file     brainsourcespacetreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
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
* @brief     BrainSourceSpaceTreeItem class declaration.
*
*/

#ifndef BRAINSOURCESPACETREEITEM_H
#define BRAINSOURCESPACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "../../helpers/abstracttreeitem.h"
#include "braintreemetaitem.h"

#include "../../helpers/types.h"
#include "../../helpers/renderable3Dentity.h"

#include "fs/label.h"
#include "fs/surface.h"

#include "mne/mne_hemisphere.h"


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

#include <Qt3DRender/QSphereMesh>
#include <Qt3DRender/QPhongMaterial>


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
* BrainSourceSpaceTreeItem provides a generic brain tree item to hold data about the source space.
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT BrainSourceSpaceTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<BrainSourceSpaceTreeItem> SPtr;             /**< Shared pointer type for BrainSourceSpaceTreeItem class. */
    typedef QSharedPointer<const BrainSourceSpaceTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainSourceSpaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit BrainSourceSpaceTreeItem(int iType = Data3DTreeModelItemTypes::SourceSpaceItem, const QString& text = "Source space");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainSourceSpaceTreeItem();

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
    * @param[in] tHemisphere        The hemisphere data of the source space.
    * @param[in] parent             The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const MNELIB::MNEHemisphere& tHemisphere, Qt3DCore::QEntity* parent);

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

#endif // BRAINSOURCESPACETREEITEM_H
