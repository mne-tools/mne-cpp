/**
 * @author Christof Pieloth
 */

#ifndef CMDREQBASE_H
#define CMDREQBASE_H

#include "ICommandRequest.h"

namespace RTCOMMUNICATIONLIB
{
/**
 * Abstract base class for requests. Implements getCommand() at the moment.
 */
class CmdReqBase: public ICommandRequest
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

} // Namespace

#endif /* CMDREQBASE_H_ */
