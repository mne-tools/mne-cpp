/**
 * @author   Christof Pieloth
 */

#ifndef CMDREQHELP_H_
#define CMDREQHELP_H_

#include <QSharedPointer>

#include <rt_communication/command_requests/CmdReqBase.h>

namespace RTSTREAMING
{
/**
 * Requests for a help text and available commands.
 */
class CmdReqHelp: public RTSTREAMING::CmdReqBase
{
public:
    typedef QSharedPointer<CmdReqHelp> SPtr;
    typedef QSharedPointer<const CmdReqHelp> ConstSPtr;

    static const CommandT COMMAND;

    CmdReqHelp();
    virtual ~CmdReqHelp();

    virtual QString getHelpText() const;

    virtual CommandRequestT getRequestPacket() const;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQHELP_H_ */
