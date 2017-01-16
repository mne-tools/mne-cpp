//=============================================================================================================
/**
* @file     bemtreeitem.h
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
* @brief     BemTreeItem class declaration.
*
*/

#ifndef BEMTREEITEM_H
#define BEMTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"

#include "../common/abstracttreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEBem;
}

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


//=============================================================================================================
/**
* BemTreeItem provides a tree item to hold BEM models.
*
* @brief Provides a generic BemTreeItem.
*/
class DISP3DNEWSHARED_EXPORT BemTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<BemTreeItem> SPtr;             /**< Shared pointer type for BemTreeItem class. */
    typedef QSharedPointer<const BemTreeItem> ConstSPtr;  /**< Const shared pointer type for BemTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit BemTreeItem(int iType = Data3DTreeModelItemTypes::BemItem, const QString& text = "");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BemTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surfaces and annotation SETS to this item.
    *
    * @param[in] tBem               The BEM data.
    * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
    */
    void addData(const MNELIB::MNEBem& tBem, Qt3DCore::QEntity* p3DEntityParent = 0);

private:
    //=========================================================================================================
    /**
    * Call this function whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);
};

} //NAMESPACE DISP3DLIB

#endif // BEMTREEITEM_H
