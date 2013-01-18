/**
 * @author  Christof Pieloth
 */

#include "CmdReqBase.h"

namespace RTSTREAMING
{

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

} /* namespace RTSTREAMING */
