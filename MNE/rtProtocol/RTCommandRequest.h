/**
 * @author  Christof Pieloth
 */

#ifndef RTCOMMANDREQUEST_H_
#define RTCOMMANDREQUEST_H_

#include <QSharedPointer>
#include <QString>

#include "RTProtocolDefinitions.h"

namespace RTSTREAMING
{
/**
 * Interface for all command requests. Implementing classes can add type-safe getter and setter for the argument list.
 */
class RTCommandRequest
{
public:
    typedef QSharedPointer<RTCommandRequest> SPtr;
    typedef QSharedPointer<const RTCommandRequest> ConstSPtr;

    virtual ~RTCommandRequest() {};

    /**
     * Gets the command of the request.
     *
     * @return  Command
     */
    virtual CommandT getCommand() const = 0;

    virtual bool isCommand(const CommandT& cmd) const = 0;

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
    virtual CommandArgListT getArguments() const = 0;

    /**
     * Sets a new argument list and all old arguments are lost.
     *
     * @param   args    A list of arguments.
     */
    virtual void setArguments(CommandArgListT args) = 0;

    /**
     * Adds a single argument to list.
     *
     * @param   arg Argument which should be add to list.
     */
    virtual void addArgument(CommandArgT arg) = 0;

    /**
     * Created a request packet out of the command and the argument list.
     *
     * @return  Ready-to-send request packet
     */
    virtual CommandRequestT getRequest() = 0;
};
} /* namespace RTSTREAMING */
#endif /* RTCOMMANDREQUEST_H_ */
