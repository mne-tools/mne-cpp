#include "pluginconnectorconnection.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnection::PluginConnectorConnection(PluginOutputConnector::SPtr sender, PluginInputConnector::SPtr receiver, QObject *parent)
: QObject(parent)
, m_pSender(sender)
, m_pReceiver(receiver)
{
    createConnection(sender, receiver);
}


//*************************************************************************************************************

PluginConnectorConnection::~PluginConnectorConnection()
{
    clearConnection();
}


//*************************************************************************************************************

void PluginConnectorConnection::clearConnection()
{
    disconnect(m_con);
}


//*************************************************************************************************************

void PluginConnectorConnection::createConnection(PluginOutputConnector::SPtr sender, PluginInputConnector::SPtr receiver)
{
    clearConnection();
    m_con = connect(sender.data(), &PluginOutputConnector::notify, receiver.data(), &PluginInputConnector::update);
}
