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
#include "pillselectorwidget.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFrame>
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

QString providerNameForMode(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return QString("OpenAI");
    }
    if(mode == QLatin1String("gemini_openai")) {
        return QString("Google Gemini");
    }

    return QString("Rule-Based");
}

QString endpointForMode(const QString& mode)
{
    if(mode == QLatin1String("openai_responses")) {
        return QString("https://api.openai.com/v1/responses");
    }
    if(mode == QLatin1String("gemini_openai")) {
        return QString("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
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

    return "<b>Rule-based fallback</b><br/>No external connection is needed. Studio will use the built-in deterministic planner for basic assistance.";
}

}

LlmSettingsDialog::LlmSettingsDialog(const LlmPlannerConfig& config, QWidget* parent)
: QDialog(parent)
, m_profileSelector(new PillSelectorWidget(this))
, m_saveProfileButton(new QPushButton("Save Profile", this))
, m_deleteProfileButton(new QPushButton("Delete Profile", this))
, m_modeSelector(new PillSelectorWidget(this))
, m_modelLineEdit(new QLineEdit(this))
, m_apiKeyLineEdit(new QLineEdit(this))
, m_suggestedModelSelector(new PillSelectorWidget(this))
, m_editModelButton(new QPushButton("Set...", this))
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
    setWindowTitle("Connect Model");
    resize(520, 380);
    setObjectName("llmSettingsDialog");
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setStyleSheet(
        "QDialog#llmSettingsDialog { background: #1f232a; color: #dce3ea; }"
        "QLabel { color: #dce3ea; }"
        "QFrame#connectCard { background: #20262f; border: 1px solid #2f3843; border-radius: 16px; }"
        "QLabel#connectHeading { color: #f0f6fc; font-size: 15px; font-weight: 700; }"
        "QLabel#connectMeta { color: #9aa6b2; font-size: 12px; }"
        "QLineEdit, QTextBrowser { background: #161b22; color: #dce3ea; border: 1px solid #2f3843; border-radius: 12px; padding: 8px 10px; }"
        "QTextBrowser { padding: 12px; }"
        "QPushButton { background: #283341; color: #e6edf3; border: none; border-radius: 12px; padding: 8px 12px; }"
        "QPushButton:hover { background: #314052; }"
        "QPushButton[text=\"Validate Connection\"], QPushButton[text=\"OK\"] { background: #0f7ae5; color: white; }"
        "QPushButton[text=\"Validate Connection\"]:hover, QPushButton[text=\"OK\"]:hover { background: #218bff; }");

    m_modeSelector->setPlaceholderText("Mode");
    m_modeSelector->setItems(QList<QPair<QString, QString>>{
        qMakePair(QString("Rule-Based"), QString("disabled")),
        qMakePair(QString("OpenAI"), QString("openai_responses")),
        qMakePair(QString("Gemini"), QString("gemini_openai"))
    });

    const QString mode = config.mode.trimmed().isEmpty() ? QString("disabled") : config.mode.trimmed().toLower();
    m_modeSelector->setCurrentValue(mode);
    m_lastAppliedMode = mode;

    m_modelLineEdit->setText(config.model);
    m_apiKeyLineEdit->setText(config.apiKey);
    m_apiKeyLineEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    m_profileSelector->setPlaceholderText("Saved profiles");
    m_profileSelector->setEmptyText("No saved profiles");
    m_modelLineEdit->setReadOnly(true);
    m_modelLineEdit->setPlaceholderText("No model selected");
    m_suggestedModelSelector->setPlaceholderText("Fetched models");
    m_suggestedModelSelector->setEmptyText("No fetched models");
    m_providerInstructionsView->setOpenExternalLinks(true);
    m_providerInstructionsView->setMaximumHeight(120);
    m_toolInventoryView->setReadOnly(true);
    m_toolInventoryView->setLineWrapMode(QTextEdit::NoWrap);
    m_toolInventoryView->setPlaceholderText("Available planner tools will appear here.");
    m_toolInventoryView->hide();
    m_testStatusLabel->setWordWrap(true);

    QHBoxLayout* profileRow = new QHBoxLayout;
    profileRow->setContentsMargins(0, 0, 0, 0);
    profileRow->setSpacing(8);
    profileRow->addWidget(m_profileSelector, 1);
    profileRow->addWidget(m_saveProfileButton);
    profileRow->addWidget(m_deleteProfileButton);

    QHBoxLayout* modelRow = new QHBoxLayout;
    modelRow->setContentsMargins(0, 0, 0, 0);
    modelRow->setSpacing(8);
    modelRow->addWidget(m_modelLineEdit, 1);
    modelRow->addWidget(m_editModelButton);
    modelRow->addWidget(m_browseModelsButton);
    modelRow->addWidget(m_suggestedModelSelector);
    modelRow->addWidget(m_applySuggestedModelButton);

    QHBoxLayout* onboardingRow = new QHBoxLayout;
    onboardingRow->setContentsMargins(0, 0, 0, 0);
    onboardingRow->setSpacing(8);
    onboardingRow->addWidget(m_openConsoleButton);
    onboardingRow->addWidget(m_openDocsButton);
    onboardingRow->addStretch(1);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);
    formLayout->addRow("Profile", profileRow);
    formLayout->addRow("Mode", m_modeSelector);
    formLayout->addRow("Current Model", modelRow);
    formLayout->addRow("API Key", m_apiKeyLineEdit);
    formLayout->addRow("Status", m_testStatusLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &LlmSettingsDialog::runPlannerTest);
    connect(m_modeSelector, &PillSelectorWidget::currentValueChanged, this, [this](const QString&) {
        updateModeDefaults();
    });
    connect(m_profileSelector, &PillSelectorWidget::currentValueChanged, this, &LlmSettingsDialog::applySelectedProfile);
    connect(m_editModelButton, &QPushButton::clicked, this, &LlmSettingsDialog::editModelManually);
    connect(m_saveProfileButton, &QPushButton::clicked, this, &LlmSettingsDialog::saveCurrentProfile);
    connect(m_deleteProfileButton, &QPushButton::clicked, this, &LlmSettingsDialog::deleteCurrentProfile);
    connect(m_applySuggestedModelButton, &QPushButton::clicked, this, &LlmSettingsDialog::applySuggestedModel);
    connect(m_browseModelsButton, &QPushButton::clicked, this, &LlmSettingsDialog::browseModels);
    connect(m_openConsoleButton, &QPushButton::clicked, this, &LlmSettingsDialog::openProviderConsole);
    connect(m_openDocsButton, &QPushButton::clicked, this, &LlmSettingsDialog::openProviderDocs);

    QLabel* headingLabel = new QLabel("Connect a Remote Model", this);
    headingLabel->setObjectName("connectHeading");
    QLabel* subheadingLabel = new QLabel("Pick a provider, choose a model, add the key once, then validate the connection.", this);
    subheadingLabel->setObjectName("connectMeta");
    subheadingLabel->setWordWrap(true);

    QFrame* connectionCard = new QFrame(this);
    connectionCard->setObjectName("connectCard");
    QVBoxLayout* connectionCardLayout = new QVBoxLayout(connectionCard);
    connectionCardLayout->setContentsMargins(14, 14, 14, 14);
    connectionCardLayout->setSpacing(12);
    connectionCardLayout->addWidget(headingLabel);
    connectionCardLayout->addWidget(subheadingLabel);
    connectionCardLayout->addLayout(formLayout);

    QLabel* providerSetupLabel = new QLabel("Provider Setup", this);
    providerSetupLabel->setObjectName("connectHeading");
    providerSetupLabel->setStyleSheet("QLabel#connectHeading { color: #f0f6fc; font-size: 14px; font-weight: 700; }");

    QFrame* setupCard = new QFrame(this);
    setupCard->setObjectName("connectCard");
    QVBoxLayout* setupCardLayout = new QVBoxLayout(setupCard);
    setupCardLayout->setContentsMargins(14, 14, 14, 14);
    setupCardLayout->setSpacing(10);
    setupCardLayout->addWidget(providerSetupLabel);
    setupCardLayout->addWidget(m_providerInstructionsView);
    setupCardLayout->addLayout(onboardingRow);
    setupCardLayout->addWidget(m_testButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    layout->addWidget(connectionCard);
    layout->addWidget(setupCard);
    layout->addWidget(buttonBox);

    refreshProfiles();
    updateModeDefaults();
}

