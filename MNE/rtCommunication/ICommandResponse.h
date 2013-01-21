/**
 * @author  Christof Pieloth
 */

#ifndef ICOMMANDRESPONSE_H_
#define ICOMMANDRESPONSE_H_

#include <QObject>
#include <QSharedPointer>

namespace RTCOMMUNICATIONLIB
{
/**
 * Interface for a command response. This is usually sent from server to client.
 */
class ICommandResponse: public QObject
{
Q_OBJECT
public:
    // TODO(cpieloth): Should later be changed to a JSON-like type
    typedef QString CommandResponseT; /**< Low-level data type for a response packet. */

    typedef QSharedPointer<ICommandResponse> SPtr;
    typedef QSharedPointer<const ICommandResponse> ConstSPtr;

    virtual ~ICommandResponse()
    {
    }

    /**
     * Creates a ready-to-send object for the server. This method hides the JSON serialization.
     *
     * @return Response data to send to the client.
     */
    virtual CommandResponseT getResponsePacket() const = 0;

signals:
    /**
     * A client can connect some logic or action for this response (optional).
     *
     * @param   resp Instace of the incoming response.
     */
    void handle(ICommandResponse::ConstSPtr resp);

    // TODO(cpieloth): Add useful functions
};
} /* namespace RTSTREAMING */
#endif /* ICOMMANDRESPONSE_H_ */
