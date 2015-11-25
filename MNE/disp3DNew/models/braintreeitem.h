//=============================================================================================================
/**
* @file     braintreeitem.h
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
* @brief     BrainTreeItem class declaration.
*
*/

#ifndef BRAINTREEITEM_H
#define BRAINTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3DNew_global.h"

#include "fs/label.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* BrainTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT BrainTreeItem
{

public:
    typedef QSharedPointer<BrainTreeItem> SPtr;             /**< Shared pointer type for BrainTreeItem class. */
    typedef QSharedPointer<const BrainTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainTreeItem class. */

    enum BrainTreeItemType {SurfName,               //0
                            FilePath,               //1
                            AnnotName,              //2
                            SurfType,               //3
                            Hemi,                   //4
                            ColorSulci,             //5
                            ColorGyri,              //6
                            Renderable3DEntity,     //7
                            RootItem};              //8

    //=========================================================================================================
    /**
    * Default constructor.
    */
    explicit BrainTreeItem(QList<QVariant> lItemData, BrainTreeItemType type, BrainTreeItem *parentItem = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainTreeItem();

    //=========================================================================================================
    /**
    * Brain tree item functions
    */
    void appendChild(BrainTreeItem *child);
    BrainTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    BrainTreeItem *parentItem();

private:
    QList<BrainTreeItem*>   m_childItems;       /**< The list of all children of this brain tree item. */
    BrainTreeItem*          m_parentItem;       /**< The parent of this brain tree item. */
    BrainTreeItemType       m_type;             /**< The BrainTreeItemType of this brain tree item (i.e. FilePath, SurfName, AnnotName, etc.). */

    QString             m_sDesc;
    QList<QVariant>     m_lItemData;

};

} //NAMESPACE DISP3DNEWLIB

#endif // BRAINTREEITEM_H
