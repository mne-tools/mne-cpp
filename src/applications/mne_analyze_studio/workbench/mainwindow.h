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
#include <llmtoolplanner.h>

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QHash>
#include <QList>
#include <QLocalSocket>
#include <QStringList>

class QCloseEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QComboBox;
class QPlainTextEdit;
class QSpinBox;
class QTableWidget;
class QTreeWidget;
class QSplitter;
class QWidget;
class QTextEdit;
class QToolButton;
class QTabWidget;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QProcess;

namespace MNEANALYZESTUDIO
{

class AgentChatDockWidget;
class AnalysisResultWidget;
class Dummy3DHostedViewWidget;
class EditorTabBar;
class EditorTabWidget;
class ExtensionHostedViewWidget;
class IRawDataView;
class ViewProviderRegistry;
class WorkflowMiniMapWidget;

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

private slots:
    void handleHostedExtensionViewOutput(const QString& message);
    void handleHostedExtensionViewStatus(const QString& message);
    void handleHostedExtensionViewCommand(const QString& sessionId, const QString& commandName, const QJsonObject& arguments);
    void handleResultRendererToolCommand(const QString& commandText);
    void handleResultRendererSelectionContext(const QJsonObject& context);
    void approvePlannerConfirmation(const QString& commandText);
    void dismissPlannerConfirmation(const QString& commandText);
    void handleAgentConnectionProfileSelected(const QString& profileName);
    void handleAgentConnectionModeSelected(const QString& mode);
    void handleAgentConnectionModelSelected(const QString& model);
    void handleAgentPlannerSafetyLevelSelected(const QString& level);

private:
    IRawDataView* activeRawDataView() const;
    QString planAgentSteps(const QString& commandText, QStringList& plannedCommands, bool& planned) const;
    QString planAgentIntent(const QString& commandText, QString& plannedCommand, bool& planned) const;
    QString resolvePlannerReferences(const QString& commandText) const;
    QJsonObject normalizedToolResultEnvelope(const QString& toolName,
                                            const QJsonObject& result,
                                            const QString& source = QString()) const;
    QJsonObject normalizedToolErrorEnvelope(const QString& toolName,
                                           const QString& message,
                                           const QString& source,
                                           const QString& failureKind,
                                           const QString& recoverability,
                                           const QJsonObject& details = QJsonObject()) const;
    void rememberToolResult(const QString& toolName, const QJsonObject& result);
    void updateActivePipelineArtifact(const QString& status, const QString& currentStep = QString());
    void failActivePipeline(const QString& failureMessage);
    QJsonObject analysisPipelineContract(const QString& pipelineId) const;
    QJsonObject defaultInputsForPipeline(const QJsonObject& pipeline) const;
    QJsonValue pipelineTemplateValueToJson(const QString& resolvedText) const;
    QJsonObject resolvePipelineStepArguments(const QJsonObject& step,
                                            const QJsonObject& pipelineInputs) const;
    QString buildToolCallCommand(const QString& toolName, const QJsonObject& arguments) const;
    QJsonObject pipelineRunArtifact(const QString& runId) const;
    QString validateAnalysisPipelineContract(const QJsonObject& pipeline,
                                            const QJsonObject& pipelineInputs) const;
    QString rerunPipelineStepCommand(const QString& runId,
                                    int stepNumber,
                                    const QString& mode,
                                    QString* errorMessage = nullptr) const;
    QString resolvePipelineCommandTemplate(const QString& commandTemplate,
                                          const QJsonObject& pipelineInputs) const;
    bool executeAnalysisPipeline(const QString& pipelineId,
                                 const QJsonObject& pipelineInputs,
                                 const QJsonObject& inputOverrides = QJsonObject());
    bool resumeAnalysisPipeline(const QJsonObject& artifactResult);
    void continuePendingPipelineExecution();
    void refreshStructuredResultHistoryUi();
    void updateStructuredResultView(const QString& toolName, const QJsonObject& result);
    QJsonArray resultHistoryForTool(const QString& toolName) const;
    QJsonObject resultRendererRuntimeContext() const;
    QJsonArray resultRendererContracts() const;
    QJsonArray extensionSettingsContracts() const;
    QJsonArray extensionSettingsState() const;
    QJsonArray analysisPipelineContracts() const;
    QJsonArray analysisPipelineToolDefinitions() const;
    QJsonArray extensionSettingsToolDefinitions() const;
    QJsonObject resolveExtensionSettingsTool(const QString& toolName) const;
    QVariant extensionSettingValue(const QString& extensionId,
                                   const QString& tabId,
                                   const QJsonObject& field) const;
    QStringList candidateSettingFieldIds(const QString& inputName) const;
    QJsonObject applyExtensionSettingDefaults(const QString& extensionId,
                                              const QJsonObject& schemaProperties,
                                              const QJsonObject& currentValues) const;
    QStringList extensionIdsForTool(const QString& toolName) const;
    QWidget* ensureBottomResultRenderer(const QString& toolName);
    QString handleLocalAgentCommand(const QString& commandText, bool& handled);
    QString handleStructuredToolCommand(const QString& commandText, bool& handled);
    QJsonArray localToolDefinitions() const;
    QJsonArray kernelToolDefinitions() const;
    void requestKernelToolDefinitions();
    void requestExtensionHostState();
    void requestExtensionHostReload();
    QJsonArray availableToolDefinitions() const;
    QJsonObject plannerAnnotatedToolDefinition(const QJsonObject& tool) const;
    QJsonArray plannerAnnotatedToolDefinitions() const;
    QJsonObject plannerSafetyMetadata(const QString& toolName) const;
    QJsonObject plannerReadinessMetadata(const QString& toolName) const;
    QJsonObject plannerExecutionMetadata(const QString& toolName) const;
    QJsonArray plannerSafeToolDefinitions() const;
    QJsonArray plannerReadyToolDefinitions() const;
    QJsonArray plannerBlockedToolDefinitions() const;
    QJsonObject toolDefinition(const QString& toolName) const;
    QJsonObject llmPlanningContext(const QString& commandText) const;
    QString toolNameFromCommand(const QString& commandText) const;
    QJsonObject toolArgumentsFromCommand(const QString& commandText) const;
    QJsonObject plannerConfirmationPresentation(const QString& commandText,
                                               int stepIndex,
                                               int totalSteps,
                                               const QString& fallbackDetails,
                                               const QString& plannerSummary,
                                               const QString& previousPlannedCommand) const;
    QJsonObject plannerConfirmationSnapshot(const QString& commandText) const;
    QJsonObject plannerConfirmationStaleness(const QJsonObject& confirmation) const;
    void refreshPlannerConfirmationsUi();
    void queuePlannerConfirmation(const QString& commandText,
                                  const QString& summary,
                                  const QString& details,
                                  const QString& reason = QString());
    bool removePlannerConfirmation(const QString& commandText);
    QJsonObject defaultArgumentsForTool(const QString& toolName) const;
    QJsonObject activeHostedViewSession() const;
    QJsonArray activeHostedViewToolDefinitions() const;
    bool editArgumentsForTool(const QString& toolName, QJsonObject& arguments);
    void reloadExtensionRegistry();
    void refreshExtensionManagerUi();
    void updateSelectedExtension(QTreeWidgetItem* item);
    void installExtensionFromDirectory();
    void toggleSelectedExtensionEnabled();
    void openSelectedExtensionSettingsTab();
    void openExtensionSettingsTab(const QString& extensionId, const QString& settingsTabId);
    void requestExtensionViewOpen(const QString& filePath, const QJsonObject& dispatch);
    void finalizeExtensionViewOpen(const QString& filePath, const QJsonObject& sessionDescriptor);
    void sendExtensionViewCommand(const QString& sessionId, const QString& commandName, const QJsonObject& arguments);
    void requestWorkflowLoad(const QString& filePath);
    void requestActiveWorkflowGraph();
    void adoptWorkflowGraph(const QJsonObject& result, const QString& fallbackFilePath = QString());
    void openWorkflowCenterView(bool focusTab = true);
    void refreshWorkflowCenterView();
    bool isWorkflowCenterViewOpen() const;
    void refreshWorkflowGraphDockingUi();
    QJsonArray workflowOperatorToolDefinitions() const;
    QJsonObject workflowOperatorToolDefinition(const QString& toolName) const;
    QJsonObject defaultArgumentsForWorkflowTool(const QString& toolName) const;
    QString selectedWorkflowArtifactUid() const;
    QString resolveStudioCompanionExecutable(const QString& executableName) const;
    void ensureBackendConnection(QLocalSocket* socket,
                                 const QString& socketName,
                                 const QString& executableName,
                                 QProcess* process,
                                 const QString& displayName);
    void shutdownManagedBackends();
    void shutdownManagedBackend(QLocalSocket* socket,
                                QProcess* process,
                                const QString& displayName);
    void setWorkflowStatusBanner(const QString& message, const QString& severity = QStringLiteral("info"));
    void refreshWorkflowStatusBanner();
    void rebuildWorkflowNavigatorUi();
    void updateSelectedWorkflowItem(QTreeWidgetItem* item);
    void appendWorkflowStep();
    void saveActiveWorkflowGraph();
    void openSelectedWorkflowFile();
    QString openableWorkflowPathForItem(QTreeWidgetItem* item) const;
    bool isExtensionHostTool(const QString& toolName) const;
    void rebuildSkillsExplorer();
    void updateSelectedSkillTool(QTreeWidgetItem* item);
    void runSelectedSkillTool();
    void openAgentSettings();
    void loadAgentSettings();
    void persistAgentSettings() const;
    void persistAgentValidationState(bool hasResult, bool succeeded, const QString& message) const;
    void refreshAgentPlannerStatus();
    void refreshAgentConnectionSelectors();
    QJsonObject buildRawWindowArguments(int windowSamples) const;
    void sendKernelToolCall(const QString& toolName, const QJsonObject& arguments);
    void sendExtensionToolCall(const QString& toolName, const QJsonObject& arguments);
    QWidget* createSidebarSection(const QString& title, QWidget* contentWidget);
    void createLayout();
    void createConnections();
    void addWorkspaceDirectory(const QString& directoryPath);
    void refreshWorkspaceArtifacts();
    void openAnalysisArtifact(const QJsonObject& entry, bool focusBottomResults = false);
    void openWorkspaceItem(QTreeWidgetItem* item);
    void openFileInView(const QString& filePath);
    bool handleThreeDFileOpen(const QString& filePath, const QJsonObject& dispatch);
    bool addFileToThreeDView(Dummy3DHostedViewWidget* targetView, const QString& filePath);
    QJsonObject dispatchForNewThreeDScene(const QString& filePath, const QJsonObject& dispatch);
    Dummy3DHostedViewWidget* currentThreeDView() const;
    Dummy3DHostedViewWidget* threeDViewContainingFile(const QString& filePath) const;
    QList<Dummy3DHostedViewWidget*> openThreeDViews() const;
    int centerTabIndexForWidget(QWidget* widget) const;
    void applyWorkbenchStyle();
    void switchPrimarySidebar(const QString& sectionName);
    void sendToolCall(const QString& commandText);
    void closeCenterTab(int index);
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
    QToolButton* m_workflowButton;
    QToolButton* m_skillsButton;
    QToolButton* m_extensionsButton;
    QTreeWidget* m_workspaceExplorer;
    QTreeWidget* m_workflowExplorer;
    QWidget* m_workflowPage;
    QLabel* m_workflowStatusBanner;
    QLabel* m_workflowMiniMapDockStateLabel;
    QPushButton* m_workflowOpenGraphButton;
    QWidget* m_workflowCenterView;
    QLabel* m_workflowCenterSummaryLabel;
    WorkflowMiniMapWidget* m_workflowCenterMiniMap;
    QPlainTextEdit* m_workflowCenterDetailsView;
    QPushButton* m_workflowCenterOpenSourceButton;
    QPushButton* m_workflowCenterDockButton;
    QPushButton* m_workflowCenterAddStepButton;
    QPushButton* m_workflowCenterSaveButton;
    WorkflowMiniMapWidget* m_workflowMiniMap;
    QPlainTextEdit* m_workflowDetailsView;
    QPushButton* m_workflowAddStepButton;
    QPushButton* m_workflowSaveButton;
    QPushButton* m_workflowOpenFileButton;
    QPushButton* m_workflowRefreshButton;
    QTreeWidget* m_skillsExplorer;
    QWidget* m_skillsPage;
    QPlainTextEdit* m_skillDetailsView;
    QPushButton* m_skillRunButton;
    QWidget* m_extensionsPage;
    QTreeWidget* m_extensionsExplorer;
    QPlainTextEdit* m_extensionDetailsView;
    QPushButton* m_extensionInstallButton;
    QPushButton* m_extensionToggleButton;
    QPushButton* m_extensionSettingsButton;
    QStackedWidget* m_leftSidebarStack;
    QSplitter* m_mainSplitter;
    QSplitter* m_centerSplitter;
    EditorTabWidget* m_centerTabs;
    EditorTabBar* m_centerTabBar;
    QTabWidget* m_bottomPanelTabs;
    QListWidget* m_outputPanel;
    QListWidget* m_problemPanel;
    QWidget* m_resultsTab;
    QLabel* m_resultsTitleLabel;
    QComboBox* m_resultsHistoryCombo;
    QStackedWidget* m_resultsStack;
    QTreeWidget* m_resultsTree;
    QTableWidget* m_resultsTable;
    QWidget* m_resultsExtensionRenderer;
    QWidget* m_terminalTab;
    QLabel* m_terminalStatusLabel;
    QTextEdit* m_terminalPanel;
    QLineEdit* m_terminalInput;
    QPushButton* m_terminalRunButton;
    QListWidgetItem* m_activeStateItem;
    QLocalSocket* m_kernelSocket;
    QLocalSocket* m_extensionSocket;
    QProcess* m_kernelProcess;
    QProcess* m_extensionHostProcess;
    QString m_lastToolName;
    QJsonObject m_lastToolResult;
    QString m_resultsCurrentToolName;
    QJsonObject m_resultSelectionContext;
    QJsonArray m_pendingPlannerConfirmations;
    QString m_selectedSkillToolName;
    QString m_selectedPipelineId;
    QString m_selectedExtensionId;
    QList<QJsonObject> m_psdResultHistory;
    QList<QJsonObject> m_structuredResultHistory;
    QString m_activePipelineId;
    QString m_activePipelineRunId;
    QString m_activePipelineDisplayName;
    QJsonObject m_activePipelineInputs;
    QJsonObject m_activePipelineInputOverrides;
    QStringList m_pendingPipelineCommands;
    QJsonArray m_activePipelineStepHistory;
    int m_activePipelineTotalSteps;
    QString m_activePipelineLastStatus;
    bool m_isAdvancingPipeline;
    bool m_isShuttingDown;
    QJsonArray m_cachedKernelToolDefinitions;
    QJsonArray m_cachedExtensionToolDefinitions;
    QJsonArray m_cachedExtensionResources;
    QJsonArray m_cachedExtensionViewSessions;
    QJsonObject m_lastExtensionReloadResult;
    QString m_plannerSafetyLevel; ///< "auto" | "confirm" | "safe" — global override for plannerExecutionMetadata
    QString m_activeWorkflowFilePath;
    QJsonObject m_activeWorkflowGraph;
    QString m_workflowStatusMessage;
    QString m_workflowStatusSeverity;
    bool m_activeWorkflowHasUnsavedChanges;
    QHash<QString, QJsonObject> m_pendingExtensionViewOpens;
    QHash<QString, QString> m_pendingExtensionViewFiles;
    QHash<QString, QString> m_pendingWorkflowLoads;
    QHash<QString, QWidget*> m_extensionViewWidgetsBySessionId;
    LlmToolPlanner m_llmPlanner;
    SceneContextRegistry m_sceneRegistry;
    ViewProviderRegistry* m_viewProviderRegistry;
    ViewManager m_viewManager;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_MAINWINDOW_H
