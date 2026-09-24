//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     IServer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    The server interface
 */

#ifndef ISERVER_H
#define ISERVER_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpServer>
#include <QThread>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=========================================================================================================
/**
 * The IConnector class is the interface class for all connectors.
 *
 * @brief The IConnector class is the interface class of all modules.
 */
class IServer : public QTcpServer
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Destroys the IConnector.
     */
    virtual ~IServer()
    {
        clearClients();
    };

    //=========================================================================================================
    /**
     * ToDo...
     */
    inline QThread* getClient(quint8 id);

    //=========================================================================================================
    /**
     * ToDo...
     */
    inline quint8 addClient(QThread* p_pThreadClient);

    //=========================================================================================================
    /**
     * ToDo...
     */
    inline void clearClients();

private:
    QMap<quint8, QThread*> m_qClientList;
    quint8          m_iNextClientId;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QThread* IServer::getClient(quint8 id)
{
    return m_qClientList[id];
}

//=============================================================================================================

quint8 IServer::addClient(QThread* p_pThreadClient)
{
    qint8 id = m_iNextClientId;
    m_qClientList.insert(id, p_pThreadClient);
    ++m_iNextClientId;
    return id;
}

//=============================================================================================================

void IServer::clearClients()
{
    QMap<quint8, QThread*>::const_iterator i = m_qClientList.constBegin();
    while (i != map.constEnd()) {
        if(i.value())
            delete i.value();
        ++i;
    }
    m_qClientList.clear();
}

} //Namespace

#endif //ISERVER_H
