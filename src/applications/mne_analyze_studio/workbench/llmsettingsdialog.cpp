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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

}

LlmSettingsDialog::LlmSettingsDialog(const LlmPlannerConfig& config, QWidget* parent)
: QDialog(parent)
, m_modeComboBox(new QComboBox(this))
, m_providerLineEdit(new QLineEdit(this))
, m_endpointLineEdit(new QLineEdit(this))
, m_modelLineEdit(new QLineEdit(this))
, m_apiKeyLineEdit(new QLineEdit(this))
, m_toolInventoryView(new QTextEdit(this))
, m_testStatusLabel(new QLabel("No planner test run yet.", this))
, m_testButton(new QPushButton("Test Planner", this))
{
    setWindowTitle("Agent Settings");
    resize(640, 520);

    m_modeComboBox->addItem("Deterministic Fallback", "disabled");
    m_modeComboBox->addItem("Mock Planner", "mock");
    m_modeComboBox->addItem("OpenAI-Compatible HTTP", "http");

    const QString mode = config.mode.trimmed().isEmpty() ? QString("disabled") : config.mode.trimmed().toLower();
    const int modeIndex = qMax(0, m_modeComboBox->findData(mode));
    m_modeComboBox->setCurrentIndex(modeIndex);

    m_providerLineEdit->setText(config.providerName);
    m_providerLineEdit->setPlaceholderText("openai-compatible");
    m_endpointLineEdit->setText(config.endpoint);
    m_endpointLineEdit->setPlaceholderText("http://localhost:11434/v1/chat/completions");
    m_modelLineEdit->setText(config.model);
    m_modelLineEdit->setPlaceholderText("gpt-4.1-mini or local model name");
    m_apiKeyLineEdit->setText(config.apiKey);
    m_apiKeyLineEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    m_apiKeyLineEdit->setPlaceholderText("Optional");
    m_toolInventoryView->setReadOnly(true);
    m_toolInventoryView->setLineWrapMode(QTextEdit::NoWrap);
    m_toolInventoryView->setPlaceholderText("Available planner tools will appear here.");
    m_testStatusLabel->setWordWrap(true);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow("Mode", m_modeComboBox);
    formLayout->addRow("Provider", m_providerLineEdit);
    formLayout->addRow("Endpoint", m_endpointLineEdit);
    formLayout->addRow("Model", m_modelLineEdit);
    formLayout->addRow("API Key", m_apiKeyLineEdit);
    formLayout->addRow("Status", m_testStatusLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &LlmSettingsDialog::runPlannerTest);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(new QLabel("Planner Tool Inventory", this));
    layout->addWidget(m_toolInventoryView, 1);
    layout->addWidget(m_testButton);
    layout->addWidget(buttonBox);
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

    m_testStatusLabel->setText(QString("Planner failed: %1")
                               .arg(result.errorMessage.isEmpty() ? QString("unknown error") : result.errorMessage));
}
