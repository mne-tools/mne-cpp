#include "stcworker.h"


#include <QDebug>


StcWorker::StcWorker(QObject *parent)
: QObject(parent)
, m_bIsLooping(true)
, m_iCurrentSample(0)
, m_iUSecIntervall(100)
{
}


//*************************************************************************************************************

void StcWorker::addData(QList<VectorXd> &data)
{
    m_qMutex.lock();
    m_data.append(data);
    m_qMutex.unlock();
}


//*************************************************************************************************************

void StcWorker::process()
{
    while(true)
    {
        QThread::usleep(m_iUSecIntervall);
        if(!m_data.isEmpty())
        {
            if(m_bIsLooping)
            {
                emit stcSample(m_data[m_iCurrentSample%m_data.size()]);
                ++m_iCurrentSample;
            }
            else
            {
                emit stcSample(m_data.front());
                m_data.pop_front();
            }
        }
    }
}

//*************************************************************************************************************

void StcWorker::setInterval(int usec)
{
    m_iUSecIntervall = usec;
}


//*************************************************************************************************************

void StcWorker::setLoop(bool looping)
{
    m_bIsLooping = looping;
}
