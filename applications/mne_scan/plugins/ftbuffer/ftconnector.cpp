#include "ftconnector.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//*************************************************************************************************************

FtConnector::FtConnector()
:m_Socket(Q_NULLPTR)
,m_iNumChannels(0)
,m_fSampleFreq(0)
,m_iNumSamples(0)
,m_iNumNewSamples(0)
,m_bNewData(false)
{
}

//*************************************************************************************************************

FtConnector::~FtConnector()
{
    delete m_Socket;
}

//*************************************************************************************************************

bool FtConnector::connect() {
    m_Socket = new QTcpSocket();

    m_Socket->connectToHost(QHostAddress(m_sAddress), m_iPort);
    int tries = 0;

    while (m_Socket->state() != QAbstractSocket::ConnectedState){
        m_Socket->waitForConnected(200);
        tries ++;
        if (tries > 5) {
            break;
        }
    }

    if(m_Socket->state() == QAbstractSocket::ConnectedState) {
        qInfo() << "Connected!";
        return true;
    } else {
        qInfo() << "Timed out: Failed to connect.";
        delete m_Socket;
        return false;
    }
}

//*************************************************************************************************************

bool FtConnector::getHeader() {
    qInfo() << "Attepting to get header on thread" << this->thread();
    qDebug() << "socket on thread" << m_Socket->thread();

    m_Socket->readAll(); //Ensure receiving buffer is empty

    qDebug()<< "1";

    // Defining parameters to send a get header message to buffer
    messagedef_t messagedef;
    messagedef.bufsize = 0;
    messagedef.command = GET_HDR;

    qDebug()<< "2";

    // Send request to buffer
    sendRequest(messagedef);

    qDebug()<< "3";

    //Waiting for response.
    while(m_Socket->bytesAvailable() < sizeof (messagedef_t)) {
        m_Socket->waitForReadyRead(10);
    }

    qDebug()<< "4";

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    int bufsize = parseMessageDef(msgBuffer);

    if (bufsize == 0) {
        qInfo() << "No header data found";
        return false;
    }

    while(m_Socket->bytesAvailable() < bufsize) {
        m_Socket->waitForReadyRead(10);
    }

    //Parse header info from buffer
    QBuffer hdrBuffer;
    prepBuffer(hdrBuffer, sizeof (headerdef_t)); // if implementing header chunks: change from sizeof (headerdef) to bufsize
    parseHeaderDef(hdrBuffer);

    m_Socket->readAll(); //Ensure receiving buffer is empty

    return true;
}

//*************************************************************************************************************

bool FtConnector::parseHeaderDef(QBuffer &readBuffer) {

    //Start parsing header parameters
    qInfo() << "Got header data. Parsing...";
    headerdef_t headerdef;

    //Get nchans, int32
    char c_chans[sizeof(headerdef.nchans)];
    readBuffer.read(c_chans, sizeof(headerdef.nchans));
    std::memcpy(&headerdef.nchans, c_chans, sizeof(headerdef.nchans));
    qDebug() << "nchans:" << headerdef.nchans;

    //Get nsamples, int32
    char c_samples[sizeof(headerdef.nsamples)];
    readBuffer.read(c_samples, sizeof(headerdef.nsamples));
    std::memcpy(&headerdef.nsamples, c_samples, sizeof(headerdef.nsamples));
    qDebug() << "nsamples:" << headerdef.nsamples;

    //Get nevents, int32
    char c_events[sizeof(headerdef.nevents)];
    readBuffer.read(c_events, sizeof(headerdef.nevents));
    std::memcpy(&headerdef.nevents, c_events, sizeof(headerdef.nevents));
    qDebug() << "nevents:" << headerdef.nevents;

    //Get fsample, float
    char c_freqsamp[sizeof(headerdef.fsample)];
    readBuffer.read(c_freqsamp, sizeof(headerdef.fsample));
    std::memcpy(&headerdef.fsample, c_freqsamp, sizeof(headerdef.fsample));
    qDebug() << "fsample:" << headerdef.fsample;

    //Get data_type, int32
    char c_datatype[sizeof(headerdef.data_type)];
    readBuffer.read(c_datatype, sizeof(headerdef.data_type));
    std::memcpy(&headerdef.data_type, c_datatype, sizeof(headerdef.data_type));
    qDebug() << "data_type:" << headerdef.data_type;

    //Get bufsize, int32
    char c_bufsize[sizeof(headerdef.bufsize)];
    readBuffer.read(c_bufsize, sizeof(headerdef.bufsize));
    std::memcpy(&headerdef.bufsize, c_bufsize, sizeof(headerdef.bufsize));
    qDebug() << "bufsize:" << headerdef.bufsize;

//        char c_[sizeof(headerdef.)];
//        readBuffer.read(c_, sizeof(headerdef.));
//        std::memcpy(&headerdef., c_, sizeof(headerdef.));

    //Save paramerters
    m_iNumChannels = headerdef.nchans;
    m_fSampleFreq = headerdef.fsample;
    m_iNumNewSamples = headerdef.nsamples;
    m_iDataType = headerdef.data_type;

    qInfo() << "Got header parameters.";

    return true;
}

