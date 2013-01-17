/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDSELCON_H_
#define RTCMDSELCON_H_

#include <QSharedPointer>

#include "RTCmdBase.h"

namespace RTSTREAMING
{
    /**
     * Request for connector selection.
     */
    class RTCmdSelCon: public RTSTREAMING::RTCmdBase
    {
    public:
        typedef QSharedPointer< RTCmdSelCon > SPtr;
        typedef QSharedPointer< const RTCmdSelCon > ConstSPtr;

        static const int NO_CONNECTOR_ID;

        RTCmdSelCon();
        virtual ~RTCmdSelCon();

        virtual QString getHelpText() const;

        /**
         * Returns the connector ID.
         *
         * @return ID of the connector.
         */
        int getConnectorId() const;

        /**
         * Sets the ID of the connector.
         * The ID is added to the argument list.
         *
         * @param id connector ID
         */
        void setConnectorId( int id );
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDSELCON_H_ */
