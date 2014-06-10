#include "stcworker.h"


#include <QDebug>


StcWorker::StcWorker(QObject *parent) :
    QObject(parent)
{
}


//*************************************************************************************************************

void StcWorker::addData(QList<VectorXd> &data)
{
//    qDebug() << "addData" << QThread::currentThread();
    m_qMutex.lock();
    m_data.append(data);
    m_qMutex.unlock();
}


//*************************************************************************************************************

void StcWorker::process()
{
    while(true)
    {
        QThread::msleep(30);
        if(!m_data.isEmpty())
        {
            qDebug() << "process currentThread" << QThread::currentThread();
            emit stcSample(m_data.front());
            m_data.pop_front();
        }
    }
}
