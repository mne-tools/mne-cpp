/**
 * @author  Christof Pieloth
 */

#ifndef RTCMDUNKOWN_H_
#define RTCMDUNKOWN_H_

#include <QSharedPointer>
#include <QString>

#include "RTCommandRequest.h"

namespace RTSTREAMING
{
    /**
     * Default request class for unknown commands.
     */
    class RTCmdUnkown: public RTSTREAMING::RTCommandRequest
    {
    public:
        typedef QSharedPointer< RTCmdUnkown > SPtr;
        typedef QSharedPointer< const RTCmdUnkown > ConstSPtr;

        RTCmdUnkown();
        virtual ~RTCmdUnkown();

        virtual QString getHelpText() const;

    private:
        static const CommandT COMMAND; /**< Constant for constructor. */
    };

} /* namespace RTSTREAMING */
#endif /* RTCMDUNKOWN_H_ */
