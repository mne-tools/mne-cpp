//=============================================================================================================
/**
 * @file     shmemsocket.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the ShmemSocket Class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "shmemsocket.h"

#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// UNIX INCLUDES
//=============================================================================================================

#include <stdio.h>      // fopen
#include <string.h>     // memcpy
#include <unistd.h>     // unlink, close

#include <sys/stat.h>   //umask
#include <sys/un.h>     //sockaddr_un
#include <sys/socket.h> //AF_UNIX
#include <sys/shm.h>    //shmdt

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGRTSERVERPLUGIN;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ShmemSocket::ShmemSocket(QObject *parent)
: QObject(parent)
, shmid(-1)
, shmptr(Q_NULLPTR)
, m_iShmemSock(-1)
, m_iShmemId(CLIENT_ID)
, fd(Q_NULLPTR)
, shmem_fd(Q_NULLPTR)
, filename(Q_NULLPTR)
, read_fd(Q_NULLPTR)
{
    filter_kinds = Q_NULLPTR;    /* Filter these tags */
    nfilt = 0;              /* How many are they */
}


//*************************************************************************************************************

ShmemSocket::~ShmemSocket()
{
    delete[] filter_kinds;
    free (filename);
}


//*************************************************************************************************************
//=============================================================================================================
// client_socket.c
//=============================================================================================================

