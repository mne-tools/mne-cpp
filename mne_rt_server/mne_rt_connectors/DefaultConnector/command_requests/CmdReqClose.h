/**
 * @author   Christof Pieloth
 */

#ifndef CMDREQCLOSE_H_
#define CMDREQCLOSE_H_

#include <QSharedPointer>

#include <rt_communication/command_requests/CmdReqBase.h>

namespace RTSTREAMING
{
/**
 * Request to close the server.
 */
class CmdReqClose: public RTSTREAMING::CmdReqBase
{
public:
    typedef QSharedPointer<CmdReqClose> SPtr;
    typedef QSharedPointer<const CmdReqClose> ConstSPtr;

    static const CommandT COMMAND;

    CmdReqClose();
    virtual ~CmdReqClose();

    virtual QString getHelpText() const;

    virtual CommandRequestT getRequestPacket() const;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQCLOSE_H_ */
