/**
 * @author   Christof Pieloth
 */

#ifndef DEFAULTCONNECTOR_H_
#define DEFAULTCONNECTOR_H_

#include <QString>

// TODO(cpieloth): Remove relative path and use it with -I./mne_rt_server/
#include "../../mne_rt_server/IConnectornew.h"
#include "../../mne_rt_server/ICommandRequestManager.h"
#include "../../mne_rt_server/IConnectornew.h"

namespace RTSTREAMING
{
/**
 * A default connector to provide basic control over the server.
 */
class DefaultConnector: public RTSTREAMING::IConnector_new
{
public:
    DefaultConnector();
    virtual ~DefaultConnector();

    virtual QString getName() const;
    virtual ICommandRequestManager::SPtr getCommandRequestManager() const;
    virtual void start();
    virtual bool isActive() const;
    virtual void stop();

private:
    ICommandRequestManager::SPtr m_cmdReqManager;

    bool m_active;
};

} /* namespace RTSTREAMING */
#endif /* DEFAULTCONNECTOR_H_ */
