//=============================================================================================================
/**
 * @file     llmtoolplanner.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the OpenAI-compatible LLM planner adapter for studio tool selection.
 */

#include "llmtoolplanner.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

using namespace MNEANALYZESTUDIO;

namespace
{

QString envValue(const char* name)
{
    return QString::fromUtf8(qgetenv(name)).trimmed();
}

QString jsonValueToCompactString(const QJsonValue& value)
{
    if(value.isString()) {
        return value.toString();
    }

    if(value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 8);
    }

    if(value.isBool()) {
        return value.toBool() ? QString("true") : QString("false");
    }

    if(value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }

    if(value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }

    return QString("null");
}

QString formatToolContracts(const QJsonArray& toolDefinitions)
{
    QStringList lines;

    for(const QJsonValue& value : toolDefinitions) {
        const QJsonObject tool = value.toObject();
        const QString name = tool.value("name").toString();
        const QString description = tool.value("description").toString();
        const QJsonObject inputSchema = tool.value("input_schema").toObject();
        const QJsonObject outputSchema = tool.value("result_schema").toObject();
        const QJsonObject properties = inputSchema.value("properties").toObject();
        const QJsonArray required = inputSchema.value("required").toArray();
        const QJsonObject outputProperties = outputSchema.value("properties").toObject();

        QStringList propertyParts;
        for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
            const QJsonObject property = it.value().toObject();
            QString part = QString("%1:%2").arg(it.key(), property.value("type").toString("string"));
            if(property.contains("default")) {
                part += QString(" default=%1").arg(jsonValueToCompactString(property.value("default")));
            }
            const QString descriptionText = property.value("description").toString();
            if(!descriptionText.isEmpty()) {
                part += QString(" [%1]").arg(descriptionText);
            }
            propertyParts << part;
        }

        QStringList resultParts;
        for(auto it = outputProperties.constBegin(); it != outputProperties.constEnd(); ++it) {
            const QJsonObject property = it.value().toObject();
            QString part = QString("%1:%2").arg(it.key(), property.value("type").toString("string"));
            const QString descriptionText = property.value("description").toString();
            if(!descriptionText.isEmpty()) {
                part += QString(" [%1]").arg(descriptionText);
            }
            resultParts << part;
        }

        QStringList requiredFields;
        for(const QJsonValue& requiredValue : required) {
            requiredFields << requiredValue.toString();
        }

        lines << QString("%1 | %2 | required: %3 | input: %4 | output: %5")
                     .arg(name,
                          description,
                          requiredFields.isEmpty() ? QString("none") : requiredFields.join(", "),
                          propertyParts.isEmpty() ? QString("none") : propertyParts.join("; "),
                          resultParts.isEmpty() ? QString("none") : resultParts.join("; "));
    }

    return lines.join("\n");
}

}

LlmToolPlanner::LlmToolPlanner(QObject* parent)
: QObject(parent)
, m_networkAccessManager(new QNetworkAccessManager(this))
{
}

void LlmToolPlanner::setConfiguration(const LlmPlannerConfig& config)
{
    m_config = config;
}

LlmPlannerConfig LlmToolPlanner::configuration() const
{
    return m_config;
}

bool LlmToolPlanner::isConfigured() const
{
    return isMockMode() || (!endpoint().isEmpty() && !model().isEmpty());
}

bool LlmToolPlanner::isMockMode() const
{
    return mode().compare("mock", Qt::CaseInsensitive) == 0;
}

QString LlmToolPlanner::providerName() const
{
    const QString configured = configOrEnv(m_config.providerName, "MNE_ANALYZE_STUDIO_LLM_PROVIDER");
    return configured.isEmpty() ? QString("openai-compatible") : configured;
}

QString LlmToolPlanner::statusSummary() const
{
    if(isMockMode()) {
        return QString("LLM: Mock mode | Model: %1").arg(model().isEmpty() ? QString("planner-mock") : model());
    }

    if(isConfigured()) {
        return QString("LLM: Connected | Provider: %1 | Model: %2")
            .arg(providerName(), model());
    }

    return "LLM: Deterministic fallback only | Set MNE_ANALYZE_STUDIO_LLM_ENDPOINT and MNE_ANALYZE_STUDIO_LLM_MODEL.";
}

