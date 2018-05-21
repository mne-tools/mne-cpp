//=============================================================================================================
/**
* @file     collectorsocket.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     declaration of the CollectorSocket Class.
*
*/

#ifndef COLLECTORSOCKET_H
#define COLLECTORSOCKET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "types_definitions.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NeuromagPlugin
//=============================================================================================================

namespace NeuromagPlugin
{


//=============================================================================================================
/**
* DECLARE CLASS CollectorSocket
*
* @brief The CollectorSocket class provides ....
*/
class CollectorSocket : public QTcpSocket
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a acquisition Server.
    */
    CollectorSocket(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Open the collector control connection
    *
    * @return
    */
    bool open();

    //=========================================================================================================
    /**
    * Close the collector connection
    *
    * @return
    */
//    int close();


    inline bool isMeasuring()
    {
        return m_bIsMeasuring;
    }

    //=========================================================================================================
    /**
    * Query the current buffer length of the Elekta acquisition system
    *
    * @return
    */
    int getMaxBuflen();


    //=========================================================================================================
    /**
    * Set the desired maximum buffer length
    *
    * @return
    */
    int setMaxBuflen(int maxbuflen);


    // new client.c to qt functions
    //=========================================================================================================
    /**
    *
    *
    * @return
    */
    bool server_command(const QString& p_sCommand);


    //=========================================================================================================
    /**
    *
    *
    * @return
    */
    bool server_login(const QString& p_sCollectorPass, const QString& p_sMyName);


    //=========================================================================================================
    /**
    *
    *
    * @return
    */
    bool server_send(QString& p_sDataSend, QByteArray& p_dataOut, int p_iInputFlag = DACQ_DRAIN_INPUT);


    //=========================================================================================================
    /**
    *
    *
    * @return
    */
    bool server_start();


    //=========================================================================================================
    /**
    *
    *
    * @return
//    */
    bool server_stop();



private:

    QString     m_sCollectorHost;

    bool        m_bIsMeasuring;


};

} // NAMESPACE

#endif // COLLECTORSOCKET_H
