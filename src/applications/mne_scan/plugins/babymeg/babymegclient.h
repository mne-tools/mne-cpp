//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymegclient.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief     BabyMEGClient class declaration.
 */

#ifndef BABYMEGCLIENT_H
#define BABYMEGCLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeginfo.h"
#include "babymeg_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QThread>
#include <QDataStream>

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeginfo.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QNetworkSession;

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
/**
 * DECLARE CLASS BabyMEGClient
 *
 * @brief The BabyMEGClient class provides a TCP/IP communication between Qt and Labview.
 */
class BABYMEGSHARED_EXPORT BabyMEGClient : public QThread
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a BabyMEG.
     */
    explicit BabyMEGClient(int myPort, QObject *parent = 0);

    ~BabyMEGClient();

    //=========================================================================================================
    /**
     * Convert an integer (4 bytes) to a 4-byte array
     *
     * @param[in] a -- <int>.
     * @param[out] Byte array.
     */
    QByteArray MGH_LM_Int2Byte(int a);

    //=========================================================================================================
    /**
     * Convert a 4-byte array to an integer
     *
     * @param[in] InByte -- Byte array.
     * @param[out] <int>.
     */
    int MGH_LM_Byte2Int(QByteArray InByte);

    //=========================================================================================================
    /**
     * Convert one 8-byte array to a double
     *
     * @param[in] InByte -- Byte array.
     * @param[out] <double>.
     */
    double MGH_LM_Byte2Double(QByteArray InByte);

    //=========================================================================================================
    /**
     * Hex display
     *
     * @param[in] a -- double number.
     */
    void HescDisplay(double a);

    //=========================================================================================================
    /**
     * Set Head Info
     *
     * @param[in] pInfo -- struct of header information.
     */
    void SetInfo(QSharedPointer<BabyMEGInfo> pInfo);

    //=========================================================================================================
    /**
     * Dispatch the data package
     *
     * @param[in] tmp -- block size.
     */
    void DispatchDataPackage(int tmp);

    //=========================================================================================================
    /**
     * Read next data block
     *
     * @param[in] tmp -- block size.
     */
    void ReadNextBlock(int tmp);

    //=========================================================================================================
    /**
     * Send command with command format as string
     *
     * @param[in] s -- string.
     */
    void SendCommand(QString s);

    //=========================================================================================================
    /**
     * Handle the data buffer connecting to the TCP socket
     *
     * @param[in] void.
     */
    void handleBuffer();

    //=========================================================================================================
    /**
     * Connect to BabyMEG server
     *
     * @param[in] void.
     */
    void ConnectToBabyMEG();

    //=========================================================================================================
    /**
     * DisConnect to BabyMEG server
     *
     * @param[in] void.
     */
    void DisconnectBabyMEG();

    //=========================================================================================================
    /**
     * Send Command to BabyMEG server
     *
     * @param[in] void.
     */
    void SendCommandToBabyMEG();

    //=========================================================================================================
    /**
     * Read data from socket to a buffer
     *
     * @param[in] void.
     */
    void ReadToBuffer();

    //=========================================================================================================
    /**
     * Send Command to BabyMEG command server with short sync connection
     *
     * @param[in] String s - the string will be sent to server.
     */
    void SendCommandToBabyMEGShortConnection(QByteArray s);

    void run();
    void DisplayError(int socketError, const QString &message);
    inline bool isConnected() const;

    QString                     name;

    quint16                     port;
    int                         numBlock;

    bool                        SkipLoop;
    bool                        DataAcqStartFlag;
    bool                        DataACK;

    QSharedPointer<BabyMEGInfo> myBabyMEGInfo;
    QByteArray                  buffer;

private:
    bool                        m_bSocketIsConnected;
    QTcpSocket*                 tcpSocket;

    QMutex                      m_qMutex;

signals:
    void DataAcq();
    void error(int socketError, const QString &message);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool BabyMEGClient::isConnected() const
{
    return m_bSocketIsConnected;
}
} // NAMESPACE

#endif // BABYMEGCLIENT_H
