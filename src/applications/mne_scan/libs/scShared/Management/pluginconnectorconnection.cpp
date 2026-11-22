//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginconnectorconnection.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginConnectorConnection class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
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

using namespace mne::scan;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnection::PluginConnectorConnection(AbstractPlugin::SPtr sender, AbstractPlugin::SPtr receiver, QObject* parent)
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
    for (i = 0; i < m_pSender->getOutputConnectors().size(); ++i) {
        for (j = 0; j < m_pReceiver->getInputConnectors().size(); ++j) {
            //ToDo make this auto connection more fancy
            // < --- Type Check --- >

            //Cast to RealTimeMultiSampleArray
            QSharedPointer<PluginOutputData<RealTimeMultiSampleArray>> senderRTMSA = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>();
            QSharedPointer<PluginInputData<RealTimeMultiSampleArray>> receiverRTMSA = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeMultiSampleArray>>();
            if (senderRTMSA && receiverRTMSA) {
                // We need to use BlockingQueuedConnection here because the FiffSimulator is still dispatching its data from a different thread via the direct connect signal method
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeEvokedSet
            QSharedPointer<PluginOutputData<RealTimeEvokedSet>> senderRTESet = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeEvokedSet>>();
            QSharedPointer<PluginInputData<RealTimeEvokedSet>> receiverRTESet = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeEvokedSet>>();
            if (senderRTESet && receiverRTESet) {
                // We cannot use BlockingQueuedConnection here because Averaging is dispatching its data from the main thread via the onNewEvokedSet method
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeCov
            QSharedPointer<PluginOutputData<RealTimeCov>> senderRTC = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeCov>>();
            QSharedPointer<PluginInputData<RealTimeCov>> receiverRTC = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeCov>>();
            if (senderRTC && receiverRTC) {
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeSourceEstimate
            QSharedPointer<PluginOutputData<RealTimeSourceEstimate>> senderRTSE = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeSourceEstimate>>();
            QSharedPointer<PluginInputData<RealTimeSourceEstimate>> receiverRTSE = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeSourceEstimate>>();
            if (senderRTSE && receiverRTSE) {
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeHpiResult
            QSharedPointer<PluginOutputData<RealTimeHpiResult>> senderRTHR = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeHpiResult>>();
            QSharedPointer<PluginInputData<RealTimeHpiResult>> receiverRTHR = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeHpiResult>>();
            if (senderRTHR && receiverRTHR) {
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }

            //Cast to RealTimeFwdSolution
            QSharedPointer<PluginOutputData<RealTimeFwdSolution>> senderRTFS = m_pSender->getOutputConnectors()[i].dynamicCast<PluginOutputData<RealTimeFwdSolution>>();
            QSharedPointer<PluginInputData<RealTimeFwdSolution>> receiverRTFS = m_pReceiver->getInputConnectors()[j].dynamicCast<PluginInputData<RealTimeFwdSolution>>();
            if (senderRTFS && receiverRTFS) {
                m_qHashConnections.insert(QPair<QString, QString>(m_pSender->getOutputConnectors()[i]->getName(),
                                                                  m_pReceiver->getInputConnectors()[j]->getName()),
                                          connect(m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                  m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
                bConnected = true;
                break;
            }
        }

        if (bConnected)
            break;
    }

    //DEBUG
    QHash<QPair<QString, QString>, QMetaObject::Connection>::iterator it;
    for (it = m_qHashConnections.begin(); it != m_qHashConnections.end(); ++it)
        mne::Log::debug("Connected: {} {}", it.key().first.toStdString(), it.key().second.toStdString());
    //DEBUG

    return bConnected;
}

//=============================================================================================================

ConnectorDataType PluginConnectorConnection::getDataType(QSharedPointer<PluginConnector> pPluginConnector)
{
    QSharedPointer<PluginOutputData<mne::scan::RealTimeEvokedSet>> RTES_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeEvokedSet>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeEvokedSet>> RTES_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeEvokedSet>>();
    if (RTES_Out || RTES_In)
        return ConnectorDataType::_RTES;

    QSharedPointer<PluginOutputData<mne::scan::RealTimeMultiSampleArray>> RTMSA_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeMultiSampleArray>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeMultiSampleArray>> RTMSA_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeMultiSampleArray>>();
    if (RTMSA_Out || RTMSA_In)
        return ConnectorDataType::_RTMSA;

    QSharedPointer<PluginOutputData<mne::scan::RealTimeCov>> RTC_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeCov>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeCov>> RTC_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeCov>>();
    if (RTC_Out || RTC_In)
        return ConnectorDataType::_RTC;

    QSharedPointer<PluginOutputData<mne::scan::RealTimeSourceEstimate>> RTSE_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeSourceEstimate>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeSourceEstimate>> RTSE_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeSourceEstimate>>();
    if (RTSE_Out || RTSE_In)
        return ConnectorDataType::_RTSE;

    QSharedPointer<PluginOutputData<mne::scan::RealTimeHpiResult>> RTHR_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeHpiResult>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeHpiResult>> RTHR_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeHpiResult>>();
    if (RTHR_Out || RTHR_In)
        return ConnectorDataType::_RTHR;

    QSharedPointer<PluginOutputData<mne::scan::RealTimeFwdSolution>> RTFS_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::RealTimeFwdSolution>>();
    QSharedPointer<PluginInputData<mne::scan::RealTimeFwdSolution>> RTFS_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::RealTimeFwdSolution>>();
    if (RTHR_Out || RTHR_In)
        return ConnectorDataType::_RTFS;

    QSharedPointer<PluginOutputData<mne::scan::Numeric>> Num_Out = pPluginConnector.dynamicCast<PluginOutputData<mne::scan::Numeric>>();
    QSharedPointer<PluginInputData<mne::scan::Numeric>> Num_In = pPluginConnector.dynamicCast<PluginInputData<mne::scan::Numeric>>();
    if (Num_Out || Num_In)
        return ConnectorDataType::_N;

    return ConnectorDataType::_None;
}

//=============================================================================================================

QWidget* PluginConnectorConnection::setupWidget()
{
    PluginConnectorConnectionWidget* pccWidget = new PluginConnectorConnectionWidget(this);
    return pccWidget;
}
