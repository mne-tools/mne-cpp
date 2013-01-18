/**
 * @author  Christof Pieloth
 */

#ifndef ICONNECTORNEW_H_
#define ICONNECTORNEW_H_

#include <QSharedPointer>
#include <QString>

#include "CommandServer.h"
#include "DataServer.h"
#include "ICommandRequestManager.h"

namespace RTSTREAMING
{
// TODO(pieloth): Rename to IConnector
/**
 * Interface for all connectors which should be loaded dynamically.
 * A connector provides new commands and the logic or actions for this commands.
 */
class IConnector_new
{
public:
    typedef QSharedPointer<IConnector_new> SPtr;
    typedef QSharedPointer<const IConnector_new> ConstSPtr;

    virtual ~IConnector_new();

    /**
     * Gets the name of this connecors.
     *
     * @return  The name.
     */
    virtual QString getName() const = 0;

    /**
     * Gets the CommandRequestManager of this connectors.
     * The server uses this to interact with the connector.
     *
     * @return  Instance of ICommandRequestManager.
     */
    virtual ICommandRequestManager::SPtr getCommandRequestManager() const = 0;

    /**
     * Starts the connector.
     */
    virtual void start() = 0;

    /**
     * Indicates if the connector is running.
     */
    virtual bool isActive() const = 0;

    /**
     * Stops the connector.
     */
    virtual void stop() = 0;
};

} /* namespace RTSTREAMING */
#endif /* ICONNECTORNEW_H_ */
