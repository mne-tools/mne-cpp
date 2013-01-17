/**
 * @author  Christof Pieloth
 */

#include "../RTDefaultCommands.h"
#include "RTCmdConList.h"

namespace RTSTREAMING
{

    RTCmdConList::RTCmdConList() :
                    RTCmdBase( Command::REQUEST_CONNECTORS )
    {
    }

    RTCmdConList::~RTCmdConList()
    {
    }

    QString RTCmdConList::getHelpText() const
    {
        QString help = m_cmd;
        help.append( DESC_SEPARATOR );
        help.append( "Sends and prints all available connectors." );
        return help;
    }

} /* namespace RTSTREAMING */
