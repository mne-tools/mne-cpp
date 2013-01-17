/**
 * @author  Christof Pieloth
 */

#include "../RTDefaultCommands.h"

#include "RTCmdSelCon.h"

namespace RTSTREAMING
{

    const int RTCmdSelCon::NO_CONNECTOR_ID = -1;

    RTCmdSelCon::RTCmdSelCon() :
                    RTCmdBase( Command::SELECT_CONNECTOR )
    {
    }

    RTCmdSelCon::~RTCmdSelCon()
    {
    }

    QString RTCmdSelCon::getHelpText() const
    {
        QString help = m_cmd;
        help.append( ARG_SEPARATOR );
        help.append( "[Connector ID]" );
        help.append( DESC_SEPARATOR );
        help.append( "Selects a new connector, if a measurement is running it will be stopped." );
        return help;
    }

    int RTCmdSelCon::getConnectorId() const
    {
        if( m_args.empty() )
        {
            return NO_CONNECTOR_ID;
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
            return NO_CONNECTOR_ID;
        }
    }

    void RTCmdSelCon::setConnectorId( int id )
    {
        m_args.clear();
        m_args.push_back( CommandArgT::number( id ) );
    }
} /* namespace RTSTREAMING */