LlmPlanResult LlmToolPlanner::plan(const QString& userCommand,
                                   const QJsonArray& toolDefinitions,
                                   const QJsonObject& context) const
{
    LlmPlanResult result;
    result.providerName = providerName();

    if(!isConfigured()) {
        result.errorMessage = "LLM planner is not configured.";
        return result;
    }

    if(isMockMode()) {
        const QString lower = userCommand.toLower();
        result.usedModel = true;
        result.success = true;
        result.summary = "Mock planner generated a local test plan.";

        if(lower.contains("strongest") && lower.contains("burst")) {
            const QString match = lower.contains("eeg") ? "EEG" : (lower.contains("meg") ? "MEG" : "EOG");
            result.plannedCommands
                << QString("tools.call neurokernel.find_peak_window {\"window_samples\":4000,\"match\":\"%1\"}").arg(match)
                << "tools.call view.raw.goto {\"sample\":${last_peak_sample}}";
            if(lower.contains("stat")) {
                result.plannedCommands << "tools.call neurokernel.raw_stats {\"window_samples\":600}";
            }
            return result;
        }

        if(lower.contains("top") && lower.contains("channel")) {
            const QString match = lower.contains("eeg") ? "EEG" : (lower.contains("meg") ? "MEG" : "");
            QStringList argumentParts;
            argumentParts << "\"window_samples\":1200";
            if(!match.isEmpty()) {
                argumentParts << QString("\"match\":\"%1\"").arg(match);
            }
            argumentParts << "\"limit\":5";
            result.plannedCommands
                << QString("tools.call neurokernel.channel_stats {%1}").arg(argumentParts.join(","));
            return result;
        }

        if(lower.contains("summary")) {
            result.plannedCommands << "tools.call view.raw.summary {}";
            return result;
        }

        result.plannedCommands << "tools.call view.raw.state {}";
        return result;
    }

    const QString systemPrompt =
        "You are a tool-planning assistant for a neuroscience IDE. "
        "Return JSON only. "
        "Choose from the provided tools. "
        "If a request needs multiple steps, return them in execution order. "
        "Respect each tool's input_schema, especially required fields, defaults, enums, and numeric bounds. "
        "Use result_schema to understand which fields a tool returns and how later steps may reference them. "
        "Use placeholders like ${last_peak_sample} only when a previous step will produce that field. "
        "Output format: "
        "{\"summary\":\"...\",\"steps\":[{\"tool_name\":\"...\",\"arguments\":{}}]}.";

    const QJsonObject userPayload{
        {"request", userCommand},
        {"tools", toolDefinitions},
        {"tool_contracts", formatToolContracts(toolDefinitions)},
        {"context", context}
    };

    const QJsonObject payload{
        {"model", model()},
        {"temperature", 0.0},
        {"messages", QJsonArray{
            QJsonObject{{"role", "system"}, {"content", systemPrompt}},
            QJsonObject{{"role", "user"}, {"content", QString::fromUtf8(QJsonDocument(userPayload).toJson(QJsonDocument::Compact))}}
        }}
    };

    QNetworkRequest request{QUrl(endpoint())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if(!apiKey().isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey()).toUtf8());
    }

    QNetworkReply* reply = m_networkAccessManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray responseBytes = reply->readAll();
    result.rawResponse = QString::fromUtf8(responseBytes);
    result.usedModel = true;

    if(reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    QJsonParseError responseError;
    const QJsonDocument responseDocument = QJsonDocument::fromJson(responseBytes, &responseError);
    reply->deleteLater();

    if(responseError.error != QJsonParseError::NoError || !responseDocument.isObject()) {
        result.errorMessage = responseError.errorString();
        return result;
    }

    const QString contentString = extractContentString(responseDocument.object());
    const QString jsonPayload = extractJsonPayload(contentString);
    if(jsonPayload.isEmpty()) {
        result.errorMessage = "LLM response did not contain a JSON plan.";
        return result;
    }

    QJsonParseError planError;
    const QJsonDocument planDocument = QJsonDocument::fromJson(jsonPayload.toUtf8(), &planError);
    if(planError.error != QJsonParseError::NoError || !planDocument.isObject()) {
        result.errorMessage = planError.errorString();
        return result;
    }

    const QJsonObject planObject = planDocument.object();
    result.summary = planObject.value("summary").toString();
    const QJsonArray steps = planObject.value("steps").toArray();
    for(const QJsonValue& stepValue : steps) {
        const QJsonObject step = stepValue.toObject();
        const QString toolName = step.value("tool_name").toString().trimmed();
        if(toolName.isEmpty()) {
            continue;
        }

        const QString arguments = QString::fromUtf8(QJsonDocument(step.value("arguments").toObject()).toJson(QJsonDocument::Compact));
        result.plannedCommands << QString("tools.call %1 %2").arg(toolName, arguments);
    }

    result.success = !result.plannedCommands.isEmpty();
    if(!result.success && result.errorMessage.isEmpty()) {
        result.errorMessage = "LLM returned no executable steps.";
    }

    return result;
}

QString LlmToolPlanner::mode() const
{
    return configOrEnv(m_config.mode, "MNE_ANALYZE_STUDIO_LLM_MODE");
}

QString LlmToolPlanner::endpoint() const
{
    return configOrEnv(m_config.endpoint, "MNE_ANALYZE_STUDIO_LLM_ENDPOINT");
}

QString LlmToolPlanner::apiKey() const
{
    return configOrEnv(m_config.apiKey, "MNE_ANALYZE_STUDIO_LLM_API_KEY");
}

QString LlmToolPlanner::model() const
{
    return configOrEnv(m_config.model, "MNE_ANALYZE_STUDIO_LLM_MODEL");
}

QString LlmToolPlanner::configOrEnv(const QString& configuredValue, const char* envName) const
{
    return configuredValue.trimmed().isEmpty() ? envValue(envName) : configuredValue.trimmed();
}

QString LlmToolPlanner::extractContentString(const QJsonObject& response) const
{
    const QJsonArray choices = response.value("choices").toArray();
    if(choices.isEmpty()) {
        return QString();
    }

    const QJsonObject message = choices.first().toObject().value("message").toObject();
    const QJsonValue contentValue = message.value("content");
    if(contentValue.isString()) {
        return contentValue.toString();
    }

    if(contentValue.isArray()) {
        QStringList textParts;
        const QJsonArray contentParts = contentValue.toArray();
        for(const QJsonValue& partValue : contentParts) {
            const QJsonObject part = partValue.toObject();
            if(part.value("type").toString() == "text") {
                textParts << part.value("text").toString();
            }
        }
        return textParts.join("\n");
    }

    return QString();
}

QString LlmToolPlanner::extractJsonPayload(const QString& text) const
{
    const int firstBrace = text.indexOf('{');
    const int lastBrace = text.lastIndexOf('}');
    if(firstBrace < 0 || lastBrace <= firstBrace) {
        return QString();
    }

    return text.mid(firstBrace, lastBrace - firstBrace + 1);
}
