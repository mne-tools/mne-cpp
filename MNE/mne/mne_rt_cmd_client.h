#ifndef MNE_RT_CMD_CLIENT_H
#define MNE_RT_CMD_CLIENT_H


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

class MNESHARED_EXPORT MNERtCmdClient : public QTcpSocket
{
    Q_OBJECT
public:
    typedef QSharedPointer<MNERtCmdClient> SPtr;            /**< Shared pointer type for MNERtCmdClient. */
    typedef QSharedPointer<const MNERtCmdClient> ConstSPtr; /**< Const shared pointer type for MNERtCmdClient. */

    explicit MNERtCmdClient(QObject *parent = 0);


    void connectToHost(QString& p_sRtServerHostName);


    QString sendCommand(QString p_sCommand);

    void requestMeasInfo(qint32 p_id);

    void requestMeasInfo(QString p_Alias);

    void requestMeas(qint32 p_id);

    void requestMeas(QString p_Alias);

    void stopAll();

signals:
    
public slots:
    
};

} // NAMESPACE

#endif // MNE_RT_CMD_CLIENT_H
