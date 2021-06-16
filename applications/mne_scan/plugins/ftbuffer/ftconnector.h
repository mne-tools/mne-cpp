//=============================================================================================================
/**
 * @file     ftconnector.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Contains the declaration of the FtConnector class.
 *
 * Definition macros and structs ported from fieldtrip codebase.
 *
 */

#ifndef FTCONNECTOR_H
#define FTCONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstring>

#include <fiff/fiff_tag.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/c/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QtCore/QtPlugin>
#include <QHostAddress>
#include <QTcpSocket>
#include <QObject>
#include <QBuffer>
#include <QThread>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
// DEFINITION MACROS
//=============================================================================================================

#define VERSION    static_cast<qint16>(0x0001)

#define PUT_HDR    static_cast<qint16>(0x0101) /* decimal 257 */
#define PUT_DAT    static_cast<qint16>(0x0102) /* decimal 258 */
#define PUT_EVT    static_cast<qint16>(0x0103) /* decimal 259 */
#define PUT_OK     static_cast<qint16>(0x0104) /* decimal 260 */
#define PUT_ERR    static_cast<qint16>(0x0105) /* decimal 261 */

#define GET_HDR    static_cast<qint16>(0x0201) /* decimal 513 */
#define GET_DAT    static_cast<qint16>(0x0202) /* decimal 514 */
#define GET_EVT    static_cast<qint16>(0x0203) /* decimal 515 */
#define GET_OK     static_cast<qint16>(0x0204) /* decimal 516 */
#define GET_ERR    static_cast<qint16>(0x0205) /* decimal 517 */

#define WAIT_DAT   static_cast<qint16>(0x0402) /* decimal 1026 */
#define WAIT_OK    static_cast<qint16>(0x0404) /* decimal 1027 */
#define WAIT_ERR   static_cast<qint16>(0x0405) /* decimal 1028 */

#define PUT_HDR_NORESPONSE static_cast<qint16>(0x0501) /* decimal 1281 */
#define PUT_DAT_NORESPONSE static_cast<qint16>(0x0502) /* decimal 1282 */
#define PUT_EVT_NORESPONSE static_cast<qint16>(0x0503) /* decimal 1283 */

//=============================================================================================================
// STRUCT DEFINITIONS
//=============================================================================================================

typedef struct {
    qint32 nchans;
    qint32 nsamples;
    qint32 data_type;
    qint32 bufsize;     /* size of the buffer in bytes */
} datadef_t;

typedef struct {
    qint32  nchans;
    qint32  nsamples;
    qint32  nevents;
    float   fsample;
    qint32  data_type;
    qint32  bufsize;     /* size of the buffer in bytes */
} headerdef_t;

typedef struct {
    qint16 version;   /* see VERSION */
    qint16 command;   /* see PUT_xxx, GET_xxx and FLUSH_xxx */
    qint32 bufsize;   /* size of the buffer in bytes */
} messagedef_t;

typedef struct {
    messagedef_t *def;
    void         *buf;
} message_t;

typedef struct {
    qint32 begsample; /* indexing starts with 0, should be >=0 */
    qint32 endsample; /* indexing starts with 0, should be <header.nsamples */
} datasel_t;

typedef struct {
    qint32 nsamples;
    qint32 nevents;
} samples_events_t;

struct BufferInfo{
    int     iNumSamples;                          /**< Number of samples we've read from the buffer. */
    int     iNumNewSamples;                       /**< Number of total samples (read and unread) in the buffer. */
    int     iMsgSamples;                          /**< Number of samples in the latest buffer transmission receied. */
    int     iNumChannels;                         /**< Number of channels in the buffer data. */
    int     iDataType;                            /**< Type of data in the buffer. */
};

struct MetaData{
    bool bFiffInfo = false;
    FIFFLIB::FiffInfo info;
    bool bFiffDigitizerData = false;
    FIFFLIB::FiffDigitizerData dig;

    void setFiffinfo(FIFFLIB::FiffInfo newinfo) {info = newinfo;
                                                 bFiffInfo = true;};

    void setFiffDigitizerData(FIFFLIB::FiffDigitizerData newdig) {dig = newdig;
                                                                  bFiffDigitizerData = true;};
};

