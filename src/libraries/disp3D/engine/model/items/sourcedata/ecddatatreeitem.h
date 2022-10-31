//=============================================================================================================
/**
 * @file     ecddatatreeitem.h
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
 * @brief     EcdDataTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_ECDDATATREEITEM_H
#define DISP3DLIB_ECDDATATREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVERSELIB {
    class ECDSet;
}

namespace Qt3DCore {
    class QEntity;
}

namespace DISP3DLIB {
    class GeometryMultiplier;
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
 * EcdDataTreeItem provides a generic item to hold information dipole fit source localization data to plot onto the brain surface.
 *
 * @brief Provides a generic brain tree item to hold real time data.
 */
class DISP3DSHARED_EXPORT EcdDataTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<EcdDataTreeItem> SPtr;             /**< Shared pointer type for EcdDataTreeItem class. */
    typedef QSharedPointer<const EcdDataTreeItem> ConstSPtr;  /**< Const shared pointer type for EcdDataTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit EcdDataTreeItem(Qt3DCore::QEntity* p3DEntityParent = 0,
                             int iType = Data3DTreeModelItemTypes::ECDDataItem,
                             const QString& text = "ECD");

    //=========================================================================================================
    /**
     * Adds actual rt data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
     *
     * @param[in] pECDSet        The ECDSet dipole fit data.
     */
    void addData(const INVERSELIB::ECDSet& pECDSet);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    //=========================================================================================================
    /**
     * Call this function whenever you want to plot the dipoles.
     *
     * @param[in] tECDSet     The dipole set data.
     */
    void plotDipoles(const INVERSELIB::ECDSet& tECDSet);

private:

    QPointer<GeometryMultiplier> m_pDipolMesh;

    QPointer<MetaTreeItem> m_pItemNumDipoles;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_ECDDATATREEITEM_H
