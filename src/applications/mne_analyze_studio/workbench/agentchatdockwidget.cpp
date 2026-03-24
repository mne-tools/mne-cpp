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
#include "pillselectorwidget.h"

#include <QJsonObject>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

QString transcriptHtml(const QString& text)
{
    const QString trimmed = text.trimmed();
    const int separatorIndex = trimmed.indexOf('>');
    const QString speaker = separatorIndex > 0 ? trimmed.left(separatorIndex).trimmed() : QString("Studio");
    const QString body = separatorIndex > 0 ? trimmed.mid(separatorIndex + 1).trimmed() : trimmed;

    QString badgeColor = "#3b82f6";
    QString bubbleColor = "#1c232d";
    QString textColor = "#e6edf3";
    bool alignRight = false;

    if(speaker == QLatin1String("You") || speaker == QLatin1String("Agent")) {
        badgeColor = "#0ea5e9";
        bubbleColor = "#16202a";
        alignRight = true;
    } else if(speaker == QLatin1String("Planner")) {
        badgeColor = "#8b5cf6";
        bubbleColor = "#1f1b2d";
    } else if(speaker == QLatin1String("Studio")) {
        badgeColor = "#10b981";
        bubbleColor = "#16241f";
    } else if(speaker == QLatin1String("Kernel")) {
        badgeColor = "#f59e0b";
        bubbleColor = "#2b2316";
    } else if(speaker == QLatin1String("Extension Host")) {
        badgeColor = "#f97316";
        bubbleColor = "#2c2018";
    }

    const QString safeBody = body.toHtmlEscaped().replace('\n', "<br/>");
    const QString safeSpeaker = speaker.toHtmlEscaped();
    const QString containerAlign = alignRight ? "right" : "left";

    return QString(
               "<div style=\"margin: 0 0 10px 0; text-align:%1;\">"
               "<div style=\"display:inline-block; max-width: 96%; text-align:left;\">"
               "<div style=\"margin-bottom:4px;\">"
               "<span style=\"display:inline-block; background:%2; color:white; border-radius:999px; padding:3px 10px; font-size:11px; font-weight:600;\">%3</span>"
               "</div>"
               "<div style=\"background:%4; color:%5; border:1px solid #30363d; border-radius:14px; padding:10px 12px; line-height:1.45;\">%6</div>"
               "</div>"
               "</div>")
        .arg(containerAlign,
             badgeColor,
             safeSpeaker,
             bubbleColor,
             textColor,
             safeBody);
}

}