int ShmemSocket::receive_tag (FiffTag::SPtr& p_pTag)
{
    struct  sockaddr_un from;	/* Address (not used) */
    socklen_t fromlen;

    dacqDataMessageRec mess;	/* This is the kind of message we receive */
    int rlen;
    int data_ok = 0;

    p_pTag = FiffTag::SPtr(new FiffTag());
    dacqShmBlock  shmem = this->get_shmem();
    dacqShmBlock  shmBlock;
    dacqShmClient shmClient;
    int           k;


    long read_loc = 0;

    if (m_iShmemSock < 0)
        return (OK);

    //
    // read from the socket
    //
    fromlen = sizeof(from);
    rlen = recvfrom(m_iShmemSock, (void *)(&mess), DATA_MESS_SIZE, 0, (sockaddr *)(&from), &fromlen);

//    qDebug() << "Mess Kind: " << mess.kind << " Type: " << mess.type << "Size: " << mess.size;

    //
    // Parse received message
    //
    if (rlen == -1)
    {
        printf("recvfrom");//dacq_perror("recvfrom");
        this->close_socket ();
        return (FAIL);
    }

    /* A sanity check to survive at least some crazy messages */

    if (mess.kind > 20000 || (unsigned long) mess.size > (size_t) 100000000)
    {
        printf("ALERT: Unreasonable data received, skipping! (size=%d)(kind=%d)", mess.kind, mess.size);
        //dacq_log("ALERT: Unreasonable data received, skipping! (size=%d)(kind=%d)", mess.kind, mess.size);
        mess.size = 0;
        mess.kind = FIFF_NOP;
    }

    p_pTag->kind = mess.kind;
    p_pTag->type = mess.type;
    p_pTag->next = 0;

    if ((unsigned long) mess.size > (size_t) 0)
    {
        p_pTag->resize(mess.size);
    } else {
        p_pTag->resize(0);
        qWarning("Received empty message");
    }

//    qDebug() << mess.loc << " " << mess.size << " " << mess.shmem_buf << " " << mess.shmem_loc;

    if (mess.loc < 0 && (unsigned long) mess.size > (size_t) 0 && mess.shmem_buf < 0 && mess.shmem_loc < 0)
    {
        fromlen = sizeof(from);
        rlen = recvfrom(m_iShmemSock, (void *)p_pTag->data(), mess.size, 0, (sockaddr *)(&from), &fromlen);
        if (rlen == -1)
        {
            printf("recvfrom");//dacq_perror("recvfrom");
            this->close_socket();
            return (FAIL);
        }
        data_ok = 1;
//        if (mess.type == FIFFT_STRING)
//            data[mess.size] = '\0';
    }
    else if ((unsigned long) mess.size > (size_t) 0) {
        /*
         * Copy data from shared memory
         */
        if (mess.shmem_buf >= 0 && m_iShmemId/10000 > 0)
        {
            shmBlock  = shmem + mess.shmem_buf;
            shmClient = shmBlock->clients;

            if (interesting_data(mess.kind))
            {
                memcpy(p_pTag->data(),shmBlock->data,mess.size);
                data_ok = 1;
            #ifdef DEBUG
                printf("client # %d read shmem buffer # %d\n", m_iShmemId, mess.shmem_buf);//dacq_log("client # %d read shmem buffer # %d\n", id,mess.shmem_buf);
            #endif
            }
            /*
            * Indicate that this client has processed the data
            */
            for (k = 0; k < SHM_MAX_CLIENT; k++,shmClient++)
                if (shmClient->client_id == m_iShmemId)
                    shmClient->done = 1;
        }
        /*
        * Read data from file
        */
        else {
            /*
            * Possibly read from shmem file
            */
            if (m_iShmemId/10000 > 0 && mess.shmem_loc >= 0) {
                read_fd  = shmem_fd;
                read_loc = mess.shmem_loc;
            }
            else {
                read_fd  = fd;
                read_loc = mess.loc;
            }
            if (interesting_data(mess.kind)) {
                if (read_fif (read_fd,read_loc,mess.size,(char *)p_pTag->data()) == -1) {
                    printf("Could not read data (tag = %d, size = %d, pos = %li)!\n", mess.kind,mess.size,read_loc);//dacq_log("Could not read data (tag = %d, size = %d, pos = %d)!\n", mess.kind,mess.size,read_loc);
                    //dacq_log("%s\n",err_get_error());
                }
                else {
                    data_ok = 1;
//                    if (mess.type == FIFFT_STRING)
//                        data[mess.size] = '\0';
                    FiffTag::convert_tag_data(p_pTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
                }
            }
        }
    }

    /*
    * Special case: close old input file
    */
    if (mess.kind == FIFF_CLOSE_FILE) {
        if (fd != NULL) {
            printf("File to be closed (lib/FIFF_CLOSE_FILE).\n");//dacq_log("File to be closed (lib/FIFF_CLOSE_FILE).\n");
            (void)fclose(fd);
            fd = NULL;
        }
        else
            printf("No file to close (lib/FIFF_CLOSE_FILE).\n");//dacq_log("No file to close (lib/FIFF_CLOSE_FILE).\n");
    }
    /*
    * Another special case: open new input file
    */
    else if (mess.kind == FIFF_NEW_FILE) {
        if (fd != NULL) {
            (void)fclose(fd);
            printf("File closed (lib/FIFF_NEW_FILE).\n");//dacq_log("File closed (lib/FIFF_NEW_FILE).\n");
        }
        fd = open_fif((char *)p_pTag->data());
        free (filename);
        filename = strdup((char *)p_pTag->data());

        if (shmem_fd == NULL)
            shmem_fd = open_fif (SHM_FAIL_FILE);
    }

    if (p_pTag->size() <= 0)
    {
        data_ok  = 0;
        return (FAIL);
    }
    return (OK);
}


//*************************************************************************************************************

FILE *ShmemSocket::open_fif (char *name)

{
    FILE *fd;
    printf("Open %s... ",name);//dacq_log ("should open %s\n",name);

    if ((fd = fopen(name,"r")) == NULL) {
        printf ("failed!\r\n");//dacq_log ("failed to open %s\n",name);
        //dacq_perror(name);
    }
    else
        printf ("[done]\r\n");

    return (fd);
}


//*************************************************************************************************************

int ShmemSocket::read_fif (FILE   *fd, long   pos, size_t size, char   *data)
{
    if (fd == NULL) {
        printf("Cannot read from NULL fd.");//err_set_error("Cannot read from NULL fd.");
        return (FAIL);
    }
    if (fseek(fd,pos,SEEK_SET) == -1) {
        printf("fseek");//err_set_sys_error("fseek");
        return (FAIL);
    }
    if (fread(data,size,1,fd) != (size_t) 1) {
        printf("Data not available.");//err_set_error("Data not available.");
        return (FAIL);
    }
    return (OK);
}


//*************************************************************************************************************

bool ShmemSocket::connect_client ()
{
    printf("About to connect to the Neuromag DACQ shared memory on this workstation (client ID %d)... ", m_iShmemId);//dacq_log("About to connect to the Neuromag DACQ shared memory on this workstation (client ID %d)...\n", shmem_id);
    int old_umask = umask(SOCKET_UMASK);
    int id = m_iShmemId;

    struct  sockaddr_un clntaddr;   /* address of client */
    char    client_path[108];       /* This our path */
    int     sock = -1;              /* This is the UNIX domain socket */

    sprintf (client_path,"%s%d",SOCKET_PATHCLNT,id);
    /*
     * Is this safe?
     */
    (void)unlink(client_path);

    /*	Create a UNIX datagram socket for client	*/

    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        //dacq_perror("socket");
        printf("socket error");
        umask(old_umask);
        m_iShmemSock = -1;
        return false;
    }
    /*	Client will bind to an address so server will get
     * 	an address in its recvfrom call and can use it to
     *	send data back to the client.
     */
    bzero(&clntaddr, sizeof(clntaddr));
    clntaddr.sun_family = AF_UNIX;
    strcpy(clntaddr.sun_path, client_path);

    if (bind(sock, (sockaddr *)(&clntaddr), sizeof(clntaddr)) < 0) {
        close(sock);
        //dacq_perror("bind");
        printf("bind error");
        umask(old_umask);
        m_iShmemSock = -1;
        return false;
    }
    if (connect_disconnect(sock,id) == FAIL)
    {
        m_iShmemSock = -1;
        umask(old_umask);
        return false;
    }
    else
    {
        m_iShmemSock = sock;
        printf("[done]\r\n");//dacq_log("Connection ok\n");
        return true;
    }
}


