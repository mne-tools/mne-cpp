//=============================================================================================================
/**
 * @file     pluginconnectorconnectionwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the PluginConnectorConnectionWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginconnectorconnectionwidget.h"
#include "pluginconnectorconnection.h"

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QGridLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnectorConnectionWidget::PluginConnectorConnectionWidget(PluginConnectorConnection* pPluginConnectorConnection, QWidget *parent)
: QWidget(parent)
, m_pPluginConnectorConnection(pPluginConnectorConnection)
{

    QWidget *rightFiller = new QWidget;
    rightFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QString sSender = m_pPluginConnectorConnection->getSender()->getName();
    QString sReceiver = m_pPluginConnectorConnection->getReceiver()->getName();

    m_pLabel = new QLabel(tr("Connector Connection: ")+sSender+" -> "+sReceiver, this);
    m_pLabel->setStyleSheet("border: 0px; font-size: 14px; font-weight: bold;");

    qint32 curRow = 0;

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_pLabel,curRow,0,1,3);
    ++curRow;

    for(qint32 i = 0; i < m_pPluginConnectorConnection->getSender()->getOutputConnectors().size(); ++i)
    {
        QComboBox* m_pComboBox = new QComboBox(this);
        QString t_sSenderName = m_pPluginConnectorConnection->getSender()->getOutputConnectors()[i]->getName();
        QLabel* m_pLabelOutput = new QLabel(t_sSenderName,this);
        m_qMapSenderToReceiverConnections.insert(t_sSenderName,m_pComboBox);

        layout->addWidget(m_pLabelOutput,curRow,0);

        ConnectorDataType t_senderConnectorDataType = PluginConnectorConnection::getDataType(m_pPluginConnectorConnection->getSender()->getOutputConnectors()[i]);

        m_pComboBox->addItem("----");

        for(qint32 j = 0; j < m_pPluginConnectorConnection->getReceiver()->getInputConnectors().size(); ++j)
        {
            ConnectorDataType t_receiverConnectorDataType = PluginConnectorConnection::getDataType(m_pPluginConnectorConnection->getReceiver()->getInputConnectors()[j]);
            if(t_senderConnectorDataType == t_receiverConnectorDataType)
                m_pComboBox->addItem(m_pPluginConnectorConnection->getReceiver()->getInputConnectors()[j]->getName());
        }

        layout->addWidget(m_pComboBox,curRow,1);
        ++curRow;
    }

    //Look for existing connections
    QHash<QPair<QString, QString>, QMetaObject::Connection>::iterator it;
    for (it = pPluginConnectorConnection->m_qHashConnections.begin(); it != pPluginConnectorConnection->m_qHashConnections.end(); ++it)
    {
        QComboBox* m_pComboBox = m_qMapSenderToReceiverConnections[it.key().first];

        for(qint32 i = 0; i < m_pComboBox->count(); ++i)
            if(QString::compare(m_pComboBox->itemText(i), it.key().second) == 0)
                m_pComboBox->setCurrentIndex(i);
    }

    //Connect Signals
    foreach(QComboBox* m_pComboBox, m_qMapSenderToReceiverConnections)
        connect(m_pComboBox,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
                this,
                &PluginConnectorConnectionWidget::updateReceiver);

    layout->addWidget(bottomFiller,curRow,0);
    ++curRow;

    layout->addWidget(rightFiller,1,3,curRow-1,1);

    this->setLayout(layout);
}

//=============================================================================================================

PluginConnectorConnectionWidget::~PluginConnectorConnectionWidget()
{
    m_qMapSenderToReceiverConnections.clear();
}

//=============================================================================================================

void PluginConnectorConnectionWidget::updateReceiver(const QString &p_sCurrentReceiver)
{
    //If selection is default
    if(QString::compare(p_sCurrentReceiver, QString("----")) == 0)
        return;

    //get active control using focus
    QString t_sCurrentSender;
    QComboBox* t_qComboBox;
    QMap<QString, QComboBox*>::iterator it;
    for (it = m_qMapSenderToReceiverConnections.begin(); it != m_qMapSenderToReceiverConnections.end(); ++it)
    {
        if(it.value()->hasFocus())
        {
            t_sCurrentSender = it.key();
            t_qComboBox = it.value();

            qint32 i = 0;
            for(i = 0; i < m_pPluginConnectorConnection->m_pSender->getOutputConnectors().size(); ++i)
                if(m_pPluginConnectorConnection->m_pSender->getOutputConnectors()[i]->getName() == t_sCurrentSender)
                    break;

            qint32 j = 0;
            for(j = 0; j < m_pPluginConnectorConnection->m_pSender->getOutputConnectors().size(); ++j)
                if(m_pPluginConnectorConnection->m_pReceiver->getInputConnectors()[j]->getName() == p_sCurrentReceiver)
                    break;

            m_pPluginConnectorConnection->m_qHashConnections.insert(QPair<QString,QString>(m_pPluginConnectorConnection->m_pSender->getOutputConnectors()[i]->getName(),
                                                                                           m_pPluginConnectorConnection->m_pReceiver->getInputConnectors()[j]->getName()),
                                                                    connect(m_pPluginConnectorConnection->m_pSender->getOutputConnectors()[i].data(), &PluginOutputConnector::notify,
                                                                            m_pPluginConnectorConnection->m_pReceiver->getInputConnectors()[j].data(), &PluginInputConnector::update, Qt::BlockingQueuedConnection));
        }
    }

    //Disconnect the rest
    for (it = m_qMapSenderToReceiverConnections.begin(); it != m_qMapSenderToReceiverConnections.end(); ++it)
    {
        if(it.value() != t_qComboBox && it.value()->currentText() == p_sCurrentReceiver)
        {
            QPair<QString, QString> t_qPair(it.key(),it.value()->currentText());
            disconnect(m_pPluginConnectorConnection->m_qHashConnections[t_qPair]);
            m_pPluginConnectorConnection->m_qHashConnections.remove(t_qPair);
            it.value()->setCurrentIndex(0);
        }
    }
}
