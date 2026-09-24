//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     agentchatdockwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the agent chat widget used by the workbench sidebar.
 */

#ifndef MNE_ANALYZE_STUDIO_AGENTCHATDOCKWIDGET_H
#define MNE_ANALYZE_STUDIO_AGENTCHATDOCKWIDGET_H

#include <QJsonArray>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QTextEdit;
class QVBoxLayout;
class QWidget;

namespace MNEANALYZESTUDIO
{

class PillSelectorWidget;

/**
 * @brief Sidebar chat widget that captures agent prompts and displays transcript history.
 *        Layout follows VS Code's chat panel conventions: a header toolbar with New Chat /
 *        History navigation, a stacked content area (current chat | history list | session
 *        detail), and a compact composer footer with connection + safety-level controls.
 */
class AgentChatDockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AgentChatDockWidget(QWidget* parent = nullptr);

    // Connection / LLM status
    void setPlannerStatus(const QString& statusText);
    void setConnectionProfiles(const QStringList& profiles, const QString& currentProfile);
    void setConnectionModes(const QList<QPair<QString, QString>>& modes, const QString& currentMode);
    void setSuggestedModels(const QStringList& models, const QString& currentModel);
    void setConnectionState(const QString& stateText, bool warning, const QString& detailMessage = QString());
    void setPlannerSafetyLevel(const QString& level);

    // Conversation state
    QJsonArray archivedConversationSessions() const;
    QJsonArray currentConversationEntries() const;
    void restoreConversationState(const QJsonArray& currentEntries, const QJsonArray& archivedSessions);

signals:
    void commandSubmitted(const QString& commandText);
    void confirmationRequested(const QString& commandText);
    void confirmationDismissed(const QString& commandText);
    void connectionProfileSelected(const QString& profileName);
    void connectionModeSelected(const QString& mode);
    void connectionModelSelected(const QString& model);
    void openConnectionSettingsRequested();
    void plannerSafetyLevelSelected(const QString& level); ///< "auto" | "confirm" | "safe"

public slots:
    void appendTranscript(const QString& text);
    void setPendingConfirmations(const QJsonArray& confirmations);

private:
    void archiveCurrentConversation();
    void refreshCurrentTranscriptView();
    void refreshArchivedSessions();
    void refreshArchivedSessionView();
    void showCurrentConversation();
    void showHistoryList();
    void showArchivedSession(int index);
    void updateHeaderForPage(int pageIndex);

    // Header
    QLabel*      m_titleLabel;
    QPushButton* m_newChatButton;
    QPushButton* m_historyButton;

    // Main stacked area: 0=current chat, 1=history list, 2=session detail
    QStackedWidget* m_mainStack;
    QWidget*        m_currentConversationPage;
    QWidget*        m_historyListPage;
    QWidget*        m_historyDetailPage;

    // Current conversation page
    QLabel*          m_confirmationLabel;
    QWidget*         m_confirmationPanel;
    QVBoxLayout*     m_confirmationLayout;
    QTextEdit*       m_transcript;

    // History list page
    QScrollArea*  m_archivedSessionsScrollArea;
    QWidget*      m_archivedSessionsPanel;
    QVBoxLayout*  m_archivedSessionsLayout;

    // History detail page
    QLabel*       m_archivedTranscriptContextLabel;
    QPushButton*  m_backToSessionsButton;
    QTextEdit*    m_archivedTranscript;

    // Composer footer
    PillSelectorWidget* m_modeSelector;
    PillSelectorWidget* m_modelSelector;
    PillSelectorWidget* m_safetySelector;
    PillSelectorWidget* m_profileSelector;
    QPushButton*        m_connectionSettingsButton;
    QLabel*             m_validationHintLabel;
    QLabel*             m_statusLabel;
    QLineEdit*          m_input;
    QPushButton*        m_sendButton;

    // State
    QJsonArray m_pendingConfirmations;
    QJsonArray m_currentConversationEntries;
    QJsonArray m_archivedConversationSessions;
    int        m_activeArchivedSessionIndex = -1;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_AGENTCHATDOCKWIDGET_H