LlmPlannerConfig LlmSettingsDialog::configuration() const
{
    return LlmPlannerConfig{
        m_modeSelector->currentValue(),
        providerNameForMode(m_modeSelector->currentValue()),
        endpointForMode(m_modeSelector->currentValue()),
        m_apiKeyLineEdit->text().trimmed(),
        m_modelLineEdit->text().trimmed()
    };
}

bool LlmSettingsDialog::hasValidationResult() const
{
    return m_hasValidationResult;
}

bool LlmSettingsDialog::lastValidationSucceeded() const
{
    return m_lastValidationSucceeded;
}

QString LlmSettingsDialog::lastValidationMessage() const
{
    return m_lastValidationMessage;
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
        m_hasValidationResult = true;
        m_lastValidationSucceeded = true;
        m_lastValidationMessage = QString("Validated successfully");
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
    m_hasValidationResult = true;
    m_lastValidationSucceeded = false;
    m_lastValidationMessage = failureText;
    m_testStatusLabel->setText(failureText);
}

void LlmSettingsDialog::updateModeDefaults()
{
    const QString mode = m_modeSelector->currentValue();
    const bool modeChanged = m_lastAppliedMode != mode;
    m_lastAppliedMode = mode;

    if(modeChanged) {
        m_modelLineEdit->clear();
        m_hasValidationResult = false;
        m_lastValidationSucceeded = false;
        m_lastValidationMessage.clear();
        m_testStatusLabel->setText("No planner test run yet.");
    }

    refreshSuggestedModels();
    refreshProviderInstructions();
    if(mode == QLatin1String("openai_responses")) {
        m_modelLineEdit->setPlaceholderText("gpt-5-mini");
        m_apiKeyLineEdit->setPlaceholderText("Required for OpenAI");
        updateDialogVisibility();
        return;
    }

    if(mode == QLatin1String("gemini_openai")) {
        m_modelLineEdit->setPlaceholderText("gemini-2.5-flash");
        m_apiKeyLineEdit->setPlaceholderText("Required for Gemini");
        updateDialogVisibility();
        return;
    }

    m_modelLineEdit->clear();
    m_modelLineEdit->setPlaceholderText("No model needed");
    m_apiKeyLineEdit->clear();
    m_apiKeyLineEdit->setPlaceholderText("No key needed");
    updateDialogVisibility();
}

