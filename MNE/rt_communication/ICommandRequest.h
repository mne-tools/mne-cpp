/**
 * @author  Christof Pieloth
 */

#ifndef ICOMMANDREQUEST_H_
#define ICOMMANDREQUEST_H_

#include <QObject>
#include <QSharedPointer>
#include <QString>

namespace RTSTREAMING
{
/**
 * Interface for a command request. This is usually sent from client to server.
 */
class ICommandRequest: public QObject
{
Q_OBJECT
public:
    typedef QString CommandT; /**< Type for the short command representation. */
    // TODO(cpieloth): Should later be changed to a JSON-like type
    typedef QString CommandRequestT; /**< Low-level data type for a request packet. */

    typedef QSharedPointer<ICommandRequest> SPtr;
    typedef QSharedPointer<const ICommandRequest> ConstSPtr;

    // Each implementation should have this static variable.
    // static const CommandT COMMAND;

    virtual ~ICommandRequest()
    {
    }

    /**
     * Gets the short command for this request. Can be used to identify the concrete implementation.
     *
     * @return Short command representation.
     */
    virtual CommandT getCommand() const = 0;

    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    virtual QString getHelpText() const = 0;

    /**
     * Creates a ready-to-send object for the client. This method hides the JSON serialization.
     *
     * @return Request data to send to the server.
     */
    virtual CommandRequestT getRequestPacket() const = 0;

    // In your implementation you should add some getter and setter for arguments.

signals:
    /**
     * The server can connect some logic or action for this request (optional).
     *
     * @param   req Instace of the incoming request.
     */
    void handle(ICommandRequest::ConstSPtr req);

};

// Each implementation should have this static variable.
// const ICommandRequest::CommandT MyRequest::COMMAND = "mycommand";
}

#endif /* ICOMMANDREQUEST_H_ */
