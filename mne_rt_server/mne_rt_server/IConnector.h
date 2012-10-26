//=============================================================================================================
/**
* @file     IConnector.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    The connector interface
*
*/

#ifndef ICONNECTOR_H
#define ICONNECTOR_H


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QtPlugin>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MSERVER
//=============================================================================================================

namespace MSERVER
{


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=========================================================================================================
/**
* Connector id enumeration. ToDo: add here IDs when creating a new connector
*/
enum ConnectorID
{
    _FIFFSIMULATOR = 1,                 /**< Connector id of the FIFF file simulator. */
    _NEUROMAG = _FIFFSIMULATOR + 1,     /**< Connector id of the Neuromag connector. */
    _default = -1                       /**< Default connector id. */
};


//=========================================================================================================
/**
* The IConnector class is the interface class for all connectors.
*
* @brief The IConnector class is the interface class of all modules.
*/
class IConnector : public QThread
{

public:

    //=========================================================================================================
    /**
    * Destroys the IConnector.
    */
    virtual ~IConnector() {};

    //=========================================================================================================
    /**
    * Starts the IModule.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
    * Stops the IModule.
    * Pure virtual method.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
    * Returns the unique connector id
    * Pure virtual method.
    *
    * @return the connector ID.
    */
    virtual ConnectorID getConnectorID() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method.
    *
    * @return the name of module.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Sets the activation status of the module.
    *
    * @param [in] status the new activation status of the module.
    */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
    * Returns the activation status of the module.
    *
    * @return true if module is activated.
    */
    inline bool isActive() const;

protected:

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread
    */
    virtual void run() = 0;

private:

    bool m_bStatus;                 /**< Holds the activation status. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void IConnector::setStatus(bool status)
{
    m_bStatus = status;
}


//*************************************************************************************************************

inline bool IConnector::isActive() const
{
    return m_bStatus;
}


} //Namespace


#define IConnector_iid "mne_rt_server/1.0"
Q_DECLARE_INTERFACE(MSERVER::IConnector, IConnector_iid)

#endif //ICONNECTOR_H
