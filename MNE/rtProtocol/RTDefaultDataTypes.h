/**
 * @author Christof Pieloth
 */

#ifndef RTDEFAULTDATATYPES_H_
#define RTDEFAULTDATATYPES_H_

#include "RTProtocolDefinitions.h"

namespace RTSTREAMING
{
    /**
     * This namespace bundles all available data type codes. 0 to 63 is reserved for library usage.
     */
    namespace DataType
    {
        const DataTypeT FIFF_INFO = 0; /**< Closes Streaming Server. */
        const DataTypeT FIFF_DATA = 1; /**< Closes Streaming Server. */
        const DataTypeT BYTE = 2; /**< Closes Streaming Server. */
    }
}

#endif /* RTDEFAULTDATATYPES_H_ */
