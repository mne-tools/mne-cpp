//=============================================================================================================
/**
* @file     dacqserver.cpp
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
#include "../../../MNE/fiff/fiff_constants.h"
#include "../../../MNE/fiff/fiff_stream.h"


//*************************************************************************************************************
//=============================================================================================================
// UNIX INCLUDES
//=============================================================================================================

#include <sys/stat.h>   //umask
#include <sys/un.h>     //sockaddr_un
#include <sys/socket.h> //AF_UNIX
#include <sys/shm.h>    //shmdt


//#include <sys/types.h>
//#include <netinet/in.h>



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QTcpSocket>
#include <QtNetwork>
#include <QDataStream>


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
, m_sCollectorHost(QHostAddress(QHostAddress::LocalHost).toString())
, m_pCollectorSock(NULL)
, m_iShmemSock(-1)
, m_iShmemId(CLIENT_ID)
, m_bIsRunning(false)
, m_bIsMeasuring(false)
, m_bMeasInfoRequest(true)
, m_bMeasRequest(true)
, m_bMeasStopRequest(false)
, m_bSetBuffersizeRequest(false)
, shmid(-1)
, shmptr(NULL)
, fd(NULL)
, shmem_fd(NULL)
, filename(NULL)
{

    filter_kinds = NULL;    /* Filter these tags */
    nfilt = 0;              /* How many are they */
    
}


//*************************************************************************************************************

DacqServer::~DacqServer()
{
    if(m_pCollectorSock)
        delete m_pCollectorSock;      
//    if(shmptr)
//        delete shmptr;
}


//*************************************************************************************************************

