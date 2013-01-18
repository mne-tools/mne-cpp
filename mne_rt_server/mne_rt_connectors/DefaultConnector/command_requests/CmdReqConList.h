/**
 * @author   Christof Pieloth
 */

#ifndef CMDREQCONLIST_H_
#define CMDREQCONLIST_H_

#include <QSharedPointer>

#include <rt_communication/command_requests/CmdReqBase.h>

namespace RTSTREAMING
{
/**
 * Request to get available connectors.
 */
class CmdReqConList: public RTSTREAMING::CmdReqBase
{
public:
    typedef QSharedPointer<CmdReqConList> SPtr;
    typedef QSharedPointer<const CmdReqConList> ConstSPtr;

    static const CommandT COMMAND;

    CmdReqConList();
    virtual ~CmdReqConList();

    virtual QString getHelpText() const;

    virtual CommandRequestT getRequestPacket() const;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQCONLIST_H_ */
