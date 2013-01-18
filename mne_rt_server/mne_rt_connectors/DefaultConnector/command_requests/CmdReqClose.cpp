/**
 * @author   Christof Pieloth
 */

#include "CmdReqClose.h"

namespace RTSTREAMING
{

CmdReqClose::CmdReqClose() :
        CmdReqBase(CmdReqClose::COMMAND)
{
}

CmdReqClose::~CmdReqClose()
{
}

QString CmdReqClose::getHelpText() const
{
    return "Closes the server and all connections.";
}

CommandRequestT CmdReqClose::getRequestPacket() const
{
    // TODO(cpieloth): do some JSON here
    return "close";
}

const ICommandRequest::CommandT CmdReqClose::COMMAND = "close";

} /* namespace RTSTREAMING */
