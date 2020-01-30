//=============================================================================================================
/**
* @file     ftbuffclient.cpp
* @author   Gabriel B. Motta <gbmotta@mgh.harvard.edu
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*           Stefan Klanke
* @version  1.0
* @date     December, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Stefan Klanke and Gabriel B. Motta. All rights reserved.
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
* Based on viewer.cc example from ftbuffer reference implementation, under the GNU GENERAL PUBLIC LICENSE Version 2:
*
* Copyright (C) 2010, Stefan Klanke
* Donders Institute for Donders Institute for Brain, Cognition and Behaviour,
* Centre for Cognitive Neuroimaging, Radboud University Nijmegen,
* Kapittelweg 29, 6525 EN Nijmegen, The Netherlands
*
* @brief    FtBuffClient class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffclient.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTest>

//*************************************************************************************************************

FtBuffClient::FtBuffClient()
: m_iNumChannels(0)
, m_uiNumSamples(0)
, m_bnewData(false)
, m_pcAddrField("localhost:1972")
, m_bGotParams(false)
, m_bChunkData(false)
{
}

//*************************************************************************************************************

FtBuffClient::FtBuffClient(char* addr)
: m_pcAddrField(addr)
, m_iNumChannels(0)
, m_uiNumSamples(0)
, m_bnewData(false)
, m_bGotParams(false)
, m_bChunkData(false)
{
}

//*************************************************************************************************************

void FtBuffClient::getDataExample() {

    while(true) {

        //Starts connection with m_ftCon_Connector
        while(!this->isConnected()) {
            this->startConnection();
            QTest::qSleep(2000);
        }

        //Handle requests, responses, incoming data after connection is made
        this->idleCall();
        QTest::qSleep(10);
    }
}

//*************************************************************************************************************

bool FtBuffClient::readHeader() {

    //c++ wrapper classes for ftbuffer implementation
    //handles connections, requests, and storage of incoming data
    qDebug() << "Creating request/response handlers...";
    SimpleStorage chunkBuffer;
    headerdef_t header_def;
    FtBufferRequest request;
    FtBufferResponse response;


    //set request command to GET_HDR, other member variables to approprit values
    qDebug() << "Preparing header...";
    request.prepGetHeader();

    qDebug() << "Attempting TCP connection...";
    //Attempt to establish TCP connection
    if (tcprequest(m_ftCon_Connector.getSocket(), request.out(), response.in()) < 0) {
        qDebug() << "Error in communication - check buffer server";
        m_ftCon_Connector.disconnect();
        m_iNumChannels = 0;
        return false;
    }

    //Attempt to retrieve and read header and chunks if applicable
    qDebug() << "Attempting to retrieve header...";
    if (!response.checkGetHeader(header_def, &chunkBuffer)) {
        qDebug() << "Could not read header.";
        return false;
    }

    //Updating channel and sample info
    m_iNumChannels = header_def.nchans;
    m_uiNumSamples = header_def.nsamples;
    m_iSampleFrequency = header_def.fsample;

    m_bGotParams = true;

    qDebug() << "***   Header Data   ***";
    qDebug() << "Channels:" << header_def.nchans;
    qDebug() << "Buffer size:" << header_def.bufsize;
    qDebug() << "Fsample:" << header_def.fsample;
    qDebug() << "NEvents:" << header_def.nevents;
    qDebug() << "NSamples" << header_def.nsamples;
    qDebug() << "Data Type:" << header_def.data_type;
    qDebug() << "***********************";

    //saving header chunks and updating extended header flag
    if (chunkBuffer.size() != 0) {
//        m_ssChunkData.reset(&chunkBuffer);

//        qDebug() << "";
//        qDebug() << "";
//        qDebug() << "Chunk Buffer size:" << chunkBuffer.size();
//        qDebug() << "";
//        qDebug() << "";

//        const ft_chunk_t* chanNames = find_chunk(chunkBuffer.data(), 0, chunkBuffer.size(), FT_CHUNK_CHANNEL_NAMES);
//        const ft_chunk_t* neuromagHead = find_chunk(chunkBuffer.data(), 0, chunkBuffer.size(), FT_CHUNK_NEUROMAG_HEADER);

//        if (chanNames != Q_NULLPTR) {
//            qDebug() << "Channel name chunk found, size" << chanNames->def.size;
//        }

//        if (neuromagHead != Q_NULLPTR) {
//            qDebug () << "Neuromag header found, size" << neuromagHead->def.size;
//        }

        m_bChunkData = true;

//        qDebug() << "";
//        qDebug() << "Set m_bChunkData to true";
//        qDebug() << "";
        qDebug() << "Extended Buffer";
        const ft_chunk_t* chanNames = find_chunk(chunkBuffer.data(), 0, chunkBuffer.size(), FT_CHUNK_CHANNEL_NAMES);
        //const ft_chunk_t* neuromagHead = find_chunk(chunkBuffer.data(), 0, chunkBuffer.size(), FT_CHUNK_NEUROMAG_HEADER);
    //    const ft_chunk_t* neuromagIso = find_chunk(chunkData.data(), 0, chunkData.size(), FT_CHUNK_NEUROMAG_ISOTRAK);
    //    const ft_chunk_t* neuromagHPI = find_chunk(chunkData.data(), 0, chunkData.size(), FT_CHUNK_NEUROMAG_HPIRESULT);
        qDebug() << "parsed header";
        bData_CHANNEL_NAMES.open(QIODevice::ReadWrite);
        bData_CHANNEL_NAMES.write(chanNames->data, chanNames->def.size);
        bData_CHANNEL_NAMES.close();
        qDebug() << "created buffer";
    }

    return true;
}

//*************************************************************************************************************

template<typename T>
void FtBuffClient::convertToFloat(float *dest, const void *src, unsigned int nsamp, unsigned int nchans) {
    const T *srcT = static_cast<const T *>(src);
    for (unsigned int j=0;j<nsamp;j++) {
        for (unsigned int i=0;i<nchans;i++) {
            dest[i] = (float) srcT[i];
        }
        dest += nchans;
        srcT += nchans;
    }
}

//*************************************************************************************************************

bool FtBuffClient::stopConnection() {
    if (m_ftCon_Connector.isOpen()) {
        qDebug() << "Disconnecting...";
        m_ftCon_Connector.disconnect();
        qDebug() << "Disconnected.";
        return true;
    } else {
        qDebug() << "Not currently connected";
        return false;
    }
}

//*************************************************************************************************************

bool FtBuffClient::startConnection() {
    if (!m_ftCon_Connector.isOpen()) {
        qDebug() << "Trying to connect...";
        if (m_ftCon_Connector.connect(m_pcAddrField)){
            qDebug() << "Connected to" << m_pcAddrField;
            return true;
        } else {
            qDebug() << "Unable to connect: no buffer found on" << m_pcAddrField;
            return false;
        }
    } else {
        qDebug() << "Unable to connect: Already connected";
        return false;
    }
}

//*************************************************************************************************************

void FtBuffClient::idleCall() {

    //creates handlesrs for buffer messages and events
    datadef_t ddef;
    FtBufferRequest request;
    FtBufferResponse response;
    unsigned int newSamples, newEvents;

    //Checks if connection is open
    if (!m_ftCon_Connector.isOpen()) return;

    //If no header is read, wait before returning to avoid spamming the buffer
    if (m_iNumChannels == 0) {
        if (!readHeader()) {
            QTest::qSleep(50);
            return;
        }
    }

    //Set params for waiting for data. 40ms of wait
    request.prepWaitData(m_uiNumSamples, 0xFFFFFFFF, 40);

    if (tcprequest(m_ftCon_Connector.getSocket(), request.out(), response.in()) < 0) {
        m_ftCon_Connector.disconnect();
        qDebug() << "Error in communication. Buffer server aborted??";
        return;
    }
    if (!response.checkWait(newSamples, newEvents)) {
        m_ftCon_Connector.disconnect();
        qDebug() << "Error in received packet - disconnecting...";
        return;
    }

    if (newSamples == m_uiNumSamples) {
        //qDebug() << "idleCall - No new data";
        return; // nothing new
    }
    if (newSamples < m_uiNumSamples) {
        // oops ? do we have a new header?
        if (!readHeader()) {
            qDebug() << "idleCall - Unable to read header.";
            return;
        }
        if (m_uiNumSamples == 0) {
            qDebug() << "idelCall - No Data";
            return; // no data yet
        }
        if (m_uiNumSamples > 1024 || m_iNumChannels > 512) {
            // "lots" of data already in the buffer
            // -> don't do anything with that data
            //    continue next idleCall
            qDebug() << "idleCall - Waiting for next function call";
            return;
        }
        // read data from the start of the buffer up to newSamples right away
        newSamples = m_uiNumSamples;
        m_uiNumSamples = 0;
    }

    request.prepGetData(m_uiNumSamples, newSamples-1);

    if (tcprequest(m_ftCon_Connector.getSocket(), request.out(), response.in()) < 0) {
        m_ftCon_Connector.disconnect();
        qDebug() << "Error in communication. Buffer server aborted??";
        return;
    }
    if (!response.checkGetData(ddef, &m_ssRawStore)) {
        m_ftCon_Connector.disconnect();
        qDebug() << "Error in received packet - disconnecting...";
        return;
    }

    m_ssFloatStore.resize(sizeof(float) * ddef.nsamples * ddef.nchans);

    float *fdata = (float *) m_ssFloatStore.data();

    switch(ddef.data_type) {
        case DATATYPE_UINT8:
            convertToFloat<uint8_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT8:
            convertToFloat<int8_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT16:
            convertToFloat<uint16_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT16:
            convertToFloat<int16_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT32:
            convertToFloat<uint32_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT32:
            convertToFloat<int32_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT64:
            convertToFloat<uint64_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT64:
            convertToFloat<int64_t>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_FLOAT32:
            convertToFloat<float>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_FLOAT64:
            convertToFloat<double>(fdata, m_ssRawStore.data(), ddef.nsamples, ddef.nchans);
            break;
    }

    Eigen::MatrixXf matData;

    matData.resize(m_iNumChannels, ddef.nsamples);

    int count = 0;
    for (int i = 0; i < int (ddef.nsamples); i++) {
        for (int j = 0; j < int (ddef.nchans); j++) {
            matData(j,i) = fdata[count];
            //if (count % 32 == 0) qDebug() << "---Blockstart---" << count/32;
            //qDebug() << fdata[count];
            count++;
        }
    }
    qDebug() << "matData" << matData.size();

    m_pMatEmit = new Eigen::MatrixXd(matData.cast<double>());
    m_bnewData = true;

    m_uiNumSamples = newSamples;

}

//*************************************************************************************************************

bool FtBuffClient::isConnected() {
    return m_ftCon_Connector.isOpen();
}

//*************************************************************************************************************

QString FtBuffClient::getAddress() {
    return QString(m_pcAddrField); //converts char* to QString
}

//*************************************************************************************************************

void FtBuffClient::getData() {
    //startConnection();
    idleCall();
}

//*************************************************************************************************************

void FtBuffClient::reset(){
    m_bnewData = false;
    delete m_pMatEmit;
    m_pMatEmit = Q_NULLPTR;
}

//*************************************************************************************************************

bool FtBuffClient::newData() {
    if (m_bnewData) qDebug() << "New data found in buffer";
    return m_bnewData;
}

//*************************************************************************************************************

Eigen::MatrixXd FtBuffClient::dataMat() {
    return *m_pMatEmit;
}

//*************************************************************************************************************

QBuffer* FtBuffClient::chunkData() {
    //return *(m_ssChunkData.data());
    return &bData_CHANNEL_NAMES;
}

//*************************************************************************************************************

bool FtBuffClient::extendedHeader() {
    if (m_bChunkData) {
        m_bChunkData = false;
        m_bGotParams = false;
        return true;
    } else {
        return m_bChunkData;
    }
}

//*************************************************************************************************************

bool FtBuffClient::regularHeader() {
    if (m_bGotParams) {
         m_bGotParams = false;
         m_bChunkData = false;
        return true;
    } else {
        return m_bGotParams;
    }
}

//*************************************************************************************************************

QPair<int,float> FtBuffClient::getChannelAndFrequency() {
    QPair<int,float> val;
    val.first = m_iNumChannels;
    val.second = m_iSampleFrequency;
    return val;
}

//*************************************************************************************************************

//
// Old idleCall code
//
/*if(m_iMatDataSampleIterator+matData.cols() <= m_matData.cols()) {
    m_matData.block(0, m_iMatDataSampleIterator, matData.rows(), matData.cols()) = matData.cast<double>();

    m_iMatDataSampleIterator += matData.cols();
} else {
    m_matData.block(0, m_iMatDataSampleIterator, matData.rows(), m_matData.cols()-m_iMatDataSampleIterator) = matData.block(0, 0, matData.rows(), m_matData.cols()-m_iMatDataSampleIterator).cast<double>();

    m_iMatDataSampleIterator = 0;
}

//qDebug() << "m_iMatDataSampleIterator" << m_iMatDataSampleIterator;

if(m_iMatDataSampleIterator == m_matData.cols()) {
    m_iMatDataSampleIterator = 0;
    //qDebug()<<"Emit data";
    matEmit = new Eigen::MatrixXd(m_matData.cast<double>());
    m_bnewData = true;
    qDebug() << "";
    qDebug() << matEmit->size();
    qDebug() << "";
}
*/
