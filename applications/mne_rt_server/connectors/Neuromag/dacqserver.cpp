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
* @brief     Defintion of the DacqServer class.
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

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGRTSERVERPLUGIN;
using namespace IOBUFFER;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DacqServer::DacqServer(Neuromag* p_pNeuromag, QObject * parent)
: QThread(parent)
, m_bIsRunning(false)
, m_bMeasInfoRequest(true)
, m_bMeasRequest(true)
, m_bMeasStopRequest(false)
, m_bSetBuffersizeRequest(false)
, m_pNeuromag(p_pNeuromag)
{
}


//*************************************************************************************************************

DacqServer::~DacqServer()
{
}


//*************************************************************************************************************

bool DacqServer::getMeasInfo(FiffInfo& p_fiffInfo)
{
    p_fiffInfo.clear();

#ifdef DACQ_AUTOSTART
    m_pCollectorSock->server_stop();
    m_pCollectorSock->server_start();
#endif

    FiffTag::SPtr t_pTag;
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
            printf("\tProjector... ");
            
            int nvec = -1;
            int nchan = -1;
            QStringList defaultList;
            QStringList names;
            MatrixXd data;
            
            fiff_int_t kind;
            bool active = false;
            QString desc; // maybe, in some cases this has to be a struct.

            //
            while(!(t_pTag->kind == FIFF_BLOCK_END && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM))
            {
                m_pShmemSock->receive_tag(t_pTag);
                
                switch(t_pTag->kind)
                {
                    case FIFF_NCHAN:
                        nchan = *(t_pTag->toInt());
                        break;
                    case FIFF_PROJ_ITEM_CH_NAME_LIST:
                        names = FiffStream::split_name_list(t_pTag->toString());
                        break;
                    case FIFF_NAME:
                        desc = t_pTag->toString();
                        printf("%s... ", desc.toUtf8().constData());
                        break;
                    case FIFF_PROJ_ITEM_KIND:
                        kind = *(t_pTag->toInt());
                        break;
//                    case FIFF_PROJ_ITEM_TIME:
//                        qDebug() << "FIFF_PROJ_ITEM_TIME";
//                        break;
                    case FIFF_PROJ_ITEM_NVEC:
                        nvec == *(t_pTag->toInt());
                        break;        
                    case FIFF_MNE_PROJ_ITEM_ACTIVE:
                        active == *(t_pTag->toInt());
                        break;
                    case FIFF_PROJ_ITEM_VECTORS:
                        data = t_pTag->toFloatMatrix().cast<double>();
                        data.transposeInPlace();
                        break;
                }
            }
            
            FiffNamedMatrix t_fiffNamedMatrix(nvec, nchan, defaultList, names, data);
            
            FiffProj one(kind, active, desc, t_fiffNamedMatrix);
            
            p_fiffInfo.projs.append(one);
            
            printf("[done]\r\n");   
        }

        switch(t_pTag->kind)
        {
            case FIFF_BLOCK_START:
                if(*(t_pTag->toInt()) == FIFFB_MEAS_INFO)
                    printf("Reading measurement info... \r\n");
                break;
            case FIFFB_PROCESSED_DATA:
                printf("Measurement ID... ");
                p_fiffInfo.meas_id = t_pTag->toFiffID();
                printf("[done]\r\n");  
                break;
            case FIFF_MEAS_DATE:
                printf("\tMeasurement date... ");
                p_fiffInfo.meas_date[0] = t_pTag->toInt()[0];
                p_fiffInfo.meas_date[1] = t_pTag->toInt()[1];
                printf("[done]\r\n"); 
                break;
            case FIFF_NCHAN:
                printf("\tNumber of channels... ");
                p_fiffInfo.nchan = *(t_pTag->toInt());
                printf("%d... [done]\r\n", p_fiffInfo.nchan);
                break;
            case FIFF_SFREQ:
                printf("\tSampling frequency... ");
                p_fiffInfo.sfreq = *(t_pTag->toFloat());
                printf("%f... [done]\r\n", p_fiffInfo.sfreq);
                break;
            case FIFF_LOWPASS:
                printf("\tLowpass frequency... ");
                p_fiffInfo.lowpass = *(t_pTag->toFloat());
                printf("%f Hz... [done]\r\n", p_fiffInfo.lowpass);
                break;
            case FIFF_HIGHPASS:
                printf("\tHighpass frequency... ");
                p_fiffInfo.highpass = *(t_pTag->toFloat());
                printf("%f Hz... [done]\r\n", p_fiffInfo.highpass);
                break;
//            case FIFF_LINE_FREQ:
//                qDebug() << "FIFF_LINE_FREQ " << *(t_pTag->toFloat());
//                break;
//            case FIFF_UNIT_AM:
//                qDebug() << "FIFF_UNIT_AM " << *(t_pTag->toInt());
//                break;
            case FIFF_CH_INFO:
//                qDebug() << "Processed FIFF_CH_INFO";
                p_fiffInfo.chs.append( t_pTag->toChInfo() );
                break;
            case FIFF_BLOCK_END:
                switch(*(t_pTag->toInt()))
                {
                    case FIFFB_MEAS_INFO:
                        t_bReadHeader = false;
                        break;
                }
                break;
//            case FIFF_HPI_NCOIL:
//                qDebug() << "FIFF_HPI_NCOIL " << *(t_pTag->toInt());
//                break;
//            case FIFFV_RESP_CH:
//                qDebug() << "FIFFV_RESP_CH " << t_pTag->toString();
//                break;
//            default:
//                qDebug() << "Unknown Tag Kind: " << t_pTag->kind << " Type: " << t_pTag->type << "Size: " << t_pTag->size();
        }
    }

    printf("\tProcessing channels... ");
    for(qint32 i = 0; i < p_fiffInfo.chs.size(); ++i)
        p_fiffInfo.ch_names.append(p_fiffInfo.chs[i].ch_name);

    printf("[done]\r\n %d", p_fiffInfo.chs.size());
    
    printf("measurement info read.\r\n");   

