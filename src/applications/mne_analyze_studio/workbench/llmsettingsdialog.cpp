//=============================================================================================================
/**
 * @file     llmsettingsdialog.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the in-app LLM planner settings dialog.
 */

#include "llmsettingsdialog.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

#ifdef Q_OS_MACOS
QString llmKeychainService()
{
    return QString("org.mnecpp.mne_analyze_studio.llm");
}

QString readSecretFromKeychain(const QString& accountName)
{
    if(accountName.trimmed().isEmpty()) {
        return QString();
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "find-generic-password"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed()
                                << "-w");
    process.waitForFinished();
    if(process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return QString();
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool writeSecretToKeychain(const QString& accountName, const QString& secret)
{
    if(accountName.trimmed().isEmpty()) {
        return false;
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "add-generic-password"
                                << "-U"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed()
                                << "-w" << secret);
    process.waitForFinished();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void deleteSecretFromKeychain(const QString& accountName)
{
    if(accountName.trimmed().isEmpty()) {
        return;
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "delete-generic-password"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed());
    process.waitForFinished();
}
#else
QString readSecretFromKeychain(const QString&)
{
    return QString();
}

bool writeSecretToKeychain(const QString&, const QString&)
{
    return false;
}

void deleteSecretFromKeychain(const QString&)
{
}
#endif

QString providerSecretAccountName(const QString& profileName)
{
    return QString("profile:%1").arg(profileName.trimmed());
}

QString formatToolInventory(const QJsonArray& toolDefinitions)
{
    QStringList lines;

    for(const QJsonValue& value : toolDefinitions) {
        const QJsonObject tool = value.toObject();
        const QString name = tool.value("name").toString().trimmed();
        const QString description = tool.value("description").toString().trimmed();
        if(name.isEmpty()) {
            continue;
        }

        lines << QString("%1\n  %2").arg(name,
                                          description.isEmpty() ? QString("No description.") : description);
    }

    return lines.isEmpty() ? QString("No tools available.") : lines.join("\n\n");
}

QStringList suggestedModelsForMode(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return QStringList() << "gpt-5-mini" << "gpt-5" << "gpt-4.1-mini";
    }

    if(mode == QLatin1String("gemini_openai")) {
        return QStringList() << "gemini-2.5-flash" << "gemini-2.5-pro" << "gemini-2.0-flash";
    }

    if(mode == QLatin1String("github_models")) {
        return QStringList() << "openai/gpt-4.1-mini"
                             << "openai/gpt-4.1"
                             << "anthropic/claude-sonnet-4"
                             << "meta/llama-3.3-70b-instruct";
    }

    if(mode == QLatin1String("anthropic_messages")) {
        return QStringList() << "claude-sonnet-4-5" << "claude-opus-4-1" << "claude-haiku-3-5";
    }

    if(mode == QLatin1String("http")) {
        return QStringList() << "gpt-4.1-mini" << "llama3.1:8b" << "qwen2.5-coder:7b";
    }

    return QStringList();
}

QString providerConsoleUrl(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return QString("https://platform.openai.com/api-keys");
    }
    if(mode == QLatin1String("gemini_openai")) {
        return QString("https://aistudio.google.com/apikey");
    }
    if(mode == QLatin1String("github_models")) {
        return QString("https://github.com/settings/tokens");
    }
    if(mode == QLatin1String("anthropic_messages")) {
        return QString("https://console.anthropic.com/settings/keys");
    }

    return QString();
}

QString providerDocsUrl(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return QString("https://platform.openai.com/docs/quickstart/authentication");
    }
    if(mode == QLatin1String("gemini_openai")) {
        return QString("https://ai.google.dev/gemini-api/docs/openai");
    }
    if(mode == QLatin1String("github_models")) {
        return QString("https://docs.github.com/en/enterprise-cloud@latest/github-models/quickstart");
    }
    if(mode == QLatin1String("anthropic_messages")) {
        return QString("https://docs.anthropic.com/en/api/getting-started");
    }

    return QString();
}

