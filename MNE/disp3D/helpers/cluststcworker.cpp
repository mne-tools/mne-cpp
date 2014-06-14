#include "cluststcworker.h"

#include <QDebug>


ClustStcWorker::ClustStcWorker(QObject *parent)
: QObject(parent)
, m_bIsLooping(true)
, m_iAverageSamples(10)
, m_iCurrentSample(0)
, m_iUSecIntervall(100)
{
    m_data.clear();
}


//*************************************************************************************************************

void ClustStcWorker::addData(QList<VectorXd> &data)
{
    m_qMutex.lock();
    m_data.append(data);
    m_qMutex.unlock();
}


//*************************************************************************************************************

void ClustStcWorker::clear()
{
    m_qMutex.lock();
    m_data.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

void ClustStcWorker::process()
{
    VectorXd m_vecAverage(0,0);

    while(true)
    {
        if(!m_data.isEmpty())
        {
            if(m_bIsLooping)
            {
                //Down sampling in loop mode
                if(m_vecAverage.rows() != m_data[0].rows())
                    m_vecAverage = m_data[m_iCurrentSample%m_data.size()];
                else
                    m_vecAverage += m_data[m_iCurrentSample%m_data.size()];
            }
            else
            {
                //Down sampling in stream mode
                if(m_vecAverage.rows() != m_data[0].rows())
                    m_vecAverage = m_data.front();
                else
                    m_vecAverage += m_data.front();

                m_data.pop_front();
            }
            ++m_iCurrentSample;

            if(m_iCurrentSample%m_iAverageSamples == 0)
            {
                m_vecAverage /= (double)m_iAverageSamples;
                emit stcSample(m_vecAverage);
                m_vecAverage = VectorXd::Zero(m_vecAverage.rows());
            }
        }

        QThread::usleep(m_iUSecIntervall);
    }
}


//*************************************************************************************************************

void ClustStcWorker::setAverage(qint32 samples)
{
    m_iAverageSamples = samples;
}


//*************************************************************************************************************

void ClustStcWorker::setInterval(int usec)
{
    m_iUSecIntervall = usec;
}


//*************************************************************************************************************

void ClustStcWorker::setLoop(bool looping)
{
    m_bIsLooping = looping;
}
