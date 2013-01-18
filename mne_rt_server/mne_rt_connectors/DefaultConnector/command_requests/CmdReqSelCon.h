/**
 * @author   Christof Pieloth
 */

#ifndef CMDREQSELCON_H_
#define CMDREQSELCON_H_

#include <QSharedPointer>

#include <rt_communication/command_requests/CmdReqBase.h>

namespace RTSTREAMING
{
/**
 * Request to select a connector.
 */
class CmdReqSelCon: public RTSTREAMING::CmdReqBase
{
public:
    typedef QSharedPointer<CmdReqSelCon> SPtr;
    typedef QSharedPointer<const CmdReqSelCon> ConstSPtr;

    static const CommandT COMMAND;
    static const int NO_CONNECTOR_ID; /**< Indicator for wrong or not found connector id */

    CmdReqSelCon();
    virtual ~CmdReqSelCon();

    virtual QString getHelpText() const;

    virtual CommandRequestT getRequestPacket() const;

    /**
     * Gets the connector id to select.
     *
     * @return choosen connector id.
     */
    int getConnectorId() const;

    /**
     * Sets the connector id.
     *
     * @param   id ID to select.
     */
    void setConnectorId(int id);

private:
    int m_connectorId;
};

} /* namespace RTSTREAMING */
#endif /* CMDREQSELCON_H_ */