QString providerInstructionsHtml(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return "<b>OpenAI</b><br/>1. Open the API keys page.<br/>2. Create a new secret key.<br/>3. Paste it here. Studio stores it in macOS Keychain when available.<br/>4. Pick a model like <code>gpt-5-mini</code>.<br/>5. Click <b>Validate Connection</b>.";
    }
    if(mode == QLatin1String("gemini_openai")) {
        return "<b>Google Gemini</b><br/>1. Open Google AI Studio.<br/>2. Create a Gemini API key.<br/>3. Paste it here. Studio stores it in macOS Keychain when available.<br/>4. Pick a model like <code>gemini-2.5-flash</code>.<br/>5. Click <b>Validate Connection</b>.";
    }
    if(mode == QLatin1String("github_models")) {
        return "<b>GitHub Models</b><br/>1. Create a GitHub token with model access.<br/>2. Paste it here. Studio stores it in macOS Keychain when available.<br/>3. Use <b>Browse Models</b> or a suggestion.<br/>4. Click <b>Validate Connection</b>.";
    }
    if(mode == QLatin1String("anthropic_messages")) {
        return "<b>Anthropic Claude</b><br/>1. Open the Anthropic console keys page.<br/>2. Create an API key.<br/>3. Paste it here. Studio stores it in macOS Keychain when available.<br/>4. Pick a Claude model.<br/>5. Click <b>Validate Connection</b>.";
    }

    return "<b>Planner setup</b><br/>Choose a provider, enter credentials if needed, then validate the connection.";
}

}

