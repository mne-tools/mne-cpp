//=============================================================================================================
/**
 * @file     fsannotationtreeitem.h
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
 * @brief     FsAnnotationTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_FSANNOTATIONTREEITEM_H
#define DISP3DLIB_FSANNOTATIONTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"
#include "../common/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class Label;
    class Surface;
    class Annotation;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * FsAnnotationTreeItem provides a generic item to hold information about brain annotation information.
 *
 * @brief Provides a generic brain tree item.
 */
class DISP3DSHARED_EXPORT FsAnnotationTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<FsAnnotationTreeItem> SPtr;             /**< Shared pointer type for FsAnnotationTreeItem class. */
    typedef QSharedPointer<const FsAnnotationTreeItem> ConstSPtr;  /**< Const shared pointer type for FsAnnotationTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit FsAnnotationTreeItem(int iType = Data3DTreeModelItemTypes::AnnotationItem, const QString& text = "Annotation" );

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on annotation information to this item.
     *
     * @param[in] tSurface           FreeSurfer surface.
     * @param[in] tAnnotation        FreeSurfer annotation.
     */
    void addData(const FSLIB::Surface& tSurface, const FSLIB::Annotation& tAnnotation);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    //=========================================================================================================
    /**
     * Call this function whenever the check box of this item was checked.
     *
     * @param[in] checkState        The current checkstate.
     */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the visibilty of the annotation set was changed.
     *
     * @param[in] isVisible      The visibility flag.
     */
    void annotationVisibiltyChanged(bool isVisible);
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_FSANNOTATIONTREEITEM_H
