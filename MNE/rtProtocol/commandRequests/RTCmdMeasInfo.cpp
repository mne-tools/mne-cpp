/**
 * @author  Christof Pieloth
 */

#include "RTDefaultCommands.h"
#include "RTCmdMeasInfo.h"

namespace RTSTREAMING
{
    const int RTCmdMeasInfo::NO_CLIENT_ID = -1;

    RTCmdMeasInfo::RTCmdMeasInfo() :
                    RTCommandRequest( Command::REQUEST_MEASUREMENT_INFO )
    {
    }

    RTCmdMeasInfo::~RTCmdMeasInfo()
    {
    }

    QString RTCmdMeasInfo::getHelpText() const
    {
        QString help = m_cmd;
        help.append( ARG_SEPARATOR );
        help.append( "[ID/Alias]" );
        help.append( DESC_SEPARATOR );
        help.append( "Sends measurement info to a specified client." );
        return help;
    }

    int RTCmdMeasInfo::getClientId()
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

    void RTCmdMeasInfo::setClientId( int id )
    {
        m_args.clear();
        m_args.push_back( CommandArgT::number( id ) );
    }

} /* namespace RTSTREAMING */
