/**
 * @author  Christof Pieloth
 */


#include "CmdRespText.h"

namespace RTSTREAMING
{

CmdRespText::CmdRespText(CommandResponseT resp) : m_cmdResp(resp)
{
}

CmdRespText::~CmdRespText()
{
}

ICommandResponse::CommandResponseT CmdRespText::getResponsePacket() const
{
    return m_cmdResp;
}

} /* namespace RTSTREAMING */
