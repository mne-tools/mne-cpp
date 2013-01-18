/**
 * @author   Christof Pieloth
 */
#include <rt_communication/command_requests/CmdReqUnkown.h>

#include "command_requests/CmdReqConList.h"
#include "command_requests/CmdReqHelp.h"
#include "command_requests/CmdReqClose.h"
#include "command_requests/CmdReqSelCon.h"
#include "RequestActions.h"

#include "DefaultCmdReqManager.h"

namespace RTSTREAMING
{

DefaultCmdReqManager::DefaultCmdReqManager()
{
}

DefaultCmdReqManager::~DefaultCmdReqManager()
{
}

ICommandRequest::SPtr DefaultCmdReqManager::parse(
        ICommandRequest::CommandRequestT req) const
{
    ICommandRequest::SPtr cmdReq = CmdReqUnkown::SPtr(new CmdReqUnkown());
    // TODO(cpieloth): you have to use your deserializer here in some way

    // NOTE:
    // We do the connect of request->action here, due to keep request representation (JSON) decoupled with logic/action.
    // So an external client program can use all and linked request in "command_request".
    // How you connect the action methods is on you. You can put the methos in one file, or you can create a class for each.
    RequestActions::SPtr action(new RequestActions());
    if (req.startsWith(CmdReqConList::COMMAND))
    {
        cmdReq = CmdReqConList::SPtr(new CmdReqConList());

        // TODO(cpieloth): connect request.handle with action method
//        connect(cmdReq, &CmdReqConList::handle(ICommandRequest::SPtr), action,
//                &RequestActions::conlist(ICommandRequest::SPtr));
        return cmdReq;
    }

    if (req.startsWith(CmdReqHelp::COMMAND))
    {
        cmdReq = CmdReqHelp::SPtr(new CmdReqHelp());

        // TODO(cpieloth): connect request.handle with action method
        //        connect(cmdReq, &CmdReqConList::handle(ICommandRequest::SPtr), action,
        //                &RequestActions::help(ICommandRequest::SPtr));
        return cmdReq;
    }

    if (req.startsWith(CmdReqClose::COMMAND))
    {
        cmdReq = CmdReqClose::SPtr(new CmdReqClose());

        // TODO(cpieloth): connect request.handle with action method
        //        connect(cmdReq, &CmdReqConList::handle(ICommandRequest::SPtr), action,
        //                &RequestActions::close(ICommandRequest::SPtr));
        return cmdReq;
    }

    if (req.startsWith(CmdReqSelCon::COMMAND))
    {
        cmdReq = CmdReqSelCon::SPtr(new CmdReqSelCon());

        // TODO(cpieloth): connect request.handle with action method
        //        connect(cmdReq, &CmdReqConList::handle(ICommandRequest::SPtr), action,
        //                &RequestActions::selcon(ICommandRequest::SPtr));
        return cmdReq;
    }

    // TODO(cpieloth): check for all other request for this connector

    // return unkown
    return cmdReq;
}

QSet<ICommandRequest::SPtr> DefaultCmdReqManager::getAvailableRequests() const
{
    QSet<ICommandRequest::SPtr> set;
    ICommandRequest::SPtr req;

    req = CmdReqConList::SPtr(new CmdReqConList());
    set.insert(req);

    req = CmdReqHelp::SPtr(new CmdReqHelp());
    set.insert(req);

    req = CmdReqClose::SPtr(new CmdReqClose());
    set.insert(req);

    req = CmdReqSelCon::SPtr(new CmdReqSelCon());
    set.insert(req);

    return set;
}

} /* namespace RTSTREAMING */
