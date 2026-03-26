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

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QJsonObject>

using namespace MNEANALYZESTUDIO;

namespace
{

// ──────────────────────────────────────────────────────────────────────────────
// Render one transcript entry as VS Code-inspired HTML.
// User messages: right-aligned with a blue-tinted bubble.
// Agent/system messages: left-aligned with a coloured left-bar accent.
// ──────────────────────────────────────────────────────────────────────────────
QString transcriptHtml(const QString& text, const QString& isoTimestamp = QString())
{
    const QString trimmed = text.trimmed();
    const int sep = trimmed.indexOf('>');
    const QString speaker = sep > 0 ? trimmed.left(sep).trimmed() : QStringLiteral("Studio");
    const QString body    = sep > 0 ? trimmed.mid(sep + 1).trimmed() : trimmed;

    // Per-speaker accent colour (badge / left-bar)
    QString accent       = QStringLiteral("#3b82f6");
    QString speakerColor = QStringLiteral("#9aa6b2");
    bool    isUser       = false;

    if(speaker == QLatin1String("You") || speaker == QLatin1String("Agent")) {
        accent       = QStringLiteral("#0ea5e9");
        speakerColor = QStringLiteral("#7ab8e8");
        isUser       = true;
    } else if(speaker == QLatin1String("Planner")) {
        accent       = QStringLiteral("#8b5cf6");
        speakerColor = QStringLiteral("#a78bfa");
    } else if(speaker == QLatin1String("Studio")) {
        accent       = QStringLiteral("#10b981");
        speakerColor = QStringLiteral("#34d399");
    } else if(speaker == QLatin1String("Kernel")) {
        accent       = QStringLiteral("#f59e0b");
        speakerColor = QStringLiteral("#fbbf24");
    } else if(speaker == QLatin1String("Extension Host")) {
        accent       = QStringLiteral("#f97316");
        speakerColor = QStringLiteral("#fb923c");
    }

    const QString safeBody    = body.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br/>"));
    const QString safeSpeaker = speaker.toHtmlEscaped();

    // Convert ISO timestamp → local "hh:mm"
    QString displayTime;
    if(!isoTimestamp.isEmpty()) {
        const QDateTime dt = QDateTime::fromString(isoTimestamp, Qt::ISODate);
        displayTime = dt.isValid() ? dt.toLocalTime().toString(QStringLiteral("hh:mm")) : QString();
    }
    if(displayTime.isEmpty()) {
        displayTime = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"));
    }

    if(isUser) {
        // Right-aligned user message with a blue bubble
        return QString(
            "<div style='margin:2px 0 10px 0;'>"
              "<table width='100%' cellspacing='0' cellpadding='0'><tr>"
                "<td width='10%'></td>"
                "<td>"
                  "<div style='text-align:right; margin-bottom:3px;'>"
                    "<span style='color:%1; font-size:11px; font-weight:600;'>%2</span>"
                    "<span style='color:#4a5568; font-size:10px;'> &middot; %3</span>"
                  "</div>"
                  "<div style='background:#1a3354; border:1px solid #2a4f7a;"
                       " border-radius:10px 2px 10px 10px; padding:8px 12px;"
                       " color:#c9d1d9; line-height:1.5;'>%4</div>"
                "</td>"
              "</tr></table>"
            "</div>")
            .arg(speakerColor, safeSpeaker, displayTime, safeBody);
    }

    // Left-aligned system / agent message with coloured left bar
    return QString(
        "<div style='margin:2px 0 10px 0;'>"
          "<div style='margin-bottom:3px;'>"
            "<span style='color:%1; font-size:11px; font-weight:600;'>%2</span>"
            "<span style='color:#4a5568; font-size:10px;'> &middot; %3</span>"
          "</div>"
          "<div style='border-left:3px solid %4; padding:6px 10px;"
               " color:#c9d1d9; line-height:1.5;'>%5</div>"
        "</div>")
        .arg(speakerColor, safeSpeaker, displayTime, accent, safeBody);
}

// Helper: append one HTML entry to a QTextEdit.
void appendHtmlEntry(QTextEdit* edit, const QString& html)
{
    QTextCursor cursor = edit->textCursor();
    cursor.movePosition(QTextCursor::End);
    if(!edit->document()->isEmpty()) {
        cursor.insertBlock();
    }
    cursor.insertHtml(html);
    cursor.insertBlock();
    edit->setTextCursor(cursor);
    edit->moveCursor(QTextCursor::End);
}

} // namespace


