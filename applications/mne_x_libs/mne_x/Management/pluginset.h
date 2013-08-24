//=============================================================================================================
/**
* @file     PluginSet.h
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
* @brief    Contains declaration of PluginSet class.
*
*/

#ifndef PLUGINSET_H
#define PLUGINSET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Interfaces/IPlugin.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//=========================================================================================================
/**
* PluginSet holds a set of plugins. This set can be handled like a plugin itself, meaning beeing started and having outputs and inputs.
*
* @brief The PluginSet class holds a set of plugins.
*/
class PluginSet : public IPlugin
{
    Q_OBJECT
public:
    typedef QSharedPointer<PluginSet> SPtr;                 /**< Shared pointer type for PluginSet. */
    typedef QSharedPointer<const PluginSet> ConstSPtr;      /**< Const shared pointer type for PluginSet. */
    typedef QList< IPlugin::SPtr > PluginList;           /**< type for a list of plugins. */

    //=========================================================================================================
    /**
    * Constructs a PluginSet.
    */
    PluginSet();
    
    //=========================================================================================================
    /**
    * Destroys the PluginSet.
    */
    virtual ~PluginSet() {};

    //=========================================================================================================
    /**
    * Starts the PluginSet.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the PluginSet.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns the PluginSet name.
    * Pure virtual method.
    *
    * @return the name of plugin set.
    */
    virtual const char* getName() const;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of the IPlugin.
    * Pure virtual method.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget();

signals:


private:
    PluginList m_pluginList;    /**< List of plugins associated with this set. */
};

} //Namespace

#endif // PLUGINSET_H