AgentChatDockWidget::AgentChatDockWidget(QWidget* parent)
: QWidget(parent)
, m_transcriptPanel(new QFrame(this))
, m_conversationTabs(new QTabWidget(this))
, m_currentConversationPage(new QWidget(this))
, m_priorSessionsPage(new QWidget(this))
, m_titleLabel(new QLabel("Conversation"))
, m_statusLabel(new QLabel("LLM: Deterministic fallback only"))
, m_currentConversationLabel(new QLabel("Current Conversation"))
, m_archivedConversationLabel(new QLabel("Prior Sessions"))
, m_transcriptContextLabel(new QLabel("Live session", this))
, m_archivedTranscriptContextLabel(new QLabel("Select a prior session", this))
, m_connectionHintLabel(new QLabel("Connect once, then switch between the active providers here."))
, m_validationHintLabel(new QLabel(this))
, m_profileSelector(new PillSelectorWidget(this))
, m_modeSelector(new PillSelectorWidget(this))
, m_modelSelector(new PillSelectorWidget(this))
, m_connectionStateLabel(new QLabel("Missing key", this))
, m_connectionSettingsButton(new QPushButton("Connect...", this))
, m_confirmationLabel(new QLabel("Pending Confirmations"))
, m_confirmationPanel(new QWidget)
, m_confirmationLayout(new QVBoxLayout(m_confirmationPanel))
, m_archivedSessionsStack(new QStackedWidget(this))
, m_archivedSessionsListPage(new QWidget(this))
, m_archivedSessionViewerPage(new QWidget(this))
, m_archivedSessionsScrollArea(new QScrollArea(this))
, m_archivedSessionsPanel(new QWidget(this))
, m_archivedSessionsLayout(new QVBoxLayout(m_archivedSessionsPanel))
, m_transcript(new QTextEdit)
, m_archivedTranscript(new QTextEdit(this))
, m_backToSessionsButton(new QPushButton("Back To Sessions", this))
, m_input(new QLineEdit)
, m_sendButton(new QPushButton("Send"))
{
    setObjectName("agentChatDock");
    setMinimumWidth(0);
    setMaximumWidth(QWIDGETSIZE_MAX);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    setStyleSheet(
        "QWidget#agentChatDock { background: #1f232a; }"
        "QFrame#agentChatTranscriptPanel, QFrame#agentComposerPanel {"
        "  background: #20262f; border: 1px solid #2f3843; border-radius: 18px; }"
        "QFrame#agentComposerPanel { background: #1c2128; }"
        "QLabel#agentChatSectionTitle { color: #e6edf3; font-weight: 700; font-size: 14px; }"
        "QLabel#agentChatMeta { color: #9aa6b2; font-size: 12px; }"
        "QLabel#agentChatSubsection { color: #c8d1da; font-weight: 600; font-size: 12px; }"
        "QLabel#agentChatStateReady { padding: 5px 12px; border-radius: 999px; background: #0f7ae5; color: white; font-weight: 600; }"
        "QLabel#agentChatStateWarning { padding: 5px 12px; border-radius: 999px; background: #b7791f; color: #111827; font-weight: 600; }"
        "QTabWidget#agentConversationTabs::pane { border: none; background: #20262f; }"
        "QTabWidget#agentConversationTabs QTabBar::tab { background: #1b2129; color: #9aa6b2; padding: 6px 12px; border: 1px solid #2f3843; border-bottom: none; border-radius: 0; margin-right: 2px; }"
        "QTabWidget#agentConversationTabs QTabBar::tab:selected { background: #20262f; color: #e6edf3; }"
        "QTabWidget#agentConversationTabs QTabBar::tab:!selected { margin-top: 2px; }"
        "QTextEdit#agentTranscript { background: transparent; color: #dce3ea; border: none; }"
        "QScrollArea#agentArchivedSessionsScroll { background: #20262f; border: none; }"
        "QWidget#agentArchivedSessionsPanel, QWidget#agentArchivedSessionsListPage, QWidget#agentPriorSessionsPage, QWidget#agentCurrentConversationPage { background: #20262f; }"
        "QLineEdit#agentChatInput { background: #161b22; color: #e6edf3; border: 1px solid #2f3843; border-radius: 14px; padding: 10px 12px; }"
        "QPushButton#agentSecondaryButton { background: #283341; color: #e6edf3; border-radius: 12px; padding: 8px 12px; }"
        "QPushButton#agentSecondaryButton:hover { background: #314052; }"
        "QPushButton#agentPrimaryButton { background: #0f7ae5; color: white; border-radius: 12px; padding: 8px 14px; font-weight: 600; }"
        "QPushButton#agentPrimaryButton:hover { background: #218bff; }");

    m_transcriptPanel->setObjectName("agentChatTranscriptPanel");
    m_conversationTabs->setObjectName("agentConversationTabs");
    m_conversationTabs->setDocumentMode(false);
    m_currentConversationPage->setObjectName("agentCurrentConversationPage");
    m_priorSessionsPage->setObjectName("agentPriorSessionsPage");
    m_archivedSessionsListPage->setObjectName("agentArchivedSessionsListPage");
    m_archivedSessionsPanel->setObjectName("agentArchivedSessionsPanel");
    m_titleLabel->setObjectName("agentChatSectionTitle");
    m_statusLabel->setObjectName("agentChatMeta");
    m_statusLabel->setWordWrap(true);
    m_currentConversationLabel->setObjectName("agentChatSubsection");
    m_archivedConversationLabel->setObjectName("agentChatSubsection");
    m_transcriptContextLabel->setObjectName("agentChatMeta");
    m_archivedTranscriptContextLabel->setObjectName("agentChatMeta");
    m_connectionHintLabel->setObjectName("agentChatMeta");
    m_connectionHintLabel->setWordWrap(true);
    m_validationHintLabel->setObjectName("agentChatMeta");
    m_validationHintLabel->setWordWrap(true);
    m_validationHintLabel->setVisible(false);
    m_connectionStateLabel->setObjectName("agentChatStateReady");
    m_connectionStateLabel->setVisible(false);
    m_profileSelector->setPlaceholderText("Profile");
    m_profileSelector->setEmptyText("No profiles");
    m_modeSelector->setPlaceholderText("Provider");
    m_modeSelector->setEmptyText("Provider");
    m_modelSelector->setPlaceholderText("Select model");
    m_modelSelector->setEmptyText("No models");
    m_confirmationLabel->setObjectName("agentChatMeta");
    m_confirmationLabel->setVisible(false);
    m_confirmationPanel->setVisible(false);
    m_confirmationLayout->setContentsMargins(0, 0, 0, 0);
    m_confirmationLayout->setSpacing(8);
    m_archivedSessionsPanel->setVisible(true);
    m_archivedSessionsLayout->setContentsMargins(0, 0, 0, 0);
    m_archivedSessionsLayout->setSpacing(8);
    m_archivedSessionsScrollArea->setObjectName("agentArchivedSessionsScroll");
    m_archivedSessionsScrollArea->setWidgetResizable(true);
    m_archivedSessionsScrollArea->setFrameShape(QFrame::NoFrame);
    m_archivedSessionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_archivedSessionsScrollArea->viewport()->setStyleSheet("background: #20262f;");
    m_archivedSessionsScrollArea->setWidget(m_archivedSessionsPanel);
    m_transcript->setObjectName("agentTranscript");
    m_transcript->setReadOnly(true);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_transcript->document()->setDocumentMargin(6);
    m_transcript->setPlaceholderText("Your current conversation and analysis trail will appear here.");
    m_archivedTranscript->setObjectName("agentTranscript");
    m_archivedTranscript->setReadOnly(true);
    m_archivedTranscript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_archivedTranscript->document()->setDocumentMargin(6);
    m_archivedTranscript->setPlaceholderText("Open a prior session to inspect its transcript here.");
    m_backToSessionsButton->setObjectName("agentSecondaryButton");
    m_backToSessionsButton->setVisible(false);
    m_input->setObjectName("agentChatInput");
    m_input->setPlaceholderText("Ask Analyze Studio or call a tool...");
    m_sendButton->setObjectName("agentPrimaryButton");
    m_connectionSettingsButton->setObjectName("agentSecondaryButton");
    m_connectionSettingsButton->setText("Connect");

    QHBoxLayout* composerLayout = new QHBoxLayout;
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(8);
    composerLayout->addWidget(m_input, 1);
    composerLayout->addWidget(m_sendButton);

    m_modeSelector->setMinimumWidth(90);
    m_modelSelector->setMinimumWidth(0);
    m_connectionSettingsButton->setMinimumWidth(0);

    QGridLayout* connectionLayout = new QGridLayout;
    connectionLayout->setContentsMargins(0, 0, 0, 0);
    connectionLayout->setHorizontalSpacing(8);
    connectionLayout->setVerticalSpacing(8);
    connectionLayout->addWidget(m_modeSelector, 0, 0);
    connectionLayout->addWidget(m_modelSelector, 1, 0);
    connectionLayout->addWidget(m_connectionSettingsButton, 2, 0);
    connectionLayout->addWidget(m_profileSelector, 3, 0);
    connectionLayout->setColumnStretch(0, 1);

    QFrame* composerPanel = new QFrame(this);
    composerPanel->setObjectName("agentComposerPanel");
    QVBoxLayout* composerPanelLayout = new QVBoxLayout(composerPanel);
    composerPanelLayout->setContentsMargins(14, 14, 14, 14);
    composerPanelLayout->setSpacing(8);
    composerPanelLayout->addLayout(composerLayout);
    composerPanelLayout->addLayout(connectionLayout);
    composerPanelLayout->addWidget(m_validationHintLabel);
    composerPanelLayout->addWidget(m_statusLabel);
    composerPanelLayout->addWidget(m_connectionHintLabel);

    QVBoxLayout* currentConversationLayout = new QVBoxLayout(m_currentConversationPage);
    currentConversationLayout->setContentsMargins(0, 0, 0, 0);
    currentConversationLayout->setSpacing(10);
    currentConversationLayout->addWidget(m_currentConversationLabel);
    currentConversationLayout->addWidget(m_transcriptContextLabel);
    currentConversationLayout->addWidget(m_confirmationLabel);
    currentConversationLayout->addWidget(m_confirmationPanel);
    currentConversationLayout->addWidget(m_transcript, 1);

    QVBoxLayout* archivedSessionsListLayout = new QVBoxLayout(m_archivedSessionsListPage);
    archivedSessionsListLayout->setContentsMargins(0, 0, 0, 0);
    archivedSessionsListLayout->setSpacing(10);
    archivedSessionsListLayout->addWidget(m_archivedConversationLabel);
    archivedSessionsListLayout->addWidget(m_archivedSessionsScrollArea, 1);

    QVBoxLayout* archivedSessionViewerLayout = new QVBoxLayout(m_archivedSessionViewerPage);
    archivedSessionViewerLayout->setContentsMargins(0, 0, 0, 0);
    archivedSessionViewerLayout->setSpacing(10);
    archivedSessionViewerLayout->addWidget(m_archivedTranscriptContextLabel);
    archivedSessionViewerLayout->addWidget(m_backToSessionsButton, 0, Qt::AlignLeft);
    archivedSessionViewerLayout->addWidget(m_archivedTranscript, 1);

    m_archivedSessionsStack->addWidget(m_archivedSessionsListPage);
    m_archivedSessionsStack->addWidget(m_archivedSessionViewerPage);
    m_archivedSessionsStack->setCurrentWidget(m_archivedSessionsListPage);

    QVBoxLayout* priorSessionsLayout = new QVBoxLayout(m_priorSessionsPage);
    priorSessionsLayout->setContentsMargins(0, 0, 0, 0);
    priorSessionsLayout->setSpacing(0);
    priorSessionsLayout->addWidget(m_archivedSessionsStack, 1);

    QVBoxLayout* transcriptPanelLayout = new QVBoxLayout(m_transcriptPanel);
    transcriptPanelLayout->setContentsMargins(14, 14, 14, 14);
    transcriptPanelLayout->setSpacing(10);
    transcriptPanelLayout->addWidget(m_titleLabel);
    m_conversationTabs->addTab(m_currentConversationPage, "Current");
    m_conversationTabs->addTab(m_priorSessionsPage, "Prior Sessions");
    transcriptPanelLayout->addWidget(m_conversationTabs, 1);

    layout->addWidget(m_transcriptPanel, 1);
    layout->addWidget(composerPanel);

    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        const QString text = m_input->text().trimmed();
        if(text.isEmpty()) {
            return;
        }

        appendTranscript(QString("You> %1").arg(text));
        emit commandSubmitted(text);
        m_input->clear();
    });
    connect(m_input, &QLineEdit::returnPressed, m_sendButton, &QPushButton::click);
    connect(m_profileSelector, &PillSelectorWidget::currentValueChanged, this, [this](const QString& value) {
        if(!value.trimmed().isEmpty()) {
            emit connectionProfileSelected(value.trimmed());
        }
    });
    connect(m_modeSelector, &PillSelectorWidget::currentValueChanged, this, [this](const QString& value) {
        if(!value.trimmed().isEmpty()) {
            emit connectionModeSelected(value.trimmed());
        }
    });
    connect(m_modelSelector, &PillSelectorWidget::currentValueChanged, this, [this](const QString& value) {
        if(!value.trimmed().isEmpty()) {
            emit connectionModelSelected(value.trimmed());
        }
    });
    connect(m_connectionSettingsButton, &QPushButton::clicked, this, &AgentChatDockWidget::openConnectionSettingsRequested);
    connect(m_backToSessionsButton, &QPushButton::clicked, this, &AgentChatDockWidget::showArchivedSessionList);

    refreshCurrentTranscriptView();
    refreshArchivedSessions();
    refreshArchivedSessionView();
}

