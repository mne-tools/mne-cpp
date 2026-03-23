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
, m_transcript(new QTextEdit)
, m_input(new QLineEdit)
, m_sendButton(new QPushButton("Send"))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    m_titleLabel->setObjectName("agentChatTitle");
    m_transcript->setReadOnly(true);
    m_transcript->setPlaceholderText("Agent history will appear here.");
    m_input->setPlaceholderText("Ask the agent or call tools/call...");

    QHBoxLayout* composerLayout = new QHBoxLayout;
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(8);
    composerLayout->addWidget(m_input, 1);
    composerLayout->addWidget(m_sendButton);

    layout->addWidget(m_titleLabel);
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

void AgentChatDockWidget::appendTranscript(const QString& text)
{
    m_transcript->append(text);
}
