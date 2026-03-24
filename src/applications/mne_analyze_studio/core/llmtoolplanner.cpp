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
    if(isMockMode()) {
        return true;
    }

    if(isOpenAIResponsesMode() || isGeminiOpenAICompatMode() || isGitHubModelsMode() || isAnthropicMessagesMode()) {
        return !apiKey().isEmpty() && !model().isEmpty();
    }

    return !endpoint().isEmpty() && !model().isEmpty();
}

bool LlmToolPlanner::isMockMode() const
{
    return mode().compare("mock", Qt::CaseInsensitive) == 0;
}

QString LlmToolPlanner::providerName() const
{
    const QString configured = configOrEnv(m_config.providerName, "MNE_ANALYZE_STUDIO_LLM_PROVIDER");
    if(!configured.isEmpty()) {
        return configured;
    }

    if(isOpenAIResponsesMode()) {
        return QString("OpenAI");
    }
    if(isGeminiOpenAICompatMode()) {
        return QString("Google Gemini");
    }
    if(isGitHubModelsMode()) {
        return QString("GitHub Models");
    }
    if(isAnthropicMessagesMode()) {
        return QString("Anthropic");
    }

    return QString("openai-compatible");
}

QString LlmToolPlanner::statusSummary() const
{
    if(isMockMode()) {
        return QString("LLM: Mock mode | Model: %1").arg(model().isEmpty() ? QString("planner-mock") : model());
    }

    if(isConfigured()) {
        QString modeLabel = QString("HTTP");
        if(isOpenAIResponsesMode()) {
            modeLabel = QString("Responses API");
        } else if(isGeminiOpenAICompatMode()) {
            modeLabel = QString("Gemini OpenAI Compat");
        } else if(isGitHubModelsMode()) {
            modeLabel = QString("GitHub Models");
        } else if(isAnthropicMessagesMode()) {
            modeLabel = QString("Anthropic Messages");
        }
        return QString("LLM: Connected | Provider: %1 | Mode: %2 | Model: %3")
            .arg(providerName(), modeLabel, model());
    }

    if(isOpenAIResponsesMode()) {
        return "LLM: Deterministic fallback only | Set MNE_ANALYZE_STUDIO_LLM_API_KEY and MNE_ANALYZE_STUDIO_LLM_MODEL.";
    }
    if(isGeminiOpenAICompatMode()) {
        return "LLM: Deterministic fallback only | Set Gemini API key and model.";
    }
    if(isGitHubModelsMode()) {
        return "LLM: Deterministic fallback only | Set GitHub token and model for GitHub Models.";
    }
    if(isAnthropicMessagesMode()) {
        return "LLM: Deterministic fallback only | Set Anthropic API key and model.";
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
        "Use only tools whose planner_safe field is true. "
        "Use only tools whose planner_ready field is true. "
        "Treat planner_policy, planner_safe_tools, and planner_ready_tools as binding constraints, not hints. "
        "Only propose auto-executable steps for tools whose execution_mode is auto_run. "
        "Treat confirm_required tools as recommendations for the user, not automatic steps. "
        "Treat suggestion_only tools as non-executable suggestions. "
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

    const QJsonObject planSchema{
        {"type", "object"},
        {"additionalProperties", false},
        {"properties", QJsonObject{
             {"summary", QJsonObject{{"type", "string"}}},
             {"steps", QJsonObject{
                  {"type", "array"},
                  {"items", QJsonObject{
                       {"type", "object"},
                       {"additionalProperties", false},
                       {"properties", QJsonObject{
                            {"tool_name", QJsonObject{{"type", "string"}}},
                            {"arguments", QJsonObject{{"type", "object"}}}
                        }},
                       {"required", QJsonArray{"tool_name", "arguments"}}
                   }}
              }}
         }},
        {"required", QJsonArray{"summary", "steps"}}
    };

    QJsonObject payload;
    if(isOpenAIResponsesMode()) {
        payload = QJsonObject{
            {"model", model()},
            {"instructions", systemPrompt},
            {"input", QString::fromUtf8(QJsonDocument(userPayload).toJson(QJsonDocument::Compact))},
            {"text", QJsonObject{
                 {"format", QJsonObject{
                      {"type", "json_schema"},
                      {"name", "studio_plan"},
                      {"strict", true},
                      {"schema", planSchema}
                  }}
             }}
        };
    } else if(isAnthropicMessagesMode()) {
        payload = QJsonObject{
            {"model", model()},
            {"max_tokens", 2000},
            {"system", systemPrompt},
            {"messages", QJsonArray{
                QJsonObject{
                    {"role", "user"},
                    {"content", QString::fromUtf8(QJsonDocument(userPayload).toJson(QJsonDocument::Compact))}
                }
            }}
        };
    } else {
        payload = QJsonObject{
            {"model", model()},
            {"temperature", 0.0},
            {"messages", QJsonArray{
                QJsonObject{{"role", "system"}, {"content", systemPrompt}},
                QJsonObject{{"role", "user"}, {"content", QString::fromUtf8(QJsonDocument(userPayload).toJson(QJsonDocument::Compact))}}
            }}
        };
    }

    QNetworkRequest request{QUrl(endpoint())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if(isAnthropicMessagesMode()) {
        request.setRawHeader("x-api-key", apiKey().toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else if(!apiKey().isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey()).toUtf8());
    }
    if(isGitHubModelsMode()) {
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    }

    QNetworkReply* reply = m_networkAccessManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray responseBytes = reply->readAll();
    result.rawResponse = QString::fromUtf8(responseBytes);
    result.usedModel = true;
    result.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QJsonParseError responseError;
    const QJsonDocument responseDocument = QJsonDocument::fromJson(responseBytes, &responseError);
    const QJsonObject responseObject = responseError.error == QJsonParseError::NoError && responseDocument.isObject()
        ? responseDocument.object()
        : QJsonObject();

    if(reply->error() != QNetworkReply::NoError) {
        result.errorMessage = extractApiErrorMessage(responseObject, &result.providerErrorType);
        if(result.errorMessage.isEmpty()) {
            result.errorMessage = reply->errorString();
        }
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if(responseError.error != QJsonParseError::NoError || !responseDocument.isObject()) {
        result.errorMessage = responseError.errorString();
        return result;
    }

    result.errorMessage = extractApiErrorMessage(responseObject, &result.providerErrorType);
    if(!result.errorMessage.isEmpty()) {
        return result;
    }

    const QString responseStatus = responseObject.value("status").toString().trimmed();
    if(isOpenAIResponsesMode()
       && !responseStatus.isEmpty()
       && responseStatus != QLatin1String("completed")) {
        QString statusMessage = QString("OpenAI response status is `%1`.").arg(responseStatus);
        const QJsonObject incompleteDetails = responseObject.value("incomplete_details").toObject();
        if(!incompleteDetails.isEmpty()) {
            statusMessage += QString(" Details: %1")
                .arg(QString::fromUtf8(QJsonDocument(incompleteDetails).toJson(QJsonDocument::Compact)));
        }
        result.errorMessage = statusMessage;
        return result;
    }

    const QString contentString = extractContentString(responseObject);
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

bool LlmToolPlanner::isOpenAIResponsesMode() const
{
    return mode().compare("openai_responses", Qt::CaseInsensitive) == 0;
}

bool LlmToolPlanner::isGeminiOpenAICompatMode() const
{
    return mode().compare("gemini_openai", Qt::CaseInsensitive) == 0;
}

bool LlmToolPlanner::isGitHubModelsMode() const
{
    return mode().compare("github_models", Qt::CaseInsensitive) == 0;
}

bool LlmToolPlanner::isAnthropicMessagesMode() const
{
    return mode().compare("anthropic_messages", Qt::CaseInsensitive) == 0;
}

QString LlmToolPlanner::endpoint() const
{
    const QString resolvedEndpoint = configOrEnv(m_config.endpoint, "MNE_ANALYZE_STUDIO_LLM_ENDPOINT");
    if(!resolvedEndpoint.isEmpty()) {
        return resolvedEndpoint;
    }

    if(isOpenAIResponsesMode()) {
        return QString("https://api.openai.com/v1/responses");
    }
    if(isGeminiOpenAICompatMode()) {
        return QString("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    }
    if(isGitHubModelsMode()) {
        return QString("https://models.inference.ai.azure.com/chat/completions");
    }
    if(isAnthropicMessagesMode()) {
        return QString("https://api.anthropic.com/v1/messages");
    }

    return QString();
}

QString LlmToolPlanner::apiKey() const
{
    return configOrEnv(m_config.apiKey, "MNE_ANALYZE_STUDIO_LLM_API_KEY");
}

QString LlmToolPlanner::model() const
{
    return configOrEnv(m_config.model, "MNE_ANALYZE_STUDIO_LLM_MODEL");
}

QString LlmToolPlanner::extractApiErrorMessage(const QJsonObject& response, QString* errorType) const
{
    if(errorType) {
        *errorType = QString();
    }

    const QJsonObject errorObject = response.value("error").toObject();
    if(!errorObject.isEmpty()) {
        if(errorType) {
            *errorType = errorObject.value("type").toString().trimmed();
        }
        return errorObject.value("message").toString().trimmed();
    }

    const QString refusal = response.value("refusal").toString().trimmed();
    if(!refusal.isEmpty()) {
        if(errorType) {
            *errorType = QString("refusal");
        }
        return refusal;
    }

    const QJsonArray output = response.value("output").toArray();
    for(const QJsonValue& itemValue : output) {
        const QJsonObject item = itemValue.toObject();
        const QString itemType = item.value("type").toString().trimmed();
        if(itemType == QLatin1String("refusal")) {
            if(errorType) {
                *errorType = QString("refusal");
            }
            const QJsonArray content = item.value("content").toArray();
            for(const QJsonValue& contentValue : content) {
                const QJsonObject contentPart = contentValue.toObject();
                const QString refusalText = contentPart.value("refusal").toString().trimmed();
                if(!refusalText.isEmpty()) {
                    return refusalText;
                }
                const QString text = contentPart.value("text").toString().trimmed();
                if(!text.isEmpty()) {
                    return text;
                }
            }
        }
    }

    return QString();
}

QString LlmToolPlanner::configOrEnv(const QString& configuredValue, const char* envName) const
{
    return configuredValue.trimmed().isEmpty() ? envValue(envName) : configuredValue.trimmed();
}

QString LlmToolPlanner::extractContentString(const QJsonObject& response) const
{
    const QJsonArray anthropicContent = response.value("content").toArray();
    if(!anthropicContent.isEmpty()) {
        QStringList textParts;
        for(const QJsonValue& partValue : anthropicContent) {
            const QJsonObject part = partValue.toObject();
            if(part.value("type").toString() == QLatin1String("text")) {
                textParts << part.value("text").toString();
            }
        }
        if(!textParts.isEmpty()) {
            return textParts.join("\n");
        }
    }

    const QString outputText = response.value("output_text").toString().trimmed();
    if(!outputText.isEmpty()) {
        return outputText;
    }

    const QJsonArray output = response.value("output").toArray();
    if(!output.isEmpty()) {
        QStringList textParts;
        for(const QJsonValue& itemValue : output) {
            const QJsonObject item = itemValue.toObject();
            const QJsonArray content = item.value("content").toArray();
            for(const QJsonValue& contentValue : content) {
                const QJsonObject contentPart = contentValue.toObject();
                if(contentPart.value("type").toString() == QLatin1String("output_text")) {
                    textParts << contentPart.value("text").toString();
                }
            }
        }

        if(!textParts.isEmpty()) {
            return textParts.join("\n");
        }
    }

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