void LlmSettingsDialog::openProviderConsole()
{
    const QString url = providerConsoleUrl(m_modeSelector->currentValue());
    if(!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

void LlmSettingsDialog::openProviderDocs()
{
    const QString url = providerDocsUrl(m_modeSelector->currentValue());
    if(!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

void LlmSettingsDialog::saveCurrentProfile()
{
    bool accepted = false;
    const QString initialName = m_profileSelector->currentValue();
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
    settings.setValue(QString("%1/mode").arg(baseKey), m_modeSelector->currentValue());
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
    m_profileSelector->setCurrentValue(profileName);
    m_testStatusLabel->setText(QString("Saved provider profile `%1`.").arg(profileName));
}

void LlmSettingsDialog::deleteCurrentProfile()
{
    const QString profileName = m_profileSelector->currentValue().trimmed();
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

void LlmSettingsDialog::applySelectedProfile(const QString& profileName)
{
    const QString trimmedProfile = profileName.trimmed();
    if(trimmedProfile.isEmpty()) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString baseKey = QString("agent/profiles/%1").arg(trimmedProfile);
    const QString mode = settings.value(QString("%1/mode").arg(baseKey)).toString().trimmed();
    m_modeSelector->setCurrentValue(mode);
    m_modelLineEdit->setText(settings.value(QString("%1/model").arg(baseKey)).toString());
    const QString storedApiKey = readSecretFromKeychain(providerSecretAccountName(trimmedProfile));
    m_apiKeyLineEdit->setText(storedApiKey.isEmpty()
                                  ? settings.value(QString("%1/api_key").arg(baseKey)).toString()
                                  : storedApiKey);
    settings.setValue("agent/selected_profile", trimmedProfile);
    updateModeDefaults();
    m_testStatusLabel->setText(QString("Loaded provider profile `%1`.").arg(trimmedProfile));
}

void LlmSettingsDialog::applySuggestedModel()
{
    const QString modelName = m_suggestedModelSelector->currentValue().trimmed();
    if(modelName.isEmpty()) {
        return;
    }

    m_modelLineEdit->setText(modelName);
}

void LlmSettingsDialog::editModelManually()
{
    bool accepted = false;
    const QString modelName = QInputDialog::getText(this,
                                                    "Set Model",
                                                    "Model name",
                                                    QLineEdit::Normal,
                                                    m_modelLineEdit->text().trimmed(),
                                                    &accepted).trimmed();
    if(!accepted) {
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

    QList<QPair<QString, QString>> modelItems;
    for(const QString& modelName : models) {
        modelItems.append(qMakePair(modelName, modelName));
    }
    m_suggestedModelSelector->setItems(modelItems);
    m_suggestedModelSelector->setCurrentValue(m_modelLineEdit->text().trimmed());
    if(m_suggestedModelSelector->currentValue().isEmpty() && !models.isEmpty()) {
        m_suggestedModelSelector->setCurrentValue(models.first());
    }

    updateDialogVisibility();
    m_testStatusLabel->setText(QString("Loaded %1 models from the selected provider.").arg(models.size()));
}

void LlmSettingsDialog::refreshProfiles()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString selectedProfile = settings.value("agent/selected_profile").toString().trimmed();
    settings.beginGroup("agent/profiles");
    const QStringList profileNames = settings.childGroups();
    settings.endGroup();

    QList<QPair<QString, QString>> profileItems;
    for(const QString& groupName : profileNames) {
        const QString profileName = groupName.trimmed();
        if(profileName.isEmpty()) {
            continue;
        }
        profileItems.append(qMakePair(profileName, profileName));
    }

    m_profileSelector->setItems(profileItems);
    m_profileSelector->setCurrentValue(selectedProfile);
    m_profileSelector->setEnabled(!profileNames.isEmpty());
    m_deleteProfileButton->setEnabled(!profileNames.isEmpty());
}

void LlmSettingsDialog::refreshSuggestedModels()
{
    m_suggestedModelSelector->setItems(QList<QPair<QString, QString>>());
}

void LlmSettingsDialog::refreshProviderInstructions()
{
    const QString mode = m_modeSelector->currentValue();
    m_providerInstructionsView->setHtml(providerInstructionsHtml(mode));
    m_openConsoleButton->setEnabled(!providerConsoleUrl(mode).isEmpty());
    m_openDocsButton->setEnabled(!providerDocsUrl(mode).isEmpty());
}

void LlmSettingsDialog::updateDialogVisibility()
{
    const QString mode = m_modeSelector->currentValue();
    const bool isRuleBased = mode == QLatin1String("disabled") || mode == QLatin1String("mock");
    const bool hasFetchedModels = m_suggestedModelSelector->hasItems();

    m_modelLineEdit->setVisible(!isRuleBased);
    m_apiKeyLineEdit->setVisible(!isRuleBased);
    m_editModelButton->setVisible(!isRuleBased);
    m_suggestedModelSelector->setVisible(!isRuleBased && hasFetchedModels);
    m_applySuggestedModelButton->setVisible(!isRuleBased && hasFetchedModels);
    m_browseModelsButton->setVisible(!isRuleBased);
}

QString LlmSettingsDialog::resolvedEndpointForMode(const QString& mode) const
{
    const QString configuredEndpoint = endpointForMode(mode);
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

    return configuredEndpoint;
}

QStringList LlmSettingsDialog::fetchAvailableModels(QString* errorMessage) const
{
    if(errorMessage) {
        *errorMessage = QString();
    }

    const QString mode = m_modeSelector->currentValue();
    const QString endpoint = resolvedEndpointForMode(mode);
    const QString apiKey = m_apiKeyLineEdit->text().trimmed();
    if(endpoint.isEmpty()) {
        if(errorMessage) {
            *errorMessage = "No model catalog endpoint is configured for this provider.";
        }
        return QStringList();
    }

    if((mode == QLatin1String("openai_responses")
        || mode == QLatin1String("gemini_openai"))
       && apiKey.isEmpty()) {
        if(errorMessage) {
            *errorMessage = "This provider requires an API key before models can be fetched.";
        }
        return QStringList();
    }

    QNetworkRequest request{QUrl(endpoint)};
    if(!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
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

    const QJsonArray modelsArray = responseObject.value("data").toArray();

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
