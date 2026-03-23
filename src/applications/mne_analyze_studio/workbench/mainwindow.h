//=============================================================================================================
/**
 * @file     mainwindow.h
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
 * @brief    Declares the main VS Code-style workbench window for MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_MAINWINDOW_H
#define MNE_ANALYZE_STUDIO_MAINWINDOW_H

#include <scenecontextregistry.h>
#include <viewmanager.h>

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocalSocket>
#include <QStringList>

class QCloseEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QSplitter;
class QTextEdit;
class QToolButton;
class QTabWidget;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace MNEANALYZESTUDIO
{

class AgentChatDockWidget;
class RawDataBrowserWidget;

/**
 * @brief Main workbench window coordinating workspace, views, output panels, and agent chat.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openInitialFiles(const QStringList& filePaths);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    RawDataBrowserWidget* activeRawBrowser() const;
    QString planAgentIntent(const QString& commandText, QString& plannedCommand, bool& planned) const;
    QString handleLocalAgentCommand(const QString& commandText, bool& handled);
    QString handleStructuredToolCommand(const QString& commandText, bool& handled);
    QJsonArray localToolDefinitions() const;
    QJsonObject buildRawWindowArguments(int windowSamples) const;
    void sendKernelToolCall(const QString& toolName, const QJsonObject& arguments);
    QWidget* createSidebarSection(const QString& title, QWidget* contentWidget);
    void createLayout();
    void createConnections();
    void addWorkspaceDirectory(const QString& directoryPath);
    void openWorkspaceItem(QTreeWidgetItem* item);
    void openFileInView(const QString& filePath);
    void applyWorkbenchStyle();
    void switchPrimarySidebar(const QString& sectionName);
    void sendToolCall(const QString& commandText);
    void appendOutputMessage(const QString& message);
    void appendProblemMessage(const QString& message);
    void appendTerminalMessage(const QString& message);
    void setActivePanelState(const QString& message);
    void restoreWorkspace();
    void persistWorkspace() const;
    bool isSupportedWorkspaceFile(const QString& filePath) const;

    QWidget* m_rootWidget;
    AgentChatDockWidget* m_agentChatDock;
    QWidget* m_activityBar;
    QToolButton* m_explorerButton;
    QToolButton* m_skillsButton;
    QTreeWidget* m_workspaceExplorer;
    QTreeWidget* m_skillsExplorer;
    QStackedWidget* m_leftSidebarStack;
    QSplitter* m_mainSplitter;
    QSplitter* m_centerSplitter;
    QTabWidget* m_centerTabs;
    QTabWidget* m_bottomPanelTabs;
    QListWidget* m_outputPanel;
    QListWidget* m_problemPanel;
    QWidget* m_terminalTab;
    QLabel* m_terminalStatusLabel;
    QTextEdit* m_terminalPanel;
    QLineEdit* m_terminalInput;
    QPushButton* m_terminalRunButton;
    QListWidgetItem* m_activeStateItem;
    QLocalSocket* m_kernelSocket;
    SceneContextRegistry m_sceneRegistry;
    ViewManager m_viewManager;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_MAINWINDOW_H
