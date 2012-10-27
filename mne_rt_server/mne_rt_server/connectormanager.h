//=============================================================================================================
/**
* @file     connectormanager.h
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
* @brief    Contains the implementation of the ConnectorManager Class.
*
*/

#ifndef CONNECTORMANAGER_H
#define CONNECTORMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MSERVER
//=============================================================================================================

namespace MSERVER
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class IConnector;


//=============================================================================================================
/**
* DECLARE CLASS ConnectorManager
*
* @brief The ConnectorManager class provides a dynamic module loader. As well as the handling of the loaded modules.
*/
class ConnectorManager : public QPluginLoader
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ConnectorManager with the given parent.
    *
    * @param [in] parent pointer to parent Object. (It's normally the default value.)
    */
    ConnectorManager(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ConnectorManager.
    */
    virtual ~ConnectorManager();

    //=========================================================================================================
    /**
    * Loads modules from given directory.
    *
    * @param dir the module directory.
    */
    void loadConnectors(const QString& dir);

    //=========================================================================================================
    /**
    * Starts all modules.
    *
    * @return true if at least one IConnector module was started successfully, false otherwise.
    */
    static bool startConnector();

    //=========================================================================================================
    /**
    * Stop active connector.
    */
    static void stopConnector();

    //=========================================================================================================
    /**
    * Finds index of module by name.
    *
    * @param name the connector name.
    * @return index of connector.
    */
    static int findByName(const QString& name);

    //=========================================================================================================
    /**
    * Returns vector containing all modules.
    *
    * @return reference to vector containing all modules.
    */
    static inline const QVector<IConnector*>& getConnectors();

    //=========================================================================================================
    /**
    * Prints a list of all connectors and their status
    */
    const void printConnectors();


    //=========================================================================================================
    /**
    * Returns vector containing active ISensor modules.
    *
    * @return reference to vector containing active ISensor modules.
    */
    static IConnector* getActiveConnector();

    static void clearConnectorActivation();

private:

    static QVector<IConnector*> s_vecConnectors;       /**< Holds vector of all modules. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<IConnector*>& ConnectorManager::getConnectors()
{
    return s_vecConnectors;
}

} // NAMESPACE

#endif // CONNECTORMANAGER_H
