//=============================================================================================================
/**
 * @file     llmsettingsdialog.h
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
 * @brief    Declares the in-app LLM planner settings dialog.
 */

#ifndef MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H
#define MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H

#include <llmtoolplanner.h>

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QTextEdit;
class QTextBrowser;

namespace MNEANALYZESTUDIO
{

class LlmSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LlmSettingsDialog(const LlmPlannerConfig& config, QWidget* parent = nullptr);

    LlmPlannerConfig configuration() const;
    void setTestScenario(const QString& prompt,
                         const QJsonArray& toolDefinitions,
                         const QJsonObject& context);

private slots:
    void runPlannerTest();
    void updateModeDefaults();
    void saveCurrentProfile();
    void deleteCurrentProfile();
    void applySelectedProfile(int index);
    void applySuggestedModel();
    void browseModels();
    void openProviderConsole();
    void openProviderDocs();

private:
    void refreshProfiles();
    void refreshSuggestedModels();
    void refreshProviderInstructions();
    QString resolvedEndpointForMode(const QString& mode) const;
    QStringList fetchAvailableModels(QString* errorMessage = nullptr) const;

    QComboBox* m_profileComboBox;
    QPushButton* m_saveProfileButton;
    QPushButton* m_deleteProfileButton;
    QComboBox* m_modeComboBox;
    QLineEdit* m_providerLineEdit;
    QLineEdit* m_endpointLineEdit;
    QLineEdit* m_modelLineEdit;
    QLineEdit* m_apiKeyLineEdit;
    QComboBox* m_suggestedModelComboBox;
    QPushButton* m_applySuggestedModelButton;
    QPushButton* m_browseModelsButton;
    QTextBrowser* m_providerInstructionsView;
    QPushButton* m_openConsoleButton;
    QPushButton* m_openDocsButton;
    QTextEdit* m_toolInventoryView;
    QLabel* m_testStatusLabel;
    QPushButton* m_testButton;
    QString m_testPrompt;
    QJsonArray m_testToolDefinitions;
    QJsonObject m_testContext;
    QNetworkAccessManager* m_networkAccessManager;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_LLMSETTINGSDIALOG_H
