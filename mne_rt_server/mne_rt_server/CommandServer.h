/**
 * @author  Christof Pieloth
 */

#ifndef COMMANDSERVER_H_
#define COMMANDSERVER_H_

#include <QSharedPointer>

#include <rt_communication/ICommandResponse.h>

namespace RTSTREAMING
{

// TODO(cpieloth): ATTENTION Old CommandServer exists
/**
 * The command server who handles all incomming requests.
 * This class is a singleton, because there should exist only one command server instance.
 */
class CommandServer
{
public:
    typedef QSharedPointer<CommandServer> SPtr;
    typedef QSharedPointer<const CommandServer> ConstSPtr;

    /**
     * Gets a instance of the command server. If no instance was created, a new one is created.
     *
     * @return  A instance of a command server.
     */
    static CommandServer::SPtr getInstance();

    virtual ~CommandServer();

    /**
     * Sends a response to a client. This method can be used by a connector.
     *
     * @param   resp Response to send.
     */
    void sendResponse(ICommandResponse::ConstSPtr resp);

    /**
     * Starts and initializes the server.
     */
    void start();

    /**
     * Stops the server.
     */
    void stop();

private:
    static CommandServer::SPtr m_instance; /**< Singleton instance. */

    CommandServer();

    // TODO(cpieloth): maybe, we don't need this method.
    /**
     * Implementation of the network stuff. Should be moved to non-blocking thread.
     */
    void run();
};

} /* namespace RTSTREAMING */
#endif /* COMMANDSERVER_H_ */