void AgentChatDockWidget::setPlannerStatus(const QString& statusText)
{
    QString simplified = statusText.trimmed();
    if(simplified.startsWith("LLM: Connected")) {
        const QStringList parts = simplified.split('|');
        QString provider;
        QString model;
        for(const QString& part : parts) {
            const QString trimmedPart = part.trimmed();
            if(trimmedPart.startsWith("Provider:")) {
                provider = trimmedPart.mid(QString("Provider:").size()).trimmed();
            } else if(trimmedPart.startsWith("Model:")) {
                model = trimmedPart.mid(QString("Model:").size()).trimmed();
            }
        }
        simplified = QString("Connected to %1%2")
                         .arg(provider.isEmpty() ? QString("LLM") : provider,
                              model.isEmpty() ? QString() : QString(" · %1").arg(model));
    } else if(simplified.contains("Deterministic fallback only")) {
        simplified = QString("Rule-based planning is active on this workspace.");
    }

    m_statusLabel->setText(simplified);
}

void AgentChatDockWidget::setConnectionState(const QString& stateText, bool warning, const QString& detailMessage)
{
    const bool isRuleBased = stateText.trimmed().compare(QString("Rule-based"), Qt::CaseInsensitive) == 0;
    m_connectionStateLabel->setText(stateText);
    m_connectionStateLabel->setObjectName(warning ? "agentChatStateWarning" : "agentChatStateReady");
    m_connectionSettingsButton->setVisible(!isRuleBased);
    m_connectionSettingsButton->setText(warning ? stateText : QString("Connected"));
    m_connectionSettingsButton->setObjectName(warning ? "agentSecondaryButton" : "agentPrimaryButton");
    style()->unpolish(m_connectionStateLabel);
    style()->polish(m_connectionStateLabel);
    style()->unpolish(m_connectionSettingsButton);
    style()->polish(m_connectionSettingsButton);
    const QString trimmedDetail = detailMessage.trimmed();
    m_validationHintLabel->setText(trimmedDetail);
    m_validationHintLabel->setVisible(!trimmedDetail.isEmpty() && !isRuleBased);
    m_connectionSettingsButton->setToolTip(trimmedDetail);
}

