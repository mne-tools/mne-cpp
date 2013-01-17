/**
 * @author  Christof Pieloth
 */

#include "../RTDefaultCommands.h"
#include "RTCmdStopAll.h"

namespace RTSTREAMING
{

    RTCmdStopAll::RTCmdStopAll() :
                    RTCmdBase( Command::STOP_ALL_MEASUREMENTS )
    {
    }

    RTCmdStopAll::~RTCmdStopAll()
    {
    }

    QString RTCmdStopAll::getHelpText() const
    {
        QString help = m_cmd;
        help.append( DESC_SEPARATOR );
        help.append( "Stops the whole acquisition process." );
        return help;
    }
} /* namespace RTSTREAMING */
