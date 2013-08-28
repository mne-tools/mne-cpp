//=============================================================================================================
/**
* @file     pluginoutputconnector.h
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
* @brief    Contains the declaration of the PluginOutputConnector class.
*
*/
#ifndef PLUGINOUTPUTCONNECTOR_H
#define PLUGINOUTPUTCONNECTOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"

#include "pluginconnector.h"
#include <xMeas/newmeasurement.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//=============================================================================================================
/**
* Class to connect plug-in data streams.
*
* @brief The PluginConnector class provides the base to connect plug-in data
*/
class MNE_X_SHARED_EXPORT PluginOutputConnector : public PluginConnector
{
    Q_OBJECT
public:
    typedef QSharedPointer<PluginOutputConnector> SPtr;               /**< Shared pointer type for PluginOutputConnector. */
    typedef QSharedPointer<const PluginOutputConnector> ConstSPtr;    /**< Const shared pointer type for PluginOutputConnector. */

    //=========================================================================================================
    /**
    * Constructs a PluginOutputConnector with the given parent.
    *
    * @param[in] parent     pointer to parent plugin
    * @param[in] name       connection name
    * @param[in] descr      connection description
    */
    PluginOutputConnector(IPlugin *parent, const QString &name, const QString &descr);

    //=========================================================================================================
    /**
    * Destructor
    */
    virtual ~PluginOutputConnector(){}

    //=========================================================================================================
    /**
     * Returns false
     *
     * @return false
     */
    virtual bool isInputConnector() const;

    //=========================================================================================================
    /**
     * Returns true
     *
     * @return true
     */
    virtual bool isOutputConnector() const;

signals:
    void notify(XMEASLIB::NewMeasurement::SPtr);

};

} // NAMESPACE

#endif // PLUGINOUTPUTCONNECTOR_H