LlmSettingsDialog::LlmSettingsDialog(const LlmPlannerConfig& config, QWidget* parent)
: QDialog(parent)
, m_profileComboBox(new QComboBox(this))
, m_saveProfileButton(new QPushButton("Save Profile", this))
, m_deleteProfileButton(new QPushButton("Delete Profile", this))
, m_modeComboBox(new QComboBox(this))
, m_providerLineEdit(new QLineEdit(this))
, m_endpointLineEdit(new QLineEdit(this))
, m_modelLineEdit(new QLineEdit(this))
, m_apiKeyLineEdit(new QLineEdit(this))
, m_suggestedModelComboBox(new QComboBox(this))
, m_applySuggestedModelButton(new QPushButton("Use Suggested Model", this))
, m_browseModelsButton(new QPushButton("Browse Models", this))
, m_providerInstructionsView(new QTextBrowser(this))
, m_openConsoleButton(new QPushButton("Open Console", this))
, m_openDocsButton(new QPushButton("Open Docs", this))
, m_toolInventoryView(new QTextEdit(this))
, m_testStatusLabel(new QLabel("No planner test run yet.", this))
, m_testButton(new QPushButton("Validate Connection", this))
, m_networkAccessManager(new QNetworkAccessManager(this))
{
    setWindowTitle("Agent Settings");
    resize(700, 560);

    m_modeComboBox->addItem("Deterministic Fallback", "disabled");
    m_modeComboBox->addItem("Mock Planner", "mock");
    m_modeComboBox->addItem("OpenAI Responses API", "openai_responses");
    m_modeComboBox->addItem("Google Gemini", "gemini_openai");
    m_modeComboBox->addItem("GitHub Models", "github_models");
    m_modeComboBox->addItem("Anthropic Claude", "anthropic_messages");
    m_modeComboBox->addItem("OpenAI-Compatible HTTP", "http");

    const QString mode = config.mode.trimmed().isEmpty() ? QString("disabled") : config.mode.trimmed().toLower();
    const int modeIndex = qMax(0, m_modeComboBox->findData(mode));
    m_modeComboBox->setCurrentIndex(modeIndex);

    m_providerLineEdit->setText(config.providerName);
    m_endpointLineEdit->setText(config.endpoint);
    m_modelLineEdit->setText(config.model);
    m_apiKeyLineEdit->setText(config.apiKey);
    m_apiKeyLineEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    m_profileComboBox->setPlaceholderText("Saved provider profiles");
    m_suggestedModelComboBox->setMinimumContentsLength(28);
    m_providerInstructionsView->setOpenExternalLinks(true);
    m_providerInstructionsView->setMaximumHeight(120);
    m_toolInventoryView->setReadOnly(true);
    m_toolInventoryView->setLineWrapMode(QTextEdit::NoWrap);
    m_toolInventoryView->setPlaceholderText("Available planner tools will appear here.");
    m_testStatusLabel->setWordWrap(true);

    QHBoxLayout* profileRow = new QHBoxLayout;
    profileRow->setContentsMargins(0, 0, 0, 0);
    profileRow->setSpacing(8);
    profileRow->addWidget(m_profileComboBox, 1);
    profileRow->addWidget(m_saveProfileButton);
    profileRow->addWidget(m_deleteProfileButton);

    QHBoxLayout* modelRow = new QHBoxLayout;
    modelRow->setContentsMargins(0, 0, 0, 0);
    modelRow->setSpacing(8);
    modelRow->addWidget(m_modelLineEdit, 1);
    modelRow->addWidget(m_suggestedModelComboBox);
    modelRow->addWidget(m_applySuggestedModelButton);
    modelRow->addWidget(m_browseModelsButton);

    QHBoxLayout* onboardingRow = new QHBoxLayout;
    onboardingRow->setContentsMargins(0, 0, 0, 0);
    onboardingRow->setSpacing(8);
    onboardingRow->addWidget(m_openConsoleButton);
    onboardingRow->addWidget(m_openDocsButton);
    onboardingRow->addStretch(1);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow("Profile", profileRow);
    formLayout->addRow("Mode", m_modeComboBox);
    formLayout->addRow("Provider", m_providerLineEdit);
    formLayout->addRow("Endpoint", m_endpointLineEdit);
    formLayout->addRow("Model", modelRow);
    formLayout->addRow("API Key", m_apiKeyLineEdit);
    formLayout->addRow("Status", m_testStatusLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &LlmSettingsDialog::runPlannerTest);
    connect(m_modeComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &LlmSettingsDialog::updateModeDefaults);
    connect(m_profileComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &LlmSettingsDialog::applySelectedProfile);
    connect(m_saveProfileButton, &QPushButton::clicked, this, &LlmSettingsDialog::saveCurrentProfile);
    connect(m_deleteProfileButton, &QPushButton::clicked, this, &LlmSettingsDialog::deleteCurrentProfile);
    connect(m_applySuggestedModelButton, &QPushButton::clicked, this, &LlmSettingsDialog::applySuggestedModel);
    connect(m_browseModelsButton, &QPushButton::clicked, this, &LlmSettingsDialog::browseModels);
    connect(m_openConsoleButton, &QPushButton::clicked, this, &LlmSettingsDialog::openProviderConsole);
    connect(m_openDocsButton, &QPushButton::clicked, this, &LlmSettingsDialog::openProviderDocs);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(new QLabel("Provider Setup", this));
    layout->addWidget(m_providerInstructionsView);
    layout->addLayout(onboardingRow);
    layout->addWidget(new QLabel("Planner Tool Inventory", this));
    layout->addWidget(m_toolInventoryView, 1);
    layout->addWidget(m_testButton);
    layout->addWidget(buttonBox);

    refreshProfiles();
    updateModeDefaults();
}

LlmPlannerConfig LlmSettingsDialog::configuration() const
{
    return LlmPlannerConfig{
        m_modeComboBox->currentData().toString(),
        m_providerLineEdit->text().trimmed(),
        m_endpointLineEdit->text().trimmed(),
        m_apiKeyLineEdit->text().trimmed(),
        m_modelLineEdit->text().trimmed()
    };
}

void LlmSettingsDialog::setTestScenario(const QString& prompt,
                                        const QJsonArray& toolDefinitions,
                                        const QJsonObject& context)
{
    m_testPrompt = prompt.trimmed();
    m_testToolDefinitions = toolDefinitions;
    m_testContext = context;
    m_toolInventoryView->setPlainText(formatToolInventory(m_testToolDefinitions));
}

