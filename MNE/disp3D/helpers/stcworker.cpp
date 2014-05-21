#include "stcworker.h"


#include <QDebug>


StcWorker::StcWorker(QObject *parent) :
    QObject(parent)
{
}


//*************************************************************************************************************

void StcWorker::addData()
{
    qDebug() << "addData" << QThread::currentThread();
}


//*************************************************************************************************************

void StcWorker::process()
{
    while(true)
    {
        qDebug() << "generateData" << QThread::currentThread();
        QThread::msleep(500);
    }
}
