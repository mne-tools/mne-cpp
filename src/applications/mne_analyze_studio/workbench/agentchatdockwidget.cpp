//=============================================================================================================
/**
 * @file     agentchatdockwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the workbench agent chat widget.
 */

#include "agentchatdockwidget.h"

#include <QJsonObject>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

AgentChatDockWidget::AgentChatDockWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel("Agentic Coworker"))
, m_statusLabel(new QLabel("LLM: Deterministic fallback only"))
, m_confirmationLabel(new QLabel("Pending Confirmations"))
, m_confirmationPanel(new QWidget)
, m_confirmationLayout(new QVBoxLayout(m_confirmationPanel))
, m_transcript(new QTextEdit)
, m_input(new QLineEdit)
, m_sendButton(new QPushButton("Send"))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    m_titleLabel->setObjectName("agentChatTitle");
    m_statusLabel->setObjectName("agentChatStatus");
    m_statusLabel->setWordWrap(true);
    m_confirmationLabel->setObjectName("agentChatStatus");
    m_confirmationLabel->setVisible(false);
    m_confirmationPanel->setVisible(false);
    m_confirmationLayout->setContentsMargins(0, 0, 0, 0);
    m_confirmationLayout->setSpacing(8);
    m_transcript->setReadOnly(true);
    m_transcript->setPlaceholderText("Agent history will appear here.");
    m_input->setPlaceholderText("Ask the agent or call tools/call...");

    QHBoxLayout* composerLayout = new QHBoxLayout;
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(8);
    composerLayout->addWidget(m_input, 1);
    composerLayout->addWidget(m_sendButton);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_confirmationLabel);
    layout->addWidget(m_confirmationPanel);
    layout->addWidget(m_transcript, 1);
    layout->addLayout(composerLayout);

    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        const QString text = m_input->text().trimmed();
        if(text.isEmpty()) {
            return;
        }

        appendTranscript(QString("Agent> %1").arg(text));
        emit commandSubmitted(text);
        m_input->clear();
    });
}

void AgentChatDockWidget::setPlannerStatus(const QString& statusText)
{
    m_statusLabel->setText(statusText);
}

void AgentChatDockWidget::appendTranscript(const QString& text)
{
    m_transcript->append(text);
}

void AgentChatDockWidget::setPendingConfirmations(const QJsonArray& confirmations)
{
    while(QLayoutItem* item = m_confirmationLayout->takeAt(0)) {
        if(QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    m_confirmationLabel->setVisible(!confirmations.isEmpty());
    m_confirmationPanel->setVisible(!confirmations.isEmpty());
    if(confirmations.isEmpty()) {
        return;
    }

    for(int i = 0; i < confirmations.size(); ++i) {
        const QJsonObject confirmation = confirmations.at(i).toObject();
        const QString commandText = confirmation.value("command").toString().trimmed();
        if(commandText.isEmpty()) {
            continue;
        }

        QWidget* row = new QWidget(m_confirmationPanel);
        QVBoxLayout* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(8, 8, 8, 8);
        rowLayout->setSpacing(6);

        QLabel* title = new QLabel(confirmation.value("title").toString(QString("Proposal %1").arg(i + 1)), row);
        title->setWordWrap(true);
        QLabel* details = new QLabel(confirmation.value("details").toString(commandText), row);
        details->setWordWrap(true);
        QLabel* reason = new QLabel(row);
        const QString reasonText = confirmation.value("reason").toString().trimmed();
        reason->setVisible(!reasonText.isEmpty());
        reason->setWordWrap(true);
        if(!reasonText.isEmpty()) {
            reason->setText(QString("Why: %1").arg(reasonText));
        }
        QLabel* warning = new QLabel(row);
        const bool stale = confirmation.value("stale").toBool(false);
        const QString staleReason = confirmation.value("stale_reason").toString().trimmed();
        warning->setVisible(stale && !staleReason.isEmpty());
        warning->setWordWrap(true);
        if(stale && !staleReason.isEmpty()) {
            warning->setText(QString("Warning: %1").arg(staleReason));
        }

        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(8);
        QPushButton* approveButton = new QPushButton(stale ? "Approve Anyway" : "Approve", row);
        QPushButton* dismissButton = new QPushButton("Dismiss", row);
        buttonLayout->addWidget(approveButton);
        buttonLayout->addWidget(dismissButton);
        buttonLayout->addStretch(1);

        rowLayout->addWidget(title);
        rowLayout->addWidget(details);
        rowLayout->addWidget(reason);
        rowLayout->addWidget(warning);
        rowLayout->addLayout(buttonLayout);
        m_confirmationLayout->addWidget(row);

        connect(approveButton, &QPushButton::clicked, this, [this, commandText]() {
            emit confirmationRequested(commandText);
        });
        connect(dismissButton, &QPushButton::clicked, this, [this, commandText]() {
            emit confirmationDismissed(commandText);
        });
    }

    m_confirmationLayout->addStretch(1);
}
