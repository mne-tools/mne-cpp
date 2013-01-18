/**
 * @author  Christof Pieloth
 */

#include "CmdReqUnkown.h"

namespace RTSTREAMING
{

const ICommandRequest::CommandT CmdReqUnkown::COMMAND = "UNKOWN";

CmdReqUnkown::CmdReqUnkown() :
        CmdReqBase(CmdReqUnkown::COMMAND)
{
}

CmdReqUnkown::~CmdReqUnkown()
{
}

QString CmdReqUnkown::getHelpText() const
{
    return "Unknown command!";
}

ICommandRequest::CommandRequestT CmdReqUnkown::getRequestPacket() const
{
    // TODO(pieloth): Create JSON
    return "unknown";
}

} /* namespace RTSTREAMING */
