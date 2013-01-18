/**
 * @author   Christof Pieloth
 */

#include "CmdReqSelCon.h"

namespace RTSTREAMING
{

const ICommandRequest::CommandT CmdReqSelCon::COMMAND = "selcon";
const int CmdReqSelCon::NO_CONNECTOR_ID = -1;

CmdReqSelCon::CmdReqSelCon() :
        m_connectorId(NO_CONNECTOR_ID), CmdReqBase(COMMAND)
{
}

CmdReqSelCon::~CmdReqSelCon()
{
}

QString CmdReqSelCon::getHelpText() const
{
    return "Selects a new connector, if a measurement is running it will be stopped.";
}

CommandRequestT CmdReqSelCon::getRequestPacket() const
{
    CommandRequestT msg;
    msg.append(getCommand());
    msg.append(" ");
    msg.append(m_connectorId);
    return msg;
}

void CmdReqSelCon::setConnectorId(int id)
{
    m_connectorId = id;
}

int CmdReqSelCon::getConnectorId() const
{
    return m_connectorId;
}

} /* namespace RTSTREAMING */
