//=============================================================================================================
/**
* @file     pluginstage.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains declaration of PluginStage class.
*
*/

#ifndef PLUGINSTAGE_H
#define PLUGINSTAGE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"
#include "../Interfaces/IPlugin.h"
#include "pluginconnectorconnection.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//=========================================================================================================
/**
* PluginStage manages plugins and connections between connectors.
*
* @brief The PluginStage class manages plugins and connections of a set of plugins.
*/
class MNE_X_SHARED_EXPORT PluginStage : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<PluginStage> SPtr;            /**< Shared pointer type for PluginStage. */
    typedef QSharedPointer<const PluginStage> ConstSPtr; /**< Const shared pointer type for PluginStage. */

    typedef QList< IPlugin::SPtr > PluginList;                                      /**< type for a list of plugins. */
    typedef QList<PluginConnectorConnection::SPtr> PluginConnectorConnectionList;   /**< Shared pointer type for PluginConnectorConnection::SPtr list */

    //=========================================================================================================
    /**
    * Constructs a PluginStage.
    */
    explicit PluginStage(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destructs a PluginStage.
    */
    ~PluginStage();

    //=========================================================================================================
    /**
    * Adds a plugin to the stage.
    *
    *@return true if plugin is added successful.
    */
    bool addPlugin(const IPlugin* pPlugin);

    //=========================================================================================================
    /**
    * Clears the PluginStage.
    */
    void clear();

signals:


private:
    PluginList m_pluginList;    /**< List of plugins associated with this set. */
    PluginConnectorConnectionList m_conConList; /**< List of connector connections. */

//    QSharedPointer<PluginSet> m_pPluginSet;     /**< The Plugin set of the stage -> ToDo: check, if more than one set on the stage is usefull. */
};

} //Namespace

#endif // PLUGINSTAGE_H
