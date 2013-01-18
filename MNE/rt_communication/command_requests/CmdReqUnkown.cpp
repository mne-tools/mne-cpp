/**
 * @author  Christof Pieloth
 */

#include "CmdReqUnkown.h"

namespace RTSTREAMING
{

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
