//=============================================================================================================
/**
* @file     types_definitions.h
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
* @brief     Neuromag Types and Defines.
*
*/

#ifndef TYPESDEFINITIONS_H
#define TYPESDEFINITIONS_H


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGRTSERVERPLUGIN
//=============================================================================================================

namespace NEUROMAGRTSERVERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define DACQ_AUTOSTART

#define DACQ_REPLY_PACKET 1
#define DACQ_REPLY_RFC    2
#define DACQ_REPLY_BINARY 4
#define DACQ_REPLY_ASCII  8

#define DACQ_DRAIN_INPUT  0
#define DACQ_KEEP_INPUT   1

#define DACQ_REPLY_GOOD   1
#define DACQ_REPLY_BAD    0
#define DACQ_REPLY_ERROR -1

#define DACQ_CMD_PASSWORD "pass"
#define DACQ_CMD_NAME     "name"
#define DACQ_CMD_ABOUT    "abou"
#define DACQ_CMD_MONITOR  "moni"
#define DACQ_CMD_HELP     "help"
#define DACQ_CMD_QUIT     "quit"




#define COLLECTOR_PORT    11122         //"collector"
#define COLLECTOR_PASS    "homunculus122"
#define COLLECTOR_BUFS    32768

#define COLLECTOR_GETVARS "vars"
#define COLLECTOR_SETVARS "vara"
#define COLLECTOR_DOSETUP "setu"
#define COLLECTOR_STAT    "stat"
#define COLLECTOR_BUFVAR  "maxBuflen"

#define MIN_BUFLEN      1*28    /**< DSP units send packets of 28 samples, which is the ultimate lower bound */
#define CLIENT_ID       13014   /**< ID of rtclient. A unique ID for us as a shared memory client. Should be more than 10000. Is also used to create the sun_path which is used to connect to the data acquisition peer over a UNIX datagram socket.*/

#define SOCKET_UMASK    0x000   /**< Acquisition system UNIX domain socket file must be world-writable */

#define SOCKET_PATH     "/neuro/dacq/sockets/dacq_server"
#define SOCKET_PATHCLNT "/neuro/dacq/sockets/dacq_client_"

//#define sockfd  int             /**< Defines a primitive data type for socket descriptor. */

#define OK      0
#define FAIL    -1

//
// compat.h
//

//typedef int socklen_t;


//
// dacq_shmem.h
//
#define SHM_FILE       "/neuro/dacq/shmem/data_server"
#define SHM_FAIL_FILE  "/neuro/dacq/raw/data_server_shmem"
#define SHM_MAX_CLIENT 10

//#ifdef OLD_SHM
//#define SHM_MAX_DATA   2*2*2*31*1024
//#define SHM_NUM_BLOCKS 10
//#else
//#define SHM_MAX_DATA   450*2*3000
//#define SHM_NUM_BLOCKS 5
//#endif
//#define SHM_NO_BUF     -1

#define SHM_MAX_DATA   500*1500*4
#define SHM_NUM_BLOCKS 100
#define SHM_NO_BUF     -1

typedef struct {
  int client_id;
  int done;
} *dacqShmClient,dacqShmClientRec;

typedef struct {
  dacqShmClientRec clients[SHM_MAX_CLIENT];
  unsigned char data[SHM_MAX_DATA];
} *dacqShmBlock,dacqShmBlockRec;

#define SHM_SIZE SHM_NUM_BLOCKS*sizeof(dacqShmBlockRec)



//
// data_message.h
//
typedef struct {
  int     kind;             /* What is this data? */
  int     type;             /* What is its type */
  int     size;             /* Size of item */
  int     loc;              /* Position in file */
  int     shmem_buf;        /* Shared mem block */
  int     shmem_loc;        /* Not used, set to -1 */
} dacqDataMessageRec,*dacqDataMessage;

#define DATA_MESS_SIZE sizeof(dacqDataMessageRec)

}

#endif // TYPESDEFINITIONS_H