void AgentChatDockWidget::setConnectionProfiles(const QStringList& profiles, const QString& currentProfile)
{
    QList<QPair<QString, QString>> items;
    for(const QString& profile : profiles) {
        items.append(qMakePair(profile, profile));
    }
    m_profileSelector->setItems(items);
    m_profileSelector->setCurrentValue(currentProfile.trimmed());
    m_profileSelector->setVisible(!profiles.isEmpty());
}

void AgentChatDockWidget::setConnectionModes(const QList<QPair<QString, QString>>& modes, const QString& currentMode)
{
    m_modeSelector->setItems(modes);
    m_modeSelector->setCurrentValue(currentMode.trimmed());
}

void AgentChatDockWidget::setSuggestedModels(const QStringList& models, const QString& currentModel)
{
    QList<QPair<QString, QString>> items;
    for(const QString& modelName : models) {
        items.append(qMakePair(modelName, modelName));
    }

    m_modelSelector->setItems(items);
    m_modelSelector->setCurrentValue(currentModel.trimmed());
    const bool hasFetchedModels = !models.isEmpty();
    m_modelSelector->setVisible(hasFetchedModels);
}

void AgentChatDockWidget::appendTranscript(const QString& text)
{
    const QString trimmed = text.trimmed();
    if(trimmed.isEmpty()) {
        return;
    }

    const bool startsNewConversation = trimmed.startsWith("You>");
    if(startsNewConversation) {
        archiveCurrentConversation();
        m_activeArchivedSessionIndex = -1;
    }

    m_currentConversationEntries.append(QJsonObject{
        {"text", trimmed},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    });
    m_conversationTabs->setCurrentWidget(m_currentConversationPage);
    refreshCurrentTranscriptView();
}

