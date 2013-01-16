/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDHELP_H_
#define RTCMDHELP_H_

#include <QSharedPointer>
#include <QString>

#include "RTCommandRequest.h"

namespace RTSTREAMING
{
    /**
     * Request for help message.
     */
    class RTCmdHelp: public RTSTREAMING::RTCommandRequest
    {
    public:
        typedef QSharedPointer< RTCmdHelp > SPtr;
        typedef QSharedPointer< const RTCmdHelp > ConstSPtr;

        RTCmdHelp();
        virtual ~RTCmdHelp();

        virtual QString getHelpText() const;
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDHELP_H_ */
