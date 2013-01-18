/**
 * @author   Christof Pieloth
 */

#include "CmdReqHelp.h"

namespace RTSTREAMING
{

const ICommandRequest::CommandT CmdReqHelp::COMMAND = "help";

CmdReqHelp::CmdReqHelp() :
        CmdReqBase(CmdReqHelp::COMMAND)
{
}

CmdReqHelp::~CmdReqHelp()
{
}

QString CmdReqHelp::getHelpText() const
{
    return "Sends and prints this help message.";
}

CommandRequestT CmdReqHelp::getRequestPacket() const
{
    // TODO(cpieloth): do some JSON here
    return "help";
}

} /* namespace RTSTREAMING */
