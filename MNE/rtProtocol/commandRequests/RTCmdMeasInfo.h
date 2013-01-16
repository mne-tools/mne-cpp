/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDMEASINFO_H_
#define RTCMDMEASINFO_H_

#include <QSharedPointer>
#include <QString>

#include "RTCommandRequest.h"

namespace RTSTREAMING
{
    /**
     * Request for measurement information.
     */
    class RTCmdMeasInfo: public RTSTREAMING::RTCommandRequest
    {
    public:
        typedef QSharedPointer< RTCmdMeasInfo > SPtr;
        typedef QSharedPointer< const RTCmdMeasInfo > ConstSPtr;

        RTCmdMeasInfo();
        virtual ~RTCmdMeasInfo();

        virtual QString getHelpText() const;

        /**
         * Returns the Client ID.
         *
         * @return ID of the client who should receive the information or NO_CLIENT_ID if no ID is set.
         */
        int getClientId();

        /**
         * Sets the ID of the client who should receive the information.
         * The ID is added to the argument list.
         *
         * @param id Client ID
         */
        void setClientId( int id );

        static const int NO_CLIENT_ID; /**< Return value if no ID is set. */
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDMEASINFO_H_ */
