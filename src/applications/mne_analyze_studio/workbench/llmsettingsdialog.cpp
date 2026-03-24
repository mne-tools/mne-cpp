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
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

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
, m_toolInventoryView(new QTextEdit(this))
, m_testStatusLabel(new QLabel("No planner test run yet.", this))
, m_testButton(new QPushButton("Validate Connection", this))
{
    setWindowTitle("Agent Settings");
    resize(700, 560);

    m_modeComboBox->addItem("Deterministic Fallback", "disabled");
    m_modeComboBox->addItem("Mock Planner", "mock");
    m_modeComboBox->addItem("OpenAI Responses API", "openai_responses");
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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
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
    settings.setValue(QString("%1/api_key").arg(baseKey), m_apiKeyLineEdit->text().trimmed());
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
    m_apiKeyLineEdit->setText(settings.value(QString("%1/api_key").arg(baseKey)).toString());
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
