//=============================================================================================================
/**
* @file     dacqserver.cpp
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dacqserver.h"
#include "neuromag.h"


//*************************************************************************************************************
//=============================================================================================================
// UNIX INCLUDES
//=============================================================================================================

#include <sys/stat.h> //umask


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NeuromagPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DacqServer::DacqServer(Neuromag* p_pNeuromag)
: m_pNeuromag(p_pNeuromag)
, m_pCollectorHost("localhost")
, m_fdCollectorSock(-1)
, m_fdShmemSock(-1)
, m_iShmemId(CLIENT_ID)
, m_bIsRunning(false)
{

}


//*************************************************************************************************************

bool DacqServer::collector_open()
{
//    if (collector_sock >= 0) {
//        dacq_log("Note: Tried to re-open an open connection\n");
//        return true;
//    }
//    if ((collector_sock = dacq_server_connect_by_name(collector_host, COLLECTOR_PORT)) == false) {
//        dacq_log("Neuromag collector connection: %s\n", err_get_error());
//        return false;
//    }
//    if (dacq_server_login(&collector_sock, COLLECTOR_PASS, "neuromag2ft") == false) {
//        dacq_log("Neuromag collector connection: %s\n", err_get_error());
//        return false;
//    }

  return true;
}


//*************************************************************************************************************

bool DacqServer::collector_close()
{
//    if (collector_sock < 0)
//        return true;
//    if (!dacq_server_close(&collector_sock, NULL)) {
//        dacq_log("Neuromag collector connection: %s\n", err_get_error());
//        return false;
//    }
    m_fdCollectorSock = -1;
    return true;
}


//*************************************************************************************************************

void DacqServer::close_socket (sockfd sock, int id)
{
    char    client_path[200];	/* This our path */

//    if (sock != -1) {
//        /*
//        * Use unlink to remove the file (inode) so that the name
//        * will be available for the next run.
//        */
//        sprintf (client_path,"%s%d",SOCKET_PATHCLNT,id);
//        unlink(client_path);
//        close(sock);
//    }
//    (void) dacq_release_shmem();
//    dacq_log ("Connection closed.\n");
}


//*************************************************************************************************************

int DacqServer::connect_disconnect (sockfd sock,int id)
{
//    struct  sockaddr_un servaddr;     /* address of server */
//    struct  sockaddr_un from;

//    socklen_t fromlen;
//    int     result;
//    int     slen, rlen;

//    if (sock < 0)
//        return (OK);
//    /*
//    * Set up address structure for server socket
//    */
//    bzero(&servaddr, sizeof(servaddr));
//    servaddr.sun_family = AF_UNIX;
//    strcpy(servaddr.sun_path, SOCKET_PATH);

//    slen = sendto(sock, (void *)(&id), sizeof(int), 0,
//        (void *)(&servaddr), sizeof(servaddr));
//    if (slen<0) {
//        dacq_perror("sendto");
//        close_socket (sock,abs(id));
//        return (FAIL);
//    }
//    else {
//        fromlen = sizeof(from);
//        rlen = recvfrom(sock, (void *)(&result), sizeof(int), 0,
//            (void *)(&from), &fromlen);
//    if (rlen == -1) {
//        dacq_perror("recvfrom");
//        close_socket (sock,abs(id));
//        return (FAIL);
//    } else
//        return result;
//    }

    //DEBUG
    return 0;
}


//*************************************************************************************************************

int DacqServer::dacq_connect_client (int id)
{
//    struct  sockaddr_un clntaddr;	/* address of client */
//    char    client_path[200];	/* This our path */
//    int     sock = -1;		/* This is the UNIX domain socket */

//    sprintf (client_path,"%s%d",SOCKET_PATHCLNT,id);
//    /*
//    * Is this safe?
//    */
//    (void)unlink(client_path);

//    /*	Create a UNIX datagram socket for client	*/

//    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
//        dacq_perror("socket");
//        return (FAIL);
//    }
//    /*	Client will bind to an address so server will get
//    * 	an address in its recvfrom call and can use it to
//    *	send data back to the client.
//    */
//    bzero(&clntaddr, sizeof(clntaddr));
//    clntaddr.sun_family = AF_UNIX;
//    strcpy(clntaddr.sun_path, client_path);

//    if (bind(sock, (void *)(&clntaddr), sizeof(clntaddr)) < 0) {
//        close(sock);
//        dacq_perror("bind");
//        return (FAIL);
//    }
//    if (connect_disconnect(sock,id) == FAIL)
//        return (FAIL);
//    else
//        return (sock);


    //DEBUG
    return 0;
}


//*************************************************************************************************************

