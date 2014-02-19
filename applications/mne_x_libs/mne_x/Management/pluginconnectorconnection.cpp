//=============================================================================================================
/**
* @file     pluginconnectorconnection.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the PluginConnectorConnection class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginconnectorconnection.h"
#include "pluginconnectorconnectionwidget.h"

#include <xMeas/newnumeric.h>
#include <xMeas/newrealtimesamplearray.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/realtimesourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnection::PluginConnectorConnection(IPlugin::SPtr sender, IPlugin::SPtr receiver, QObject *parent)
: QObject(parent)
, m_bConnectionState(false)
, m_pSender(sender)
, m_pReceiver(receiver)
{
    m_bConnectionState = createConnection();
}


//*************************************************************************************************************

PluginConnectorConnection::~PluginConnectorConnection()
{
    clearConnection();
}


//*************************************************************************************************************

void PluginConnectorConnection::clearConnection()
{
    if(m_bConnectionState)
    {
        disconnect(m_con);
        m_bConnectionState = false;
    }
}


//*************************************************************************************************************

bool PluginConnectorConnection::createConnection()
{
    clearConnection();

    bool bConnected = false;

    //search for suiting connection
    qint32 i, j;
    for(i = 0; i < m_pSender->getOutputConnectors().size(); ++i)
    {
        for(j = 0; j < m_pReceiver->getInputConnectors().size(); ++j)
        {
            //ToDo make this auto connection more fancy
            // < --- Type Check --- >

            //Cast to NewRealTimeSampleArray
            QSharedPointer< PluginOutputData<NewRealTimeSampleArray> > senderRTSA = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<NewRealTimeSampleArray> >();
            QSharedPointer< PluginInputData<NewRealTimeSampleArray> > receiverRTSA = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<NewRealTimeSampleArray> >();
            if(senderRTSA && receiverRTSA)
            {
                m_con = connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection);
                bConnected = true;
                break;
            }

            //Cast to NewRealTimeMultiSampleArray
            QSharedPointer< PluginOutputData<NewRealTimeMultiSampleArray> > senderRTMSA = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >();
            QSharedPointer< PluginInputData<NewRealTimeMultiSampleArray> > receiverRTMSA = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<NewRealTimeMultiSampleArray> >();
            if(senderRTMSA && receiverRTMSA)
            {
                m_con = connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection);
                bConnected = true;
                break;
            }
        }

        if(bConnected)
            break;
    }


//    m_con = connect(sender.data(), &PluginOutputConnector::notify, receiver.data(), &PluginInputConnector::update);

    return bConnected;
}


//*************************************************************************************************************

ConnectorDataType PluginConnectorConnection::getDataType(QSharedPointer<PluginConnector> pPluginConnector)
{
    QSharedPointer< PluginOutputData<XMEASLIB::NewRealTimeSampleArray> > RTSA_Out = pPluginConnector.dynamicCast< PluginOutputData<XMEASLIB::NewRealTimeSampleArray> >();
    QSharedPointer< PluginInputData<XMEASLIB::NewRealTimeSampleArray> > RTSA_In = pPluginConnector.dynamicCast< PluginInputData<XMEASLIB::NewRealTimeSampleArray> >();
    if(RTSA_Out || RTSA_In)
        return ConnectorDataType::_RTSA;

    QSharedPointer< PluginOutputData<XMEASLIB::NewRealTimeMultiSampleArray> > RTMSA_Out = pPluginConnector.dynamicCast< PluginOutputData<XMEASLIB::NewRealTimeMultiSampleArray> >();
    QSharedPointer< PluginInputData<XMEASLIB::NewRealTimeMultiSampleArray> > RTMSA_In = pPluginConnector.dynamicCast< PluginInputData<XMEASLIB::NewRealTimeMultiSampleArray> >();
    if(RTMSA_Out || RTMSA_In)
        return ConnectorDataType::_RTMSA;

    QSharedPointer< PluginOutputData<XMEASLIB::RealTimeSourceEstimate> > RTSE_Out = pPluginConnector.dynamicCast< PluginOutputData<XMEASLIB::RealTimeSourceEstimate> >();
    QSharedPointer< PluginInputData<XMEASLIB::RealTimeSourceEstimate> > RTSE_In = pPluginConnector.dynamicCast< PluginInputData<XMEASLIB::RealTimeSourceEstimate> >();
    if(RTSE_Out || RTSE_In)
        return ConnectorDataType::_RTSE;

    QSharedPointer< PluginOutputData<XMEASLIB::NewNumeric> > Num_Out = pPluginConnector.dynamicCast< PluginOutputData<XMEASLIB::NewNumeric> >();
    QSharedPointer< PluginInputData<XMEASLIB::NewNumeric> > Num_In = pPluginConnector.dynamicCast< PluginInputData<XMEASLIB::NewNumeric> >();
    if(Num_Out || Num_In)
        return ConnectorDataType::_N;

    return ConnectorDataType::_None;
}


//*************************************************************************************************************

QWidget* PluginConnectorConnection::setupWidget()
{
    PluginConnectorConnectionWidget* pccWidget = new PluginConnectorConnectionWidget(this);
    return pccWidget;
}
