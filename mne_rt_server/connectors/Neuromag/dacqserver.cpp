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
#include "collectorsocket.h"
#include "shmemsocket.h"
#include "../../../MNE/fiff/fiff_constants.h"
#include "../../../MNE/fiff/fiff_stream.h"


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

DacqServer::DacqServer(Neuromag* p_pNeuromag, QObject * parent)
: QThread(parent)
, m_pNeuromag(p_pNeuromag)
, m_pCollectorSock(NULL)
, m_pShmemSock(NULL)
, m_bIsRunning(false)
, m_bMeasInfoRequest(true)
, m_bMeasRequest(true)
, m_bMeasStopRequest(false)
, m_bSetBuffersizeRequest(false)
{

    
}


//*************************************************************************************************************

DacqServer::~DacqServer()
{
    if(m_pCollectorSock)
        delete m_pCollectorSock;
    if(m_pShmemSock)
        delete m_pShmemSock;
}


//*************************************************************************************************************

bool DacqServer::getMeasInfo(FiffInfo*& p_pFiffInfo)
{

    if (p_pFiffInfo)
        delete p_pFiffInfo;
    p_pFiffInfo = new FiffInfo();

    m_pCollectorSock->server_stop();
    m_pCollectorSock->server_start();

    FiffTag* t_pTag = NULL;
    bool t_bReadHeader = true;

    while(t_bReadHeader)
    {
        if (m_pShmemSock->receive_tag(t_pTag) == -1)
            break;
        //
        // Projector Item
        //
        if( t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM )
        {
            int nvec = -1;
            int nchan = -1;
            QStringList defaultList;
            QStringList names;
            MatrixXd* data = NULL;
            
            fiff_int_t kind;
            bool active;
            QString desc; // maybe, in some cases this has to be a struct.

            //
            while(!(t_pTag->kind == FIFF_BLOCK_END && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM))
            {
                m_pShmemSock->receive_tag(t_pTag);
                
                switch(t_pTag->kind)
                {
                    case FIFF_NCHAN:
                        nchan = *(t_pTag->toInt());
                        qDebug() << "Parse FIFF_NCHAN";
                        break;
                    case FIFF_PROJ_ITEM_CH_NAME_LIST:
                        names = FiffStream::split_name_list(t_pTag->toString());
                        qDebug() << "Parse FIFF_PROJ_ITEM_CH_NAME_LIST ";
                        break;
                    case FIFF_NAME:
                        desc = t_pTag->toString();
                        qDebug() << "Parse FIFF_NAME ";
                        break;
                    case FIFF_PROJ_ITEM_KIND:
                        kind = *(t_pTag->toInt());
                        qDebug() << "Parse FIFF_PROJ_ITEM_KIND ";
                        break;
                    case FIFF_PROJ_ITEM_TIME:
                        qDebug() << "Parse FIFF_PROJ_ITEM_TIME " << *(t_pTag->toFloat());
                        break;
                    case FIFF_PROJ_ITEM_NVEC:
                        nvec == *(t_pTag->toInt());
                        qDebug() << "Parse FIFF_PROJ_ITEM_NVEC ";
                        break;        
                    case FIFF_MNE_PROJ_ITEM_ACTIVE:
                        active == *(t_pTag->toInt());
                        qDebug() << "Parse FIFF_MNE_PROJ_ITEM_ACTIVE ";
                        break;
                    case FIFF_PROJ_ITEM_VECTORS:
                        data = t_pTag->toFloatMatrix();
                        data->transposeInPlace();
                        qDebug() << "Parse FIFF_PROJ_ITEM_VECTORS ";
                        break;
                }
            }
            
            FiffNamedMatrix* t_fiffNamedMatrix = new FiffNamedMatrix(nvec, nchan, defaultList, names, data);
            
            FiffProj* one = new FiffProj(kind, active, desc, t_fiffNamedMatrix);
            
            p_pFiffInfo->projs.append(one);
        }


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
                qDebug() << "Processed FIFFB_PROCESSED_DATA";
                p_pFiffInfo->meas_id = t_pTag->toFiffID();
                break;
            case FIFF_MEAS_DATE:
                qDebug() << "Processed FIFF_MEAS_DATE";
                p_pFiffInfo->meas_date[0] = t_pTag->toInt()[0];
                p_pFiffInfo->meas_date[1] = t_pTag->toInt()[1];
                break;
            case FIFF_NCHAN:
                qDebug() << "Processed FIFF_NCHAN";
                p_pFiffInfo->nchan = *(t_pTag->toInt());
                break;
            case FIFF_SFREQ:
                qDebug() << "Processed FIFF_SFREQ";
                p_pFiffInfo->sfreq = *(t_pTag->toFloat());
                break;
            case FIFF_LOWPASS:
                qDebug() << "Processed FIFF_LOWPASS";
                p_pFiffInfo->lowpass = *(t_pTag->toFloat());
                break;
            case FIFF_HIGHPASS:
                qDebug() << "Processed FIFF_HIGHPASS";
                p_pFiffInfo->highpass = *(t_pTag->toFloat());
                break;
            case FIFF_LINE_FREQ:
                qDebug() << "FIFF_LINE_FREQ " << *(t_pTag->toFloat());
                break;
            case FIFF_UNIT_AM:
                qDebug() << "FIFF_UNIT_AM " << *(t_pTag->toInt());
                break;
            case FIFF_CH_INFO:
                qDebug() << "Processed FIFF_CH_INFO";
                p_pFiffInfo->chs.append( t_pTag->toChInfo() );
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

    for(qint32 i = 0; i < p_pFiffInfo->chs.size(); ++i)
        p_pFiffInfo->ch_names.append(p_pFiffInfo->chs[i].ch_name);


    if (!m_bMeasRequest)
        m_pCollectorSock->server_stop();
        
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


    if(m_pCollectorSock)
        delete m_pCollectorSock;
    m_pCollectorSock = new CollectorSocket();
    
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
        if (!m_pCollectorSock->open()) {
            printf("Cannot change the Neuromag buffer length: Could not open collector connection\n");//dacq_log("Cannot change the Neuromag buffer length: Could not open collector connection\n");
            return;
        }
        printf("Changing the Neuromag buffer length to %d... ", m_pNeuromag->getBufferSampleSize());//dacq_log("Changing the Neuromag buffer length to %d\n", newMaxBuflen);
        if (m_pCollectorSock->setMaxBuflen(m_pNeuromag->getBufferSampleSize())) {
            printf("Setting a new Neuromag buffer length failed\r\n");//dacq_log("Setting a new Neuromag buffer length failed\n");
            m_pCollectorSock->close();
            return;
        }
        printf("[done]\r\n");        
    }
////////////

    if(m_pShmemSock)
        delete m_pShmemSock;
    m_pShmemSock = new ShmemSocket();

    m_pShmemSock->set_data_filter (NULL, 0);

    /* Connect to the Elekta Neuromag shared memory system */
    if (!m_pShmemSock->connect_client()) {
        printf("Could not connect!\r\n");//dacq_log("Could not connect!\n");
        return;//(2);
    }

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
//    m_pCollectorSock->server_start();
    
    //
    // Receive shmem tags
    //
    
    qint32 nchan = -1;

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
            if (m_pShmemSock->receive_tag(t_pTag) == -1)
                break;
        }

        qDebug() << count;
