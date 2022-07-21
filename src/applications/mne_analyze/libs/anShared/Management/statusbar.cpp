//=============================================================================================================
/**
 * @file     statusbar.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    StatusBar class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "statusbar.h"
#include "communicator.h"
#include "event.h"
#include "../Utils/metatypes.h"

#include <disp/viewers/progressview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QDebug>
#include <QProgressBar>
#include <QMenu>
#include <QHoverEvent>
#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StatusBar::StatusBar(QWidget *pParent)
    : QStatusBar(pParent)
    , m_iMsgTimeout(20000)
{
    QVector<EVENT_TYPE> vSubs = {EVENT_TYPE::STATUS_BAR_MSG, EVENT_TYPE::LOADING_START, EVENT_TYPE::LOADING_END};
    m_pCommunicator = new Communicator(std::move(vSubs));

    this->setStyleSheet("QStatusBar { color: red}");

    connect(m_pCommunicator, &Communicator::receivedEvent,
            this, &StatusBar::onNewMessageReceived);

    m_pProgressView = new DISPLIB::ProgressView(true,
                                                "QLabel { color : red; }");

    this->addPermanentWidget(m_pProgressView);
    m_pProgressView->hide();
}

//=============================================================================================================

StatusBar::~StatusBar()
{
    delete m_pCommunicator;
}

//=============================================================================================================

void StatusBar::onNewMessageReceived(const QSharedPointer<Event> pEvent)
{
    switch(pEvent->getType()) {
        case EVENT_TYPE::STATUS_BAR_MSG: {
            if(pEvent->getData().canConvert<QString>()) {
                showMessage(pEvent->getData().toString(), m_iMsgTimeout);
                break;
            }
            qWarning() << "[StatusBar::onNewMessageReceived] Received a message/event that cannot be parsed";
            break;
        }
        case EVENT_TYPE::LOADING_START : {
            if(pEvent->getData().canConvert<QString>()) {
                m_LoadingStack.push(pEvent->getData().toString());
                m_pProgressView->setMessage(m_LoadingStack.top());
            }
            m_pProgressView->show();
            break;
        }
        case EVENT_TYPE::LOADING_END : {
            if(pEvent->getData().canConvert<QString>()){
                if(m_LoadingStack.contains(pEvent->getData().toString())){
                    m_LoadingStack[m_LoadingStack.indexOf(pEvent->getData().toString())] = "";
                }
            }
            if(!m_LoadingStack.isEmpty()){
                while(m_LoadingStack.top() == ""){
                    m_LoadingStack.pop();
                    if(m_LoadingStack.isEmpty()){
                        m_pProgressView->hide();
                        m_pProgressView->setMessage("");
                        if (m_pHoverWidget){
                            m_pHoverWidget->hide();
                            delete  m_pHoverWidget;
                        }
                        break;
                    }
                }
            }
            if(!m_LoadingStack.isEmpty()){
                m_pProgressView->setMessage(m_LoadingStack.top());
            }
            break;
        }
        default:
            qWarning() << "[StatusBar::onNewMessageReceived] Received a message/event that is not handled by switch-cases";
            break;
    }
}

//=============================================================================================================

void StatusBar::enterEvent(QEvent *event)
{
    if(m_LoadingStack.isEmpty()){
        return;
    }

    m_pHoverWidget = new QWidget(this);
    m_pHoverWidget->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    QVBoxLayout* layout = new QVBoxLayout(m_pHoverWidget);
    m_pHoverWidget->setLayout(layout);

    QLabel* pMessageHeader = new QLabel("<u><b>Current Processes:</b></u>");
    layout->addWidget(pMessageHeader);
    for (QString message: m_LoadingStack){
        if(message != ""){
            layout->addWidget(new QLabel(message));
        }
    }

    m_pHoverWidget->move(this->mapToGlobal(QPoint(static_cast<QHoverEvent*>(event)->pos().y(), static_cast<QHoverEvent*>(event)->pos().x())).x(), this->parentWidget()->mapToGlobal(this->pos()).y() - this->height() - m_pHoverWidget->height() - 5);
    m_pHoverWidget->show();

    QWidget::enterEvent(static_cast<QEnterEvent*>(event));
}

//=============================================================================================================

void StatusBar::leaveEvent(QEvent *event)
{
    if (!m_pHoverWidget){
        return;
    }

    m_pHoverWidget->hide();
    delete  m_pHoverWidget;
    QWidget::leaveEvent(event);
}

//=============================================================================================================
