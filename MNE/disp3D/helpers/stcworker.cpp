#include "stcworker.h"

#include <QDebug>


StcWorker::StcWorker(QObject *parent)
: QObject(parent)
, m_bIsLooping(true)
, m_iAverageSamples(10)
, m_iCurrentSample(0)
, m_iUSecIntervall(100)
{
}


//*************************************************************************************************************

void StcWorker::addData(QList<VectorXd> &data)
{
    qDebug() << "addData" << data.size();
    m_qMutex.lock();
    m_data.append(data);
    m_qMutex.unlock();
}


//*************************************************************************************************************

void StcWorker::clear()
{
    m_qMutex.lock();
    m_data.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

void StcWorker::process()
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

//                qDebug() << "non Loop" << m_iCurrentSample;
                m_data.pop_front();
            }
            ++m_iCurrentSample;

            if(m_iCurrentSample%m_iAverageSamples == 0)
            {
//                qDebug() << "emit" << m_iAverageSamples;
                m_vecAverage /= (double)m_iAverageSamples;
                emit stcSample(m_vecAverage);
                m_vecAverage = VectorXd::Zero(m_vecAverage.rows());
            }
        }

        QThread::usleep(m_iUSecIntervall);
    }
}


//*************************************************************************************************************

void StcWorker::setAverage(qint32 samples)
{
    m_iAverageSamples = samples;
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
