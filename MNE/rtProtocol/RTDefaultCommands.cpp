/**
 * @author Christof Pieloth
 */

#include <list>

#include "RTDefaultCommands.h"

namespace RTSTREAMING
{
    /**
     * This namespace bundles all available commands.
     */
    namespace Command
    {
        std::list< CommandT > getDefaultCommands()
        {
            std::list< CommandT > list;
            list.push_back( HELP );
            list.push_back( CLOSE );
            list.push_back( REQUEST_CONNECTORS );
            list.push_back( SELECT_CONNECTOR );
            list.push_back( REQUEST_MEASUREMENT_INFO );
            list.push_back( START_MEASUREMENT );
            list.push_back( STOP_MEASUREMENT );
            list.push_back( STOP_ALL_MEASUREMENTS );
            return list;
        }
    }
}
