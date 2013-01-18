/**
 * @author  Christof Pieloth
 */

#ifndef ICOMMANDREQUESTPARSER_H_
#define ICOMMANDREQUESTPARSER_H_

#include <QSet>
#include <QSharedPointer>

#include <rt_communication/ICommandRequest.h>

namespace RTSTREAMING
{
/**
 * This class holds a set of requests, which can be interpreted by the related connector.
 * An implementation should also connect/link the action/logic to a known request.
 */
class ICommandRequestManager
{
public:
    typedef QSharedPointer<ICommandRequestManager> SPtr;
    typedef QSharedPointer<const ICommandRequestManager> ConstSPtr;

    virtual ~ICommandRequestManager();

    /**
     * Tries to parse the incomming request.
     *
     * @param   req Raw input data.
     *
     * @return  A concrete instance of a ICommandRequest or CmdReqUnknown.
     */
    virtual ICommandRequest::SPtr parse(
            ICommandRequest::CommandRequestT req) const = 0;

    /**
     * Returns a set of all commands, which can be handle by this implementation.
     */
    virtual QSet<ICommandRequest::SPtr> getAvailableRequests() const = 0;
};

} /* namespace RTSTREAMING */
#endif /* ICOMMANDREQUESTPARSER_H_ */
