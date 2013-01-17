/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDSTARTMEAS_H_
#define RTCMDSTARTMEAS_H_

#include <QSharedPointer>

#include "RTCmdBase.h"

namespace RTSTREAMING
{
    /**
     * Requests for streaming data.
     */
    class RTCmdStartMeas: public RTSTREAMING::RTCmdBase
    {
    public:
        typedef QSharedPointer< RTCmdStartMeas > SPtr;
        typedef QSharedPointer< const RTCmdStartMeas > ConstSPtr;

        static const int NO_CLIENT_ID; /**< Return value if no ID is set. */

        RTCmdStartMeas();
        virtual ~RTCmdStartMeas();

        virtual QString getHelpText() const;

        /**
         * Returns the Client ID.
         *
         * @return ID of the client who should receive the data or NO_CLIENT_ID if no ID is set.
         */
        int getClientId() const;

        /**
         * Sets the ID of the client who should receive the data.
         * The ID is added to the argument list.
         *
         * @param id Client ID
         */
        void setClientId( int id );
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDSTARTMEAS_H_ */
