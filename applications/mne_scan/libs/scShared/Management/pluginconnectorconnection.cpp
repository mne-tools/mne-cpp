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
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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

#include <scMeas/newnumeric.h>
#include <scMeas/newrealtimesamplearray.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimesourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnection::PluginConnectorConnection(IPlugin::SPtr sender, IPlugin::SPtr receiver, QObject *parent)
: QObject(parent)
, m_pSender(sender)
, m_pReceiver(receiver)
{
    createConnection();
}


//*************************************************************************************************************

PluginConnectorConnection::~PluginConnectorConnection()
{
    clearConnection();
}


//*************************************************************************************************************

void PluginConnectorConnection::clearConnection()
{
    QHash<QPair<QString, QString>, QMetaObject::Connection>::iterator it;
    for (it = m_qHashConnections.begin(); it != m_qHashConnections.end(); ++it)
        disconnect(it.value());

    m_qHashConnections.clear();
}


//*************************************************************************************************************

bool PluginConnectorConnection::createConnection()
{
    // Creat initial connection
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
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(), m_pReceiver->getInputConnectors()[j]->getName()), connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to NewRealTimeMultiSampleArray
            QSharedPointer< PluginOutputData<NewRealTimeMultiSampleArray> > senderRTMSA = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >();
            QSharedPointer< PluginInputData<NewRealTimeMultiSampleArray> > receiverRTMSA = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<NewRealTimeMultiSampleArray> >();
            if(senderRTMSA && receiverRTMSA)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(), m_pReceiver->getInputConnectors()[j]->getName()), connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeEvokedSet
            QSharedPointer< PluginOutputData<RealTimeEvokedSet> > senderRTESet = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeEvokedSet> >();
            QSharedPointer< PluginInputData<RealTimeEvokedSet> > receiverRTESet = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeEvokedSet> >();
            if(senderRTESet && receiverRTESet)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(), m_pReceiver->getInputConnectors()[j]->getName()), connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeCov
            QSharedPointer< PluginOutputData<RealTimeCov> > senderRTC = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeCov> >();
            QSharedPointer< PluginInputData<RealTimeCov> > receiverRTC = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeCov> >();
            if(senderRTC && receiverRTC)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(), m_pReceiver->getInputConnectors()[j]->getName()), connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeSourceEstimate
            QSharedPointer< PluginOutputData<RealTimeSourceEstimate> > senderRTSE = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeSourceEstimate> >();
            QSharedPointer< PluginInputData<RealTimeSourceEstimate> > receiverRTSE = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeSourceEstimate> >();
            if(senderRTSE && receiverRTSE)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(), m_pReceiver->getInputConnectors()[j]->getName()), connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                        m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }
        }

        if(bConnected)
            break;
    }

    //DEBUG
    QHash<QPair<QString, QString>, QMetaObject::Connection>::iterator it;
    for (it = m_qHashConnections.begin(); it != m_qHashConnections.end(); ++it)
        qDebug() << "Connected: " << it.key().first << it.key().second;
    //DEBUG

    return bConnected;
}


//*************************************************************************************************************

ConnectorDataType PluginConnectorConnection::getDataType(QSharedPointer<PluginConnector> pPluginConnector)
{
    QSharedPointer< PluginOutputData<SCMEASLIB::NewRealTimeSampleArray> > RTSA_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::NewRealTimeSampleArray> >();
    QSharedPointer< PluginInputData<SCMEASLIB::NewRealTimeSampleArray> > RTSA_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::NewRealTimeSampleArray> >();
    if(RTSA_Out || RTSA_In)
        return ConnectorDataType::_RTSA;

    QSharedPointer< PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray> > RTMSA_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray> >();
    QSharedPointer< PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray> > RTMSA_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray> >();
    if(RTMSA_Out || RTMSA_In)
        return ConnectorDataType::_RTMSA;

    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeCov> > RTC_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeCov> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeCov> > RTC_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeCov> >();
    if(RTC_Out || RTC_In)
        return ConnectorDataType::_RTC;

    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeSourceEstimate> > RTSE_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeSourceEstimate> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeSourceEstimate> > RTSE_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeSourceEstimate> >();
    if(RTSE_Out || RTSE_In)
        return ConnectorDataType::_RTSE;

    QSharedPointer< PluginOutputData<SCMEASLIB::NewNumeric> > Num_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::NewNumeric> >();
    QSharedPointer< PluginInputData<SCMEASLIB::NewNumeric> > Num_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::NewNumeric> >();
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
