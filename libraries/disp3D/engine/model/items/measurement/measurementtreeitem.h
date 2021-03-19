//=============================================================================================================
/**
 * @file     measurementtreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief     MeasurementTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_MEASUREMENTTREEITEM_H
#define DISP3DLIB_MEASUREMENTTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"

#include <mne/mne_forwardsolution.h>
#include <connectivity/network/network.h>

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

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
    class Surface;
    class Annotation;
}

namespace MNELIB {
    class MNESourceSpace;
    class MNESourceEstimate;
    class MNEBemSurface;
}

namespace FIFFLIB{
    class FiffDigPointSet;
}

namespace INVERSELIB{
    class ECDSet;
}

namespace Qt3DCore {
    class QEntity;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class MneDataTreeItem;
class SensorDataTreeItem;
class NetworkTreeItem;
class EcdDataTreeItem;
class FsSurfaceTreeItem;
class SourceSpaceTreeItem;
class DigitizerSetTreeItem;

//=============================================================================================================
/**
 * MeasurementTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
 *
 * @brief Provides a generic MeasurementTreeItem.
 */
class DISP3DSHARED_EXPORT MeasurementTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<MeasurementTreeItem> SPtr;             /**< Shared pointer type for MeasurementTreeItem class. */
    typedef QSharedPointer<const MeasurementTreeItem> ConstSPtr;  /**< Const shared pointer type for MeasurementTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit MeasurementTreeItem(int iType = Data3DTreeModelItemTypes::MeasurementItem,
                                 const QString& text = "Unknown measurement");

    //=========================================================================================================
    /**
     * Adds source space data to this item.
     *
     * @param[in] tSourceSpace       The source space data.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    QList<SourceSpaceTreeItem*> addData(const MNELIB::MNESourceSpace& tSourceSpace,
                                 Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
     * Adds source estimated activation data (MNE, RTC-MUSIC) to this item.
     *
     * @param[in] tSourceEstimate    The MNESourceEstimate.
     * @param[in] tForwardSolution   The MNEForwardSolution.
     * @param[in] tSurfSet           The surface set holding the left and right hemisphere surfaces.
     * @param[in] tAnnotSet          The annotation set holding the left and right hemisphere annotations.
     * @param[in] p3DEntityParent    Pointer to the QEntity parent.
     * @param[in] bUseGPU            Whether to use GPU support for visualizing real-time data.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    MneDataTreeItem* addData(const MNELIB::MNESourceEstimate& tSourceEstimate,
                             const MNELIB::MNEForwardSolution& tForwardSolution,
                             const FSLIB::SurfaceSet &tSurfSet,
                             const FSLIB::AnnotationSet &tAnnotSet,
                             Qt3DCore::QEntity *p3DEntityParent,
                             bool bUseGPU = false);

    //=========================================================================================================
    /**
     * Adds interpolated activation data to the csh version of this item.
     *
     * @param[in] tSensorData            The SensorData.
     * @param[in] bemSurface             Holds all Bem data used in this item.
     * @param[in] fiffInfo               Holds all information needed about the sensors.
     * @param[in] sSensorType            Name of the sensor type EEG or MEG.
     * @param[in] p3DEntityParent        Pointer to the QEntity parent.
     * @param[in] bUseGPU                Whether to use GPU support for visualizing real-time data.
     *
     * @return                           Returns a pointer to the added tree item. (Default would be a NULL pointer if no item was added.).
     */
    SensorDataTreeItem *addData(const Eigen::MatrixXd& tSensorData,
                                const MNELIB::MNEBemSurface &bemSurface,
                                const FIFFLIB::FiffInfo &fiffInfo,
                                const QString &sSensorType,
                                Qt3DCore::QEntity *p3DEntityParent,
                                bool bUseGPU = false);

    //=========================================================================================================
    /**
     * Adds source estimated activation data (dipole fit) to this item.
     *
     * @param[in] tECDSet            The ECDSet dipole data.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    EcdDataTreeItem* addData(const INVERSELIB::ECDSet& tECDSet,
                             Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
     * Adds digitizer data to this item.
     *
     * @param[in] digitizerkind      The kind of the digitizer data.
     * @param[in] tDigitizer         The digitizer data.
     * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    DigitizerSetTreeItem* addData(const FIFFLIB::FiffDigPointSet& tDigitizer,
                                  Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
     * Adds connectivity estimation data.
     *
     * @param[in] tNetworkData       The connectivity data.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    NetworkTreeItem* addData(const CONNECTIVITYLIB::Network& tNetworkData,
                             Qt3DCore::QEntity* p3DEntityParent = 0);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    //=========================================================================================================
    /**
     * Call this function whenever new colors for the activation data plotting are available: source level.
     *
     * @param[in] vertColors     The color values for each estimated source for left and right hemisphere.
     */
    void onSourceColorChanged(const QVariant& vertColors);

    QPointer<MneDataTreeItem>       m_pMneDataTreeItem;         /**< The rt source loc data item of this item. */

    QPointer<SensorDataTreeItem>    m_pEEGSensorDataTreeItem;   /**< The rt sensor EEG data item of this item. */
    QPointer<SensorDataTreeItem>    m_pMEGSensorDataTreeItem;   /**< The rt sensor MEG data item of this item. */

    QPointer<NetworkTreeItem>       m_pNetworkTreeItem;         /**< The rt connectivity data item of this item. */

    QPointer<EcdDataTreeItem>       m_EcdDataTreeItem;          /**< The rt dipole fit data item of this item. */

signals:
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_MEASUREMENTTREEITEM_H
