//=============================================================================================================
/**
 * @file     hemispheretreeitem.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief     HemisphereTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_HEMISPHERETREEITEM_H
#define DISP3DLIB_HEMISPHERETREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"

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
    class Annotation;
}

namespace MNELIB {
    class MNEHemisphere;
    class MNESourceSpace;
    class MNESourceEstimate;
    class MNEForwardSolution;
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
class FsAnnotationTreeItem;
class SourceSpaceTreeItem;

//=============================================================================================================
/**
 * HemisphereTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
 *
 * @brief Provides a generic brain tree item.
 */
class DISP3DSHARED_EXPORT HemisphereTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<HemisphereTreeItem> SPtr;             /**< Shared pointer type for HemisphereTreeItem class. */
    typedef QSharedPointer<const HemisphereTreeItem> ConstSPtr;  /**< Const shared pointer type for HemisphereTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor from single surface.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit HemisphereTreeItem(int iType = Data3DTreeModelItemTypes::HemisphereItem,
                                const QString& text = "Unknown");

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surfaces and annotation data to this item.
     *
     * @param[in] tSurface           FreeSurfer surface.
     * @param[in] tAnnotation        FreeSurfer annotation.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item.
     */
    FsSurfaceTreeItem* addData(const FSLIB::Surface& tSurface,
                               const FSLIB::Annotation& tAnnotation,
                               Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
     * Adds source space information.
     *
     * @param[in] tHemisphere        The MNEHemisphere.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SourceSpaceTreeItem* addData(const MNELIB::MNEHemisphere& tHemisphere,
                                 Qt3DCore::QEntity* p3DEntityParent = 0);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_HEMISPHERETREEITEM_H