//*************************************************************************************************************

int ShmemSocket::disconnect_client ()
{
    int sock = m_iShmemSock;
    int id = m_iShmemId;
    int result = connect_disconnect(sock,-id);
    if (result != FAIL)
    {
        qWarning("connect_disconnect did not close socket. attempting to close socket again.");
        this->close_socket();
    }
    return (result);
}


//*************************************************************************************************************

void ShmemSocket::set_data_filter (int *kinds, int nkind)
{
    if (nkind <= 0) {
        delete[] filter_kinds;
        nfilt = 0;
    }
    else {
        nfilt = nkind;
        if(filter_kinds)
            delete[] filter_kinds;
        filter_kinds = new int[nfilt];
        memcpy(filter_kinds,kinds,nfilt*sizeof(int));
    }
    return;
}


//*************************************************************************************************************

void ShmemSocket::close_socket ()
{
    int sock = m_iShmemSock;
    int id = m_iShmemId;


    char    client_path[200];   /* This our path */

    if (sock != -1) {
    /*
     * Use unlink to remove the file (inode) so that the name
     * will be available for the next run.
     */
        sprintf (client_path,"%s%d",SOCKET_PATHCLNT,id);
        unlink(client_path);
        close(sock);
    }
    (void) release_shmem();
    //dacq_log ("Connection closed.\n");
    printf ("Connection closed.\r\n");
}


//*************************************************************************************************************

int ShmemSocket::connect_disconnect (int sock,int id)
{
    struct  sockaddr_un servaddr;       /* address of server */
    struct  sockaddr_un from;

    socklen_t fromlen;
    int     result;
    int     slen, rlen;

    if (sock < 0)
        return (OK);
    /*
     * Set up address structure for server socket
     */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, SOCKET_PATH);

    slen = sendto(sock, (void *)(&id), sizeof(int), 0,
          (sockaddr *)(&servaddr), sizeof(servaddr));
    if (slen<0) {
        printf("sendto error");//dacq_perror("sendto");
        this->close_socket ();
        return (FAIL);
    }
    else {
        fromlen = sizeof(from);
        rlen = recvfrom(sock, (void *)(&result), sizeof(int), 0,
              (sockaddr *)(&from), &fromlen);
        if (rlen == -1) {
            printf("recvfrom");//dacq_perror("recvfrom");
            this->close_socket ();
            return (FAIL);
        }
        else
            return result;
    }
}


//*************************************************************************************************************

int ShmemSocket::interesting_data (int kind)
{
    int k;
    for (k = 0; k < nfilt; k++)
        if (kind == filter_kinds[k])
            return (0);
    return (1);
}


//*************************************************************************************************************
//=============================================================================================================
// shmem.c
//=============================================================================================================

dacqShmBlock ShmemSocket::get_shmem()
{
  if (init_shmem() == -1)
    return(NULL);
  else
    return(shmptr);
}


//*************************************************************************************************************

int ShmemSocket::init_shmem()
{
    key_t key = ftok(SHM_FILE,'A');

    if (shmid == -1) {
        if ((shmid = shmget(key,SHM_SIZE,IPC_CREAT | 0666)) == -1) {
            printf("shmget");//err_set_sys_error("shmget");
            return (-1);
        }
    }
    if (shmptr == NULL) {
        if (!(shmptr = (dacqShmBlockRec*)shmat(shmid,0,0))) {
            printf("shmat");//err_set_sys_error("shmat");
            shmptr = NULL;
            return (-1);
        }
    }
    return (0);
}


//*************************************************************************************************************

int ShmemSocket::release_shmem()
{
    if (shmid == -1)
    return (0);
    if (shmptr != NULL) {
    if (shmdt(shmptr) == -1) {
        //err_set_sys_error("shmdt");
        printf("shmdt");
        return (-1);
    }
    shmptr = NULL;
    }
    return (0);
}
