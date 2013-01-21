/**
 * @author  Christof Pieloth
 */

#include "cmdreqbase.h"

using namespace RTCOMMUNICATIONLIB;

CmdReqBase::CmdReqBase(CommandT cmd)
{
    m_cmd = cmd;
}

CmdReqBase::~CmdReqBase()
{
}

ICommandRequest::CommandT CmdReqBase::getCommand() const
{
    return m_cmd;
}
