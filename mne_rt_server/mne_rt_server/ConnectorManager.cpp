/**
 * @author  Christof Pieloth
 */

#include <QMutableHashIterator>

#include "ConnectorManager.h"

#include "../mne_rt_connectors/DefaultConnector/DefaultConnector.h"

namespace RTSTREAMING
{

ConnectorManager::ConnectorManager() :
        m_selectedConnector(-1)
{
    m_defaultConnector = DefaultConnector::SPtr(new DefaultConnector());
    m_defaultConnector->start();
}

ConnectorManager::~ConnectorManager()
{
}

ConnectorManager::SPtr ConnectorManager::getInstance()
{
    if (m_instance.isNull())
    {
        m_instance = ConnectorManager::SPtr(new ConnectorManager());
    }
    return m_instance;
}

IConnector_new::SPtr ConnectorManager::getDefaultConnector() const
{
    return m_defaultConnector;
}

void ConnectorManager::loadConnectors()
{
    // TODO(cpieloth): Load connectors via Qt plugin system
//    for (IConnector::SPtr c : list_from_Qt_loader)
//    {
//        m_connectors.insert(0, c);
//    }
}
void ConnectorManager::detatchConnectors()
{
    QMutableHashIterator<int, IConnector_new::SPtr> it(m_connectors);
    while (it.hasNext())
    {
        it.value()->stop();
        it.remove();
    }
}

QHash<int, IConnector_new::SPtr> ConnectorManager::getAvailableConnectors() const
{
    return m_connectors;
}

bool ConnectorManager::selectConnector(int id)
{
    QHash<int, IConnector_new::SPtr>::const_iterator it = m_connectors.find(id);
    if (it != m_connectors.end())
    {
        return false;
    }
    else
    {
        m_selectedConnector = id;
        return true;
    }
}

IConnector_new::SPtr ConnectorManager::getSelectedConnector() const
{
    QHash<int, IConnector_new::SPtr>::const_iterator it = m_connectors.find(m_selectedConnector);
    return it.value();
}

} /* namespace RTSTREAMING */
