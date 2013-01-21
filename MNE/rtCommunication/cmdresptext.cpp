/**
 * @author  Christof Pieloth
 */


#include "cmdresptext.h"

using namespace RTCOMMUNICATIONLIB;

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