QJsonArray AgentChatDockWidget::archivedConversationSessions() const
{
    return m_archivedConversationSessions;
}

QJsonArray AgentChatDockWidget::currentConversationEntries() const
{
    return m_currentConversationEntries;
}

void AgentChatDockWidget::restoreConversationState(const QJsonArray& currentEntries, const QJsonArray& archivedSessions)
{
    m_currentConversationEntries = currentEntries;
    m_archivedConversationSessions = archivedSessions;
    m_activeArchivedSessionIndex = -1;
    m_conversationTabs->setCurrentWidget(m_currentConversationPage);
    m_archivedSessionsStack->setCurrentWidget(m_archivedSessionsListPage);
    refreshCurrentTranscriptView();
    refreshArchivedSessions();
    refreshArchivedSessionView();
}

void AgentChatDockWidget::setPendingConfirmations(const QJsonArray& confirmations)
{
    m_pendingConfirmations = confirmations;
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

        QFrame* row = new QFrame(m_confirmationPanel);
        row->setStyleSheet("QFrame { background: #191f27; border: 1px solid #30363d; border-radius: 14px; } QLabel { color: #dce3ea; } QPushButton { border-radius: 10px; }");
        QVBoxLayout* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(10, 10, 10, 10);
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

void AgentChatDockWidget::refreshCurrentTranscriptView()
{
    m_transcript->clear();
    m_transcriptContextLabel->setText("Live session");
    m_confirmationLabel->setVisible(!m_pendingConfirmations.isEmpty());
    m_confirmationPanel->setVisible(!m_pendingConfirmations.isEmpty());

    for(const QJsonValue& value : std::as_const(m_currentConversationEntries)) {
        const QString text = value.toObject().value("text").toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }
        QTextCursor cursor = m_transcript->textCursor();
        cursor.movePosition(QTextCursor::End);
        if(!m_transcript->document()->isEmpty()) {
            cursor.insertBlock();
        }
        cursor.insertHtml(transcriptHtml(text));
        cursor.insertBlock();
        m_transcript->setTextCursor(cursor);
    }
    m_transcript->moveCursor(QTextCursor::End);
}

