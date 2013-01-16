/**
 * @author  Christof Pieloth
 */

#include <QStringList>

#include "../RTDefaultCommands.h"

#include "RTCommandRequest.h"
#include "RTCmdHelp.h"
#include "RTCmdMeasInfo.h"
#include "RTCmdUnkown.h"

namespace RTSTREAMING
{
    RTCommandRequest::RTCommandRequest( CommandT cmd ) :
                    m_cmd( cmd )
    {
    }

    RTCommandRequest::~RTCommandRequest()
    {
    }

    CommandT RTCommandRequest::getCommand() const
    {
        return m_cmd;
    }

    CommandArgListT RTCommandRequest::getArguments() const
    {
        return m_args;
    }

    void RTCommandRequest::setArguments( CommandArgListT args )
    {
        m_args = args;
    }

    void RTCommandRequest::addArgument( CommandArgT arg )
    {
        m_args.push_back( arg );
    }

    CommandRequestT RTCommandRequest::getRequest()
    {
        CommandRequestT request;
        request.append( m_cmd );
        if( !m_args.empty() )
        {
            CommandArgListT::iterator it = m_args.begin();
            for( ; it != m_args.end(); ++it )
            {
                request.append( ARG_SEPARATOR );
                request.append( *it );
            }
        }
        request.append( ETX );
        return request;
    }

    RTCommandRequest::SPtr RTCommandRequest::parseRequest( const CommandRequestT& req )
    {
        RTCommandRequest::SPtr cmdRequest = RTCmdUnkown::SPtr( new RTCmdUnkown() );
        if( !req.endsWith( ETX ) )
        {
            return cmdRequest;
        }

        CommandRequestT request = req;
        request.remove( ETX );
        // TODO(pieloth): code smell - CommandRequestT -> QStringList
        QStringList splits = request.split( ARG_SEPARATOR );

        if( splits.empty() )
        {
            return cmdRequest;
        }
        QStringList::iterator it = splits.begin();
        const CommandT cmd = *it;

        cmdRequest = RTCommandRequest::getDefaultCommandInstance( cmd );

        ++it;
        for( ; it != splits.end(); ++it )
        {
            cmdRequest->addArgument( *it );
        }

        return cmdRequest;
    }

    RTCommandRequest::SPtr RTCommandRequest::getDefaultCommandInstance( const CommandT& cmd )
    {
        RTCommandRequest::SPtr request;

        if( Command::HELP.compare( cmd ) == 0 )
        {
            request = RTCmdHelp::SPtr( new RTCmdHelp() );
            return request;
        }
        if( Command::REQUEST_MEASUREMENT_INFO.compare( cmd ) == 0 )
        {
            request = RTCmdMeasInfo::SPtr( new RTCmdMeasInfo() );
            return request;
        }
        // TODO(pieloth): Create instance of all default commands

        request = RTCmdUnkown::SPtr( new RTCmdUnkown() );
        return request;
    }

    std::list< RTCommandRequest::SPtr > getDefaultCommandRequests()
    {
        std::list< CommandT > cmdList = Command::getDefaultCommands();
        std::list< RTCommandRequest::SPtr > reqList;

        std::list< CommandT >::iterator it = cmdList.begin();
        for( ; it != cmdList.end(); ++it )
        {
            reqList.push_back( RTCommandRequest::getDefaultCommandInstance( *it ) );
        }
        return reqList;
    }
} /* namespace RTSTREAMING */
