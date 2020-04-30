//=============================================================================================================
/**
 * @file     digitizersettreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief     DigitizerSetTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_DIGITIZERSETTREEITEM_H
#define DISP3DLIB_DIGITIZERSETTREEITEM_H

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

namespace FIFFLIB{
    class FiffDigPoint;
    class FiffDigPointSet;
}

namespace Qt3DCore {
    class QEntity;
}

namespace Qt3DCore {
    class QTransform;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class Renderable3DEntity;

//=============================================================================================================
/**
 * DigitizerSetTreeItem provides a tree item to hold the set of digitizer data.
 *
 * @brief DigitizerSetTreeItem provides a tree item to hold the set of digitizer data.
 */
class DISP3DSHARED_EXPORT DigitizerSetTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<DigitizerSetTreeItem> SPtr;            /**< Shared pointer type for DigitizerSetTreeItem. */
    typedef QSharedPointer<const DigitizerSetTreeItem> ConstSPtr; /**< Const shared pointer type for DigitizerSetTreeItem. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit DigitizerSetTreeItem(int iType = Data3DTreeModelItemTypes::DigitizerSetItem, const QString& text = "Source space");

    //=========================================================================================================
    /**
     * Adds digitizer data to this item.
     *
     * @param[in] tDigitizer         The digitizer data.
     */
    void addData(const FIFFLIB::FiffDigPointSet& tDigitizer, Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void setTransform(const Qt3DCore::QTransform &transform);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void setTransform(const FIFFLIB::FiffCoordTrans& transform,
                              bool bApplyInverse = false);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void applyTransform(const Qt3DCore::QTransform& transform);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void applyTransform(const FIFFLIB::FiffCoordTrans& transform,
                                bool bApplyInverse = false);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    QPointer<Renderable3DEntity>      m_pRenderable3DEntity;           /**< This item holds all renderable digitizer items. */
};
} // NAMESPACE DISP3DLIB

#endif // DISP3DLIB_DIGITIZERSETTREEITEM_H
