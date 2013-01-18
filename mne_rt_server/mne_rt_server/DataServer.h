/**
 * @author  Christof Pieloth
 */

#ifndef DATASERVER_H_
#define DATASERVER_H_

#include <QSharedPointer>

namespace RTSTREAMING
{
/**
 * The data server who sends data to the clients.
 * This class is a singleton, because there should exist only one data server instance.
 */
class DataServer
{
public:

    typedef QSharedPointer<DataServer> SPtr;
    typedef QSharedPointer<const DataServer> ConstSPtr;

    /**
     * Gets a instance of the data server. If no instance was created, a new one is created.
     *
     * @return  A instance of a data server.
     */
    static DataServer::SPtr getInstance();

    virtual ~DataServer();

    // TODO(cpieloth): choose data type
    /**
     * Sends data to the clients. This method can be used by a connector.
     */
    void sendData();

    /**
     * Starts and initializes the data server.
     */
    void start();

    /**
     * Stops the data server.
     */
    void stop();

private:
    static DataServer::SPtr m_instance; /**< Singleton instance. */

    DataServer();

    // TODO(cpieloth): maybe, we don't need this method.
    /**
     * Implementation of the network stuff. Should be moved to non-blocking thread.
     */
    void run();
};

} /* namespace RTSTREAMING */
#endif /* DATASERVER_H_ */