//*************************************************************************************************************

int FtConnector::parseMessageDef(QBuffer &readBuffer) {
    messagedef_t response;

    //Get version, int16
    char c_version[sizeof(response.version)];
    readBuffer.read(c_version, sizeof(response.version));
    std::memcpy(&response.version, c_version, sizeof(response.version));
    qDebug() << "Version:" << response.version;

    //Get command, int16
    char c_command[sizeof(response.command)];
    readBuffer.read(c_command, sizeof(response.command));
    std::memcpy(&response.command, c_command, sizeof(response.command));
    qDebug() << "Command:" << response.command;

    //Get bufsize, int32
    char c_buffsize[sizeof(response.bufsize)];
    readBuffer.read(c_buffsize, sizeof(response.bufsize));
    std::memcpy(&response.bufsize, c_buffsize, sizeof(response.bufsize));
    qDebug() << "Bufsize:" << response.bufsize;

    return response.bufsize;
}

//*************************************************************************************************************

void FtConnector::sendRequest(messagedef_t &messagedef){
    messagedef.version = VERSION;

    m_Socket->write(reinterpret_cast<char*>(&messagedef.version), sizeof(messagedef.version));
    m_Socket->write(reinterpret_cast<char*>(&messagedef.command), sizeof(messagedef.command));
    m_Socket->write(reinterpret_cast<char*>(&messagedef.bufsize), sizeof(messagedef.bufsize));
}

//*************************************************************************************************************

