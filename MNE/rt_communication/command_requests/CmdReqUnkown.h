/**
 * @author  Christof Pieloth
 */

#ifndef CMDREQUNKOWN_H_
#define CMDREQUNKOWN_H_

#include <QSharedPointer>

#include "CmdReqBase.h"

namespace RTSTREAMING
{
/**
 * This request object is used as a fallback type, when a command could not be identified/parsed.
 */
class CmdReqUnkown: public RTSTREAMING::CmdReqBase
{
public:
    typedef QSharedPointer<CmdReqUnkown> SPtr;
    typedef QSharedPointer<const CmdReqUnkown> ConstSPtr;

    static const CommandT COMMAND;

    CmdReqUnkown();
    virtual ~CmdReqUnkown();

    virtual QString getHelpText() const;

    virtual CommandRequestT getRequestPacket() const;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQUNKOWN_H_ */
