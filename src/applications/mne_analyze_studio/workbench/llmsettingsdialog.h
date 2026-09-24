//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     llmsettingsdialog.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the in-app LLM planner settings dialog.
 */

#ifndef MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H
#define MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H

#include <llmtoolplanner.h>

#include <QDialog>

class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QTextEdit;
class QTextBrowser;

namespace MNEANALYZESTUDIO
{

class PillSelectorWidget;

class LlmSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LlmSettingsDialog(const LlmPlannerConfig& config, QWidget* parent = nullptr);

    LlmPlannerConfig configuration() const;
    bool hasValidationResult() const;
    bool lastValidationSucceeded() const;
    QString lastValidationMessage() const;
    void setTestScenario(const QString& prompt,
                         const QJsonArray& toolDefinitions,
                         const QJsonObject& context);

private slots:
    void runPlannerTest();
    void updateModeDefaults();
    void saveCurrentProfile();
    void deleteCurrentProfile();
    void applySelectedProfile(const QString& profileName);
    void applySuggestedModel();
    void editModelManually();
    void browseModels();
    void openProviderConsole();
    void openProviderDocs();

private:
    void refreshProfiles();
    void refreshSuggestedModels();
    void refreshProviderInstructions();
    void updateDialogVisibility();
    QString resolvedEndpointForMode(const QString& mode) const;
    QStringList fetchAvailableModels(QString* errorMessage = nullptr) const;

    PillSelectorWidget* m_profileSelector;
    QPushButton* m_saveProfileButton;
    QPushButton* m_deleteProfileButton;
    PillSelectorWidget* m_modeSelector;
    QLineEdit* m_modelLineEdit;
    QLineEdit* m_apiKeyLineEdit;
    PillSelectorWidget* m_suggestedModelSelector;
    QPushButton* m_editModelButton;
    QPushButton* m_applySuggestedModelButton;
    QPushButton* m_browseModelsButton;
    QTextBrowser* m_providerInstructionsView;
    QPushButton* m_openConsoleButton;
    QPushButton* m_openDocsButton;
    QTextEdit* m_toolInventoryView;
    QLabel* m_testStatusLabel;
    QPushButton* m_testButton;
    bool m_hasValidationResult = false;
    bool m_lastValidationSucceeded = false;
    QString m_lastValidationMessage;
    QString m_lastAppliedMode;
    QString m_testPrompt;
    QJsonArray m_testToolDefinitions;
    QJsonObject m_testContext;
    QNetworkAccessManager* m_networkAccessManager;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H