#ifdef DACQ_AUTOSTART
    if (!m_bMeasRequest)
        m_pCollectorSock->server_stop();
#endif
        
    //t_bReadHeader is still true --> that means a break occured
    if (t_bReadHeader)
        return false;
        
    return true;
}


//*************************************************************************************************************

void DacqServer::run()
{
    m_bIsRunning = true;

    connect(this, &DacqServer::measInfoAvailable,
            m_pNeuromag.data(), &Neuromag::releaseMeasInfo);

    m_pCollectorSock = QSharedPointer<CollectorSocket>(new CollectorSocket);

    // Make sure the buffer size is at least as big as the minimal buffer size
    if(m_pNeuromag->m_uiBufferSampleSize < MIN_BUFLEN)
        m_pNeuromag->m_uiBufferSampleSize = MIN_BUFLEN;

    int  t_iOriginalMaxBuflen = 1500;// set the standard size as long as we can't get it without setting it
    // this doesn't work without reseting it
    //        t_iOriginalMaxBuflen = collector_getMaxBuflen();
    //        if (t_iOriginalMaxBuflen < 1) {
    //            printf("Could not query the current Neuromag buffer length\r\n");//dacq_log("Could not query the current Neuromag buffer length\n");
    //            collector_close();
    //            return;
    //        }
    Q_UNUSED(t_iOriginalMaxBuflen);

    if (m_pNeuromag->m_uiBufferSampleSize < MIN_BUFLEN) {
        fprintf(stderr, "%ui: Too small Neuromag buffer length requested, should be at least %d\n", m_pNeuromag->m_uiBufferSampleSize, MIN_BUFLEN);
        return;
    }
    else {
        /* Connect to the Elekta Neuromag acquisition control server, change the buffer length and exit*/
        if (!m_pCollectorSock->open()) {
            printf("Cannot change the Neuromag buffer length: Could not open collector connection\n");//dacq_log("Cannot change the Neuromag buffer length: Could not open collector connection\n");
            return;
        }
        printf("Changing the Neuromag buffer length to %d... ", m_pNeuromag->m_uiBufferSampleSize);//dacq_log("Changing the Neuromag buffer length to %d\n", newMaxBuflen);
        if (m_pCollectorSock->setMaxBuflen(m_pNeuromag->m_uiBufferSampleSize)) {
            printf("Setting a new Neuromag buffer length failed\r\n");//dacq_log("Setting a new Neuromag buffer length failed\n");
            m_pCollectorSock->close();
            return;
        }
        printf("[done]\r\n");
    }

    m_pShmemSock = QSharedPointer<ShmemSocket>(new ShmemSocket);

    m_pShmemSock->set_data_filter (NULL, 0);

    /* Connect to the Elekta Neuromag shared memory system */
    if (!m_pShmemSock->connect_client()) {
        printf("Could not connect!\r\n");//dacq_log("Could not connect!\n");
        return;//(2);
    }


    // Mainloop
//    printf("Will scale up MEG mags by %g, grads by %g and EEG data by %g\n",
//         meg_mag_multiplier, meg_grad_multiplier, eeg_multiplier);
    printf("Waiting for the measurement to start...\n");

    // Receive shmem tags
    qint32 nchan = -1;
    float sfreq = -1.0f;

    FiffTag::SPtr t_pTag;
    
    qint32 t_nSamples = 0;
    qint32 t_nSamplesNew = 0;

    // Requesting new header info: read it every time a measurement starts or a measurement info is requested
    if(m_pNeuromag->m_info.isEmpty() || m_bMeasInfoRequest)
    {
        m_pNeuromag->mutex.lock();
        if(getMeasInfo(m_pNeuromag->m_info))
        {
            if(m_bMeasInfoRequest)
            {
                emit measInfoAvailable();
                m_bMeasInfoRequest = false;
            }

            // Create buffer
            if(!m_pNeuromag->m_info.isEmpty())
                m_pNeuromag->m_pRawMatrixBuffer = RawMatrixBuffer::SPtr(new RawMatrixBuffer(RAW_BUFFFER_SIZE, m_pNeuromag->m_info.nchan, m_pNeuromag->m_uiBufferSampleSize));
        }
        else
            m_bIsRunning = false;
        m_pNeuromag->mutex.unlock();

        //Stop if only meas info is requested
        if(!m_pNeuromag->m_bIsRunning)
            this->wait();
    }

    // Control measurement start through Neuromag connector. ToDo: in Case Realtime measurement should be performed during normal acqusition process, change this!!
#ifdef DACQ_AUTOSTART
    if(m_bMeasRequest)
        m_pCollectorSock->server_start();
#endif

    while(m_bIsRunning)
    {
        if(m_bMeasRequest)
        {
            if (m_pShmemSock->receive_tag(t_pTag) == -1)
                break;
        }
        else
        {
            // break while loop when no measurement request
            break;
        }

        if (nchan < 0 && !m_pNeuromag->m_info.isEmpty())
        {
            nchan = m_pNeuromag->m_info.nchan;
            sfreq = m_pNeuromag->m_info.sfreq;
        }


        switch(t_pTag->kind)
        {
            case FIFF_DATA_BUFFER:
                if(nchan > 0)
                {
                    t_nSamplesNew = t_nSamples + m_pNeuromag->m_uiBufferSampleSize - 1;
                    printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", t_nSamples, t_nSamplesNew, ((float)t_nSamples) / sfreq, ((float)t_nSamplesNew) / sfreq );
                    t_nSamples += m_pNeuromag->m_uiBufferSampleSize;

                    float a;
                    float meg_mag_multiplier = 1.0;
                    float meg_grad_multiplier = 1.0;
                    float eeg_multiplier = 1.0;

                    MatrixXf* t_pMatrix = new MatrixXf(nchan, m_pNeuromag->m_uiBufferSampleSize);

                    fiff_int_t *data32 = (fiff_int_t *)t_pTag->data();
                    for (qint32 ch = 0; ch < nchan; ch++) {
                        switch(m_pNeuromag->m_info.chs[ch].kind) {
                            case FIFFV_MAGN_CH:
                                if (m_pNeuromag->m_info.chs[ch].unit == FIFF_UNIT_T_M)
                                    a = meg_grad_multiplier;
                                else
                                    a = meg_mag_multiplier;
                                break;
                            case FIFFV_EL_CH:
                                a = eeg_multiplier;
                                break;
                            default:
                                a = 1.0;
                        }
                        for (qint32 ns = 0; ns < m_pNeuromag->m_uiBufferSampleSize; ns++)
                            (*t_pMatrix)(ch,ns) = a * m_pNeuromag->m_info.chs[ch].cal * m_pNeuromag->m_info.chs[ch].range * data32[nchan*ns+ch];
                    }

                    m_pNeuromag->m_pRawMatrixBuffer->push(t_pMatrix);

                    delete t_pMatrix;
/*                    
                    MatrixXf* t_pMatrix = new MatrixXf( (Map<MatrixXi>( (int*) t_pTag->data(), nchan, m_pNeuromag->m_uiBufferSampleSize)).cast<float>());
//                    std::cout << "Matrix Xf " << t_pMatrix->block(0,0,1,4);
                    m_pNeuromag->m_pRawMatrixBuffer->push(t_pMatrix);
                    delete t_pMatrix;
*/
                    printf(" [done]\r\n");
                }
                break;
            case FIFF_BLOCK_START:
//                qDebug() << "FIFF_BLOCK_START";
                switch(*(t_pTag->toInt()))
                {
                    case FIFFB_RAW_DATA:
                        printf("Processing raw data...\r\n");
                        break;
//                    default:
//                        qDebug() << "    Unknown " << *(t_pTag->toInt());
                }
                break;
            case FIFF_ERROR_MESSAGE:
                printf("Error: %s\r\n", t_pTag->data());
                m_bIsRunning = false;
                break;
            case FIFF_CLOSE_FILE:
                printf("Measurement stopped.\r\n");
                break;
            default:
                printf("Unknow tag; Kind: %d, Type: %d, Size: %d \r\n", t_pTag->kind, t_pTag->type, t_pTag->size());
        }
    }

    // Stop and clean up
#ifdef DACQ_AUTOSTART
    m_pCollectorSock->server_stop();
#endif
    
    m_pShmemSock->disconnect_client();
    m_pCollectorSock->close();

    printf("\r\n");
}