bool DacqServer::getMeasInfo(FiffInfo*& p_pFiffInfo)
{

    if (p_pFiffInfo)
        delete p_pFiffInfo;
    p_pFiffInfo = new FiffInfo();

    dacq_server_stop();
    dacq_server_start();

    QStringList names;
    MatrixXd* data = NULL;
    FiffTag* t_pTag = NULL;
    bool t_bReadHeader = true;
    while(t_bReadHeader)
    {
        if (dacq_client_receive_tag(t_pTag) == -1)
            break;
    

        switch(t_pTag->kind)
        {
            case FIFFV_MEG_CH:
                qDebug() << "FIFFV_MEG_CH " << t_pTag->toString();
                break;

            case FIFF_BLOCK_START:
                qDebug() << "FIFF_BLOCK_START";
                switch(*(t_pTag->toInt()))
                {
                    
                    case FIFFB_MEAS:
                        qDebug() << "    FIFFB_MEAS";
                        break;
                    case FIFFB_MEAS_INFO:
                        qDebug() << "    FIFFB_MEAS_INFO";
                        break;
                    case FIFFB_PROJ:
                        qDebug() << "    FIFFB_PROJ";
                        break;
                    case FIFFB_PROJ_ITEM:
                        qDebug() << "    FIFFB_PROJ_ITEM";
                        break;
                    case FIFFB_PROCESSING_HISTORY:
                        qDebug() << "    FIFFB_PROCESSING_HISTORY";
                        break;
                    case FIFFB_RAW_DATA:
                        qDebug() << "    FIFFB_RAW_DATA";
                        break;
                    default:
                        qDebug() << "    Unknown " << *(t_pTag->toInt());
                }
                break;
            
            case FIFFB_PROCESSED_DATA:
                qDebug() << "FIFFB_PROCESSED_DATA " << t_pTag->toFiffID().version;
                break;

            case FIFF_NCHAN:
                qDebug() << "FIFF_NCHAN " << *(t_pTag->toInt());
                break;
            case FIFF_PROJ_ITEM_CH_NAME_LIST:
                names = FiffStream::split_name_list(t_pTag->toString());
                qDebug() << "FIFF_PROJ_ITEM_CH_NAME_LIST " << names.size();// << t_pTag->toString();
                break;
            case FIFF_NAME:
                qDebug() << "FIFF_NAME " << t_pTag->toString();
                break;
            case FIFF_PROJ_ITEM_KIND:
                qDebug() << "FIFF_PROJ_ITEM_KIND " << *(t_pTag->toInt());
                break;
            case FIFF_PROJ_ITEM_TIME:
                qDebug() << "FIFF_PROJ_ITEM_TIME " << *(t_pTag->toFloat());
                break;
            case FIFF_PROJ_ITEM_NVEC:
                qDebug() << "FIFF_PROJ_ITEM_NVEC " << *(t_pTag->toInt());
                break;        
            case FIFF_MNE_PROJ_ITEM_ACTIVE:
                qDebug() << "FIFF_MNE_PROJ_ITEM_ACTIVE " << *(t_pTag->toInt());
                break;
            case FIFF_PROJ_ITEM_VECTORS:
                qDebug() << "FIFF_PROJ_ITEM_VECTORS ";
                data = t_pTag->toFloatMatrix();
                data->transposeInPlace();
                qDebug() << data->rows() << "x" << data->cols();
                delete data;
                data = NULL;
                break;
            case FIFF_MEAS_DATE:
                qDebug() << "FIFF_MEAS_DATE " << t_pTag->toInt()[0] << t_pTag->toInt()[1];
    //            meas_date[0] = t_pTag->toInt()[0];
    //            meas_date[1] = t_pTag->toInt()[1];
                break;
            case FIFF_SFREQ:
                qDebug() << "FIFF_SFREQ " << *(t_pTag->toFloat());
                break;
            case FIFF_LOWPASS:
                qDebug() << "FIFF_LOWPASS " << *(t_pTag->toFloat());
                break;
            case FIFF_HIGHPASS:
                qDebug() << "FIFF_HIGHPASS " << *(t_pTag->toFloat());
                break;
            case FIFF_LINE_FREQ:
                qDebug() << "FIFF_LINE_FREQ " << *(t_pTag->toFloat());
                break;
            case FIFF_UNIT_AM:
                qDebug() << "FIFF_UNIT_AM " << *(t_pTag->toInt());
                break;
            case FIFF_CH_INFO:
                qDebug() << "FIFF_CH_INFO";
                break;
            case FIFF_BLOCK_END:
                qDebug() << "FIFF_BLOCK_END " << *(t_pTag->toInt());
                switch(*(t_pTag->toInt()))
                {
                    
                    case FIFFB_MEAS:
                        qDebug() << "    FIFFB_MEAS";
                        break;
                    case FIFFB_MEAS_INFO:
                        qDebug() << "    FIFFB_MEAS_INFO";
                        t_bReadHeader = false;
                        break;
                    case FIFFB_PROJ:
                        qDebug() << "    FIFFB_PROJ";
                        break;
                    case FIFFB_PROJ_ITEM:
                        qDebug() << "     FIFFB_PROJ_ITEM";
                        break;
                    case FIFFB_RAW_DATA:
                        qDebug() << "    FIFFB_RAW_DATA";
                        break;
                    case FIFFB_PROCESSING_HISTORY:
                        qDebug() << "    FIFFB_PROCESSING_HISTORY";
                        break;
                    default:
                        qDebug() << "    Unknown " << *(t_pTag->toInt());
                }
                break;
            case FIFF_HPI_NCOIL:
                qDebug() << "FIFF_HPI_NCOIL " << *(t_pTag->toInt());
                break;
            case FIFFV_RESP_CH:
                qDebug() << "FIFFV_RESP_CH " << t_pTag->toString();
                break;
            default:
                qDebug() << "Unknown Tag Kind: " << t_pTag->kind << " Type: " << t_pTag->type << "Size: " << t_pTag->size();
        }
        
        
        
        
        
        
        
    }



    delete t_pTag;

    if (!m_bMeasRequest)
        dacq_server_stop();
        
    //t_bReadHeader is still true --> that means a break occured
    if (t_bReadHeader)
        return false;
        
    return true;
}




//*************************************************************************************************************
//=============================================================================================================
// run
//=============================================================================================================