//=============================================================================================================

class FtConnector : public QObject
{
    Q_OBJECT

    friend class FtBufferSetupWidget;
    friend class FtBuffer;
    friend class FtBuffProducer;

public:
    //=========================================================================================================
    /**
     * FtConnector constructs an object of the FtConnector class. Only initializes variables to zero.
     */
    FtConnector();

    //=========================================================================================================
    /**
     * ~FtConnector destroys and object of the FtConnector class. Disconnects and deletes m_pSocket.
     */
    ~FtConnector();

    //=========================================================================================================
    /**
     * Connects to buffer at address m_sAddress and port m_iPort
     *
     * @return true if successful, false if unsuccessful.
     */
    bool connect();

    //=========================================================================================================
    /**
     * Disconnects m_pSocket
     *
     * @return true if successful, false if unsuccessful.
     */
    bool disconnect();

    //=========================================================================================================
    /**
     * Requests and receives header data from buffer, saves relevant parameters internally.
     *
     * @return true if successful, false if unsuccessful.
     */
    bool getHeader();

    //=========================================================================================================
    /**
     * equests and receives data from buffer, parses it, and stores it in m_pMatEmit
     *
     * @return true if successful, false if unsuccessful.
     */
    bool getData();

    //=========================================================================================================
    /**
     * Gets address currently stored in private member m_sAddress
     *
     * @return returns m_sAddress.
     */
    QString getAddr();

    //=========================================================================================================
    /**
     * Sets private member m_sAddress to a new address
     *
     * @param[in] sNewAddress   A QString with an address (not checked to se if valid).
     *
     * @return true if successful, false if unsuccessful.
     */
    bool setAddr(const QString &sNewAddress);

    //=========================================================================================================
    /**
     * Gets port numbr currently stored in private member m_iPort
     *
     * @return returns m_iPort.
     */
    int getPort();

    //=========================================================================================================
    /**
     * Sets private member m_iPort to a new port number
     *
     * @param[in] iPort     An int with a new desired port number.
     *
     * @return true if successful, false if unsuccessful.
     */
    bool setPort(const int& iPort);

    //=========================================================================================================
    /**
     * Prints relevant class data to terminal. Useful for debugging.
     */
    void echoStatus();

    //=========================================================================================================
    /**
     * Returns member m_pMatEmit, newest buffer data formatted as an Eigen MatrixXd
     *
     * @return returns m_pMatEmit.
     */
    Eigen::MatrixXd getMatrix();

    //=========================================================================================================
    /**
     * Returns whether or not new data has been read from buffer
     *
     * @return returns m_bNewData, flag of whether there is new data.
     */
    bool newData();

    //=========================================================================================================
    /**
     * Sets m_bNewData to false, deletes m_pMatEmit
     */
    void resetEmitData();

    //=========================================================================================================
    /**
     * Parses the extended header chunk with neuromag data and gets the relevant fiff info
     *
     * @return returns the FiffInfo from the parsed fif file from the neuromag header chunk.
     */
    MetaData parseExtenedHeaders();

    //=========================================================================================================
    /**
     * Gets the current number of samples in the buffer and stores it in m_iNumSamples
     */
    void catchUpToBuffer();

    BufferInfo getBufferInfo();

private:
    //=========================================================================================================
    /**
     * Sends a formated message to the buffer. command and bufsize must be set before calling.
     *
     * @param[in] messagedef    request structure with the appropriate command and bufszie paramters set.
     */
    void sendRequest(messagedef_t &messagedef);

    //=========================================================================================================
    /**
     * Sends a formated datasel message, for defining the first and last sample we are requesting from the buffer
     *
     * @param[in] datasel   Formattd first and last sample index we are requesting from the buffer.
     */
    void sendDataSel(datasel_t &datasel);

    //=========================================================================================================
    /**
     * Sends a formated sampleevents message, used for receving updated sample an event numbers from buffer
     *
     * @param[in] threshold     Buffer will respond once sample/event numbers reach the thresholds.
     */
    void sendSampleEvents(samples_events_t &threshold);

