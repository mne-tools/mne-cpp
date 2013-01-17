/**
 * @author  Christof Pieloth
 */

#include "../RTDefaultCommands.h"
#include "RTCmdUnkown.h"

namespace RTSTREAMING
{
    const CommandT RTCmdUnkown::COMMAND = "UNKOWN";

    RTCmdUnkown::RTCmdUnkown() :
                    RTCmdBase( COMMAND )
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
