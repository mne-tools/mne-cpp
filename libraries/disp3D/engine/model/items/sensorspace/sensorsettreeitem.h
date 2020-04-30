//=============================================================================================================
/**
 * @file     sensorsettreeitem.h
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
 * @brief     SensorSetTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_SENSORSETTREEITEM_H
#define DISP3DLIB_SENSORSETTREEITEM_H

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

namespace MNELIB {
    class MNEBem;
}

namespace FIFFLIB {
    class FiffChInfo;
}

namespace Qt3DCore {
    class QEntity;
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
 * SensorSetTreeItem provides a tree item to sensor information: surface, sensor locations, etc.
 *
 * @brief Provides a generic SensorSetTreeItem.
 */
class DISP3DSHARED_EXPORT SensorSetTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SensorSetTreeItem> SPtr;             /**< Shared pointer type for SensorSetTreeItem class. */
    typedef QSharedPointer<const SensorSetTreeItem> ConstSPtr;  /**< Const shared pointer type for SensorSetTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit SensorSetTreeItem(int iType = Data3DTreeModelItemTypes::BemItem, const QString& text = "");

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surfaces and annotation sets to this item.
     *
     * @param[in] tSensor            The BEM data.
     * @param[in] lChInfo            The channel information used to plot the MEG channels.
     * @param[in] sDataType          The data type: EEG, MEG.
     * @param[in] bads               The bad channel list.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     */
    void addData(const MNELIB::MNEBem& tSensor,
                 const QList<FIFFLIB::FiffChInfo>& lChInfo,
                 const QString &sDataType,
                 const QStringList &bads = QStringList(),
                 Qt3DCore::QEntity* p3DEntityParent = 0);

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
    virtual void setTransform(const FIFFLIB::FiffCoordTrans& transform, bool bApplyInverse = false);

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
    virtual void applyTransform(const FIFFLIB::FiffCoordTrans& transform, bool bApplyInverse = false);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    QPointer<Renderable3DEntity>      m_pRenderable3DEntity;           /**< This item holds all renderable items. */
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_SENSORSETTREEITEM_H