void LlmSettingsDialog::runPlannerTest()
{
    LlmToolPlanner planner;
    planner.setConfiguration(configuration());

    const QString prompt = m_testPrompt.isEmpty()
        ? QString("go to the strongest EEG burst and then show me the top EEG channels there")
        : m_testPrompt;

    const QJsonArray tools = m_testToolDefinitions.isEmpty()
        ? QJsonArray{
              QJsonObject{{"name", "view.raw.summary"}, {"description", "Return summary metadata for the active raw browser."}},
              QJsonObject{{"name", "neurokernel.channel_stats"}, {"description", "Compute per-channel statistics for a raw sample window."}},
              QJsonObject{{"name", "neurokernel.psd_summary"}, {"description", "Compute a Welch PSD summary for a raw sample window."}},
              QJsonObject{{"name", "neurokernel.find_peak_window"}, {"description", "Find the strongest absolute-amplitude sample inside a raw window."}}
          }
        : m_testToolDefinitions;

    const QJsonObject context = m_testContext.isEmpty()
        ? QJsonObject{
              {"active_view", "raw_browser"},
              {"active_file", "sample_audvis_raw.fif"},
              {"raw_summary", "Raw Data Browser: sample_audvis_raw.fif | Channels: 376 | Sampling rate: 600.61 Hz"},
              {"raw_state", "Visible range: 25800 to 26500 samples | Cursor: 26100"},
              {"cursor_sample", 26100}
          }
        : m_testContext;

    const LlmPlanResult result = planner.plan(prompt, tools, context);

    if(result.success) {
        m_testStatusLabel->setText(QString("Planner OK: %1 | Steps: %2")
                                   .arg(result.summary.isEmpty() ? QString("plan generated") : result.summary,
                                        result.plannedCommands.join(" | ")));
        return;
    }

    QString failureText = QString("Planner failed: %1")
                              .arg(result.errorMessage.isEmpty() ? QString("unknown error") : result.errorMessage);
    if(result.httpStatusCode > 0) {
        failureText += QString(" | HTTP %1").arg(result.httpStatusCode);
    }
    if(!result.providerErrorType.isEmpty()) {
        failureText += QString(" | type=%1").arg(result.providerErrorType);
    }
    if(!result.rawResponse.trimmed().isEmpty()) {
        const QString compactResponse = result.rawResponse.simplified();
        failureText += QString(" | response=%1").arg(compactResponse.left(220));
    }
    m_testStatusLabel->setText(failureText);
}

