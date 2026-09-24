//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     pluginconnectorconnectionwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2014
 * @brief    Contains the declaration of the PluginConnectorConnectionWidget class.
 */
#ifndef PLUGINCONNECTORCONNECTIONWIDGET_H
#define PLUGINCONNECTORCONNECTIONWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QWidget>
#include <QComboBox>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class PluginConnectorConnection;

//=============================================================================================================
/**
 * Class implements the plug-in connector connection widget.
 *
 * @brief The PluginConnectorConnectionWidget class provides an user interface for connector connections
 */
class SCSHAREDSHARED_EXPORT PluginConnectorConnectionWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a PluginConnectorConnectionWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new PluginConnectorConnectionWidget becomes a window. If parent is another widget, PluginConnectorConnectionWidget becomes a child window inside parent. PluginConnectorConnectionWidget is deleted when its parent is deleted.
     * @param[in] pPluginConnectorConnection a pointer to the corresponding Connector Connection.
     */
    PluginConnectorConnectionWidget(PluginConnectorConnection* pPluginConnectorConnection, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructor
     *
     */
    ~PluginConnectorConnectionWidget();

    //=========================================================================================================
    /**
     * New selection in one of the combo box
     *
     * @param[in] p_sCurrentReceiver   the receivers name.
     */
    void updateReceiver(const QString &p_sCurrentReceiver);

signals:

public slots:

private:
    QLabel* m_pLabel;                                           /**< Holds the start up widget label. */

    PluginConnectorConnection*  m_pPluginConnectorConnection;   /**< a pointer to corresponding PluginConnectorConnection.*/

    QMap<QString, QComboBox*> m_qMapSenderToReceiverConnections;/**< To each output a possible list of inputs. */
};
} // NAMESPACE

#endif // PLUGINCONNECTORCONNECTIONWIDGET_H