//#if defined(DACQ_OLD_CONNECTION_SCHEME)
//        if (dacq_client_receive_tag(t_pTag) == -1)        
//#else
//        if (dacq_client_receive_tag(&m_iShmemSock, m_iShmemId) == -1)
//#endif
//            break;
            


        if (nchan > 0 && m_pNeuromag->m_pInfo)
        {
            nchan = m_pNeuromag->m_pInfo->nchan;
        }


        if (t_pTag->kind == FIFF_DATA_BUFFER && nchan > 0)
        {
//            MatrixXd* p_pMatrix = new MatrixXd((Map<MatrixXf>( (float*)this->data(),pDims[0], pDims[1])).cast<double>());

            Map<MatrixXf> tmp( (float*)t_pTag->data(), nchan, m_pNeuromag->getBufferSampleSize());

            std::cout << "Matrix Xf " << tmp.block(0,0,1,10);

//            m_pNeuromag->m_pRawMatrixBuffer->push(&tmp);
        }
        else if(t_pTag->kind == FIFF_ERROR_MESSAGE)
        {
            printf("Error: %s\r\n", t_pTag->data());
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
    m_pCollectorSock->server_stop();
    
    m_pShmemSock->disconnect_client();
    m_pCollectorSock->close();
    
    delete t_pTag;
    delete m_pCollectorSock;
    m_pCollectorSock = NULL;

    printf("\r\n");
}
