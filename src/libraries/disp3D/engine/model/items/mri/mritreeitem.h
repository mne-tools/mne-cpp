//=============================================================================================================
/**
 * @file     mritreeitem.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief     MriTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_MRITREEITEM_H
#define DISP3DLIB_MRITREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"

#include <mne/mne_forwardsolution.h>
#include <connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
    class Surface;
    class Annotation;
}

namespace Qt3DCore {
    class QEntity;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class FsSurfaceTreeItem;

//=============================================================================================================
/**
 * MriTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
 *
 * @brief Provides a generic MriTreeItem.
 */
class DISP3DSHARED_EXPORT MriTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<MriTreeItem> SPtr;             /**< Shared pointer type for MriTreeItem class. */
    typedef QSharedPointer<const MriTreeItem> ConstSPtr;  /**< Const shared pointer type for MriTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit MriTreeItem(int iType = Data3DTreeModelItemTypes::MriItem,
                         const QString& text = "MRI");

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surfaces and annotation SETS to this item.
     *
     * @param[in] tSurfaceSet        FreeSurfer surface set.
     * @param[in] tAnnotationSet     FreeSurfer annotation set.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a QList with the added surface tree items. The ordering.
     *                               of the list hereby corresponds to the ordering of the input surface set.
     *                               The list is empty if no item was added.
     */
    QList<FsSurfaceTreeItem*> addData(const FSLIB::SurfaceSet& tSurfaceSet,
                                      const FSLIB::AnnotationSet& tAnnotationSet,
                                      Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surfaces and annotation data to this item.
     *
     * @param[in] tSurface           FreeSurfer surface.
     * @param[in] tAnnotation        FreeSurfer annotation.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    FsSurfaceTreeItem* addData(const FSLIB::Surface& tSurface,
                               const FSLIB::Annotation& tAnnotation,
                               Qt3DCore::QEntity* p3DEntityParent = 0);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

signals:
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_MRITREEITEM_H
