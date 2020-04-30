//=============================================================================================================
/**
 * @file     IServer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Matti Hamalainen. All rights reserved.
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
 * @brief    The server interface
 *
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
