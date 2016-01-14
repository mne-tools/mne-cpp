//=============================================================================================================
/**
* @file     cluststctabledelegate.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    ClustStcTableDelegate class declaration
*
*/

#ifndef CLUSTSTCTABLEDELEGATE_H
#define CLUSTSTCTABLEDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractItemDelegate>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
/**
* Delegate to view stc model data in a table view
*
* @brief Delegate for QTableView
*/
class DISP3DSHARED_EXPORT ClustStcTableDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    typedef QSharedPointer<ClustStcTableDelegate> SPtr;            /**< Shared pointer type for ClustStcTableDelegate class. */
    typedef QSharedPointer<const ClustStcTableDelegate> ConstSPtr; /**< Const shared pointer type for ClustStcTableDelegate class. */

    ClustStcTableDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Use the painter and style option to render the item specified by the item index.
    *
    * (sizeHint() must be implemented also)
    *
    * @param[in] painter    Low-level painting on widgets and other paint devices
    * @param[in] option     Describes the parameters used to draw an item in a view widget
    * @param[in] index      Used to locate data in a data model.
    */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //=========================================================================================================
    /**
    * Item size
    *
    * @param[in] option     Describes the parameters used to draw an item in a view widget
    * @param[in] index      Used to locate data in a data model.
    */
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:

};

} // NAMESPACE

#endif // CLUSTSTCTABLEDELEGATE_H
