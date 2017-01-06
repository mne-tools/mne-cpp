//=============================================================================================================
/**
* @file     ecddatatreeitem.h
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
* @brief     EcdDataTreeItem class declaration.
*
*/

#ifndef ECDDATATREEITEM_H
#define ECDDATATREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"
#include "../../common/abstracttreeitem.h"

#include <inverse/dipoleFit/ecd_set.h>


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

namespace INVERSELIB {
    class ECDSet;
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

class Renderable3DEntity;


//=============================================================================================================
/**
* EcdDataTreeItem provides a generic item to hold information dipole fit source localization data to plot onto the brain surface.
*
* @brief Provides a generic brain tree item to hold real time data.
*/
class DISP3DNEWSHARED_EXPORT EcdDataTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<EcdDataTreeItem> SPtr;             /**< Shared pointer type for EcdDataTreeItem class. */
    typedef QSharedPointer<const EcdDataTreeItem> ConstSPtr;  /**< Const shared pointer type for EcdDataTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit EcdDataTreeItem(int iType = Data3DTreeModelItemTypes::ECDSetDataItem, const QString& text = "Dipole fit data");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~EcdDataTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Initializes the rt data item with neccessary information for visualization computations.
    *
    * @param[in] parent                 The Qt3D entity parent of the new item.
    *
    * @return   Returns true if successful.
    */
    bool init(Qt3DCore::QEntity *parent);

    //=========================================================================================================
    /**
    * Adds actual rt data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
    *
    * @param[in] pECDSet        The ECDSet dipole fit data.
    *
    * @return   Returns true if successful.
    */
    bool addData(QSharedPointer<INVERSELIB::ECDSet> pECDSet);

    //=========================================================================================================
    /**
    * Updates the rt data which is streamed by this item's worker thread item.
    *
    * @return                       Returns true if this item is initialized.
    */
    inline bool isInit() const;

private:
    //=========================================================================================================
    /**
    * Call this function whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * Call this function whenever you want to change the visibilty of the 3D rendered content.
    *
    * @param[in] state     The visiblity flag.
    */
    void setVisible(bool state);

    //=========================================================================================================
    /**
    * Call this function whenever you want to plot the dipoles.
    *
    * @param[in] pECDSet     The dipole set data.
    */
    void plotDipoles(QSharedPointer<INVERSELIB::ECDSet> pECDSet);

    bool                                    m_bIsInit;                      /**< The init flag. */

    QPointer<Renderable3DEntity>            m_pRenderable3DEntity;          /**< The renderable 3D entity. */

    QList<QPointer<Renderable3DEntity> >    m_lDipoles;                     /**< The currently displayed dipoles as 3D objects. */

signals:

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool EcdDataTreeItem::isInit() const
{
    return m_bIsInit;
}

} //NAMESPACE DISP3DLIB

#ifndef metatype_ecdsetsptr
#define metatype_ecdsetsptr
Q_DECLARE_METATYPE(INVERSELIB::ECDSet::SPtr);
#endif

#endif // EcdDataTreeItem_H