void AgentChatDockWidget::refreshArchivedSessions()
{
    while(QLayoutItem* item = m_archivedSessionsLayout->takeAt(0)) {
        if(QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if(m_archivedConversationSessions.isEmpty()) {
        QLabel* emptyLabel = new QLabel("No prior sessions yet.", m_archivedSessionsPanel);
        emptyLabel->setWordWrap(true);
        emptyLabel->setStyleSheet("QLabel { color: #9aa6b2; }");
        m_archivedSessionsLayout->addWidget(emptyLabel);
        m_archivedSessionsLayout->addStretch(1);
        return;
    }

    for(int i = 0; i < m_archivedConversationSessions.size(); ++i) {
        const QJsonObject session = m_archivedConversationSessions.at(i).toObject();
        const QString title = session.value("title").toString(QString("Earlier Session %1").arg(i + 1));
        const QString preview = session.value("preview").toString().trimmed();

        QFrame* card = new QFrame(m_archivedSessionsPanel);
        card->setStyleSheet("QFrame { background: #191f27; border: 1px solid #30363d; border-radius: 14px; } QLabel { color: #dce3ea; }");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(4);

        QLabel* titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet("QLabel { font-weight: 600; color: #e6edf3; }");
        QLabel* previewLabel = new QLabel(preview.isEmpty() ? QString("No summary available.") : preview, card);
        previewLabel->setWordWrap(true);
        previewLabel->setStyleSheet("QLabel { color: #9aa6b2; }");
        QPushButton* openButton = new QPushButton("Open", card);
        openButton->setObjectName("agentSecondaryButton");

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(previewLabel);
        cardLayout->addWidget(openButton, 0, Qt::AlignLeft);
        m_archivedSessionsLayout->addWidget(card);

        connect(openButton, &QPushButton::clicked, this, [this, i]() {
            showArchivedSession(i);
        });
    }

    m_archivedSessionsLayout->addStretch(1);
}

void AgentChatDockWidget::showCurrentConversation()
{
    m_conversationTabs->setCurrentWidget(m_currentConversationPage);
}

void AgentChatDockWidget::showArchivedSession(int index)
{
    if(index < 0 || index >= m_archivedConversationSessions.size()) {
        return;
    }

    m_activeArchivedSessionIndex = index;
    refreshArchivedSessionView();
    m_archivedSessionsStack->setCurrentWidget(m_archivedSessionViewerPage);
    m_conversationTabs->setCurrentWidget(m_priorSessionsPage);
}

void AgentChatDockWidget::showArchivedSessionList()
{
    m_activeArchivedSessionIndex = -1;
    m_archivedSessionsStack->setCurrentWidget(m_archivedSessionsListPage);
    refreshArchivedSessionView();
}

void AgentChatDockWidget::archiveCurrentConversation()
{
    if(m_currentConversationEntries.isEmpty()) {
        return;
    }

    QStringList previewLines;
    QString title = QString("Session %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    for(const QJsonValue& value : std::as_const(m_currentConversationEntries)) {
        const QString text = value.toObject().value("text").toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }

        const int separatorIndex = text.indexOf('>');
        const QString speaker = separatorIndex > 0 ? text.left(separatorIndex).trimmed() : QString();
        const QString body = separatorIndex > 0 ? text.mid(separatorIndex + 1).trimmed() : text;
        if(speaker == QLatin1String("You") && !body.isEmpty()) {
            title = body.left(56);
            break;
        }
    }

    for(const QJsonValue& value : std::as_const(m_currentConversationEntries)) {
        const QString text = value.toObject().value("text").toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }

        const int separatorIndex = text.indexOf('>');
        const QString body = separatorIndex > 0 ? text.mid(separatorIndex + 1).trimmed() : text;
        if(!body.isEmpty()) {
            previewLines << body;
        }
        if(previewLines.size() >= 2) {
            break;
        }
    }

    m_archivedConversationSessions.prepend(QJsonObject{
        {"title", title},
        {"preview", previewLines.join(" | ").left(180)},
        {"entries", m_currentConversationEntries},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    });
    while(m_archivedConversationSessions.size() > 8) {
        m_archivedConversationSessions.removeLast();
    }

    m_currentConversationEntries = QJsonArray();
    refreshArchivedSessions();
}

