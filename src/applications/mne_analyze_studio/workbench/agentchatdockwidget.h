//=============================================================================================================
/**
 * @file     agentchatdockwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