void DacqServer::run()
{
    m_bIsRunning = true;


    connect(this, &DacqServer::measInfoAvailable,
            m_pNeuromag, &Neuromag::releaseMeasInfo);



    dacq_set_data_filter (NULL, 0);

    if(m_pCollectorSock)
        delete m_pCollectorSock;
    m_pCollectorSock = new QTcpSocket();
    
    //
    // Make sure the buffer size is at least as big as the minimal buffer size
    //
    if(m_pNeuromag->getBufferSampleSize() < MIN_BUFLEN)
        m_pNeuromag->setBufferSampleSize(MIN_BUFLEN);

///////necessary? -> without that it sometimes doesn't work -> this sets the buffer size without questioning it


    int  t_iOriginalMaxBuflen = 1500;// set the standard size as long as we can't get it without setting it
    if (m_pNeuromag->getBufferSampleSize() < MIN_BUFLEN) {
        fprintf(stderr, "%s: Too small Neuromag buffer length requested, should be at least %d\n", m_pNeuromag->getBufferSampleSize(), MIN_BUFLEN);
        return;
    }
    else {
        /* Connect to the Elekta Neuromag acquisition control server, change the buffer length and exit*/
        if (!collector_open()) {
            printf("Cannot change the Neuromag buffer length: Could not open collector connection\n");//dacq_log("Cannot change the Neuromag buffer length: Could not open collector connection\n");
            return;
        }
        printf("Changing the Neuromag buffer length to %d... ", m_pNeuromag->getBufferSampleSize());//dacq_log("Changing the Neuromag buffer length to %d\n", newMaxBuflen);
        if (collector_setMaxBuflen(m_pNeuromag->getBufferSampleSize())) {
            printf("Setting a new Neuromag buffer length failed\r\n");//dacq_log("Setting a new Neuromag buffer length failed\n");
            collector_close();
            return;
        }
        printf("[done]\r\n");        
    }
////////////

    /* Connect to the Elekta Neuromag shared memory system */
    printf("About to connect to the Neuromag DACQ shared memory on this workstation (client ID %d)... ", m_iShmemId);//dacq_log("About to connect to the Neuromag DACQ shared memory on this workstation (client ID %d)...\n", shmem_id);
    int old_umask = umask(SOCKET_UMASK);
    if ((m_iShmemSock = dacq_connect_client(m_iShmemId)) == -1) {
        umask(old_umask);
        printf("Could not connect!\r\n");//dacq_log("Could not connect!\n");
        return;//(2);
    }
    printf("[done]\r\n");//dacq_log("Connection ok\n");

//    int  t_iOriginalMaxBuflen = -1;
//    /* Connect to the Elekta Neuromag acquisition control server and
//     * fiddle with the buffer length parameter */
//    if (m_pNeuromag->getBufferSampleSize() > 0) {
//        if (!collector_open()) {
//            printf("Cannot change the Neuromag buffer length: Could not open collector connection\r\n");//dacq_log("Cannot change the Neuromag buffer length: Could not open collector connection\n");
//            return;
//        }
//        if ((t_iOriginalMaxBuflen = collector_getMaxBuflen()) < 1) {
//            printf("Cannot change the Neuromag buffer length: Could not query the current value\r\n");//dacq_log("Cannot change the Neuromag buffer length: Could not query the current value\n");
//            collector_close();
//            return;
//        }
//        
//        if (t_iOriginalMaxBuflen != m_pNeuromag->getBufferSampleSize())
//        {
//            printf("Changing the Neuromag buffer length %d -> %d\r\n", t_iOriginalMaxBuflen, m_pNeuromag->getBufferSampleSize());
//            if (collector_setMaxBuflen(m_pNeuromag->getBufferSampleSize())) {
//                printf("Setting a new Neuromag buffer length failed");//dacq_log("Setting a new Neuromag buffer length failed\n");
//                collector_close();
//                return;
//            }
//            printf("[done]\r\n");    
//        }
//    }
//    /* Even if we're not supposed to change the buffer length, let's show it to the user */
//    else {
//        if (collector_open()) {
//            printf("Cannot find Neuromag buffer length: Could not open collector connection\r\n");//dacq_log("Cannot find Neuromag buffer length: Could not open collector connection\n");
//            return;
//        }
//        t_iOriginalMaxBuflen = collector_getMaxBuflen();
//        if (t_iOriginalMaxBuflen < 1) {
//            printf("Could not query the current Neuromag buffer length\r\n");//dacq_log("Could not query the current Neuromag buffer length\n");
//            collector_close();
//            return;
//        }
//        else
//            printf("Current buffer length value = %d\r\n",t_iOriginalMaxBuflen);//dacq_log("Current buffer length value = %d\n",originalMaxBuflen);
//    }

    /* Mainloop */
//    printf("Will scale up MEG mags by %g, grads by %g and EEG data by %g\n",
//         meg_mag_multiplier, meg_grad_multiplier, eeg_multiplier);
    printf("Waiting for the measurement to start...\n");
    
    //
    // Control measurement start through Neuromag connector. ToDo: in Case Realtime measurement should be performed during normal acqusition process, change this!!
    //
//    dacq_server_start();
    
    //
    // Receive shmem tags
    //
    
    FiffTag* t_pTag = NULL;
    
    qint32 count = 0;
    while(m_bIsRunning)
    {
        if(m_bMeasInfoRequest)
        {
            //Requesting new info
            if(getMeasInfo(m_pNeuromag->m_pInfo))
            {
                emit measInfoAvailable();
                m_bMeasInfoRequest = false;
            }
        }
        
        if(m_bMeasRequest)
        {
            if (dacq_client_receive_tag(t_pTag) == -1)
                break;
        }
        
        
        

        qDebug() << count;
//#if defined(DACQ_OLD_CONNECTION_SCHEME)
//        if (dacq_client_receive_tag(t_pTag) == -1)        
//#else
//        if (dacq_client_receive_tag(&m_iShmemSock, m_iShmemId) == -1)
//#endif
//            break;
            


            
        if(t_pTag->kind == FIFF_ERROR_MESSAGE)
        {
            qDebug() << "Error: " << *t_pTag;
            break;            
        }
        
        else if(t_pTag->kind == FIFF_CLOSE_FILE)
        {
            printf("Measurement stopped.\r\n");
        }
        
        else
        {
            qDebug() << "Tag Kind: " << t_pTag->kind << " Type: " << t_pTag->type << "Size: " << t_pTag->size();
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        


        ++count;
    }
    
    
    //
    // Stop and clean up
    //    
    dacq_server_stop();
    
    dacq_disconnect_client(m_iShmemSock, m_iShmemId);
    collector_close();
    
    delete t_pTag;
    delete m_pCollectorSock;
    m_pCollectorSock = NULL;

    printf("\r\n");
}








//*************************************************************************************************************

bool DacqServer::collector_open()
{
    printf("About to connect to collector... ");
    if(m_pCollectorSock->state() == QAbstractSocket::ConnectedState)
    {
        printf("Note: Tried to re-open an open connection... [done]\r\n");//dacq_log("Note: Tried to re-open an open connection\n");
        return true;
    }
    m_pCollectorSock->abort();
    m_pCollectorSock->connectToHost(m_sCollectorHost, COLLECTOR_PORT);
    m_pCollectorSock->waitForConnected( -1 );//qDebug() << "Socket Stat: " << m_pCollectorSock->state();
    printf("[done]\r\n");

    if (!dacq_server_login(QString(COLLECTOR_PASS), QString("mne_rt_server"))) {
        printf("Neuromag collector connection: Error\r\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return false;
    }

    return true;
}


//*************************************************************************************************************

int DacqServer::collector_close()
{

    qDebug() << "DacqServer::collector_close()";

    if (m_pCollectorSock == NULL)
        return(0);
//    if (dacq_server_close(&m_pCollectorSock, NULL)) {
//        printf("Neuromag collector connection: Eror\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
//        return(-1);
//    }

    m_pCollectorSock->waitForReadyRead(100);

    m_pCollectorSock->close();

//    m_iCollectorSock = -1;
    return(0);
}


//*************************************************************************************************************
// ToDo doesn't work without setting first buffersize first
int DacqServer::collector_getMaxBuflen()
{

    int maxbuflen = -1;

    QString t_sSend = QString("%1\r\n").arg(COLLECTOR_GETVARS);

    QByteArray t_buf;
    if (!dacq_server_send(t_sSend, t_buf, DACQ_KEEP_INPUT)) {
        printf("Neuromag collector connection: Error\r\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return -1;
    }

    QList<QByteArray> t_lLinesBuffer = t_buf.split('\n');

    QByteArray bufVar1(COLLECTOR_BUFVAR);
    QByteArray bufVar2("Vectors in a buffer");
    for(qint32 i = 0; i < t_lLinesBuffer.size(); ++i)
    {
        if(t_lLinesBuffer[i].contains(bufVar1)) //option 1
        {
            char var_name[1024];
            char var_value[1024];
            char var_type[1024];
            sscanf(t_lLinesBuffer[i].data()+4, "%s %s %s", var_name, var_value, var_type);
            maxbuflen = QString(var_value).toInt();
            return maxbuflen;
        }
        else if(t_lLinesBuffer[i].contains(bufVar2)) //option 2
        {
            QList<QByteArray> t_lMaxBuflen = t_lLinesBuffer[i].split(':');
            maxbuflen = t_lMaxBuflen[t_lMaxBuflen.size()-1].trimmed().toInt();
            return maxbuflen;
        }
    }

    return -1;
}


//*************************************************************************************************************

int DacqServer::collector_setMaxBuflen(int maxbuflen)
{
    if (maxbuflen < 1)
        return(0);

    if (!dacq_server_command(QString("%1 %2 %3\n").arg(COLLECTOR_SETVARS).arg(COLLECTOR_BUFVAR).arg(maxbuflen)) ||
            !dacq_server_command(QString("%1\n").arg(COLLECTOR_DOSETUP)))
    {
        printf("Neuromag collector connection: Error\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return(-1);
    }

    return(0);
}


//*************************************************************************************************************
//=============================================================================================================
// client_socket.c
//=============================================================================================================

int DacqServer::dacq_client_receive_tag (FiffTag*& p_pTag )
{
    struct  sockaddr_un from;	/* Address (not used) */
    socklen_t fromlen;

    dacqDataMessageRec mess;	/* This is the kind of message we receive */
    int rlen;
    int data_ok = 0;

    if(p_pTag)
        delete p_pTag;
    p_pTag = new FiffTag();
    dacqShmBlock  shmem = dacq_get_shmem();
    dacqShmBlock  shmBlock;
    dacqShmClient shmClient;
    int           k;
    

    long read_loc;

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
        close_socket (m_iShmemSock, m_iShmemId);
        return (FAIL);
    }

    /* A sanity check to survive at least some crazy messages */

    if (mess.kind > 20000 || mess.size > (size_t) 100000000)
    {
        printf("ALERT: Unreasonable data received, skipping! (size=%d)(kind=%d)", mess.kind, mess.size);
        //dacq_log("ALERT: Unreasonable data received, skipping! (size=%d)(kind=%d)", mess.kind, mess.size);
        mess.size = 0;
        mess.kind = FIFF_NOP;
    }
    
    p_pTag->kind = mess.kind;
    p_pTag->type = mess.type;
    p_pTag->next = 0;
    
    if (mess.size > (size_t) 0)
    {
        p_pTag->resize(mess.size);
    }

//    qDebug() << mess.loc << " " << mess.size << " " << mess.shmem_buf << " " << mess.shmem_loc;

    if (mess.loc < 0 && mess.size > (size_t) 0 && mess.shmem_buf < 0 && mess.shmem_loc < 0)
    {
        fromlen = sizeof(from);
        rlen = recvfrom(m_iShmemSock, (void *)p_pTag->data(), mess.size, 0, (sockaddr *)(&from), &fromlen);
        if (rlen == -1)
        {
            printf("recvfrom");//dacq_perror("recvfrom");
            close_socket(m_iShmemSock, m_iShmemId);
            return (FAIL);
        }
        data_ok = 1;
//        if (mess.type == FIFFT_STRING)
//            data[mess.size] = '\0';
    }
    else if (mess.size > (size_t) 0) {
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
            //#ifdef DEBUG
                printf("client # %d read shmem buffer # %d\n", m_iShmemId, mess.shmem_buf);//dacq_log("client # %d read shmem buffer # %d\n", id,mess.shmem_buf);
            //#endif
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
                    printf("Could not read data (tag = %d, size = %d, pos = %d)!\n", mess.kind,mess.size,read_loc);//dacq_log("Could not read data (tag = %d, size = %d, pos = %d)!\n", mess.kind,mess.size,read_loc);
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

FILE *DacqServer::open_fif (char *name)

{
    FILE *fd;
    printf("should open %s\n",name);//dacq_log ("should open %s\n",name);
    if ((fd = fopen(name,"r")) == NULL) {
        printf ("failed to open %s\n",name);//dacq_log ("failed to open %s\n",name);
        //dacq_perror(name);
    }
    return (fd);
}


//*************************************************************************************************************

int DacqServer::read_fif (FILE   *fd,		/* File to read from */
		     long   pos,		/* Position in file */
		     size_t size,		/* How long */
		     char   *data)              /* Put data here */
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

int DacqServer::dacq_connect_client (int id)
     /*
      * Connect to the data server process
      */
{
    struct  sockaddr_un clntaddr;   /* address of client */
    char    client_path[200];       /* This our path */
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
        return (FAIL);
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
        return (FAIL);
    }
    if (connect_disconnect(sock,id) == FAIL)
        return (FAIL);
    else
        return (sock);
}


//*************************************************************************************************************

int DacqServer::dacq_disconnect_client (int sock, int id)
{
    int result = connect_disconnect(sock,-id);
    close_socket (sock,id);
    return (result);
}


//*************************************************************************************************************

void DacqServer::dacq_set_data_filter (int *kinds, int nkind)
{
    if (nkind <= 0) {
        free (filter_kinds);
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

void DacqServer::close_socket (int sock, int id)
{
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
    (void) dacq_release_shmem();
    //dacq_log ("Connection closed.\n");
    printf ("Connection closed.\r\n");
}


//*************************************************************************************************************

int DacqServer::connect_disconnect (int sock,int id)
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
        close_socket (sock,abs(id));
        return (FAIL);
    }
    else {
        fromlen = sizeof(from);
        rlen = recvfrom(sock, (void *)(&result), sizeof(int), 0,
              (sockaddr *)(&from), &fromlen);
        if (rlen == -1) {
            printf("recvfrom");//dacq_perror("recvfrom");
            close_socket (sock,abs(id));
            return (FAIL);
        }
        else
            return result;
    }
}


//*************************************************************************************************************

int DacqServer::interesting_data (int kind)
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

dacqShmBlock DacqServer::dacq_get_shmem(void)
{
  if (dacq_init_shmem() == -1)
    return(NULL);
  else
    return(shmptr);
}


//*************************************************************************************************************

int DacqServer::dacq_init_shmem(void)
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

int DacqServer::dacq_release_shmem(void)
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


//*************************************************************************************************************
//=============================================================================================================
// new client.c to qt functions
//=============================================================================================================

bool DacqServer::dacq_server_command(const QString& p_sCommand)
{

    if (m_pCollectorSock->state() != QAbstractSocket::ConnectedState)
        return false;

    //ToDo Command Check

    QByteArray t_arrCommand = p_sCommand.toLocal8Bit();

    if(t_arrCommand.size() > 0)
    {
        m_pCollectorSock->write(t_arrCommand);
        m_pCollectorSock->flush();
    }

    //ToDo check if command was succefull processed by the collector 
    m_pCollectorSock->waitForReadyRead(-1);
    m_pCollectorSock->readAll();//readAll that QTcpSocket is empty again
    

    return true;
}


//*************************************************************************************************************

bool DacqServer::dacq_server_login(const QString& p_sCollectorPass, const QString& p_sMyName)
{
    printf("Login... ");

    QString t_sCommand = QString("%1 %2\r\n").arg(DACQ_CMD_PASSWORD).arg(p_sCollectorPass);
    dacq_server_command(t_sCommand);
    
    t_sCommand = QString("%1 %2\r\n").arg(DACQ_CMD_NAME).arg(p_sMyName);
    dacq_server_command(t_sCommand);

    printf("[done]\r\n");

    return true;
}


//*************************************************************************************************************

bool DacqServer::dacq_server_send(QString& p_sDataSend, QByteArray& p_dataOut, int p_iInputFlag)
{
    if (m_pCollectorSock->state() != QAbstractSocket::ConnectedState)
        return false;

    QByteArray t_arrSend = p_sDataSend.toLocal8Bit();

    if(t_arrSend.size() > 0)
    {
        m_pCollectorSock->write(t_arrSend);
        m_pCollectorSock->flush();
    }
    
    m_pCollectorSock->waitForReadyRead(-1);
    if ( p_iInputFlag == DACQ_DRAIN_INPUT )
    {
        m_pCollectorSock->readAll();//readAll that QTcpSocket is empty again -> prevent overflow
    }
    else if( p_iInputFlag == DACQ_KEEP_INPUT ) 
    {
        p_dataOut = m_pCollectorSock->readAll();//readAll that QTcpSocket is empty again -> prevent overflow
    }

    return true;
}


//*************************************************************************************************************

bool DacqServer::dacq_server_start()
{
    if(!m_bIsMeasuring)
    {
        printf("Start measurement... ");

        QString t_sCommand = QString("%1\r\n").arg("meas");
        dacq_server_command(t_sCommand);

        m_bIsMeasuring = true;
        printf("[done]\r\n");
    }

    return true;
}


//*************************************************************************************************************

bool DacqServer::dacq_server_stop()
{
    if(m_bIsMeasuring)
    {
        printf("Stop measurement... ");

        QString t_sCommand = QString("%1\r\n").arg("stop");
        dacq_server_command(t_sCommand);

        m_bIsMeasuring = false;
        printf("[done]\r\n");
    }

    return true;
}