void AgentChatDockWidget::refreshArchivedSessionView()
{
    m_archivedTranscript->clear();
    if(m_activeArchivedSessionIndex < 0 || m_activeArchivedSessionIndex >= m_archivedConversationSessions.size()) {
        m_archivedTranscriptContextLabel->setText("Select a prior session");
        m_backToSessionsButton->setVisible(false);
        return;
    }

    const QJsonObject session = m_archivedConversationSessions.at(m_activeArchivedSessionIndex).toObject();
    const QString title = session.value("title").toString(QString("Earlier Session %1").arg(m_activeArchivedSessionIndex + 1));
    m_archivedTranscriptContextLabel->setText(title);
    m_backToSessionsButton->setVisible(true);

    const QJsonArray entries = session.value("entries").toArray();
    for(const QJsonValue& value : entries) {
        const QString text = value.toObject().value("text").toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }
        QTextCursor cursor = m_archivedTranscript->textCursor();
        cursor.movePosition(QTextCursor::End);
        if(!m_archivedTranscript->document()->isEmpty()) {
            cursor.insertBlock();
        }
        cursor.insertHtml(transcriptHtml(text));
        cursor.insertBlock();
        m_archivedTranscript->setTextCursor(cursor);
    }
    m_archivedTranscript->moveCursor(QTextCursor::End);
}
