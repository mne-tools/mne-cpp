/**
 * @author  Christof Pieloth
 */

#include "../RTDefaultCommands.h"
#include "RTCmdStartMeas.h"

namespace RTSTREAMING
{
    const int RTCmdStartMeas::NO_CLIENT_ID = -1;

    RTCmdStartMeas::RTCmdStartMeas() :
                    RTCommandRequest( Command::START_MEASUREMENT )
    {
    }

    RTCmdStartMeas::~RTCmdStartMeas()
    {
    }

    QString RTCmdStartMeas::getHelpText() const
    {
        QString help = m_cmd;
        help.append( ARG_SEPARATOR );
        help.append( "[Client ID]" );
        help.append( DESC_SEPARATOR );
        help.append(
                        "Adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already started, it is triggered." );
        return help;
    }

    int RTCmdStartMeas::getClientId() const
    {
        if( m_args.empty() )
        {
            return NO_CLIENT_ID;
        }

        CommandArgT strId = m_args.front();
        bool ok;
        int id = strId.toInt( &ok );

        if( ok )
        {
            return id;
        }
        else
        {
            return NO_CLIENT_ID;
        }
    }

    void RTCmdStartMeas::setClientId( int id )
    {
        m_args.clear();
        m_args.push_back( CommandArgT::number( id ) );
    }

} /* namespace RTSTREAMING */
