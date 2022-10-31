//=============================================================================================================
/**
 * @file     pluginconnectorconnectionwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the PluginConnectorConnectionWidget class.
 *
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