    //=========================================================================================================
    /**
     * Parses headerdef message and saves parameters(channels, frequency, datatype, newsamples)
     *
     * @param[in] readBuffer    QBuffer with return headerdef_t data from buffer.
     *
     * @return true if successful, false if unsuccessful.
     */
    bool parseHeaderDef(QBuffer &readBuffer);

    //=========================================================================================================
    /**
     * Parses messadef and returns bufsize
     *
     * @param[in] readBuffer    QBuffer with return messagedef_t data from buffer.
     *
     * @return returns messagedef_t.bufsize.
     */
    int parseMessageDef(QBuffer &readBuffer);

    //=========================================================================================================
    /**
     * Parses datadef and returns bufsize
     *
     * @param[in] dataBuffer    QBuffer with return datadef_t data from buffer.
     *
     * @return returns datadef_t.bufsize.
     */
    int parseDataDef(QBuffer &dataBuffer);

    //=========================================================================================================
    /**
     * Parses sample data received from buffer, formates it and saves it to m_pMatEmit;
     *
     * @param[in] datasampBuffer    QBuffer with return data from buffer.
     * @param[in] bufsize           Buffer size of sample data.
     *
     * @return true if successful, false if unsuccessful.
     */
    bool parseData(QBuffer &datasampBuffer,
                   int bufsize);

    //=========================================================================================================
    /**
     * @brief Opens Buffer, reads numBytes from socket and sets index to zero
     *
     * @param[out] buffer       QBuffer to which daa will be written.
     * @param[in] numBytes      How many bytes to read from socket.
     */
    void prepBuffer(QBuffer &buffer,
                    int numBytes);

    //=========================================================================================================
    /**
     * Returns total amount of samples written to buffer
     *
     * @return returns total amount of samples written to buffer.
     */
    int totalBuffSamples();

    //=========================================================================================================
    /**
     * Returns FiffInfo object initilized based on fieldtrip neuromag header
     *
     * @param[in] neuromagBuffer    buffer with the neuromag header
     *
     * @return FiffInfo object based on fieldtrip neuromag header
     */
    FIFFLIB::FiffInfo infoFromNeuromagHeader(QBuffer& neuromagBuffer);

    //=========================================================================================================
    /**
     * Returns FiffInfo object initilized based on base filedtrip header info
     *
     * @return FiffInfo object based on filedtrip header
     */
    FIFFLIB::FiffInfo infoFromSimpleHeader();

    //=========================================================================================================
    /**
     * @brief digDataFromIsotrakHeader
     *
     * @param isotrakBuffer
     *
     * @return
     */
    FIFFLIB::FiffDigitizerData digDataFromIsotrakHeader(QBuffer& isotrakBuffer);

    int getExtendedHeaderType(QBuffer& buffer, qint32& iReadCount);

    void moveBufferData(QBuffer& from, QBuffer& to, qint32& iReadCount);

    void skipBufferData(QBuffer& buffer, qint32& iReadCount);

    int                                     m_iNumSamples;                          /**< Number of samples we've read from the buffer. */
    int                                     m_iNumNewSamples;                       /**< Number of total samples (read and unread) in the buffer. */
    int                                     m_iMsgSamples;                          /**< Number of samples in the latest buffer transmission receied. */
    int                                     m_iNumChannels;                         /**< Number of channels in the buffer data. */
    int                                     m_iDataType;                            /**< Type of data in the buffer. */
    int                                     m_iExtendedHeaderSize;                          /**< Size of extended header chunks. */
    quint16                                 m_iPort;                                /**< Port where the ft bufferis found. */

    bool                                    m_bNewData;                             /**< Indicate whether we've received new data. */

    float                                   m_fSampleFreq;                          /**< Sampling frequency of data in the buffer. */

    QString                                 m_sAddress;                             /**< Address where the ft buffer is found. */

    QTcpSocket*                             m_pSocket;                              /**< Socket that manages the connection to the ft buffer. */

    Eigen::MatrixXd*                        m_pMatEmit;                             /**< Container to format data to tansmit to FtBuffProducer. */
};

}//namespace end bracket

#endif // FTCONNECTOR_H
