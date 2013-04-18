#ifndef BABYMEGINFO_H
#define BABYMEGINFO_H

#include <QObject>
#include <QQueue>
#include <QtCore>

class BabyMEGInfo
{
public:
    BabyMEGInfo();
public:
    int chnNum;
    int dataLength;

    //BB_QUEUE
    QQueue<QByteArray> g_queue;
    int g_maxlen;
    QMutex g_mutex;
    QWaitCondition g_queueNotFull;
    QWaitCondition g_queueNotEmpty;

public:
    void MGH_LM_Parse_Para(QByteArray cmdstr);
    void EnQueue(QByteArray DataIn);
    QByteArray DeQueue();

};

#endif // BABYMEGINFO_H
