//=============================================================================================================
/**
* @file     pluginconnectorconnectionwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//*************************************************************************************************************
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
class MNE_X_SHARED_EXPORT PluginConnectorConnectionWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a PluginConnectorConnectionWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new PluginConnectorConnectionWidget becomes a window. If parent is another widget, PluginConnectorConnectionWidget becomes a child window inside parent. PluginConnectorConnectionWidget is deleted when its parent is deleted.
    * @param [in] pPluginConnectorConnection a pointer to the corresponding Connector Connection.
    */
    PluginConnectorConnectionWidget(PluginConnectorConnection* pPluginConnectorConnection, QWidget *parent = 0);

signals:

public slots:


private:
    QLabel* m_pLabel;                                           /**< Holds the start up widget label. */

    PluginConnectorConnection*  m_pPluginConnectorConnection;   /**< a pointer to corresponding PluginConnectorConnection.*/

};

} // NAMESPACE

#endif // PLUGINCONNECTORCONNECTIONWIDGET_H