int DacqServer::dacq_disconnect_client (int sock, int id)
{
    int result = connect_disconnect(sock,-id);
    close_socket (sock,id);
    return (result);
}


//*************************************************************************************************************

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
 * \return Status OK or FAIL.
 */

int DacqServer::dacq_client_receive_tag (int sock, /**< Socket to read */
                 int id )  /**< My id number */

{
//    struct  sockaddr_un from;	/* Address (not used) */
//    socklen_t fromlen;

//    dacqDataMessageRec mess;	/* This is the kind of message we receive */
//    int rlen;
//    int data_ok = 0;

//    static unsigned char *data = NULL; /* The data */
//    static int           cursize = 0; /* Its current allocated size */

//    static FILE   *fd = NULL;		/* The temporary file */
//    static FILE   *shmem_fd = NULL;
//    static char   *filename = NULL;

//    long read_loc;
//    FILE *read_fd;

//    fiffTagRec    tag;			/* We convert it to a tag... */
//    extern dacqShmBlock dacq_get_shmem();
//    dacqShmBlock  shmem = dacq_get_shmem();
//    dacqShmBlock  shmBlock;
//    dacqShmClient shmClient;
//    int           k;

//    if (sock < 0)
//    return (OK);

//    fromlen = sizeof(from);
//    rlen = recvfrom(sock, (void *)(&mess), DATA_MESS_SIZE, 0, (void *)(&from), &fromlen);
//    if (rlen == -1) {
//    dacq_perror("recvfrom");
//    close_socket (sock,id);
//    return (FAIL);
//    }

//    /* A sanity check to survive at least some crazy messages */

//    if (mess.kind > 20000 || mess.size > (size_t) 100000000){
//    dacq_log("ALERT: Unreasonable data received, skipping! (size=%d)(kind=%d)",
//         mess.kind, mess.size);
//    mess.size = 0;
//    mess.kind = FIFF_NOP;
//    }

//    if (mess.size > (size_t) 0) {
//    int newsize = (mess.type == FIFFT_STRING) ? mess.size+1 : mess.size;
//    if (data == NULL)
//      data = malloc(newsize);
//    else
//      data = realloc(data,newsize);
//    cursize = newsize;
//    }

//    if (mess.loc < 0 && mess.size > (size_t) 0 &&
//      mess.shmem_buf < 0 && mess.shmem_loc < 0) {
//    fromlen = sizeof(from);
//    rlen = recvfrom(sock, (void *)data, mess.size, 0, (void *)(&from), &fromlen);
//    if (rlen == -1) {
//      dacq_perror("recvfrom");
//      close_socket(sock,id);
//      return (FAIL);
//    }
//    data_ok = 1;
//    if (mess.type == FIFFT_STRING)
//      data[mess.size] = '\0';
//    }
//    else if (mess.size > (size_t) 0) {
//    /*
//     * Copy data from shared memory
//     */
//    if (mess.shmem_buf >= 0 && id/10000 > 0) {
//      shmBlock  = shmem + mess.shmem_buf;
//      shmClient = shmBlock->clients;
//      if (interesting_data(mess.kind)) {
//    memcpy(data,shmBlock->data,mess.size);
//    data_ok = 1;
//    #ifdef DEBUG
//    dacq_log("client # %d read shmem buffer # %d\n",
//          id,mess.shmem_buf);
//    #endif
//      }
//      /*
//       * Indicate that this client has processed the data
//       */
//      for (k = 0; k < SHM_MAX_CLIENT; k++,shmClient++)
//    if (shmClient->client_id == id)
//      shmClient->done = 1;
//    }
//    /*
//     * Read data from file
//     */
//    else {
//      /*
//       * Possibly read from shmem file
//       */
//      if (id/10000 > 0 && mess.shmem_loc >= 0) {
//    read_fd  = shmem_fd;
//    read_loc = mess.shmem_loc;
//      }
//      else {
//    read_fd  = fd;
//    read_loc = mess.loc;
//      }
//      if (interesting_data(mess.kind)) {
//    if (read_fif (read_fd,read_loc,mess.size,(char *)data) == -1) {
//      dacq_log("Could not read data (tag = %d, size = %d, pos = %d)!\n",
//            mess.kind,mess.size,read_loc);
//      dacq_log("%s\n",err_get_error());
//    }
//    else {
//      data_ok = 1;
//      if (mess.type == FIFFT_STRING)
//        data[mess.size] = '\0';
//    }
//    #ifdef DEBUG
//    if (fd == shmem_fd)
//      dacq_log("client # %d read shmem file pos %d\n",
//            id,mess.shmem_loc);
//    #endif
//      }
//    }
//    }
//    /*
//    * Special case: close old input file
//    */
//    if (mess.kind == FIFF_CLOSE_FILE) {
//    if (fd != NULL) {
//      dacq_log("File to be closed (lib/FIFF_CLOSE_FILE).\n");
//      (void)fclose(fd);
//      fd = NULL;
//    }
//    else
//      dacq_log("No file to close (lib/FIFF_CLOSE_FILE).\n");
//    }
//    /*
//    * Another special case: open new input file
//    */
//    else if (mess.kind == FIFF_NEW_FILE) {
//    if (fd != NULL) {
//      (void)fclose(fd);
//    dacq_log("File closed (lib/FIFF_NEW_FILE).\n");
//    }
//    fd = open_fif((char *)data);
//    free (filename);
//    filename = strdup((char *)data);

//    if (shmem_fd == NULL)
//      shmem_fd = open_fif (SHM_FAIL_FILE);
//    }
//    tag.kind = mess.kind;
//    tag.type = mess.type;
//    tag.size = mess.size;
//    tag.next = 0;
//    tag.data = data_ok ? data : NULL;
//    if (tag.size <= 0) {
//    tag.data = NULL;
//    data_ok  = 0;
//    }
//    if (dacq_client_process_tag != NULL) {
//    if (dacq_client_process_tag(&tag)) {/* Non null = don't reuse data */
//      data = NULL;
//      cursize = 0;
//    }
//    else if (data_ok)		/* Client may have done a secret
//                 * 1-to-1 exchange */
//      data = tag.data;
//    }
//    return (OK);

    return 0;
}


