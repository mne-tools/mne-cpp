/**
 * @author   Christof Pieloth
 */

#ifndef DEFAULTCMDREQMANAGER_H_
#define DEFAULTCMDREQMANAGER_H_

#include <QSharedPointer>

#include <rt_communication/ICommandRequest.h>

#include "../../mne_rt_server/ICommandRequestManager.h"

namespace RTSTREAMING
{
/**
 * CommandRequestManager for the default connector.
 * Available commands: help, close, conlist, selcon
 */
class DefaultCmdReqManager: public RTSTREAMING::ICommandRequestManager
{
public:
    typedef QSharedPointer<DefaultCmdReqManager> SPtr;
    typedef QSharedPointer<const DefaultCmdReqManager> ConstSPtr;

    DefaultCmdReqManager();
    virtual ~DefaultCmdReqManager();

    virtual ICommandRequest::SPtr parse(ICommandRequest::CommandRequestT req) const;

    virtual QSet<ICommandRequest::SPtr> getAvailableRequests() const;
};

} /* namespace RTSTREAMING */
#endif /* DEFAULTCMDREQMANAGER_H_ */
