#ifndef BABYMEGCLIENT_H
#define BABYMEGCLIENT_H

#include <QObject>
#include <QtNetWork/QTcpSocket>
#include <QMutex>
#include <QThread>
#include <QDataStream>

#include "babymeginfo.h"


class QTcpSocket;
class QNetworkSession;



class BabyMEGClient : public QThread
{
    Q_OBJECT

public:
    explicit BabyMEGClient(int myPort, QObject *parent = 0);
public:
    QString name;
    int port;
    bool SocketIsConnected;
    bool SkipLoop;
    bool DataAcqStartFlag;
    BabyMEGInfo *myBabyMEGInfo;

    QByteArray buffer;
    int numBlock;
    bool DataACK;

private:
    QTcpSocket *tcpSocket;
    QMutex m_qMutex;
signals:
    void DataAcq();
    void error(int socketError, const QString &message);

public slots:
    void ConnectToBabyMEG();
    void DisConnectBabyMEG();
    void SendCommandToBabyMEG();
    void DisplayError(int socketError, const QString &message);
    void ReadToBuffer();
    void run();
    void SendCommandToBabyMEGShortConnection();

// public function
public:
    QByteArray MGH_LM_Int2Byte(int a);
    int MGH_LM_Byte2Int(QByteArray InByte);
    double MGH_LM_Byte2Double(QByteArray InByte);
    void HexDisplay(double a);
    void SetInfo(BabyMEGInfo *pInfo);

    void DispatchDataPackage(int tmp);
    void ReadNextBlock(int tmp);
    void SendCommand(QString s);
    void handleBuffer();
};


#endif // BABYMEGCLIENT_H
