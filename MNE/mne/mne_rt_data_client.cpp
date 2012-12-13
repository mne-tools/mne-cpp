#include "mne_rt_data_client.h"

MNERtDataClient::MNERtDataClient(QObject *parent)
: QTcpSocket(parent)
, m_clientID(-1)
{
    getClientId();
}


//*************************************************************************************************************

void MNERtDataClient::connectToHost(QString& p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4218);
}
