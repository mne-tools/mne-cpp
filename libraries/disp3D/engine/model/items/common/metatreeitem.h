//=============================================================================================================
/**
 * @file     metatreeitem.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2016
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
 * @brief     MetaTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_METATREEITEM_H
#define DISP3DLIB_METATREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "abstracttreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
 * MetaTreeItem provides a generic brain tree item to hold meta information about other brain tree items.
 *
 * @brief Provides a generic brain tree item.
 */
class DISP3DSHARED_EXPORT MetaTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<MetaTreeItem> SPtr;             /**< Shared pointer type for MetaTreeItem class. */
    typedef QSharedPointer<const MetaTreeItem> ConstSPtr;  /**< Const shared pointer type for MetaTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit MetaTreeItem(int iType = MetaTreeItemTypes::UnknownItem, const QString& text = "");

    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the data of this item changed.
     */
    void dataChanged(const QVariant& data);
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_METATREEITEM_H
