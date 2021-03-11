//=============================================================================================================
/**
 * @file     sensorpositiontreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief     SensorPositionTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_SENSORPOSITIONTREEITEM_H
#define DISP3DLIB_SENSORPOSITIONTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"

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

namespace FIFFLIB {
    class FiffChInfo;
}

namespace Qt3DCore {
    class QEntity;
}

namespace Qt3DExtras {
    class QCuboidGeometry;
    class QSphereGeometry;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class GeometryMultiplier;

//=============================================================================================================
/**
 * SensorPositionTreeItem provides a tree item to visualize sensor position data.
 *
 * @brief SensorPositionTreeItem provides a tree item to visualize sensor position data.
 */
class DISP3DSHARED_EXPORT SensorPositionTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SensorPositionTreeItem> SPtr;             /**< Shared pointer type for SensorPositionTreeItem class. */
    typedef QSharedPointer<const SensorPositionTreeItem> ConstSPtr;  /**< Const shared pointer type for SensorPositionTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType              The type of the item. See types.h for declaration and definition.
     * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit SensorPositionTreeItem(Qt3DCore::QEntity* p3DEntityParent = 0,
                                    int iType = Data3DTreeModelItemTypes::SensorPositionItem,
                                    const QString& text = "Sensor Position");

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surface and annotation data to this item.
     *
     * @param[in] lChInfo            The channel information used to plot the MEG channels.
     * @param[in] sDataType          The data type: EEG, MEG.
     * @param[in] bads               The bad channel list.
     */
    void addData(const QList<FIFFLIB::FiffChInfo> &lChInfo,
                 const QString& sDataType,
                 const QStringList& bads = QStringList());

protected:
    //=========================================================================================================
    /**
     * Plots the MEG sensors.
     *
     * @param[in] lChInfo            The channel information used to plot the MEG channels.
     * @param[in] bads               The bad channel list.
     */
    void plotMEGSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo,
                        const QStringList& bads = QStringList());

    //=========================================================================================================
    /**
     * Plots the EEG sensors.
     *
     * @param[in] lChInfo            The channel information used to plot the EEG channels.
     * @param[in] bads               The bad channel list.
     */
    void plotEEGSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo,
                        const QStringList& bads = QStringList());

    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    QPointer<QEntity>                               m_pMEGSensorEntity;                /**< The MEG sensor entity. */
    QPointer<GeometryMultiplier>                    m_pMEGSensors;                     /**< The MEG sensors. */
    QSharedPointer<Qt3DExtras::QCuboidGeometry>     m_pMEGSensorGeometry;              /**< The MEG sensor geometry. */

    QPointer<QEntity>                               m_pEEGSensorEntity;                /**< The EEG sensor entity. */
    QPointer<GeometryMultiplier>                    m_pEEGSensors;                     /**< The EEG sensors. */
    QSharedPointer<Qt3DExtras::QSphereGeometry>     m_pEEGSensorGeometry;              /**< The EEG sensor geometry. */
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_SENSORPOSITIONTREEITEM_H
