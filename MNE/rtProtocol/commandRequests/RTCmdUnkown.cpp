/**
 * @author  Christof Pieloth
 */

#include "RTDefaultCommands.h"
#include "RTCmdUnkown.h"

namespace RTSTREAMING
{
    const CommandT COMMAND = "UNKOWN";

    RTCmdUnkown::RTCmdUnkown() :
                    RTCommandRequest( COMMAND )
    {
    }

    RTCmdUnkown::~RTCmdUnkown()
    {
    }

    QString RTCmdUnkown::getHelpText() const
    {
        QString help = m_cmd;
        help.append( DESC_SEPARATOR );
        help.append( "Placeholder for unkown commands!" );
        return help;
    }

} /* namespace RTSTREAMING */