AgentChatDockWidget::AgentChatDockWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel(QStringLiteral("Conversation"), this))
, m_newChatButton(new QPushButton(QStringLiteral("+ New"), this))
, m_historyButton(new QPushButton(QStringLiteral("History"), this))
, m_mainStack(new QStackedWidget(this))
, m_currentConversationPage(new QWidget(this))
, m_historyListPage(new QWidget(this))
, m_historyDetailPage(new QWidget(this))
, m_confirmationLabel(new QLabel(QStringLiteral("Pending Confirmations"), this))
, m_confirmationPanel(new QWidget(this))
, m_confirmationLayout(new QVBoxLayout(m_confirmationPanel))
, m_transcript(new QTextEdit(this))
, m_archivedSessionsScrollArea(new QScrollArea(this))
, m_archivedSessionsPanel(new QWidget(this))
, m_archivedSessionsLayout(new QVBoxLayout(m_archivedSessionsPanel))
, m_archivedTranscriptContextLabel(new QLabel(this))
, m_backToSessionsButton(new QPushButton(QStringLiteral("\u2190 Back"), this))
, m_archivedTranscript(new QTextEdit(this))
, m_modeSelector(new PillSelectorWidget(this))
, m_modelSelector(new PillSelectorWidget(this))
, m_safetySelector(new PillSelectorWidget(this))
, m_profileSelector(new PillSelectorWidget(this))
, m_connectionSettingsButton(new QPushButton(QStringLiteral("\u2699"), this))
, m_validationHintLabel(new QLabel(this))
, m_statusLabel(new QLabel(QStringLiteral("Rule-based planning is active."), this))
, m_input(new QLineEdit(this))
, m_sendButton(new QPushButton(QStringLiteral("\u2191"), this))
{
    setObjectName(QStringLiteral("agentChatDock"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ── Global stylesheet ──────────────────────────────────────────────────
    setStyleSheet(
        // Root widget – flat dark background, no borders
        "QWidget#agentChatDock { background: #1a1e27; }"

        // Header strip
        "QWidget#agentChatHeader { background: #1a1e27; border-bottom: 1px solid #252c38; }"
        "QLabel#agentChatTitle { color: #e6edf3; font-weight: 600; font-size: 13px; }"

        // Icon-style header buttons – flat, text only
        "QPushButton#agentHeaderBtn {"
        "  background: transparent; color: #8b95a1; border: none;"
        "  border-radius: 6px; padding: 4px 8px; font-size: 12px; }"
        "QPushButton#agentHeaderBtn:hover { background: #252c38; color: #e6edf3; }"
        "QPushButton#agentHeaderBtn:pressed { background: #2e3748; }"

        // Transcript – transparent, no scrollbar styling needed
        "QTextEdit#agentTranscript { background: transparent; color: #c9d1d9; border: none; }"
        "QScrollArea#agentHistoryScroll { background: transparent; border: none; }"
        "QWidget#agentHistoryPanel { background: transparent; }"

        // History session cards
        "QFrame#agentSessionCard { background: #20262f; border: 1px solid #2f3843; border-radius: 8px; }"
        "QFrame#agentSessionCard:hover { background: #252c38; border-color: #3b82f6; }"

        // History detail header
        "QPushButton#agentBackBtn {"
        "  background: transparent; color: #8b95a1; border: none;"
        "  border-radius: 6px; padding: 4px 8px; font-size: 12px; }"
        "QPushButton#agentBackBtn:hover { background: #252c38; color: #e6edf3; }"
        "QLabel#agentSessionTitleLabel { color: #9aa6b2; font-size: 12px; }"

        // Confirmation panel
        "QLabel#agentConfirmHeader { color: #fbbf24; font-size: 11px; font-weight: 600; }"
        "QFrame#agentConfirmCard { background: #20262f; border: 1px solid #f59e0b;"
        "  border-radius: 8px; }"

        // Composer panel – sits at the bottom, slightly raised background
        "QFrame#agentComposer { background: #1f242e; border-top: 1px solid #252c38; }"

        // Status / meta labels inside composer
        "QLabel#agentStatusLabel { color: #4a5568; font-size: 10px; }"
        "QLabel#agentValidationLabel { color: #f87171; font-size: 11px; }"

        // Input field
        "QLineEdit#agentInput {"
        "  background: #161b22; color: #e6edf3; border: 1px solid #2f3843;"
        "  border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        "QLineEdit#agentInput:focus { border-color: #3b82f6; }"

        // Send button
        "QPushButton#agentSendBtn {"
        "  background: #0f7ae5; color: white; border: none;"
        "  border-radius: 8px; padding: 8px 14px; font-size: 16px; font-weight: 600; }"
        "QPushButton#agentSendBtn:hover { background: #218bff; }"
        "QPushButton#agentSendBtn:disabled { background: #253040; color: #4a5568; }"

        // Connect settings button
        "QPushButton#agentConnectBtn {"
        "  background: #252c38; color: #9aa6b2; border: 1px solid #2f3843;"
        "  border-radius: 6px; padding: 5px 8px; font-size: 14px; }"
        "QPushButton#agentConnectBtn:hover { background: #2e3748; color: #e6edf3; }"

        // Approve / Dismiss confirmation buttons
        "QPushButton#agentApproveBtn {"
        "  background: #0f7ae5; color: white; border: none;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "QPushButton#agentApproveBtn:hover { background: #218bff; }"
        "QPushButton#agentDismissBtn {"
        "  background: #252c38; color: #9aa6b2; border: 1px solid #2f3843;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "QPushButton#agentDismissBtn:hover { background: #2e3748; color: #e6edf3; }"
    );

    // ── Header ──────────────────────────────────────────────────────────────
    QWidget* header = new QWidget(this);
    header->setObjectName(QStringLiteral("agentChatHeader"));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 6, 8, 6);
    headerLayout->setSpacing(4);

    m_titleLabel->setObjectName(QStringLiteral("agentChatTitle"));
    m_newChatButton->setObjectName(QStringLiteral("agentHeaderBtn"));
    m_newChatButton->setToolTip(QStringLiteral("Start a new conversation"));
    m_historyButton->setObjectName(QStringLiteral("agentHeaderBtn"));
    m_historyButton->setToolTip(QStringLiteral("Browse prior sessions"));

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_newChatButton);
    headerLayout->addWidget(m_historyButton);

    // ── Page 0: Current conversation ─────────────────────────────────────
    m_confirmationLabel->setObjectName(QStringLiteral("agentConfirmHeader"));
    m_confirmationLabel->setVisible(false);
    m_confirmationPanel->setVisible(false);
    m_confirmationLayout->setContentsMargins(0, 0, 0, 0);
    m_confirmationLayout->setSpacing(6);

    m_transcript->setObjectName(QStringLiteral("agentTranscript"));
    m_transcript->setReadOnly(true);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_transcript->document()->setDocumentMargin(10);
    m_transcript->setPlaceholderText(
        QStringLiteral("Your conversation and analysis trail will appear here."));

    QVBoxLayout* currentLayout = new QVBoxLayout(m_currentConversationPage);
    currentLayout->setContentsMargins(0, 0, 0, 0);
    currentLayout->setSpacing(0);
    currentLayout->addWidget(m_confirmationLabel);
    currentLayout->addWidget(m_confirmationPanel);
    currentLayout->addWidget(m_transcript, 1);

    // ── Page 1: History list ──────────────────────────────────────────────
    m_archivedSessionsPanel->setObjectName(QStringLiteral("agentHistoryPanel"));
    m_archivedSessionsLayout->setContentsMargins(0, 0, 0, 0);
    m_archivedSessionsLayout->setSpacing(6);

    m_archivedSessionsScrollArea->setObjectName(QStringLiteral("agentHistoryScroll"));
    m_archivedSessionsScrollArea->setWidgetResizable(true);
    m_archivedSessionsScrollArea->setFrameShape(QFrame::NoFrame);
    m_archivedSessionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_archivedSessionsScrollArea->viewport()->setStyleSheet(
        QStringLiteral("background: transparent;"));
    m_archivedSessionsScrollArea->setWidget(m_archivedSessionsPanel);

    QVBoxLayout* historyListLayout = new QVBoxLayout(m_historyListPage);
    historyListLayout->setContentsMargins(10, 8, 10, 8);
    historyListLayout->setSpacing(0);
    historyListLayout->addWidget(m_archivedSessionsScrollArea, 1);

    // ── Page 2: History session detail ───────────────────────────────────
    m_backToSessionsButton->setObjectName(QStringLiteral("agentBackBtn"));
    m_archivedTranscriptContextLabel->setObjectName(QStringLiteral("agentSessionTitleLabel"));
    m_archivedTranscript->setObjectName(QStringLiteral("agentTranscript"));
    m_archivedTranscript->setReadOnly(true);
    m_archivedTranscript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_archivedTranscript->document()->setDocumentMargin(10);
    m_archivedTranscript->setPlaceholderText(
        QStringLiteral("Open a prior session to inspect its transcript here."));

    QHBoxLayout* detailHeaderLayout = new QHBoxLayout;
    detailHeaderLayout->setContentsMargins(0, 0, 0, 0);
    detailHeaderLayout->setSpacing(4);
    detailHeaderLayout->addWidget(m_backToSessionsButton);
    detailHeaderLayout->addWidget(m_archivedTranscriptContextLabel, 1);

    QVBoxLayout* detailLayout = new QVBoxLayout(m_historyDetailPage);
    detailLayout->setContentsMargins(10, 8, 10, 8);
    detailLayout->setSpacing(6);
    detailLayout->addLayout(detailHeaderLayout);
    detailLayout->addWidget(m_archivedTranscript, 1);

    // ── Assemble main stack ───────────────────────────────────────────────
    m_mainStack->addWidget(m_currentConversationPage); // index 0
    m_mainStack->addWidget(m_historyListPage);          // index 1
    m_mainStack->addWidget(m_historyDetailPage);        // index 2
    m_mainStack->setCurrentIndex(0);

    // ── Composer panel ────────────────────────────────────────────────────
    QFrame* composer = new QFrame(this);
    composer->setObjectName(QStringLiteral("agentComposer"));

    // Safety selector options
    m_safetySelector->setPlaceholderText(QStringLiteral("Safety"));
    m_safetySelector->setEmptyText(QStringLiteral("Safety"));
    m_safetySelector->setItems({
        {QStringLiteral("auto"),    QStringLiteral("Auto")},
        {QStringLiteral("confirm"), QStringLiteral("Confirm")},
        {QStringLiteral("safe"),    QStringLiteral("Safe")}
    });
    m_safetySelector->setCurrentValue(QStringLiteral("auto"));
    m_safetySelector->setToolTip(
        QStringLiteral("Auto — auto-run safe steps\n"
                        "Confirm — ask before every step\n"
                        "Safe — suggestions only, nothing auto-runs"));

    m_modeSelector->setPlaceholderText(QStringLiteral("Provider"));
    m_modeSelector->setEmptyText(QStringLiteral("Provider"));
    m_modelSelector->setPlaceholderText(QStringLiteral("Model"));
    m_modelSelector->setEmptyText(QStringLiteral("No models"));
    m_modelSelector->setVisible(false);

    m_profileSelector->setPlaceholderText(QStringLiteral("Profile"));
    m_profileSelector->setEmptyText(QStringLiteral("No profiles"));
    m_profileSelector->setVisible(false);

    m_connectionSettingsButton->setObjectName(QStringLiteral("agentConnectBtn"));
    m_connectionSettingsButton->setToolTip(QStringLiteral("Open connection settings"));
    m_connectionSettingsButton->setFixedWidth(36);

    // Controls row: [Mode] [Model] [Safety] [⚙]
    QHBoxLayout* controlsRow = new QHBoxLayout;
    controlsRow->setContentsMargins(0, 0, 0, 0);
    controlsRow->setSpacing(4);
    controlsRow->addWidget(m_modeSelector, 2);
    controlsRow->addWidget(m_modelSelector, 3);
    controlsRow->addWidget(m_safetySelector, 2);
    controlsRow->addWidget(m_connectionSettingsButton, 0);

    // Profile row (only shown when profiles exist)
    QHBoxLayout* profileRow = new QHBoxLayout;
    profileRow->setContentsMargins(0, 0, 0, 0);
    profileRow->setSpacing(4);
    profileRow->addWidget(m_profileSelector, 1);
    profileRow->addStretch(2);

    m_input->setObjectName(QStringLiteral("agentInput"));
    m_input->setPlaceholderText(
        QStringLiteral("Ask Analyze Studio or call a tool... (Enter to send)"));

    m_sendButton->setObjectName(QStringLiteral("agentSendBtn"));
    m_sendButton->setFixedWidth(40);
    m_sendButton->setToolTip(QStringLiteral("Send (Enter)"));

    QHBoxLayout* inputRow = new QHBoxLayout;
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->setSpacing(6);
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(m_sendButton, 0);

    m_validationHintLabel->setObjectName(QStringLiteral("agentValidationLabel"));
    m_validationHintLabel->setWordWrap(true);
    m_validationHintLabel->setVisible(false);

    m_statusLabel->setObjectName(QStringLiteral("agentStatusLabel"));
    m_statusLabel->setWordWrap(false);

    QVBoxLayout* composerLayout = new QVBoxLayout(composer);
    composerLayout->setContentsMargins(10, 10, 10, 10);
    composerLayout->setSpacing(6);
    composerLayout->addLayout(controlsRow);
    composerLayout->addLayout(profileRow);
    composerLayout->addLayout(inputRow);
    composerLayout->addWidget(m_validationHintLabel);
    composerLayout->addWidget(m_statusLabel);

    // ── Root layout ───────────────────────────────────────────────────────
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(header, 0);
    rootLayout->addWidget(m_mainStack, 1);
    rootLayout->addWidget(composer, 0);

    // ── Connections ───────────────────────────────────────────────────────
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

    connect(m_newChatButton, &QPushButton::clicked, this, [this]() {
        archiveCurrentConversation();
        m_activeArchivedSessionIndex = -1;
        showCurrentConversation();
    });

    connect(m_historyButton, &QPushButton::clicked, this, [this]() {
        if(m_mainStack->currentIndex() == 0) {
            showHistoryList();
        } else {
            showCurrentConversation();
        }
    });

    connect(m_backToSessionsButton, &QPushButton::clicked, this, &AgentChatDockWidget::showHistoryList);

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
    connect(m_safetySelector, &PillSelectorWidget::currentValueChanged, this, [this](const QString& value) {
        if(!value.trimmed().isEmpty()) {
            emit plannerSafetyLevelSelected(value.trimmed());
        }
    });
    connect(m_connectionSettingsButton, &QPushButton::clicked,
            this, &AgentChatDockWidget::openConnectionSettingsRequested);

    refreshCurrentTranscriptView();
    refreshArchivedSessions();
    refreshArchivedSessionView();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void AgentChatDockWidget::setPlannerStatus(const QString& statusText)
{
    QString simplified = statusText.trimmed();
    if(simplified.startsWith(QLatin1String("LLM: Connected"))) {
        const QStringList parts = simplified.split(QLatin1Char('|'));
        QString provider;
        QString model;
        for(const QString& part : parts) {
            const QString p = part.trimmed();
            if(p.startsWith(QLatin1String("Provider:"))) {
                provider = p.mid(9).trimmed();
            } else if(p.startsWith(QLatin1String("Model:"))) {
                model = p.mid(6).trimmed();
            }
        }
        simplified = QString("Connected \u2014 %1%2")
                         .arg(provider.isEmpty() ? QStringLiteral("LLM") : provider,
                              model.isEmpty() ? QString() : QString(" \u00b7 %1").arg(model));
    } else if(simplified.contains(QLatin1String("Deterministic fallback only"))) {
        simplified = QStringLiteral("Rule-based planning active");
    }
    m_statusLabel->setText(simplified);
}

void AgentChatDockWidget::setConnectionState(const QString& stateText,
                                              bool            warning,
                                              const QString&  detailMessage)
{
    const bool isRuleBased =
        stateText.trimmed().compare(QLatin1String("Rule-based"), Qt::CaseInsensitive) == 0;

    // Gear button becomes a warning indicator if something is wrong
    m_connectionSettingsButton->setText(warning ? QStringLiteral("!") : QStringLiteral("\u2699"));
    m_connectionSettingsButton->setObjectName(warning ? QStringLiteral("agentConnectBtnWarn")
                                                      : QStringLiteral("agentConnectBtn"));
    m_connectionSettingsButton->setVisible(!isRuleBased);
    m_connectionSettingsButton->setToolTip(
        detailMessage.isEmpty() ? QStringLiteral("Open connection settings") : detailMessage);

    const QString trimmedDetail = detailMessage.trimmed();
    m_validationHintLabel->setText(trimmedDetail);
    m_validationHintLabel->setVisible(!trimmedDetail.isEmpty() && warning && !isRuleBased);

    style()->unpolish(m_connectionSettingsButton);
    style()->polish(m_connectionSettingsButton);
}

void AgentChatDockWidget::setConnectionProfiles(const QStringList& profiles,
                                                 const QString&     currentProfile)
{
    QList<QPair<QString, QString>> items;
    for(const QString& p : profiles) {
        items.append(qMakePair(p, p));
    }
    m_profileSelector->setItems(items);
    m_profileSelector->setCurrentValue(currentProfile.trimmed());
    m_profileSelector->setVisible(!profiles.isEmpty());
}

void AgentChatDockWidget::setConnectionModes(const QList<QPair<QString, QString>>& modes,
                                              const QString& currentMode)
{
    m_modeSelector->setItems(modes);
    m_modeSelector->setCurrentValue(currentMode.trimmed());
}

void AgentChatDockWidget::setSuggestedModels(const QStringList& models,
                                              const QString&     currentModel)
{
    QList<QPair<QString, QString>> items;
    for(const QString& m : models) {
        items.append(qMakePair(m, m));
    }
    m_modelSelector->setItems(items);
    m_modelSelector->setCurrentValue(currentModel.trimmed());
    m_modelSelector->setVisible(!models.isEmpty());
}

void AgentChatDockWidget::setPlannerSafetyLevel(const QString& level)
{
    const QString normalized = level.trimmed().toLower();
    if(normalized == QLatin1String("auto")
       || normalized == QLatin1String("confirm")
       || normalized == QLatin1String("safe")) {
        m_safetySelector->setCurrentValue(normalized);
    }
}

void AgentChatDockWidget::appendTranscript(const QString& text)
{
    const QString trimmed = text.trimmed();
    if(trimmed.isEmpty()) {
        return;
    }

    // A new user turn archives the previous conversation automatically.
    if(trimmed.startsWith(QLatin1String("You>"))) {
        archiveCurrentConversation();
        m_activeArchivedSessionIndex = -1;
    }

    const QString isoNow = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_currentConversationEntries.append(QJsonObject{
        {QStringLiteral("text"),      trimmed},
        {QStringLiteral("timestamp"), isoNow}
    });

    // Switch to current conversation page and append incrementally.
    if(m_mainStack->currentIndex() != 0) {
        showCurrentConversation();
    }
    appendHtmlEntry(m_transcript, transcriptHtml(trimmed, isoNow));
}

QJsonArray AgentChatDockWidget::archivedConversationSessions() const
{
    return m_archivedConversationSessions;
}

QJsonArray AgentChatDockWidget::currentConversationEntries() const
{
    return m_currentConversationEntries;
}

void AgentChatDockWidget::restoreConversationState(const QJsonArray& currentEntries,
                                                    const QJsonArray& archivedSessions)
{
    m_currentConversationEntries    = currentEntries;
    m_archivedConversationSessions  = archivedSessions;
    m_activeArchivedSessionIndex    = -1;
    m_mainStack->setCurrentIndex(0);
    refreshCurrentTranscriptView();
    refreshArchivedSessions();
    refreshArchivedSessionView();
    updateHeaderForPage(0);
}

void AgentChatDockWidget::setPendingConfirmations(const QJsonArray& confirmations)
{
    m_pendingConfirmations = confirmations;
    while(QLayoutItem* item = m_confirmationLayout->takeAt(0)) {
        if(QWidget* w = item->widget()) {
            w->deleteLater();
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
        const QString commandText = confirmation.value(QStringLiteral("command")).toString().trimmed();
        if(commandText.isEmpty()) {
            continue;
        }

        QFrame* card = new QFrame(m_confirmationPanel);
        card->setObjectName(QStringLiteral("agentConfirmCard"));
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(5);

        auto makeLabel = [&](const QString& txt, bool wordWrap = true) {
            QLabel* lbl = new QLabel(txt, card);
            lbl->setWordWrap(wordWrap);
            lbl->setStyleSheet(QStringLiteral("color: #c9d1d9; font-size: 12px;"));
            return lbl;
        };

        QLabel* titleLbl = makeLabel(
            confirmation.value(QStringLiteral("title")).toString(
                QString("Proposal %1").arg(i + 1)));
        titleLbl->setStyleSheet(
            QStringLiteral("color: #e6edf3; font-size: 12px; font-weight: 600;"));

        QLabel* detailsLbl = makeLabel(
            confirmation.value(QStringLiteral("details")).toString(commandText));

        const QString reasonText = confirmation.value(QStringLiteral("reason")).toString().trimmed();
        QLabel* reasonLbl = makeLabel(reasonText.isEmpty()
                                          ? QString()
                                          : QString("Why: %1").arg(reasonText));
        reasonLbl->setVisible(!reasonText.isEmpty());
        reasonLbl->setStyleSheet(QStringLiteral("color: #9aa6b2; font-size: 11px;"));

        const bool    stale       = confirmation.value(QStringLiteral("stale")).toBool(false);
        const QString staleReason = confirmation.value(QStringLiteral("stale_reason")).toString().trimmed();
        QLabel* warningLbl        = makeLabel(
            (stale && !staleReason.isEmpty()) ? QString("Warning: %1").arg(staleReason) : QString());
        warningLbl->setVisible(stale && !staleReason.isEmpty());
        warningLbl->setStyleSheet(QStringLiteral("color: #fbbf24; font-size: 11px;"));

        QPushButton* approveBtn = new QPushButton(
            stale ? QStringLiteral("Approve Anyway") : QStringLiteral("Approve"), card);
        approveBtn->setObjectName(QStringLiteral("agentApproveBtn"));
        QPushButton* dismissBtn = new QPushButton(QStringLiteral("Dismiss"), card);
        dismissBtn->setObjectName(QStringLiteral("agentDismissBtn"));

        QHBoxLayout* btnRow = new QHBoxLayout;
        btnRow->setContentsMargins(0, 0, 0, 0);
        btnRow->setSpacing(6);
        btnRow->addWidget(approveBtn);
        btnRow->addWidget(dismissBtn);
        btnRow->addStretch(1);

        cardLayout->addWidget(titleLbl);
        cardLayout->addWidget(detailsLbl);
        cardLayout->addWidget(reasonLbl);
        cardLayout->addWidget(warningLbl);
        cardLayout->addLayout(btnRow);
        m_confirmationLayout->addWidget(card);

        connect(approveBtn, &QPushButton::clicked, this, [this, commandText]() {
            emit confirmationRequested(commandText);
        });
        connect(dismissBtn, &QPushButton::clicked, this, [this, commandText]() {
            emit confirmationDismissed(commandText);
        });
    }
    m_confirmationLayout->addStretch(1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void AgentChatDockWidget::updateHeaderForPage(int pageIndex)
{
    switch(pageIndex) {
    case 0:
        m_titleLabel->setText(QStringLiteral("Conversation"));
        m_historyButton->setText(QStringLiteral("History"));
        m_historyButton->setToolTip(QStringLiteral("Browse prior sessions"));
        break;
    case 1:
        m_titleLabel->setText(QStringLiteral("History"));
        m_historyButton->setText(QStringLiteral("\u2190 Chat"));
        m_historyButton->setToolTip(QStringLiteral("Return to current conversation"));
        break;
    case 2:
        m_titleLabel->setText(QStringLiteral("Session"));
        m_historyButton->setText(QStringLiteral("\u2190 Chat"));
        m_historyButton->setToolTip(QStringLiteral("Return to current conversation"));
        break;
    default:
        break;
    }
}

void AgentChatDockWidget::showCurrentConversation()
{
    m_mainStack->setCurrentIndex(0);
    updateHeaderForPage(0);
    refreshCurrentTranscriptView();
}

void AgentChatDockWidget::showHistoryList()
{
    m_activeArchivedSessionIndex = -1;
    m_mainStack->setCurrentIndex(1);
    updateHeaderForPage(1);
    refreshArchivedSessions();
}

void AgentChatDockWidget::showArchivedSession(int index)
{
    if(index < 0 || index >= m_archivedConversationSessions.size()) {
        return;
    }
    m_activeArchivedSessionIndex = index;
    refreshArchivedSessionView();
    m_mainStack->setCurrentIndex(2);
    updateHeaderForPage(2);
}

void AgentChatDockWidget::refreshCurrentTranscriptView()
{
    m_transcript->clear();
    m_confirmationLabel->setVisible(!m_pendingConfirmations.isEmpty());
    m_confirmationPanel->setVisible(!m_pendingConfirmations.isEmpty());

    for(const QJsonValue& value : std::as_const(m_currentConversationEntries)) {
        const QJsonObject entry     = value.toObject();
        const QString     text      = entry.value(QStringLiteral("text")).toString().trimmed();
        const QString     timestamp = entry.value(QStringLiteral("timestamp")).toString();
        if(text.isEmpty()) {
            continue;
        }
        appendHtmlEntry(m_transcript, transcriptHtml(text, timestamp));
    }
}

void AgentChatDockWidget::refreshArchivedSessions()
{
    while(QLayoutItem* item = m_archivedSessionsLayout->takeAt(0)) {
        if(QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    if(m_archivedConversationSessions.isEmpty()) {
        QLabel* emptyLabel = new QLabel(
            QStringLiteral("No prior sessions yet."), m_archivedSessionsPanel);
        emptyLabel->setWordWrap(true);
        emptyLabel->setStyleSheet(QStringLiteral("color: #4a5568; font-size: 12px;"));
        m_archivedSessionsLayout->addWidget(emptyLabel);
        m_archivedSessionsLayout->addStretch(1);
        return;
    }

    for(int i = 0; i < m_archivedConversationSessions.size(); ++i) {
        const QJsonObject session  = m_archivedConversationSessions.at(i).toObject();
        const QString     title    = session.value(QStringLiteral("title")).toString(
            QString("Earlier Session %1").arg(i + 1));
        const QString     preview  = session.value(QStringLiteral("preview")).toString().trimmed();
        const QString     tsIso    = session.value(QStringLiteral("timestamp")).toString();
        QString displayTime;
        if(!tsIso.isEmpty()) {
            const QDateTime dt = QDateTime::fromString(tsIso, Qt::ISODate);
            displayTime = dt.isValid()
                ? dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"))
                : QString();
        }

        QFrame* card = new QFrame(m_archivedSessionsPanel);
        card->setObjectName(QStringLiteral("agentSessionCard"));
        card->setCursor(Qt::PointingHandCursor);
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(3);

        QLabel* titleLbl = new QLabel(title, card);
        titleLbl->setStyleSheet(
            QStringLiteral("color: #e6edf3; font-size: 12px; font-weight: 600;"));
        titleLbl->setWordWrap(false);

        QLabel* timeLbl = new QLabel(displayTime, card);
        timeLbl->setStyleSheet(
            QStringLiteral("color: #4a5568; font-size: 10px;"));

        QLabel* previewLbl = new QLabel(
            preview.isEmpty() ? QStringLiteral("No preview available.") : preview, card);
        previewLbl->setWordWrap(true);
        previewLbl->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 11px;"));

        cardLayout->addWidget(titleLbl);
        if(!displayTime.isEmpty()) {
            cardLayout->addWidget(timeLbl);
        }
        cardLayout->addWidget(previewLbl);

        m_archivedSessionsLayout->addWidget(card);

        // Clicking anywhere on the card opens the session.
        connect(card, &QFrame::destroyed, this, []() {}); // ensure object lifetime
        // Use a QPushButton-style click by installing an event filter via lambda is tricky;
        // instead, add a transparent "Open" button that covers the card via mouse press.
        // Simpler: re-use a QPushButton with flat styling filling the card.
        QPushButton* openBtn = new QPushButton(QString(), card);
        openBtn->setFlat(true);
        openBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        openBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: none; }"
        ));
        openBtn->setGeometry(card->rect());
        openBtn->raise();
        cardLayout->addWidget(openBtn);

        connect(openBtn, &QPushButton::clicked, this, [this, i]() {
            showArchivedSession(i);
        });
    }

    m_archivedSessionsLayout->addStretch(1);
}

void AgentChatDockWidget::refreshArchivedSessionView()
{
    m_archivedTranscript->clear();
    if(m_activeArchivedSessionIndex < 0
       || m_activeArchivedSessionIndex >= m_archivedConversationSessions.size()) {
        m_archivedTranscriptContextLabel->setText(
            QStringLiteral("Select a prior session"));
        return;
    }

    const QJsonObject session = m_archivedConversationSessions
                                    .at(m_activeArchivedSessionIndex).toObject();
    const QString title = session.value(QStringLiteral("title")).toString(
        QString("Session %1").arg(m_activeArchivedSessionIndex + 1));
    m_archivedTranscriptContextLabel->setText(title);

    const QJsonArray entries = session.value(QStringLiteral("entries")).toArray();
    for(const QJsonValue& value : entries) {
        const QJsonObject entry     = value.toObject();
        const QString     text      = entry.value(QStringLiteral("text")).toString().trimmed();
        const QString     timestamp = entry.value(QStringLiteral("timestamp")).toString();
        if(text.isEmpty()) {
            continue;
        }
        appendHtmlEntry(m_archivedTranscript, transcriptHtml(text, timestamp));
    }
}

void AgentChatDockWidget::archiveCurrentConversation()
{
    if(m_currentConversationEntries.isEmpty()) {
        return;
    }

    // Derive the title from the first user message.
    QString title =
        QStringLiteral("Session ")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"));

    for(const QJsonValue& v : std::as_const(m_currentConversationEntries)) {
        const QString text = v.toObject().value(QStringLiteral("text")).toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }
        const int sep = text.indexOf('>');
        const QString speaker = sep > 0 ? text.left(sep).trimmed() : QString();
        const QString body    = sep > 0 ? text.mid(sep + 1).trimmed() : text;
        if(speaker == QLatin1String("You") && !body.isEmpty()) {
            title = body.left(60);
            break;
        }
    }

    // Build a short preview from the first two bodies.
    QStringList previewLines;
    for(const QJsonValue& v : std::as_const(m_currentConversationEntries)) {
        const QString text = v.toObject().value(QStringLiteral("text")).toString().trimmed();
        if(text.isEmpty()) {
            continue;
        }
        const int sep = text.indexOf('>');
        const QString body = sep > 0 ? text.mid(sep + 1).trimmed() : text;
        if(!body.isEmpty()) {
            previewLines << body;
        }
        if(previewLines.size() >= 2) {
            break;
        }
    }

    m_archivedConversationSessions.prepend(QJsonObject{
        {QStringLiteral("title"),     title},
        {QStringLiteral("preview"),   previewLines.join(QStringLiteral(" | ")).left(180)},
        {QStringLiteral("entries"),   m_currentConversationEntries},
        {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    });

    // Keep at most 20 prior sessions.
    while(m_archivedConversationSessions.size() > 20) {
        m_archivedConversationSessions.removeLast();
    }

    m_currentConversationEntries = QJsonArray();
    m_transcript->clear();
}
