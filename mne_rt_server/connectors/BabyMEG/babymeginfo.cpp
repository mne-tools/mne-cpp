#include "babymeginfo.h"

BabyMEGInfo::BabyMEGInfo()
{
    chnNum = 0; dataLength = 0;
    g_maxlen = 500;
}

void BabyMEGInfo::MGH_LM_Parse_Para(QByteArray cmdstr)
{
    chnNum = 400;
    dataLength = 5000;
    return;
}
/*
void BabyMEGInfo::EnQueue(QByteArray DataIn)
{
    g_mutex.lock();
    if (g_queue.size() == g_maxlen) {
        qDebug() << "g_queue is full, data lost!";
    }
    else
    {
        g_queue.enqueue(DataIn);
        qDebug() << "Data In...[size="<<g_queue.size()<<"]";
    }
    g_mutex.unlock();

}
QByteArray BabyMEGInfo::DeQueue()
{
    QByteArray val;
    g_mutex.lock();
    if (g_queue.isEmpty()) {
        qDebug() << "g_queue empty, no data acquired!";
        val.clear();
    }
    else
    {
        val = g_queue.dequeue();

    }
    qDebug() << "Data Out...[size="<<g_queue.size()<<"]";
    g_mutex.unlock();
    return val;
}
*/
void BabyMEGInfo::EnQueue(QByteArray DataIn)
{
    g_mutex.lock();
    if (g_queue.size() == g_maxlen) {
        qDebug() << "g_queue is full, waiting!";
        g_queueNotFull.wait(&g_mutex);
    }
    g_queue.enqueue(DataIn);
    g_queueNotEmpty.wakeAll();
    g_mutex.unlock();
    qDebug() << "Data In...[size="<<g_queue.size()<<"]";
}
QByteArray BabyMEGInfo::DeQueue()
{
    QMutexLocker locker(&g_mutex);
    if (g_queue.isEmpty()) {
    qDebug() << "g_queue empty, waiting!";
    g_queueNotEmpty.wait(&g_mutex);
    }
    QByteArray val = g_queue.dequeue();
    g_queueNotFull.wakeAll();
    qDebug() << "Data Out...[size="<<g_queue.size()<<"]";
    return val;
}