//*************************************************************************************************************

void DacqServer::run()
{
    m_bIsRunning = true;


    if(m_pNeuromag->getBufferSampleSize() < MIN_BUFLEN)
        m_pNeuromag->setBufferSampleSize(MIN_BUFLEN);



    /* Connect to the Elekta Neuromag shared memory system */
    printf("About to connect to the Neuromag DACQ shared memory on this workstation (client ID %d)...\r\n", m_iShmemId);

    int old_umask;
    old_umask = umask(SOCKET_UMASK);
    if ((m_fdShmemSock = dacq_connect_client(m_iShmemId)) == -1) {
        umask(old_umask);
        printf("Could not connect!\r\n");
        return;
    }
    printf("Connection ok\r\n");

    int t_iOriginalMaxBuflen = -1;
    /* Connect to the Elekta Neuromag acquisition control server and
     * fiddle with the buffer length parameter */
    if (m_pNeuromag->getBufferSampleSize() > 0) {
        if (collector_open()) {
            printf("Cannot change the Neuromag buffer length: Could not open collector connection\r\n");
            return;
        }
        if ((t_iOriginalMaxBuflen = collector_getMaxBuflen()) < 1) {
            printf("Cannot change the Neuromag buffer length: Could not query the current value\r\n");
            collector_close();
            return;
        }
        printf("Changing the Neuromag buffer length %d -> %d\r\n", t_iOriginalMaxBuflen, m_pNeuromag->getBufferSampleSize());
        if (collector_setMaxBuflen(m_pNeuromag->getBufferSampleSize())) {
            printf("Setting a new Neuromag buffer length failed\r\n");
            collector_close();
            return;
        }
    }
    /* Even if we're not supposed to change the buffer length, let's show it to the user */
    else {
        if (collector_open()) {
            printf("Cannot find Neuromag buffer length: Could not open collector connection\r\n");
            return;
        }
        t_iOriginalMaxBuflen = collector_getMaxBuflen();
        if (t_iOriginalMaxBuflen < 1) {
            printf("Could not query the current Neuromag buffer length\r\n");
            collector_close();
            return;
        }
        else
            printf("Current buffer length value = %d\r\n", t_iOriginalMaxBuflen);

        collector_close();
        // just so we know no clean up is necessary
        t_iOriginalMaxBuflen = -1;
    }




    /* Mainloop */
//    printf("Will scale up MEG mags by %g, grads by %g and EEG data by %g\n",
//         meg_mag_multiplier, meg_grad_multiplier, eeg_multiplier);
    printf("Waiting for the measurement to start... Press Ctrl-C to terminate this program\n");

    qint32 count = 0;
    while(m_bIsRunning)
    {
#if defined(DACQ_OLD_CONNECTION_SCHEME)
        if (dacq_client_receive_tag(m_fdShmemSock, m_iShmemId) == -1)
#else
        if (dacq_client_receive_tag(&m_fdShmemSock, m_iShmemId) == -1)
#endif
            break;

        ++count;

        qDebug() << count;

        usleep(1000000);
    }

     printf("\n");


}
