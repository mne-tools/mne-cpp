//=============================================================================================================
/**
 * @file     networktreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2016
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
 * @brief     NetworkTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_NETWORKTREEITEM_H
#define DISP3DLIB_NETWORKTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

#include "../common/abstractmeshtreeitem.h"
#include "../common/types.h"

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

namespace MNELIB {
    class MNEForwardSolution;
}

namespace FSLIB {
    class Surface;
}

namespace Qt3DCore {
    class QEntity;
}

namespace Qt3DExtras {
    class QCylinderGeometry;
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

class MetaTreeItem;
class GeometryMultiplier;

//=============================================================================================================
/**
 * NetworkTreeItem provides a generic item to hold information about real time connectivity data to plot onto the brain surface.
 *
 * @brief Provides a generic brain tree item to hold real time data.
 */
class DISP3DSHARED_EXPORT NetworkTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<NetworkTreeItem> SPtr;             /**< Shared pointer type for NetworkTreeItem class. */
    typedef QSharedPointer<const NetworkTreeItem> ConstSPtr;  /**< Const shared pointer type for NetworkTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType              The type of the item. See types.h for declaration and definition.
     * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit NetworkTreeItem(Qt3DCore::QEntity *p3DEntityParent = 0,
                             int iType = Data3DTreeModelItemTypes::NetworkItem,
                             const QString& text = "Connectivity Data");

    //=========================================================================================================
    /**
     * Adds actual rt connectivity data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
     *
     * @param[in] tNetworkData       The new connectivity data.
     */
    void addData(const CONNECTIVITYLIB::Network& tNetworkData);

    //=========================================================================================================
    /**
     * This function set the threshold values.
     *
     * @param[in] vecThresholds     The new threshold values used for normalizing the data.
     */
    void setThresholds(const QVector3D& vecThresholds);

private:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    //=========================================================================================================
    /**
     * This function gets called whenever the network threshold changes.
     *
     * @param[in] vecThresholds     The new threshold values used for threshold the network.
     */
    void onNetworkThresholdChanged(const QVariant &vecThresholds);

    //=========================================================================================================
    /**
     * Call this function whenever the surface color was changed.
     *
     * @param[in] color        The new surface color.
     */
    virtual void onColorChanged(const QVariant& color);

    //=========================================================================================================
    /**
     * This function gets called whenever the used colormap type changed.
     *
     * @param[in] sColormapType     The name of the new colormap type.
     */
    void onColormapTypeChanged(const QVariant& sColormapType);

    //=========================================================================================================
    /**
     * Call this function whenever you want to calculate the indices/tris for a network.
     *
     * @param[in] tNetworkData     The network data.
     */
    void plotNetwork(const CONNECTIVITYLIB::Network& tNetworkData);

    //=========================================================================================================
    /**
     * Call this function whenever you want plot the network nodes.
     *
     * @param[in] tNetworkData     The network data.
     */
    void plotNodes(const CONNECTIVITYLIB::Network &tNetworkData);

    //=========================================================================================================
    /**
     * Call this function whenever you want plot the network edges.
     *
     * @param[in] tNetworkData     The network data.
     */
    void plotEdges(const CONNECTIVITYLIB::Network& tNetworkData);

    QPointer<MetaTreeItem>                          m_pItemNetworkThreshold;        /**< The item to access the threshold values. */

    QPointer<QEntity>                               m_pNodesEntity;                 /**< The network node entity. */
    QSharedPointer<Qt3DExtras::QSphereGeometry>     m_pNodesGeometry;               /**< The network node geometry. */
    QPointer<GeometryMultiplier>                    m_pNodes;                       /**< The network nodes. */

    QPointer<QEntity>                               m_pEdgeEntity;                  /**< The network edge entity. */
    QSharedPointer<Qt3DExtras::QCylinderGeometry>   m_pEdgesGeometry;               /**< The network geomtries for edges. */
    QPointer<GeometryMultiplier>                    m_pEdges;                       /**< The geometry multiplier for edges. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_NETWORKTREEITEM_H
