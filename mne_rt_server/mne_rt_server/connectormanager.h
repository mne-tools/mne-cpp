/**
 * @author  Christof Pieloth
 */

#ifndef CONNECTORMANAGER_H_
#define CONNECTORMANAGER_H_

#include <QHash>
#include <QSharedPointer>

#include "IConnectornew.h"

namespace RTSTREAMING
{
// TODO(cpieloth): ATTENTION Old CommandServer exists,
/**
 * The connector manager for the server. This manager loads all available connectors and provides access to these.
 * This class is a singleton, because there should exist only one connector manager instance.
 */
class ConnectorManager
{
public:
    typedef QSharedPointer<ConnectorManager> SPtr;
    typedef QSharedPointer<const ConnectorManager> ConstSPtr;

    /**
     * Gets a instance of the connector manager. If no instance was created, a new one is created.
     *
     * @return  A instance of a connector manager.
     */
    static ConnectorManager::SPtr getInstance();

    virtual ~ConnectorManager();

    /**
     * Gets the default connector for the basic server interaction.
     *
     * @return  A default connector.
     */
    IConnector_new::SPtr getDefaultConnector() const;

    /**
     * Loads all connectors with the help of Qt's plugin system.
     */
    void loadConnectors();

    /**
     * Stops all connectors and removes them from the internal list.
     */
    void detatchConnectors();

    /**
     * Returns all available connectors, except the default connector.
     *
     * @return  A map<ID, Connector>.
     */
    QHash<int, IConnector_new::SPtr> getAvailableConnectors() const;

    /**
     * Selects an available connector.
     *
     * @return  true if connector was found, else false.
     */
    bool selectConnector(int id);

    /**
     * Gets the selected connector.
     *
     * @return  Selected connector.
     */
    IConnector_new::SPtr getSelectedConnector() const;

private:
    ConnectorManager();
    static ConnectorManager::SPtr m_instance; /**< Singleton instance. */

    IConnector_new::SPtr m_defaultConnector;

    int m_selectedConnector;
    QHash<int, IConnector_new::SPtr> m_connectors;
};

} /* namespace RTSTREAMING */
#endif /* CONNECTORMANAGER_H_ */
