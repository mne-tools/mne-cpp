//=============================================================================================================
/**
 * @file     ftconnector.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the definition of the FtConnector class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftconnector.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QtEndian>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtConnector::FtConnector()
:m_iNumSamples(0)
,m_iNumNewSamples(0)
,m_iNumChannels(0)
,m_bNewData(false)
,m_fSampleFreq(0)
,m_pSocket(Q_NULLPTR)
{
    qInfo() << "[FtConnector::FtConnector] Object created.";
}

//=============================================================================================================

FtConnector::~FtConnector()
{
    //disconnect();
    delete m_pSocket;
    //m_pSocket = Q_NULLPTR;
}

//=============================================================================================================

bool FtConnector::connect()
{
    if(m_pSocket != Q_NULLPTR) {
        delete m_pSocket;
        m_pSocket = Q_NULLPTR;
    }

    m_pSocket = new QTcpSocket();
    m_pSocket->connectToHost(QHostAddress(m_sAddress), m_iPort);
    qint8 iTries = 0;

    //wait for connect max 5 tries, also windows safeguard
    while (m_pSocket->state() != QAbstractSocket::ConnectedState) {
        m_pSocket->waitForConnected(200);
        iTries ++;
        if (iTries > 5) {
            break;
        }
    }

    if(m_pSocket->state() == QAbstractSocket::ConnectedState) {
        qInfo() << "[FtConnector::connect] Connected!";
        return true;
    } else {
        qWarning() << "[FtConnector::connect] Timed out: Failed to connect.";
        delete m_pSocket;
        m_pSocket = Q_NULLPTR;
        return false;
    }
}

//=============================================================================================================

bool FtConnector::getHeader()
{
    qInfo() << "[FtConnector::getHeader] Attepting to get header...";

    m_pSocket->readAll(); //Ensure receiving buffer is empty

    // Defining parameters to send a get header message to buffer
    messagedef_t messagedef;
    messagedef.bufsize = 0;
    messagedef.command = GET_HDR;

    // Send request to buffer
    sendRequest(messagedef);

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < sizeof (messagedef_t)) {
        m_pSocket->waitForReadyRead(10);
    }

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    int bufsize = parseMessageDef(msgBuffer);

    if (bufsize == 0) {
        qInfo() << "[FtConnector::getHeader] No header data found";
        return false;
    }

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < bufsize) {
        m_pSocket->waitForReadyRead(10);
    }

    //Parse header info from buffer
    QBuffer hdrBuffer;
    prepBuffer(hdrBuffer, sizeof (headerdef_t)); // if implementing header chunks: change from sizeof (headerdef) to bufsize
    parseHeaderDef(hdrBuffer);

    return true;
}

//=============================================================================================================

bool FtConnector::parseHeaderDef(QBuffer &readBuffer)
{

    //Start parsing header parameters
    qInfo() << "[FtConnector::parseHeaderDef] Got header data. Parsing...";
    headerdef_t headerdef;

    //Get nchans, int32
    char c_chans[sizeof(headerdef.nchans)];
    readBuffer.read(c_chans, sizeof(headerdef.nchans));
    std::memcpy(&headerdef.nchans, c_chans, sizeof(headerdef.nchans));

    //Get nsamples, int32
    char c_samples[sizeof(headerdef.nsamples)];
    readBuffer.read(c_samples, sizeof(headerdef.nsamples));
    std::memcpy(&headerdef.nsamples, c_samples, sizeof(headerdef.nsamples));

    //Get nevents, int32
    char c_events[sizeof(headerdef.nevents)];
    readBuffer.read(c_events, sizeof(headerdef.nevents));
    std::memcpy(&headerdef.nevents, c_events, sizeof(headerdef.nevents));

    //Get fsample, float
    char c_freqsamp[sizeof(headerdef.fsample)];
    readBuffer.read(c_freqsamp, sizeof(headerdef.fsample));
    std::memcpy(&headerdef.fsample, c_freqsamp, sizeof(headerdef.fsample));

    //Get data_type, int32
    char c_datatype[sizeof(headerdef.data_type)];
    readBuffer.read(c_datatype, sizeof(headerdef.data_type));
    std::memcpy(&headerdef.data_type, c_datatype, sizeof(headerdef.data_type));

    //Get bufsize, int32
    char c_bufsize[sizeof(headerdef.bufsize)];
    readBuffer.read(c_bufsize, sizeof(headerdef.bufsize));
    std::memcpy(&headerdef.bufsize, c_bufsize, sizeof(headerdef.bufsize));

//        char c_[sizeof(headerdef.)];
//        readBuffer.read(c_, sizeof(headerdef.));
//        std::memcpy(&headerdef., c_, sizeof(headerdef.));

    //Save paramerters
    m_iNumChannels = headerdef.nchans;
    m_fSampleFreq = headerdef.fsample;
    m_iNumNewSamples = headerdef.nsamples;
    m_iDataType = headerdef.data_type;
    m_iNeuromagHeader = headerdef.bufsize;

    qInfo() << "[FtConnector::parseHeaderDef] Got header parameters.";

    if (m_iDataType != 9) {
        qCritical() << "Data type not supported. Plugin will not behave correctly.";
    }

    return true;
}

//=============================================================================================================

int FtConnector::parseMessageDef(QBuffer &readBuffer)
{
    messagedef_t response;

    //Get version, int16
    char c_version[sizeof(response.version)];
    readBuffer.read(c_version, sizeof(response.version));
    std::memcpy(&response.version, c_version, sizeof(response.version));

    //Get command, int16
    char c_command[sizeof(response.command)];
    readBuffer.read(c_command, sizeof(response.command));
    std::memcpy(&response.command, c_command, sizeof(response.command));

    //Get bufsize, int32
    char c_buffsize[sizeof(response.bufsize)];
    readBuffer.read(c_buffsize, sizeof(response.bufsize));
    std::memcpy(&response.bufsize, c_buffsize, sizeof(response.bufsize));

    return response.bufsize;
}

//=============================================================================================================

void FtConnector::sendRequest(messagedef_t &messagedef)
{
    messagedef.version = VERSION; //we only use VERSION == 1

    m_pSocket->write(reinterpret_cast<char*>(&messagedef.version), sizeof(messagedef.version));
    m_pSocket->write(reinterpret_cast<char*>(&messagedef.command), sizeof(messagedef.command));
    m_pSocket->write(reinterpret_cast<char*>(&messagedef.bufsize), sizeof(messagedef.bufsize));
}

//=============================================================================================================

bool FtConnector::getData()
{
    m_iNumNewSamples = totalBuffSamples();

    if (m_iNumNewSamples == m_iNumSamples) {
        // no new unread data in buffer
        return false;
    }

    qInfo() << "[FtConnector::getData] Attempting to get data...";
    m_pSocket->readAll(); //Ensure receiving buffer is empty

    // Get data message + data selection params
    messagedef_t messagedef;
    messagedef.bufsize = sizeof (datasel_t);
    messagedef.command = GET_DAT;

    datasel_t datasel;
    datasel.begsample = m_iNumSamples;
    datasel.endsample = m_iNumNewSamples - 1;

    sendRequest(messagedef);
    sendDataSel(datasel);

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < sizeof (messagedef_t)) {
        m_pSocket->waitForReadyRead(10);
    }

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    int bufsize = parseMessageDef(msgBuffer);

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < bufsize) {
        m_pSocket->waitForReadyRead(10);
    }

    //Parse return data def from buffer
    QBuffer datadefBuffer;
    prepBuffer(datadefBuffer, sizeof (datadef_t));
    bufsize = parseDataDef(datadefBuffer);

    //Parse actual data from buffer
    QBuffer datasampBuffer;
    prepBuffer(datasampBuffer, bufsize);
    parseData(datasampBuffer, bufsize);

    //update sample tracking
    m_iNumSamples = m_iNumNewSamples;

    //echoStatus();

    return true;
}

//=============================================================================================================

bool FtConnector::setAddr(const QString &sNewAddress)
{
    m_sAddress.clear();
    m_sAddress.append(sNewAddress);

    return true;
}

//=============================================================================================================

bool FtConnector::setPort(const int &iPort)
{
    m_iPort = iPort;

    return true;
}

//=============================================================================================================

void FtConnector::prepBuffer(QBuffer &buffer,
                             int numBytes)
{
    buffer.open(QIODevice::ReadWrite);
    buffer.write(m_pSocket->read(numBytes));
    buffer.reset(); //Start reading from beggining of buffer
}

//=============================================================================================================

int FtConnector::parseDataDef(QBuffer &dataBuffer)
{
    qInfo() << "[FtConnector::parseDataDef] Got datadef, parsing...";
    datadef_t datadef;

    //Get nchans, int32
    char c_chans[sizeof(datadef.nchans)];
    dataBuffer.read(c_chans, sizeof(datadef.nchans));
    std::memcpy(&datadef.nchans, c_chans, sizeof(datadef.nchans));

    //Get nsamples, int32
    char c_nsamples[sizeof(datadef.nsamples)];
    dataBuffer.read(c_nsamples, sizeof(datadef.nsamples));
    std::memcpy(&datadef.nsamples, c_nsamples, sizeof(datadef.nsamples));

    //Get data_type, int32
    char c_datatype[sizeof(datadef.data_type)];
    dataBuffer.read(c_datatype, sizeof(datadef.data_type));
    std::memcpy(&datadef.data_type, c_datatype, sizeof(datadef.data_type));

    //Get bufsize, int32
    char c_bufsize[sizeof(datadef.bufsize)];
    dataBuffer.read(c_bufsize, sizeof(datadef.bufsize));
    std::memcpy(&datadef.bufsize, c_bufsize, sizeof(datadef.bufsize));

//    if(datadef.nchans != m_iNumChannels) {
//        qWarning() << "Data has different number of channels than expected.";
//        return false;
//    }

    m_iMsgSamples = datadef.nsamples;

    return datadef.bufsize;
}

//=============================================================================================================

void FtConnector::sendDataSel(datasel_t &datasel)
{
    m_pSocket->write(reinterpret_cast<char*>(&datasel.begsample), sizeof(datasel.begsample));
    m_pSocket->write(reinterpret_cast<char*>(&datasel.endsample), sizeof(datasel.endsample));
}

//=============================================================================================================

void FtConnector::echoStatus()
{
    qInfo() << "|================================";
    qInfo() << "| [FtConnector::echoStatus]";
    qInfo() << "| Socket:      " << m_pSocket->state();
    qInfo() << "| Address:     " << m_sAddress << ":" << m_iPort;
    qInfo() << "| Channels:    " << m_iNumChannels;
    qInfo() << "| Frequency:   " << m_fSampleFreq;
    qInfo() << "| Samples read:" << m_iNumSamples;
    qInfo() << "| New samples: " << m_iNumNewSamples;
    qInfo() << "|================================";
}

//=============================================================================================================

int FtConnector::totalBuffSamples()
{
    m_pSocket->readAll(); //Ensure receiving buffer is empty

    messagedef_t messagedef;
    messagedef.bufsize = sizeof(samples_events_t) + sizeof (qint32);
    messagedef.command = WAIT_DAT;

    //Set threshold to return more than number samples read.
    samples_events_t threshold;
    threshold.nsamples = m_iNumSamples;
    threshold.nevents = 0xFFFFFFFF;

    // timeout for waiting in milliseconds
    qint32 timeout = 20;

    sendRequest(messagedef);
    sendSampleEvents(threshold);
    m_pSocket->write(reinterpret_cast<char*>(&timeout), sizeof (qint32));

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < sizeof (messagedef_t)) {
        m_pSocket->waitForReadyRead(10);
    }

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    parseMessageDef(msgBuffer);

    //Waiting for response.
    while(m_pSocket->bytesAvailable() < sizeof (samples_events_t)) {
        m_pSocket->waitForReadyRead(10);
    }

    qint32 iNumSamp;

    QBuffer sampeventsBuffer;
    prepBuffer(sampeventsBuffer, sizeof(samples_events_t));

    char cSamps[sizeof(iNumSamp)];
    sampeventsBuffer.read(cSamps, sizeof(iNumSamp));
    std::memcpy(&iNumSamp, cSamps, sizeof(iNumSamp));

    return iNumSamp;
}

//=============================================================================================================

void FtConnector::sendSampleEvents(samples_events_t &threshold)
{
    m_pSocket->write(reinterpret_cast<char*>(&threshold.nsamples), sizeof(threshold.nsamples));
    m_pSocket->write(reinterpret_cast<char*>(&threshold.nevents), sizeof(threshold.nevents));
}

//=============================================================================================================

bool FtConnector::parseData(QBuffer &datasampBuffer,
                            int bufsize)
{
    //start interpreting data as float instead of char
    QByteArray dataArray = datasampBuffer.readAll();
    float* fdata = (float*) dataArray.data();

//TODO: Implement receiving other types of data
//    switch (m_iDataType) {
//        case DATATYPE_FLOAT32:
//            auto data = reinterpret_cast<float*>(dataArray.data(), bufsize);
//            qDebug() << "*** Would you look at that, we're all the way here ***";
//            qDebug() << "Data sample:";

//            for (int i = 0; i < 10 ; i++) {
//                qDebug() << data[i];
//            }
//            break;
//    }

    //format data into eigen matrix to pass up
    Eigen::MatrixXf matData;
    matData.resize(m_iNumChannels, m_iMsgSamples);

    int count = 0;
    for (int i = 0; i < int (m_iMsgSamples); i++) {
        for (int j = 0; j < int (m_iNumChannels); j++) {
            matData(j,i) = fdata[count];
            count++;
        }
    }

    //store and flag new data
    m_pMatEmit = new Eigen::MatrixXd(matData.cast<double>());
    m_bNewData = true;

    return m_bNewData;
}

//=============================================================================================================

void FtConnector::resetEmitData()
{
    m_bNewData = false;
    delete m_pMatEmit;
    m_pMatEmit = Q_NULLPTR;
}

//=============================================================================================================

bool FtConnector::disconnect()
{
    m_pSocket->disconnectFromHost();
    m_pSocket->waitForDisconnected(200);

    return true;
}

//=============================================================================================================

Eigen::MatrixXd FtConnector::getMatrix()
{
    return *m_pMatEmit;
}

//=============================================================================================================

bool FtConnector::newData()
{
    return m_bNewData;
}

//=============================================================================================================

QString FtConnector::getAddr()
{
    return m_sAddress;
}

//=============================================================================================================

FIFFLIB::FiffInfo FtConnector::parseNeuromagHeader()
{
    qInfo() << "[FtConnector::parseNeuromagHeader] Attepting to get extended header...";

    QBuffer chunkBuffer;
    QBuffer neuromagBuffer;

    getHeader();

    prepBuffer(chunkBuffer, m_iNeuromagHeader);

    bool condition = true;

    while(condition) {
        qint32 iType;
        char cType[sizeof(qint32)];
        chunkBuffer.read(cType, sizeof(qint32));
        std::memcpy(&iType, cType, sizeof(qint32));

        //Headers we don't care about
        if(iType < 8) {
            qint32 iSize;
            char cSize[sizeof(qint32)];

            //read size of chunk
            chunkBuffer.read(cSize, sizeof(qint32));
            std::memcpy(&iSize, cSize, sizeof(qint32));

            //read rest of chunk (to clear buffer to read next chunk)
            char cRest[iSize];
            chunkBuffer.read(cRest, iSize);
        } else if(iType == 8) { // Header we care about, FT_CHUNK_NEUROMAG_HEADER = 8
            qint32 iSize;
            char cSize[sizeof(qint32)];

            //read size of chunk
            chunkBuffer.read(cSize, sizeof(qint32));
            std::memcpy(&iSize, cSize, sizeof(qint32));

            //Read relevant chunk info
            neuromagBuffer.open(QIODevice::ReadWrite);
            neuromagBuffer.write(chunkBuffer.read(iSize));

            qint32_be iIntToChar;
            char cCharFromInt[sizeof (qint32)];

            //Append read info with -1 to have a Fiff tag with 'next' == -1
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            neuromagBuffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            neuromagBuffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            neuromagBuffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            neuromagBuffer.write(cCharFromInt);

            neuromagBuffer.reset();

            //Format data into Little endian FiffStream so we can read it with the fiff library
            FIFFLIB::FiffStream::SPtr pStream(new FIFFLIB::FiffStream(&neuromagBuffer));
            pStream->setByteOrder(QDataStream::LittleEndian);

            //Opens and created a dir tree (this is why we had to append -1)
            if(!pStream->open()) {
                qCritical() << "Unable to open neuromag fiff data. Plugin behavior undefined";
                FIFFLIB::FiffInfo defaultInfo;
                return defaultInfo;
            }

            FIFFLIB::FiffInfo FifInfo;
            FIFFLIB::FiffDirNode::SPtr DirNode;

            //Get Fiff info we care about
            if(!pStream->read_meas_info(pStream->dirtree(), FifInfo, DirNode)) {
                qCritical() << "Unable to parse neuromag fiff data. Plugin behavior undefined";
                FIFFLIB::FiffInfo defaultInfo;
                return defaultInfo;
            }

            return FifInfo; //Returns this if all went well
        } else {
            qCritical() << "Unable to recongine chunk data. Plugin behavior undefined";
            FIFFLIB::FiffInfo defaultInfo;
            return defaultInfo;
        }
    }
    //Should never be reached, but returns default fiffinfo for safety
    FIFFLIB::FiffInfo defaultInfo;
    return defaultInfo;
}
