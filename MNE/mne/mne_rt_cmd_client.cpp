#include "mne_rt_cmd_client.h"

MNERtCmdClient::MNERtCmdClient(QObject *parent)
: QTcpSocket(parent)
{

}


//*************************************************************************************************************

void MNERtCmdClient::connectToHost(QString& p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4217);
}


//*************************************************************************************************************

QString MNERtCmdClient::sendCommand(QString p_sCommand)
{
    QString t_sCommand = QString("%1\n").arg(p_sCommand);
    QString p_sReply;

    if(this->state() == QAbstractSocket::ConnectedState)
    {
        this->write(t_sCommand.toUtf8().constData(),t_sCommand.size());
        this->waitForBytesWritten();

        //thats not the most elegant way
        this->waitForReadyRead(1000);
        QByteArray t_qByteArrayRaw;
        while (this->bytesAvailable() > 0 && this->canReadLine())
            t_qByteArrayRaw += this->readAll();

        p_sReply = QString(t_qByteArrayRaw);
    }
    return p_sReply;
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeasInfo(qint32 p_id)
{
    QString t_sCommand = QString("measinfo %1").arg(p_id);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeasInfo(QString p_Alias)
{
    QString t_sCommand = QString("measinfo %1").arg(p_Alias);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeas(qint32 p_id)
{
    QString t_sCommand = QString("meas %1").arg(p_id);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeas(QString p_Alias)
{
    QString t_sCommand = QString("meas %1").arg(p_Alias);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::stopAll()
{
    QString t_sCommand = QString("stop-all");
    this->sendCommand(t_sCommand);
}
