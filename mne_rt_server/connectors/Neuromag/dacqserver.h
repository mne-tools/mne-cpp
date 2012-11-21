//=============================================================================================================
/**
* @file     dacqserver.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Gustavo Sudre;
*           Lauri Parkkonen
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh, Matti Hamalainen, Gustavo Sudre and Lauri Parkkonen. All rights reserved.
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
* @brief    Contains the implementation of the DacqServer Class.
*
*/


#ifndef DACQSERVER_H
#define DACQSERVER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "types_definitions.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>

#include <QTcpSocket>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NeuromagPlugin
//=============================================================================================================

namespace NeuromagPlugin
{



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Neuromag;


//=============================================================================================================
/**
* DECLARE CLASS DacqServer
*
* @brief The DacqServer class provides a Neuromag MEG connector.
*/
class DacqServer : public QThread
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a acquisition Server.
    */
    DacqServer(Neuromag* p_pNeuromag);
    
    //=========================================================================================================
    /**
     * Open the collector control connection
     *
     * @return
     */
    int collector_open();

    //=========================================================================================================
    /**
     * Close the collector connection
     *
     * @return
     */
    int collector_close();

    //=========================================================================================================
    /**
     * Query the current buffer length of the Elekta acquisition system
     *
     * @return
     */
    bool collector_getMaxBuflen();

    //=========================================================================================================
    /**
     * Set the desired maximum buffer length
     *
     * @return
     */
    bool collector_setMaxBuflen(int maxbuflen);

    //=========================================================================================================
    /**
     * Quit function
     *
     * @return
     */
    void clean_up();


signals:


//public slots: --> in Qt 5 not anymore declared as slot


protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:

//    //=========================================================================================================
//    /**
//     * Disconnect from the data server process
//     *
//     * @return
//     */
//    int dacq_disconnect_client (sockfd sock,int id);



//    /**
//     * Receive one tag from the data server.
//     *
//     * This routine reads a message from the data server
//     * socket and grabs the data. The data may actually
//     * be in a shared memory segment noted in the message.
//     *
//     * The id parameter is needed for two purposes. The
//     * data transfer mechanism varies depending on the client
//     * number. Clients with id above 10000 use shared memory
//     * transfer while other used a regular file to transfer the
//     * data.It is needed also if the conndedtion needs to be
//     * closed after an error.
//     *
//     * \return Status OK or FAIL.
//     */

//    int dacq_client_receive_tag (int sock, /**< Socket to read */ int id );  /**< My id number */




    //newly written stuff ported to qt

    QString         m_sCollectorHost;
    QTcpSocket*     m_pCollectorSock;

//    QTcpSocket* dacq_server_connect_by_name(QString& p_sCollectorHost, int p_iCollectorPort);




// client_socket.c
    void close_socket (int sock, int id);

    int connect_disconnect (int sock,int id);

    //ToDo Connect is different? to: telnet localhost collector ???
    //=========================================================================================================
    /**
     * Connect to the data server process
     *
     * @return
     */
    int dacq_connect_client (int id);


// shmem.c

    /**
     * Release the shared memory
     */

    int dacq_release_shmem(void);

    int shmid;
    dacqShmBlock shmptr;




//dacqserver
    bool m_bIsRunning;



    Neuromag* m_pNeuromag;

    int     m_iShmemSock;
    int     m_iShmemId;



};

} // NAMESPACE

#endif // DACQSERVER_H
