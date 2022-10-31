//=============================================================================================================
/**
 * @file     networkview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    NetworkView class declaration.
 *
 */

#ifndef DISP3DLIB_NETWORKVIEW_H
#define DISP3DLIB_NETWORKVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace CONNECTIVITYLIB {
    class Network;
}

namespace DISPLIB {
    class ConnectivitySettingsView;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class NetworkTreeItem;

//=============================================================================================================
/**
 * Adapter which provides visualization for network data and a control widget.
 *
 * @brief Visualizes ECD data.
 */
class DISP3DSHARED_EXPORT NetworkView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<NetworkView> SPtr;             /**< Shared pointer type for NetworkView class. */
    typedef QSharedPointer<const NetworkView> ConstSPtr;  /**< Const shared pointer type for NetworkView class. */

    //=========================================================================================================
    /**
     * Default constructor
     *
     */
    explicit NetworkView(QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Default destructor
     */
    ~NetworkView();

    //=========================================================================================================
    /**
     * Adds a list of connectivity estimation data.
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] networkData            The list of connectivity data.
     *
     * @return                           Returns a lsit with pointers to the added tree items.
     */
    QList<NetworkTreeItem*> addData(const QString& sSubject,
                                    const QString& sMeasurementSetName,
                                    const QList<CONNECTIVITYLIB::Network> &tNetworkData);

    //=========================================================================================================
    /**
     * Adds connectivity estimation data.
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] networkData            The connectivity data.
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    NetworkTreeItem* addData(const QString& sSubject,
                             const QString& sMeasurementSetName,
                             const CONNECTIVITYLIB::Network& tNetworkData);

    QPointer<DISPLIB::ConnectivitySettingsView> getConnectivitySettingsView();

protected:
    QPointer<DISPLIB::ConnectivitySettingsView>       m_pConnectivitySettingsView;    /** The connectivity settings view. */
};
} // NAMESPACE

#endif // DISP3DLIB_NETWORKVIEW_H
