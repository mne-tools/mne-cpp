/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDBASE_H_
#define RTCMDBASE_H_

#include <QSharedPointer>
#include <QString>

#include "../RTCommandRequest.h"
#include "../RTProtocolDefinitions.h"

namespace RTSTREAMING
{
    /**
     * Abstract base class for all command requests. Derived class can add type-safe getter and setter for the argument list.
     */
    class RTCmdBase : public RTCommandRequest
    {
    public:
        typedef QSharedPointer< RTCmdBase > SPtr;
        typedef QSharedPointer< const RTCmdBase > ConstSPtr;

        explicit RTCmdBase( CommandT cmd );
        virtual ~RTCmdBase();

        /**
         * Gets the command of the request.
         *
         * @return  Command
         */
        virtual CommandT getCommand() const;

        virtual bool isCommand(const CommandT& cmd) const;

        /**
         * Gets a help text for that command.
         *
         * @return  help text
         */
        virtual QString getHelpText() const = 0;

        /**
         * Gets the argument list.
         *
         * @return argument list
         */
        virtual CommandArgListT getArguments() const;

        /**
         * Sets a new argument list and all old arguments are lost.
         *
         * @param   args    A list of arguments.
         */
        virtual void setArguments( CommandArgListT args );

        /**
         * Adds a single argument to list.
         *
         * @param   arg Argument which should be add to list.
         */
        virtual void addArgument( CommandArgT arg );

        /**
         * Created a request packet out of the command and the argument list.
         *
         * @return  Ready-to-send request packet
         */
        virtual CommandRequestT getRequest();

        // TODO(pieloth):  Parse request from plugins to.
        /**
         * Parse an incoming request packet to the correct request object.
         *
         * @param   req Request to parse.
         *
         * @return  Ready-to-cast instance or RTCmdUnkown.
         */
        static RTCommandRequest::SPtr parseRequest( const CommandRequestT& req );

        // TODO(pieloth):  Return request from plugins to.
        /**
         * Creates a list of default request.
         *
         * @return  List of request instances.
         */
        static std::list< RTCommandRequest::SPtr > getDefaultCommandRequests();

        // TODO(pieloth):  Return request instance from plugins to.
        /**
         * Creates an empty request instance for related cmd.
         *
         * @return  Request instance or RTCmdUnkown.
         */
        static RTCommandRequest::SPtr getDefaultCommandInstance( const CommandT& cmd );

    protected:
        const CommandT m_cmd; /**< Command for this request. */
        CommandArgListT m_args; /**< Argument list. */
    };
} /* namespace RTSTREAMING */
#endif /* RTCMDBASE_H_ */
