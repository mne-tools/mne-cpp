//=============================================================================================================
/**
* @file     connector.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
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
* @brief    Contains the declaration of the Connector class.
*
*/

#ifndef CONNECTOR_H
#define CONNECTOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"
#include <xMeas/Nomenclature/nomenclature.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XDISPLIB
{
class DisplayManager;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS Connector
*
* @brief The Connector class is providing static functions which care about the plugin runtime connection.
*/
class MNE_X_SHARED_EXPORT Connector
{
//    Q_OBJECT
//
//    friend class MainCSART;

public:

    //=========================================================================================================
    /**
    * Constructs a Connector.
    */
    Connector();

    //=========================================================================================================
    /**
    * Destroys the Connector.
    */
    virtual ~Connector();

    //=========================================================================================================
    /**
    * Initialize a Connector.
    */
    static void init();

    //=========================================================================================================
    /**
    * Connects all measurements to measurement acceptors, depending on corresponding id's.
    */
    static void connectMeasurements();

    //=========================================================================================================
    /**
    * Disconnects all measurements to measurement acceptors, depending on corresponding id's.
    */
    static void disconnectMeasurements();

    //=========================================================================================================
    /**
    * Connects all measurement widgets which are provided by plugins of the plugin id list.
    *
    * @param [in] idList list of plugin id's of which the provided measurements should be connected for displayed.
    * @param [in] t time needed to initialise real time sample array widgets.
    */
    static void connectMeasurementWidgets(QList<PLG_ID::Plugin_ID>& idList, QTime* t);

    //=========================================================================================================
    /**
    * Disconnects all measurement widgets which are provided by plugins of the plugin id list.
    *
    * @param [in] idList list of plugin id's of which the provided measurements should be disconnected from display.
    */
    static void disconnectMeasurementWidgets(QList<PLG_ID::Plugin_ID>& idList);

};

} // NAMESPACE

#endif // CONNECTOR_H
