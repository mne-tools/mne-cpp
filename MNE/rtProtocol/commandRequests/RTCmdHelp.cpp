/**
 * @author  Christof Pieloth
 */

#include "RTDefaultCommands.h"
#include "RTCmdHelp.h"

namespace RTSTREAMING
{

    RTCmdHelp::RTCmdHelp() :
                    RTCommandRequest( Command::HELP )
    {
    }

    RTCmdHelp::~RTCmdHelp()
    {
    }

    QString RTCmdHelp::getHelpText() const
    {
        QString help = m_cmd;
        help.append( DESC_SEPARATOR );
        help.append( "Sends and prints this command list." );
        return help;
    }

} /* namespace RTSTREAMING */
