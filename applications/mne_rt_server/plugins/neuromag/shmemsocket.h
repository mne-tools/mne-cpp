//=============================================================================================================
/**
 * @file     shmemsocket.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     declaration of the ShmemSocket Class.
 *
 */

#ifndef SHMEMSOCKET_H
#define SHMEMSOCKET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "types_definitions.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffTag;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGRTSERVERPLUGIN
//=============================================================================================================

namespace NEUROMAGRTSERVERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// NEUROMAGRTSERVERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================



//=============================================================================================================
/**
 * DECLARE CLASS ShmemSocket
 *
 * @brief The ShmemSocket class provides...
 */

class ShmemSocket : public QObject
{
    Q_OBJECT

public:
    explicit ShmemSocket(QObject *parent = 0);

    virtual ~ShmemSocket();

    //=========================================================================================================
    /**
    * Receive one tag from the data server.
    *
    * This routine reads a message from the data server
    * socket and grabs the data. The data may actually
    * be in a shared memory segment noted in the message.
    *
    * The id parameter is needed for two purposes. The
    * data transfer mechanism varies depending on the client
    * number. Clients with id above 10000 use shared memory
    * transfer while other used a regular file to transfer the
    * data.It is needed also if the conndedtion needs to be
    * closed after an error.
    *
    * Ported from client_socket.c
    *
    * @param[in] p_pTag ToDo
    *
    * \return Status OK or FAIL.
    */
    int receive_tag (QSharedPointer<FIFFLIB::FiffTag>& p_pTag);

    //ToDo Connect is different? to: telnet localhost collector ???
    //=========================================================================================================
    /**
    * Connect to the data server process
    *
    * @return
    */
    bool connect_client ();

    //=========================================================================================================
    /**
    * Disconnect from the data server process
    *
    * @return
    */
    int disconnect_client ();

    //=========================================================================================================
    /*
    * Select tags that we are not interested in!
    *
    */
    void set_data_filter (int *kinds, int nkind);

    //=========================================================================================================
    /**
    *
    * @return
    */
    void close_socket ();

    //=========================================================================================================
    /**
    *
    * @return
    */
    int connect_disconnect (int sock,int id);

    //=========================================================================================================
    /**
    * Filter out some large data blocks
    * which are not of interest
    *
    * @return
    */
    int interesting_data (int kind);

private:
    //Ported from shmem.c
    //=========================================================================================================
    /**
    *
    * @return
    */
    dacqShmBlock get_shmem();

    //=========================================================================================================
    /**
    * Initialize data acquisition shared memory segment
    *
    * @return
    */
    int init_shmem();

    //=========================================================================================================
    /**
    * Release the shared memory
    *
    * @return
    */
    int release_shmem();

    //=========================================================================================================
    /**
    *
    * @return
    */
    FILE* open_fif (char *name);

    //=========================================================================================================
    /**
    *
    *
    * @param[in] fd     File to read from
    * @param[in] pos    Position in file
    * @param[in] size   How long
    * @param[in] data   Put data here
    *
    * @return
    */
    int read_fif (FILE *fd, long pos, size_t size, char *data);

private:
    int*            filter_kinds;   /**< Filter these tags */
    int             nfilt;          /**< How many are they */

    int             shmid;
    dacqShmBlock    shmptr;

    int             m_iShmemSock;
    int             m_iShmemId;

    FILE*           fd;             /**<  The temporary file */
    FILE*           shmem_fd;
    char*           filename;

    FILE*           read_fd;
};

} // NAMESPACE

#endif // SHMEMSOCKET_H
