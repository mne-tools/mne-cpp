/**
 * @author Christof Pieloth
 */

#ifndef CMDREQBASE_H_
#define CMDREQBASE_H_

#include "ICommandRequest.h"

namespace RTSTREAMING
{
/**
 * Abstract base class for requests. Implements getCommand() at the moment.
 */
class CmdReqBase: public RTSTREAMING::ICommandRequest
{
public:
    typedef QSharedPointer<CmdReqBase> SPtr;
    typedef QSharedPointer<const CmdReqBase> ConstSPtr;

    CmdReqBase(CommandT cmd);

    virtual CommandT getCommand() const;

    virtual ~CmdReqBase();
    CmdReqBase();

protected:
    CommandT m_cmd;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQBASE_H_ */
