/**
 * @author Christof Pieloth
 */

#ifndef RTDEFAULTCOMMANDS_H_
#define RTDEFAULTCOMMANDS_H_

#include <list>

#include "RTProtocolDefinitions.h"

namespace RTSTREAMING
{
    class RTCommandRequest;

    /**
     * This namespace bundles all available commands.
     */
    namespace Command
    {
        const CommandT HELP = "help"; /**< Prints a help message with all available commands. */
        const CommandT CLOSE = "close"; /**< Closes Streaming Server. */

        const CommandT REQUEST_CONNECTORS = "conlist"; /**< Prints and sends all available connectors. */
        const CommandT SELECT_CONNECTOR = "selcon"; /**< selects a new connector, if a measurement is running it will be stopped. */

        const CommandT REQUEST_MEASUREMENT_INFO = "measinfo"; /**< Sends measurement info to a specified client. */
        const CommandT START_MEASUREMENT = "meas"; /**< Starts the streaming for a specified client. */
        const CommandT STOP_MEASUREMENT = "stop"; /**< Stops the streaming for a specified client. */
        const CommandT STOP_ALL_MEASUREMENTS = "stop-all"; /**< Stops the whole acquisition process. */

        /**
         * Gets a list of all default commands.
         *
         * @return A list of all default commands.
         */
        std::list< CommandT > getDefaultCommands();
    }
}

#endif /* RTDEFAULTCOMMANDS_H_ */
