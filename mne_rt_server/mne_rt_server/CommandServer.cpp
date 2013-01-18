/**
 * @author  Christof Pieloth
 */

#include <rt_communication/ICommandRequest.h>
#include <rt_communication/command_requests/CmdReqUnkown.h>

#include "ConnectorManager.h"
#include "ICommandRequestManager.h"
#include "CommandServer.h"

namespace RTSTREAMING
{

CommandServer::CommandServer()
{
}

CommandServer::~CommandServer()
{
}

CommandServer::SPtr CommandServer::getInstance()
{
    if (m_instance.isNull())
    {
        m_instance = CommandServer::SPtr(new CommandServer());
    }
    return m_instance;
}

void CommandServer::sendResponse(ICommandResponse::ConstSPtr resp)
{
    // TODO(cpieloth): Implement network transfer - this must be place into an thread
    ICommandResponse::CommandResponseT msg = resp->getResponsePacket();
    // send it to client
}

void CommandServer::start()
{
    // TODO(cpieloth): Implement
    ConnectorManager::getInstance()->loadConnectors();
}
void CommandServer::stop()
{
    // TODO(cpieloth): Implement
    ConnectorManager::getInstance()->detatchConnectors();
}

// TODO(cpieloth): this method is may not needed.
void CommandServer::run()
{
    // TODO(cpieloth): Implement - this must be place into an thread
    ICommandRequest::CommandRequestT msg;
    ICommandRequest::SPtr req;
    ICommandRequestManager::SPtr cmdReqManager;
    // while(!m_isStopped)
    {
        // ICommandRequest::CommandRequestT msg = TcpIp.read...;
        req = CmdReqUnkown::SPtr(new CmdReqUnkown());
        cmdReqManager =
                ConnectorManager::getInstance()->getDefaultConnector()->getCommandRequestManager();
        req = cmdReqManager->parse(msg);
        if (CmdReqUnkown::COMMAND.compare(req->getCommand()) == 0)
        {
            cmdReqManager =
                    ConnectorManager::getInstance()->getSelectedConnector()->getCommandRequestManager();
            req = cmdReqManager->parse(msg);
        }
        req->handle(req);
    }
}

} /* namespace RTSTREAMING */