void LlmSettingsDialog::updateModeDefaults()
{
    const QString mode = m_modeComboBox->currentData().toString();
    refreshSuggestedModels();
    refreshProviderInstructions();
    if(mode == QLatin1String("openai_responses")) {
        if(m_providerLineEdit->text().trimmed().isEmpty()) {
            m_providerLineEdit->setText("OpenAI");
        }
        m_providerLineEdit->setPlaceholderText("OpenAI");
        m_endpointLineEdit->setPlaceholderText("https://api.openai.com/v1/responses");
        m_modelLineEdit->setPlaceholderText("gpt-5-mini");
        m_apiKeyLineEdit->setPlaceholderText("Required for OpenAI");
        return;
    }

    if(mode == QLatin1String("gemini_openai")) {
        if(m_providerLineEdit->text().trimmed().isEmpty()) {
            m_providerLineEdit->setText("Google Gemini");
        }
        m_providerLineEdit->setPlaceholderText("Google Gemini");
        m_endpointLineEdit->setPlaceholderText("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
        m_modelLineEdit->setPlaceholderText("gemini-2.5-flash");
        m_apiKeyLineEdit->setPlaceholderText("Required for Gemini");
        return;
    }

    if(mode == QLatin1String("github_models")) {
        if(m_providerLineEdit->text().trimmed().isEmpty()) {
            m_providerLineEdit->setText("GitHub Models");
        }
        m_providerLineEdit->setPlaceholderText("GitHub Models");
        m_endpointLineEdit->setPlaceholderText("https://models.inference.ai.azure.com/chat/completions");
        m_modelLineEdit->setPlaceholderText("openai/gpt-4.1-mini or anthropic/claude-sonnet-4");
        m_apiKeyLineEdit->setPlaceholderText("Required GitHub token");
        return;
    }

    if(mode == QLatin1String("anthropic_messages")) {
        if(m_providerLineEdit->text().trimmed().isEmpty()) {
            m_providerLineEdit->setText("Anthropic");
        }
        m_providerLineEdit->setPlaceholderText("Anthropic");
        m_endpointLineEdit->setPlaceholderText("https://api.anthropic.com/v1/messages");
        m_modelLineEdit->setPlaceholderText("claude-sonnet-4-5");
        m_apiKeyLineEdit->setPlaceholderText("Required for Anthropic");
        return;
    }

    if(mode == QLatin1String("http")) {
        if(m_providerLineEdit->text().trimmed().isEmpty()) {
            m_providerLineEdit->setText("openai-compatible");
        }
        m_providerLineEdit->setPlaceholderText("openai-compatible");
        m_endpointLineEdit->setPlaceholderText("http://localhost:11434/v1/chat/completions");
        m_modelLineEdit->setPlaceholderText("gpt-4.1-mini or local model name");
        m_apiKeyLineEdit->setPlaceholderText("Optional");
        return;
    }

    m_providerLineEdit->setPlaceholderText("openai-compatible");
    m_endpointLineEdit->setPlaceholderText("Optional");
    m_modelLineEdit->setPlaceholderText("Optional");
    m_apiKeyLineEdit->setPlaceholderText("Optional");
}

void LlmSettingsDialog::openProviderConsole()
{
    const QString url = providerConsoleUrl(m_modeComboBox->currentData().toString());
    if(!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

void LlmSettingsDialog::openProviderDocs()
{
    const QString url = providerDocsUrl(m_modeComboBox->currentData().toString());
    if(!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

void LlmSettingsDialog::saveCurrentProfile()
{
    bool accepted = false;
    const QString initialName = m_profileComboBox->currentData().toString();
    const QString profileName = QInputDialog::getText(this,
                                                      "Save Provider Profile",
                                                      "Profile name",
                                                      QLineEdit::Normal,
                                                      initialName,
                                                      &accepted).trimmed();
    if(!accepted || profileName.isEmpty()) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString baseKey = QString("agent/profiles/%1").arg(profileName);
    settings.setValue(QString("%1/mode").arg(baseKey), m_modeComboBox->currentData().toString());
    settings.setValue(QString("%1/provider").arg(baseKey), m_providerLineEdit->text().trimmed());
    settings.setValue(QString("%1/endpoint").arg(baseKey), m_endpointLineEdit->text().trimmed());
    settings.setValue(QString("%1/model").arg(baseKey), m_modelLineEdit->text().trimmed());
    const QString apiKey = m_apiKeyLineEdit->text().trimmed();
    if(!apiKey.isEmpty() && writeSecretToKeychain(providerSecretAccountName(profileName), apiKey)) {
        settings.remove(QString("%1/api_key").arg(baseKey));
    } else {
        if(apiKey.isEmpty()) {
            deleteSecretFromKeychain(providerSecretAccountName(profileName));
        }
        settings.setValue(QString("%1/api_key").arg(baseKey), apiKey);
    }
    settings.setValue("agent/selected_profile", profileName);
    refreshProfiles();
    const int index = m_profileComboBox->findData(profileName);
    if(index >= 0) {
        m_profileComboBox->setCurrentIndex(index);
    }
    m_testStatusLabel->setText(QString("Saved provider profile `%1`.").arg(profileName));
}

void LlmSettingsDialog::deleteCurrentProfile()
{
    const QString profileName = m_profileComboBox->currentData().toString().trimmed();
    if(profileName.isEmpty()) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.remove(QString("agent/profiles/%1").arg(profileName));
    deleteSecretFromKeychain(providerSecretAccountName(profileName));
    if(settings.value("agent/selected_profile").toString() == profileName) {
        settings.remove("agent/selected_profile");
    }
    refreshProfiles();
    m_testStatusLabel->setText(QString("Deleted provider profile `%1`.").arg(profileName));
}

void LlmSettingsDialog::applySelectedProfile(int index)
{
    if(index < 0) {
        return;
    }

    const QString profileName = m_profileComboBox->itemData(index).toString().trimmed();
    if(profileName.isEmpty()) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString baseKey = QString("agent/profiles/%1").arg(profileName);
    const QString mode = settings.value(QString("%1/mode").arg(baseKey)).toString().trimmed();
    const int modeIndex = m_modeComboBox->findData(mode);
    if(modeIndex >= 0) {
        m_modeComboBox->setCurrentIndex(modeIndex);
    }
    m_providerLineEdit->setText(settings.value(QString("%1/provider").arg(baseKey)).toString());
    m_endpointLineEdit->setText(settings.value(QString("%1/endpoint").arg(baseKey)).toString());
    m_modelLineEdit->setText(settings.value(QString("%1/model").arg(baseKey)).toString());
    const QString storedApiKey = readSecretFromKeychain(providerSecretAccountName(profileName));
    m_apiKeyLineEdit->setText(storedApiKey.isEmpty()
                                  ? settings.value(QString("%1/api_key").arg(baseKey)).toString()
                                  : storedApiKey);
    settings.setValue("agent/selected_profile", profileName);
    updateModeDefaults();
    m_testStatusLabel->setText(QString("Loaded provider profile `%1`.").arg(profileName));
}

void LlmSettingsDialog::applySuggestedModel()
{
    const QString modelName = m_suggestedModelComboBox->currentData().toString().trimmed();
    if(modelName.isEmpty()) {
        return;
    }

    m_modelLineEdit->setText(modelName);
}

void LlmSettingsDialog::browseModels()
{
    QString errorMessage;
    const QStringList models = fetchAvailableModels(&errorMessage);
    if(models.isEmpty()) {
        m_testStatusLabel->setText(errorMessage.isEmpty()
                                       ? QString("No models were returned by the selected provider.")
                                       : QString("Browse Models failed: %1").arg(errorMessage));
        return;
    }

    {
        const QSignalBlocker blocker(m_suggestedModelComboBox);
        m_suggestedModelComboBox->clear();
        for(const QString& modelName : models) {
            m_suggestedModelComboBox->addItem(modelName, modelName);
        }
    }

    const int currentIndex = m_suggestedModelComboBox->findData(m_modelLineEdit->text().trimmed());
    if(currentIndex >= 0) {
        m_suggestedModelComboBox->setCurrentIndex(currentIndex);
    } else {
        m_suggestedModelComboBox->setCurrentIndex(0);
    }

    m_testStatusLabel->setText(QString("Loaded %1 models from the selected provider.").arg(models.size()));
}

void LlmSettingsDialog::refreshProfiles()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString selectedProfile = settings.value("agent/selected_profile").toString().trimmed();
    settings.beginGroup("agent/profiles");
    const QStringList profileNames = settings.childGroups();
    settings.endGroup();

    QSignalBlocker blocker(m_profileComboBox);
    m_profileComboBox->clear();
    for(const QString& groupName : profileNames) {
        const QString profileName = groupName.trimmed();
        if(profileName.isEmpty()) {
            continue;
        }
        m_profileComboBox->addItem(profileName, profileName);
    }

    const int index = m_profileComboBox->findData(selectedProfile);
    if(index >= 0) {
        m_profileComboBox->setCurrentIndex(index);
    }
}

void LlmSettingsDialog::refreshSuggestedModels()
{
    const QString mode = m_modeComboBox->currentData().toString();
    const QStringList suggestions = suggestedModelsForMode(mode);

    QSignalBlocker blocker(m_suggestedModelComboBox);
    m_suggestedModelComboBox->clear();
    for(const QString& modelName : suggestions) {
        m_suggestedModelComboBox->addItem(modelName, modelName);
    }

    const int currentIndex = m_suggestedModelComboBox->findData(m_modelLineEdit->text().trimmed());
    if(currentIndex >= 0) {
        m_suggestedModelComboBox->setCurrentIndex(currentIndex);
    }
}

void LlmSettingsDialog::refreshProviderInstructions()
{
    const QString mode = m_modeComboBox->currentData().toString();
    m_providerInstructionsView->setHtml(providerInstructionsHtml(mode));
    m_openConsoleButton->setEnabled(!providerConsoleUrl(mode).isEmpty());
    m_openDocsButton->setEnabled(!providerDocsUrl(mode).isEmpty());
}

QString LlmSettingsDialog::resolvedEndpointForMode(const QString& mode) const
{
    const QString configuredEndpoint = m_endpointLineEdit->text().trimmed();
    if(mode == QLatin1String("openai_responses")) {
        if(configuredEndpoint.isEmpty()) {
            return QString("https://api.openai.com/v1/models");
        }
        const QUrl configuredUrl(configuredEndpoint);
        if(configuredUrl.isValid() && !configuredUrl.scheme().isEmpty() && !configuredUrl.host().isEmpty()) {
            QUrl modelsUrl = configuredUrl;
            modelsUrl.setPath("/v1/models");
            modelsUrl.setQuery(QString());
            return modelsUrl.toString();
        }
        return QString("https://api.openai.com/v1/models");
    }

    if(mode == QLatin1String("gemini_openai")) {
        if(configuredEndpoint.isEmpty()) {
            return QString("https://generativelanguage.googleapis.com/v1beta/openai/models");
        }
        const QUrl configuredUrl(configuredEndpoint);
        if(configuredUrl.isValid() && !configuredUrl.scheme().isEmpty() && !configuredUrl.host().isEmpty()) {
            QUrl modelsUrl = configuredUrl;
            modelsUrl.setPath("/v1beta/openai/models");
            modelsUrl.setQuery(QString());
            return modelsUrl.toString();
        }
        return QString("https://generativelanguage.googleapis.com/v1beta/openai/models");
    }

    if(mode == QLatin1String("github_models")) {
        return QString("https://models.github.ai/catalog/models");
    }

    if(mode == QLatin1String("anthropic_messages")) {
        return QString("https://api.anthropic.com/v1/models");
    }

    return configuredEndpoint;
}

QStringList LlmSettingsDialog::fetchAvailableModels(QString* errorMessage) const
{
    if(errorMessage) {
        *errorMessage = QString();
    }

    const QString mode = m_modeComboBox->currentData().toString();
    const QString endpoint = resolvedEndpointForMode(mode);
    const QString apiKey = m_apiKeyLineEdit->text().trimmed();
    if(endpoint.isEmpty()) {
        if(errorMessage) {
            *errorMessage = "No model catalog endpoint is configured for this provider.";
        }
        return QStringList();
    }

    if((mode == QLatin1String("openai_responses")
        || mode == QLatin1String("gemini_openai")
        || mode == QLatin1String("github_models")
        || mode == QLatin1String("anthropic_messages"))
       && apiKey.isEmpty()) {
        if(errorMessage) {
            *errorMessage = "This provider requires an API key before models can be fetched.";
        }
        return QStringList();
    }

    QNetworkRequest request{QUrl(endpoint)};
    if(mode == QLatin1String("anthropic_messages")) {
        request.setRawHeader("x-api-key", apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else if(!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }
    if(mode == QLatin1String("github_models")) {
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    }

    QNetworkReply* reply = m_networkAccessManager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray responseBytes = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString networkError = reply->error() != QNetworkReply::NoError ? reply->errorString() : QString();
    reply->deleteLater();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(responseBytes, &parseError);
    const QJsonObject responseObject = document.isObject() ? document.object() : QJsonObject();

    if(!networkError.isEmpty()) {
        const QString providerMessage = responseObject.value("error").toObject().value("message").toString().trimmed();
        if(errorMessage) {
            *errorMessage = providerMessage.isEmpty()
                ? QString("%1 (HTTP %2)").arg(networkError).arg(httpStatus)
                : QString("%1 (HTTP %2)").arg(providerMessage).arg(httpStatus);
        }
        return QStringList();
    }

    if(parseError.error != QJsonParseError::NoError) {
        if(errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return QStringList();
    }

    QJsonArray modelsArray;
    if(mode == QLatin1String("github_models")) {
        modelsArray = document.isArray() ? document.array() : responseObject.value("data").toArray();
    } else {
        modelsArray = responseObject.value("data").toArray();
    }

    QStringList models;
    for(const QJsonValue& value : modelsArray) {
        const QJsonObject object = value.toObject();
        const QString id = object.value("id").toString().trimmed();
        const QString name = object.value("name").toString().trimmed();
        const QString modelName = id.isEmpty() ? name : id;
        if(!modelName.isEmpty() && !models.contains(modelName)) {
            models.append(modelName);
        }
    }

    return models;
}
