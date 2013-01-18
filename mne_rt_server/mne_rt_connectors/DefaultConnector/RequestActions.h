/**
 * @author   Christof Pieloth
 */

#ifndef REQUESTACTIONS_H_
#define REQUESTACTIONS_H_

#include <QSharedPointer>

#include <rt_communication/ICommandRequest.h>

namespace RTSTREAMING
{
/**
 * Implementation of the logic for each request. You could create a class for each method, too.
 * The methods are connected via SIGNAL-SLOT with ICommandRequest by DefaultCmdReqManager.
 */
class RequestActions
{
public:
    typedef QSharedPointer<RequestActions> SPtr;
    typedef QSharedPointer<const RequestActions> ConstSPtr;

    RequestActions();
    virtual ~RequestActions();

    void conlist(ICommandRequest::SPtr req);

    void help(ICommandRequest::SPtr req);

    void close(ICommandRequest::SPtr req);

    void selcon(ICommandRequest::SPtr req);
};

} /* namespace RTSTREAMING */
#endif /* REQUESTACTIONS_H_ */
