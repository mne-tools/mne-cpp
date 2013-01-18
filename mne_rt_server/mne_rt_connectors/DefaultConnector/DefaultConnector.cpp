/**
 * @author   Christof Pieloth
 */

#include "DefaultCmdReqManager.h"
#include "DefaultConnector.h"

namespace RTSTREAMING
{

DefaultConnector::DefaultConnector() :
        m_active(false)
{
    m_cmdReqManager = DefaultCmdReqManager::SPtr(new DefaultCmdReqManager());
}

DefaultConnector::~DefaultConnector()
{
}

QString DefaultConnector::getName() const
{
    return "DefaultConnector";
}

ICommandRequestManager::SPtr DefaultConnector::getCommandRequestManager() const
{
    return m_cmdReqManager;
}

void DefaultConnector::start()
{
    m_active = true;
}

bool DefaultConnector::isActive() const
{
    return m_active;
}

void DefaultConnector::stop()
{
    m_active = false;
}
} /* namespace RTSTREAMING */