bool FtConnector::getData() {
    m_iNumNewSamples = totalBuffSamples();

    if (m_iNumNewSamples == m_iNumSamples) {
        // no new unread data in buffer
        //waits a bit before returning
        return false;
    }

    qInfo() << "Attempting to get data...";
    m_Socket->readAll(); //Ensure receiving buffer is empty

    // Get data message + data selection params
    messagedef_t messagedef;
    messagedef.bufsize = sizeof (datasel_t);
    messagedef.command = GET_DAT;

    datasel_t datasel;
    datasel.begsample = m_iNumSamples;
    datasel.endsample = m_iNumNewSamples - 1;

    sendRequest(messagedef);
    sendDataSel(datasel);

    while(m_Socket->bytesAvailable() < sizeof (messagedef_t)) {
        m_Socket->waitForReadyRead(10);
    }

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    int bufsize = parseMessageDef(msgBuffer);
    qDebug() << "@@@@@@@@@@ Buffsize:" << bufsize;

    while(m_Socket->bytesAvailable() < bufsize) {
        m_Socket->waitForReadyRead(10);
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

    echoStatus();

    return true;
}

//*************************************************************************************************************

bool FtConnector::setAddr(const QString &sNewAddress) {
    m_sAddress.clear();
    m_sAddress.append(sNewAddress);
    return true;
}

//*************************************************************************************************************

bool FtConnector::setPort(const int &iPort) {
    m_iPort = iPort;
    return true;
}

//*************************************************************************************************************

void FtConnector::prepBuffer(QBuffer &buffer, int numBytes) {
    buffer.open(QIODevice::ReadWrite);
    buffer.write(m_Socket->read(numBytes));
    buffer.reset(); //Start reading from beggining of buffer
}

//*************************************************************************************************************

int FtConnector::parseDataDef(QBuffer &dataBuffer) {
    qInfo() << "Got datadef, parsing...";
    datadef_t datadef;

    //Get nchans, int32
    char c_chans[sizeof(datadef.nchans)];
    dataBuffer.read(c_chans, sizeof(datadef.nchans));
    std::memcpy(&datadef.nchans, c_chans, sizeof(datadef.nchans));
    qDebug() << "nchans:" << datadef.nchans;

    //Get nsamples, int32
    char c_nsamples[sizeof(datadef.nsamples)];
    dataBuffer.read(c_nsamples, sizeof(datadef.nsamples));
    std::memcpy(&datadef.nsamples, c_nsamples, sizeof(datadef.nsamples));
    qDebug() << "nsamples:" << datadef.nsamples;

    //Get data_type, int32
    char c_datatype[sizeof(datadef.data_type)];
    dataBuffer.read(c_datatype, sizeof(datadef.data_type));
    std::memcpy(&datadef.data_type, c_datatype, sizeof(datadef.data_type));
    qDebug() << "datatype:" << datadef.data_type;

    //Get bufsize, int32
    char c_bufsize[sizeof(datadef.bufsize)];
    dataBuffer.read(c_bufsize, sizeof(datadef.bufsize));
    std::memcpy(&datadef.bufsize, c_bufsize, sizeof(datadef.bufsize));
    qDebug() << "bufsize:" << datadef.bufsize;

//    if(datadef.nchans != m_iNumChannels) {
//        qWarning() << "Data has different number of channels than expected.";
//        return false;
//    }

    m_iMsgSamples = datadef.nsamples;

    return datadef.bufsize;
}

//*************************************************************************************************************

void FtConnector::sendDataSel(datasel_t &datasel) {
    m_Socket->write(reinterpret_cast<char*>(&datasel.begsample), sizeof(datasel.begsample));
    m_Socket->write(reinterpret_cast<char*>(&datasel.endsample), sizeof(datasel.endsample));
}

//*************************************************************************************************************

void FtConnector::echoStatus() {
    qInfo() << "=============================";
    qInfo() << "FtConnector STATUS:";
    qInfo() << "Socket:      " << m_Socket->state();
    qInfo() << "Address:     " << m_sAddress << ":" << m_iPort;
    qInfo() << "Channels:    " << m_iNumChannels;
    qInfo() << "Frequency:   " << m_fSampleFreq;
    qInfo() << "Samples read:" << m_iNumSamples;
    qInfo() << "New samples: " << m_iNumNewSamples;
    qInfo() << "============================";
}

//*************************************************************************************************************

int FtConnector::totalBuffSamples() {
    m_Socket->readAll(); //Ensure receiving buffer is empty

    messagedef_t messagedef;
    messagedef.bufsize = 12;
    messagedef.command = WAIT_DAT;

    //Set threshold to zero so we always get the data we need.
    samples_events_t threshold;
    threshold.nsamples = m_iNumSamples;
    threshold.nevents = 0xFFFFFFFF;

    qint32 timeout = 20;

    sendRequest(messagedef);

    sendSampleEvents(threshold);

    m_Socket->write(reinterpret_cast<char*>(&timeout), sizeof (qint32));

    while(m_Socket->bytesAvailable() < sizeof (messagedef_t)) {
        m_Socket->waitForReadyRead(10);
    }

    //Parse return message from buffer
    QBuffer msgBuffer;
    prepBuffer(msgBuffer, sizeof (messagedef_t));
    parseMessageDef(msgBuffer);

    while(m_Socket->bytesAvailable() < sizeof (samples_events_t)) {
        m_Socket->waitForReadyRead(10);
    }

    qint32 samp;

    QBuffer dataBuffer;
    prepBuffer(dataBuffer, 8);

    char csamps[sizeof(samp)];
    dataBuffer.read(csamps, sizeof(samp));
    std::memcpy(&samp, csamps, sizeof(samp));
    qDebug() << "samp?:" << samp;

    return samp;

}

//*************************************************************************************************************

void FtConnector::sendSampleEvents(samples_events_t &threshold) {
    m_Socket->write(reinterpret_cast<char*>(&threshold.nsamples), sizeof(threshold.nsamples));
    m_Socket->write(reinterpret_cast<char*>(&threshold.nevents), sizeof(threshold.nevents));
}

//*************************************************************************************************************

bool FtConnector::parseData(QBuffer &datasampBuffer, int bufsize) {

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

    m_pMatEmit = new Eigen::MatrixXd(matData.cast<double>());
    m_bNewData  = true;

    return true;
}

//*************************************************************************************************************

void FtConnector::resetEmitData() {
    m_bNewData = false;
    delete m_pMatEmit;
    m_pMatEmit = Q_NULLPTR;
}

//*************************************************************************************************************

bool FtConnector::disconnect() {
    m_Socket->disconnectFromHost();
    m_Socket->waitForDisconnected(200);

    return true;
}

//*************************************************************************************************************

Eigen::MatrixXd FtConnector::getMatrix() {
    return *m_pMatEmit;
}

//*************************************************************************************************************

bool FtConnector::newData() {
    return m_bNewData;
}

//*************************************************************************************************************

QString FtConnector::getAddr() {
    return m_sAddress;
}
