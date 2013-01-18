/**
 * @author  Christof Pieloth
 */

#ifndef CMDRESPTEXT_H_
#define CMDRESPTEXT_H_

#include <QSharedPointer>

#include "../ICommandResponse.h"

namespace RTSTREAMING
{
/**
 * A response object to send simple string to the client.
 */
class CmdRespText: public RTSTREAMING::ICommandResponse
{
public:
    typedef QSharedPointer<CmdRespText> SPtr;
    typedef QSharedPointer<const CmdRespText> ConstSPtr;

    CmdRespText(CommandResponseT resp);
    virtual ~CmdRespText();

    virtual CommandResponseT getResponsePacket() const;

private:
    CommandResponseT m_cmdResp;
};

} /* namespace RTSTREAMING */
#endif /* CMDRESPTEXT_H_ */
