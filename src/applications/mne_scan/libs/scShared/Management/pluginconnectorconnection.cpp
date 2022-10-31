//=============================================================================================================
/**
 * @file     pluginconnectorconnection.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginconnectorconnection.h"
#include "pluginconnectorconnectionwidget.h"

#include <scMeas/numeric.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimefwdsolution.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnection::PluginConnectorConnection(AbstractPlugin::SPtr sender, AbstractPlugin::SPtr receiver, QObject *parent)
: QObject(parent)
, m_pSender(sender)
, m_pReceiver(receiver)
{
    createConnection();
}

//=============================================================================================================

PluginConnectorConnection::~PluginConnectorConnection()
{
    clearConnection();
}

//=============================================================================================================

void PluginConnectorConnection::clearConnection()
{
    QHash<QPair<QString, QString>, QMetaObject::Connection>::iterator it;
    for (it = m_qHashConnections.begin(); it != m_qHashConnections.end(); ++it)
        disconnect(it.value());

    m_qHashConnections.clear();
}

//=============================================================================================================

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

            //Cast to RealTimeMultiSampleArray
            QSharedPointer< PluginOutputData<RealTimeMultiSampleArray> > senderRTMSA = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeMultiSampleArray> >();
            QSharedPointer< PluginInputData<RealTimeMultiSampleArray> > receiverRTMSA = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeMultiSampleArray> >();
            if(senderRTMSA && receiverRTMSA)
            {
                // We need to use BlockingQueuedConnection here because the FiffSimulator is still dispatching its data from a different thread via the direct connect signal method
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                 m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeEvokedSet
            QSharedPointer< PluginOutputData<RealTimeEvokedSet> > senderRTESet = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeEvokedSet> >();
            QSharedPointer< PluginInputData<RealTimeEvokedSet> > receiverRTESet = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeEvokedSet> >();
            if(senderRTESet && receiverRTESet)
            {
                // We cannot use BlockingQueuedConnection here because Averaging is dispatching its data from the main thread via the onNewEvokedSet method
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                 m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeCov
            QSharedPointer< PluginOutputData<RealTimeCov> > senderRTC = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeCov> >();
            QSharedPointer< PluginInputData<RealTimeCov> > receiverRTC = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeCov> >();
            if(senderRTC && receiverRTC)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                 m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeSourceEstimate
            QSharedPointer< PluginOutputData<RealTimeSourceEstimate> > senderRTSE = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeSourceEstimate> >();
            QSharedPointer< PluginInputData<RealTimeSourceEstimate> > receiverRTSE = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeSourceEstimate> >();
            if(senderRTSE && receiverRTSE)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                 m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeHpiResult
            QSharedPointer< PluginOutputData<RealTimeHpiResult> > senderRTHR = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeHpiResult> >();
            QSharedPointer< PluginInputData<RealTimeHpiResult> > receiverRTHR = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeHpiResult> >();
            if(senderRTHR && receiverRTHR)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeFwdSolution
            QSharedPointer< PluginOutputData<RealTimeFwdSolution> > senderRTFS = m_pSender->getOutputConnectors()[i].dynamicCast< PluginOutputData<RealTimeFwdSolution> >();
            QSharedPointer< PluginInputData<RealTimeFwdSolution> > receiverRTFS = m_pReceiver->getInputConnectors()[j].dynamicCast< PluginInputData<RealTimeFwdSolution> >();
            if(senderRTFS && receiverRTFS)
            {
                m_qHashConnections.insert(QPair<QString,QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
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

//=============================================================================================================

ConnectorDataType PluginConnectorConnection::getDataType(QSharedPointer<PluginConnector> pPluginConnector)
{
    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeEvokedSet> > RTES_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeEvokedSet> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeEvokedSet> > RTES_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeEvokedSet> >();
    if(RTES_Out || RTES_In)
        return ConnectorDataType::_RTES;

    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > RTMSA_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> > RTMSA_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> >();
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

    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeHpiResult> > RTHR_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeHpiResult> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeHpiResult> > RTHR_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeHpiResult> >();
    if(RTHR_Out || RTHR_In)
        return ConnectorDataType::_RTHR;

    QSharedPointer< PluginOutputData<SCMEASLIB::RealTimeFwdSolution> > RTFS_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::RealTimeFwdSolution> >();
    QSharedPointer< PluginInputData<SCMEASLIB::RealTimeFwdSolution> > RTFS_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::RealTimeFwdSolution> >();
    if(RTHR_Out || RTHR_In)
        return ConnectorDataType::_RTFS;

    QSharedPointer< PluginOutputData<SCMEASLIB::Numeric> > Num_Out = pPluginConnector.dynamicCast< PluginOutputData<SCMEASLIB::Numeric> >();
    QSharedPointer< PluginInputData<SCMEASLIB::Numeric> > Num_In = pPluginConnector.dynamicCast< PluginInputData<SCMEASLIB::Numeric> >();
    if(Num_Out || Num_In)
        return ConnectorDataType::_N;

    return ConnectorDataType::_None;
}

//=============================================================================================================

QWidget* PluginConnectorConnection::setupWidget()
{
    PluginConnectorConnectionWidget* pccWidget = new PluginConnectorConnectionWidget(this);
    return pccWidget;
}
