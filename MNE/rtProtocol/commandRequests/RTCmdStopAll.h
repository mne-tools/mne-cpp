/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDSTOPALL_H_
#define RTCMDSTOPALL_H_

#include <QSharedPointer>

#include "RTCmdBase.h"

namespace RTSTREAMING
{

    class RTCmdStopAll: public RTSTREAMING::RTCmdBase
    {
    public:
        typedef QSharedPointer< RTCmdStopAll > SPtr;
        typedef QSharedPointer< const RTCmdStopAll > ConstSPtr;

        RTCmdStopAll();
        virtual ~RTCmdStopAll();

        virtual QString getHelpText() const;
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDSTOPALL_H_ */
