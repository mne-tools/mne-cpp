/**
 * @author  Christof Pieloth
 */

#include "DataServer.h"

namespace RTSTREAMING
{

DataServer::DataServer()
{
}

DataServer::~DataServer()
{
}

DataServer::SPtr DataServer::getInstance()
{
    if (m_instance.isNull())
    {
        m_instance = DataServer::SPtr(new DataServer());
    }
    return m_instance;
}

} /* namespace RTSTREAMING */
