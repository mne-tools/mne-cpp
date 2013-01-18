/**
 * @author   Christof Pieloth
 */

#include <QHash>
#include <QString>

#include <rt_communication/ICommandRequest.h>
#include <rt_communication/ICommandResponse.h>
#include <rt_communication/command_responses/CmdRespText.h>

#include "../../mne_rt_server/ConnectorManager.h"
#include "../../mne_rt_server/CommandServer.h"
#include "../../mne_rt_server/DataServer.h"

#include "command_requests/CmdReqSelCon.h"
#include "RequestActions.h"

namespace RTSTREAMING
{

RequestActions::RequestActions()
{
}

RequestActions::~RequestActions()
{
}

void RequestActions::conlist(ICommandRequest::SPtr req)
{
    QString helpText;
    QHash<int, IConnector_new::SPtr> connectors =
            ConnectorManager::getInstance()->getAvailableConnectors();
    QHash<int, IConnector_new::SPtr>::iterator it = connectors.begin();
    for (; it != connectors.end(); ++it)
    {
        helpText.append(it.key());
        helpText.append("   ");
        helpText.append(it.value()->getName());
        helpText.append("\n");
    }

    CmdRespText::SPtr resp(new CmdRespText(helpText));
    CommandServer::getInstance()->sendResponse(resp);
}

void RequestActions::help(ICommandRequest::SPtr req)
{
    QString helpText;
    IConnector_new::SPtr con;
    ICommandRequestManager::SPtr conReqManager;
    QSet<ICommandRequest::SPtr> commands;

    con = ConnectorManager::getInstance()->getDefaultConnector();
    conReqManager = con->getCommandRequestManager();
    commands.intersect(conReqManager->getAvailableRequests());

    con = ConnectorManager::getInstance()->getSelectedConnector();
    conReqManager = con->getCommandRequestManager();
    commands.intersect(conReqManager->getAvailableRequests());

    QSet<ICommandRequest::SPtr>::iterator it = commands.begin();
    for (; it != commands.end(); ++it)
    {
        helpText.append((*it)->getCommand());
        helpText.append("   ");
        helpText.append((*it)->getHelpText());
        helpText.append("\n");
    }

    CmdRespText::SPtr resp(new CmdRespText(helpText));
    CommandServer::getInstance()->sendResponse(resp);
}

void RequestActions::close(ICommandRequest::SPtr req)
{
    DataServer::getInstance()->stop();
    CommandServer::getInstance()->stop();
}

void RequestActions::selcon(ICommandRequest::SPtr req)
{
    CmdReqSelCon::SPtr cmdReq = req.dynamicCast<CmdReqSelCon>();
    if (cmdReq.isNull())
    {
        // error, wrong req object
        return;
    }

    // TODO(cpieloth): may do some error checks, if id is available and so ...
    ConnectorManager::getInstance()->selectConnector(cmdReq->getConnectorId());
}

} /* namespace RTSTREAMING */
