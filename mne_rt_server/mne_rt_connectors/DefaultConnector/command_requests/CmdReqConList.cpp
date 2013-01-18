/**
 * @author   Christof Pieloth
 */

#include "CmdReqConList.h"

namespace RTSTREAMING
{

CmdReqConList::CmdReqConList() :
        CmdReqBase(CmdReqConList::COMMAND)
{
}

CmdReqConList::~CmdReqConList()
{
}

QString CmdReqConList::getHelpText() const
{
    return "Sends and prints all available connectors.";
}

ICommandRequest::CommandRequestT CmdReqConList::getRequestPacket() const
{
    // TODO(cpieloth): place JSON here
    return "conlist";
}

const ICommandRequest::CommandT CmdReqConList::COMMAND = "conlist";

} /* namespace RTSTREAMING */
