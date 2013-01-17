/**
 * @author  Christof Pieloth
 */

#include "../RTProtocolDefinitions.h"
#include "../RTDefaultCommands.h"

#include "RTCommandRequest.h"
#include "RTCmdConList.h"
#include "RTCmdHelp.h"
#include "RTCmdMeasInfo.h"
#include "RTCmdSelCon.h"
#include "RTCmdStartMeas.h"
#include "RTCmdStopAll.h"
#include "RTCmdUnkown.h"

namespace RTSTREAMING
{
RTCommandRequest::RTCommandRequest(CommandT cmd) :
        m_cmd(cmd)
{
}

RTCommandRequest::~RTCommandRequest()
{
}

CommandT RTCommandRequest::getCommand() const
{
    return m_cmd;
}

bool RTCommandRequest::isCommand(const CommandT& cmd) const
{
    return m_cmd.compare(cmd) == 0;
}

CommandArgListT RTCommandRequest::getArguments() const
{
    return m_args;
}

void RTCommandRequest::setArguments(CommandArgListT args)
{
    m_args = args;
}

void RTCommandRequest::addArgument(CommandArgT arg)
{
    m_args.push_back(arg);
}

CommandRequestT RTCommandRequest::getRequest()
{
    CommandRequestT req = createCommandRequest(m_cmd, m_args);
    return req;
}

RTCommandRequest::SPtr RTCommandRequest::parseRequest(
        const CommandRequestT& req)
{
    RTCommandRequest::SPtr cmdRequest = RTCmdUnkown::SPtr(new RTCmdUnkown());

    CommandT cmd;
    CommandArgListT args;
    if (!parseCommandRequest(req, &cmd, &args))
    {
        return cmdRequest;
    }

    cmdRequest = RTCommandRequest::getDefaultCommandInstance(cmd);
    cmdRequest->setArguments(args);

    return cmdRequest;
}

RTCommandRequest::SPtr RTCommandRequest::getDefaultCommandInstance(
        const CommandT& cmd)
{
    RTCommandRequest::SPtr request;

    if (Command::REQUEST_CONNECTORS.compare(cmd) == 0)
    {
        request = RTCmdConList::SPtr(new RTCmdConList());
        return request;
    }

    if (Command::HELP.compare(cmd) == 0)
    {
        request = RTCmdHelp::SPtr(new RTCmdHelp());
        return request;
    }
    if (Command::REQUEST_MEASUREMENT_INFO.compare(cmd) == 0)
    {
        request = RTCmdMeasInfo::SPtr(new RTCmdMeasInfo());
        return request;
    }
    if (Command::SELECT_CONNECTOR.compare(cmd) == 0)
    {
        request = RTCmdSelCon::SPtr(new RTCmdSelCon());
        return request;
    }
    if (Command::START_MEASUREMENT.compare(cmd) == 0)
    {
        request = RTCmdStartMeas::SPtr(new RTCmdStartMeas());
        return request;
    }
    if (Command::STOP_ALL_MEASUREMENTS.compare(cmd) == 0)
    {
        request = RTCmdStopAll::SPtr(new RTCmdStopAll());
        return request;
    }
    // TODO(pieloth): Create instance of all default commands

    request = RTCmdUnkown::SPtr(new RTCmdUnkown());
    return request;
}

std::list<RTCommandRequest::SPtr> getDefaultCommandRequests()
{
    std::list<CommandT> cmdList = Command::getDefaultCommands();
    std::list<RTCommandRequest::SPtr> reqList;

    std::list<CommandT>::iterator it = cmdList.begin();
    for (; it != cmdList.end(); ++it)
    {
        reqList.push_back(RTCommandRequest::getDefaultCommandInstance(*it));
    }
    return reqList;
}
} /* namespace RTSTREAMING */
