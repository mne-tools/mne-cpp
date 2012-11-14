//=============================================================================================================
/**
* @file     connectormanager.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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

#include "IConnector.h"


//*************************************************************************************************************
//=============================================================================================================
// Fiff INCLUDES
//=============================================================================================================

#include "../../MNE/fiff/fiff_info.h"


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
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// Function Pointers
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//class IConnector;


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
    * Sends the command to the active connector.
    *
    * @param[in] p_qListCommand the command.
    * @param[out] p_blockOutputInfo the bytearray which contains parsing information to be send back to CommandClient.
    *
    * @return true if successful, false otherwise
    */
    bool parseConnectorCommand(QStringList& p_qListCommand, QByteArray& p_blockOutputInfo);

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
    * Returns vector containing all modules.
    *
    * @return reference to vector containing all modules.
    */
    static inline const QVector<IConnector*>& getConnectors();

    //=========================================================================================================
    /**
    * Returns vector containing active ISensor modules.
    *
    * @return reference to vector containing active ISensor modules.
    */
    static IConnector* getActiveConnector();

//    void getActiveMeasInfo(qint32 ID);

//    void getActiveMeasInfo(qint32 ID);
    void connectActiveConnector();


    //=========================================================================================================
    /**
    * Prints a list of all connectors and their status
    */
    QByteArray getByteArrayConnectorList() const;




    static void clearConnectorActivation();


signals:
    void sendMeasInfo(qint32, FIFFLIB::FiffInfo*);

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
