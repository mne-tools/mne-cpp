#include "realtimemultisamplearrayworker.h"

#include <iostream>


RealTimeMultiSampleArrayWorker::RealTimeMultiSampleArrayWorker(QObject *parent)
: QObject(parent)
, m_iCurrentSample(0)
, m_iDownSampling(10)
{
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWorker::addData(const QVector<VectorXd> &data)
{
    std::cout << "RealTimeMultiSampleArrayWorker::addData \n";
    m_qMutex.lock();
    for(qint32 i = 0; i < data.size(); ++i)
        m_data.append(data[i]);
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWorker::clear()
{
    m_qMutex.lock();
    m_data.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWorker::process()
{
    while(true)
    {
        m_qMutex.lock();
        if(!m_data.isEmpty())
        {
            if(m_iCurrentSample%m_iDownSampling == 0)
                emit stcSample(m_data.front());

            m_data.pop_front();
            ++m_iCurrentSample;
        }
        m_qMutex.unlock();
    }
}



//*************************************************************************************************************

void RealTimeMultiSampleArrayWorker::setDownSampling(qint32 ds)
{
    m_qMutex.lock();
    m_iDownSampling = ds;
    clear();
    m_qMutex.unlock();
}
