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
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

AgentChatDockWidget::AgentChatDockWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel("Agentic Coworker"))
, m_statusLabel(new QLabel("LLM: Deterministic fallback only"))
, m_connectionLabel(new QLabel("Quick Connect"))
, m_connectionHintLabel(new QLabel("Connect once, then switch provider/model here."))
, m_profileComboBox(new QComboBox(this))
, m_modeComboBox(new QComboBox(this))
, m_modelComboBox(new QComboBox(this))
, m_connectionStateLabel(new QLabel("Missing key", this))
, m_connectionSettingsButton(new QPushButton("Connect...", this))
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
    m_connectionLabel->setObjectName("agentChatStatus");
    m_connectionHintLabel->setObjectName("agentChatStatus");
    m_connectionHintLabel->setWordWrap(true);
    m_connectionStateLabel->setObjectName("agentChatStatus");
    m_connectionStateLabel->setStyleSheet("QLabel { padding: 2px 8px; border-radius: 10px; background: #1f6feb; color: white; }");
    m_profileComboBox->setPlaceholderText("Profile");
    m_modeComboBox->setPlaceholderText("Provider");
    m_modelComboBox->setPlaceholderText("Model");
    m_profileComboBox->setMinimumContentsLength(12);
    m_modeComboBox->setMinimumContentsLength(14);
    m_modelComboBox->setMinimumContentsLength(20);
    m_confirmationLabel->setObjectName("agentChatStatus");
    m_confirmationLabel->setVisible(false);
    m_confirmationPanel->setVisible(false);
    m_confirmationLayout->setContentsMargins(0, 0, 0, 0);
    m_confirmationLayout->setSpacing(8);
    m_transcript->setReadOnly(true);
    m_transcript->setPlaceholderText("Agent history will appear here.");
    m_input->setPlaceholderText("Ask Analyze Studio or call a tool...");

    QHBoxLayout* composerLayout = new QHBoxLayout;
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(8);
    composerLayout->addWidget(m_input, 1);
    composerLayout->addWidget(m_sendButton);

    QHBoxLayout* connectionLayout = new QHBoxLayout;
    connectionLayout->setContentsMargins(0, 0, 0, 0);
    connectionLayout->setSpacing(8);
    connectionLayout->addWidget(m_connectionLabel);
    connectionLayout->addWidget(m_profileComboBox, 2);
    connectionLayout->addWidget(m_modeComboBox, 2);
    connectionLayout->addWidget(m_modelComboBox, 3);
    connectionLayout->addStretch(1);
    connectionLayout->addWidget(m_connectionStateLabel);
    connectionLayout->addWidget(m_connectionSettingsButton);

    QFrame* composerPanel = new QFrame(this);
    composerPanel->setObjectName("agentComposerPanel");
    composerPanel->setStyleSheet(
        "QFrame#agentComposerPanel {"
        " border: 1px solid palette(mid);"
        " border-radius: 14px;"
        " background: palette(base);"
        "}"
        "QFrame#agentComposerPanel QLineEdit { border: none; background: transparent; padding: 4px 2px; }"
        "QFrame#agentComposerPanel QComboBox { border: none; background: transparent; padding: 2px 4px; }"
        "QFrame#agentComposerPanel QPushButton { border-radius: 10px; padding: 4px 10px; }");
    QVBoxLayout* composerPanelLayout = new QVBoxLayout(composerPanel);
    composerPanelLayout->setContentsMargins(12, 10, 12, 10);
    composerPanelLayout->setSpacing(8);
    composerPanelLayout->addLayout(composerLayout);
    composerPanelLayout->addLayout(connectionLayout);
    composerPanelLayout->addWidget(m_connectionHintLabel);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_confirmationLabel);
    layout->addWidget(m_confirmationPanel);
    layout->addWidget(m_transcript, 1);
    layout->addWidget(composerPanel);

    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        const QString text = m_input->text().trimmed();
        if(text.isEmpty()) {
            return;
        }

        appendTranscript(QString("Agent> %1").arg(text));
        emit commandSubmitted(text);
        m_input->clear();
    });
    connect(m_profileComboBox,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString& text) {
                if(!text.trimmed().isEmpty()) {
                    emit connectionProfileSelected(text.trimmed());
                }
            });
    connect(m_modeComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if(index >= 0) {
                    emit connectionModeSelected(m_modeComboBox->itemData(index).toString());
                }
            });
    connect(m_modelComboBox,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString& text) {
                if(!text.trimmed().isEmpty()) {
                    emit connectionModelSelected(text.trimmed());
                }
            });
    connect(m_connectionSettingsButton, &QPushButton::clicked, this, &AgentChatDockWidget::openConnectionSettingsRequested);
}

void AgentChatDockWidget::setPlannerStatus(const QString& statusText)
{
    m_statusLabel->setText(statusText);
}

void AgentChatDockWidget::setConnectionState(const QString& stateText, bool warning)
{
    m_connectionStateLabel->setText(stateText);
    m_connectionStateLabel->setStyleSheet(warning
        ? "QLabel { padding: 2px 8px; border-radius: 10px; background: #d29922; color: #111111; }"
        : "QLabel { padding: 2px 8px; border-radius: 10px; background: #1f6feb; color: white; }");
}

void AgentChatDockWidget::setConnectionProfiles(const QStringList& profiles, const QString& currentProfile)
{
    const QSignalBlocker blocker(m_profileComboBox);
    m_profileComboBox->clear();
    m_profileComboBox->addItem("No Profile");
    for(const QString& profile : profiles) {
        m_profileComboBox->addItem(profile);
    }

    const int index = m_profileComboBox->findText(currentProfile.trimmed());
    m_profileComboBox->setCurrentIndex(index >= 0 ? index : 0);
}

void AgentChatDockWidget::setConnectionModes(const QList<QPair<QString, QString>>& modes, const QString& currentMode)
{
    const QSignalBlocker blocker(m_modeComboBox);
    m_modeComboBox->clear();
    for(const QPair<QString, QString>& mode : modes) {
        m_modeComboBox->addItem(mode.first, mode.second);
    }

    const int index = m_modeComboBox->findData(currentMode.trimmed());
    m_modeComboBox->setCurrentIndex(index >= 0 ? index : 0);
}

void AgentChatDockWidget::setSuggestedModels(const QStringList& models, const QString& currentModel)
{
    const QSignalBlocker blocker(m_modelComboBox);
    m_modelComboBox->clear();
    for(const QString& model : models) {
        m_modelComboBox->addItem(model, model);
    }
    if(!currentModel.trimmed().isEmpty() && m_modelComboBox->findData(currentModel.trimmed()) < 0) {
        m_modelComboBox->addItem(currentModel.trimmed(), currentModel.trimmed());
    }

    const int index = m_modelComboBox->findData(currentModel.trimmed());
    m_modelComboBox->setCurrentIndex(index >= 0 ? index : 0);
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
