//=============================================================================================================
/**
* @file     fiffstreamserver.h
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
* @brief    Contains the implementation of the FiffStreamServer Class.
*
*/

#ifndef FIFFSTREAMSERVER_H
#define FIFFSTREAMSERVER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ICommandParser.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QTcpServer>


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
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamThread;

//=============================================================================================================
/**
* DECLARE CLASS FiffStreamServer
*
* @brief The FiffStreamServer class provides
*/
class FiffStreamServer : public QTcpServer, ICommandParser
{
    Q_OBJECT

    friend class FiffStreamThread;

public:

    FiffStreamServer(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the FiffStreamServer.
    */
    ~FiffStreamServer();


    virtual QByteArray availableCommands();


    //=========================================================================================================
    /**
    * ToDo...
    */
    inline FiffStreamThread* getClient(qint32 id);



    virtual bool parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo);


//    //=========================================================================================================
//    /**
//    * ToDo...
//    */
//    void clearClients();

//public slots: --> in Qt 5 not anymore declared as slot
    void forwardMeasInfo(qint32 ID, FiffInfo::SDPtr p_FiffInfo);
    void forwardRawBuffer(Eigen::MatrixXf m_matRawData);

signals:
    void requestMeasInfo(qint32 ID);

    void startMeasFiffStreamClient(qint32 ID);
    void stopMeasFiffStreamClient(qint32 ID);

    void remitMeasInfo(qint32 ID, FIFFLIB::FiffInfo::SDPtr p_FiffInfo);
    void remitRawBuffer(Eigen::MatrixXf);

    void closeFiffStreamServer();

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    QByteArray parseToId(QString& p_sRawId, qint32& p_iParsedId);

    QMap<qint32, FiffStreamThread*> m_qClientList;
    qint32                          m_iNextClientId;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

FiffStreamThread* FiffStreamServer::getClient(qint32 id)
{
    return m_qClientList[id];
}

} // NAMESPACE

#endif //FIFFSTREAMSERVER_H
