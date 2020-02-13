#ifndef FTCONNECTOR_H
#define FTCONNECTOR_H

//*************************************************************************************************************
//=============================================================================================================
// QT Includes
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QtCore/QtPlugin>
#include <QBuffer>
#include <QThread>

//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// OTHER INCLUDES
//=============================================================================================================

#include <cstring>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FtBufferToolboxPlugin
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//*************************************************************************************************************
//=============================================================================================================
// DEFINITIONS
//=============================================================================================================

#define VERSION    (qint16)0x0001

#define PUT_HDR    (qint16)0x0101 /* decimal 257 */
#define PUT_DAT    (qint16)0x0102 /* decimal 258 */
#define PUT_EVT    (qint16)0x0103 /* decimal 259 */
#define PUT_OK     (qint16)0x0104 /* decimal 260 */
#define PUT_ERR    (qint16)0x0105 /* decimal 261 */

#define GET_HDR    (qint16)0x0201 /* decimal 513 */
#define GET_DAT    (qint16)0x0202 /* decimal 514 */
#define GET_EVT    (qint16)0x0203 /* decimal 515 */
#define GET_OK     (qint16)0x0204 /* decimal 516 */
#define GET_ERR    (qint16)0x0205 /* decimal 517 */

#define FLUSH_HDR  (qint16)0x0301 /* decimal 769 */
#define FLUSH_DAT  (qint16)0x0302 /* decimal 770 */
#define FLUSH_EVT  (qint16)0x0303 /* decimal 771 */
#define FLUSH_OK   (qint16)0x0304 /* decimal 772 */
#define FLUSH_ERR  (qint16)0x0305 /* decimal 773 */

#define WAIT_DAT   (qint16)0x0402 /* decimal 1026 */
#define WAIT_OK    (qint16)0x0404 /* decimal 1027 */
#define WAIT_ERR   (qint16)0x0405 /* decimal 1028 */

#define PUT_HDR_NORESPONSE (qint16)0x0501 /* decimal 1281 */
#define PUT_DAT_NORESPONSE (qint16)0x0502 /* decimal 1282 */
#define PUT_EVT_NORESPONSE (qint16)0x0503 /* decimal 1283 */

/* these are used in the data_t and event_t structure */
#define DATATYPE_CHAR    (qint32)0
#define DATATYPE_UINT8   (qint32)1
#define DATATYPE_UINT16  (qint32)2
#define DATATYPE_UINT32  (qint32)3
#define DATATYPE_UINT64  (qint32)4
#define DATATYPE_INT8    (qint32)5
#define DATATYPE_INT16   (qint32)6
#define DATATYPE_INT32   (qint32)7
#define DATATYPE_INT64   (qint32)8
#define DATATYPE_FLOAT32 (qint32)9
#define DATATYPE_FLOAT64 (qint32)10

//*************************************************************************************************************
//=============================================================================================================
// FIELDTRIP MESSAGE STRUCTS
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

//*************************************************************************************************************

class FtConnector : public QObject
{
    Q_OBJECT

    friend class FtBufferSetupWidget;
public:
    FtConnector();

    //=========================================================================================================
    ~FtConnector();

    //=========================================================================================================
    bool connect();

    //=========================================================================================================
    bool disconnect();

    //=========================================================================================================
    bool getHeader();

    //=========================================================================================================
    bool getData();

    //=========================================================================================================
    QString getAddr();

    //=========================================================================================================
    bool setAddr(const QString &sNewAddress);

    //=========================================================================================================
    int getPort();

    //=========================================================================================================
    bool setPort(const int& iPort);

    //=========================================================================================================
    void echoStatus();

    //=========================================================================================================
    Eigen::MatrixXd getMatrix();

    //=========================================================================================================
    bool newData();

    //=========================================================================================================
    void resetEmitData();

private:

    void sendRequest(messagedef_t &messagedef);

    //=========================================================================================================
    void sendDataSel(datasel_t &datasel);

    //=========================================================================================================
    void sendSampleEvents(samples_events_t &threshold);

    //=========================================================================================================
    bool parseHeaderDef(QBuffer &readBuffer);

    //=========================================================================================================
    int parseMessageDef(QBuffer &readBuffer);

    //=========================================================================================================
    int parseDataDef(QBuffer &dataBuffer);

    //=========================================================================================================
    int parseSampleEvents(QBuffer &seBuffer);

    //=========================================================================================================
    bool parseData(QBuffer &datasampBuffer, int bufsize);

    //=========================================================================================================
    void prepBuffer(QBuffer &buffer, int numBytes);

    //=========================================================================================================
    int totalBuffSamples();

    //=========================================================================================================
    /**
    * @brief newDataAvailable - Sends new buffer data
    * @param matData - formated data from buffer
    */
    void newDataAvailable(const Eigen::MatrixXd &matData);

//*************************************************************************************************************

    QTcpSocket*         m_Socket;                               /**< Socket that manages the connection to the ft buffer */

    QString             m_sAddress          = "127.0.0.1";      /**< Address where the ft buffer is found */
    int                 m_iPort             = 1972;             /**< Port where the ft bufferis found */

    int                 m_iNumChannels;                         /**< Number of channels in the buffer data */
    float               m_fSampleFreq;                          /**< Sampling frequency of data in the buffer */
    int                 m_iDataType;                            /**< Type of data in the buffer */

    int                 m_iNumSamples;                          /**< Number of samples we've read from the buffer */
    int                 m_iNumNewSamples;                       /**< Number of total samples (read and unread) in the buffer */
    int                 m_iMsgSamples;                          /**< Number of samples in the latest buffer transmission receied */

    Eigen::MatrixXd*    m_pMatEmit;                             /**< Container to format data to tansmit to FtBuffProducer */

    bool                m_bNewData;                             /**< Indicate whether we've received new data */
};

}//namespace

#endif // FTCONNECTOR_H
