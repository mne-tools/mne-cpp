//=============================================================================================================
/**
 * @file     llmtoolplanner.h
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
 * @brief    Declares an OpenAI-compatible LLM planner adapter for studio tool selection.
 */

#ifndef MNE_ANALYZE_STUDIO_LLMTOOLPLANNER_H
#define MNE_ANALYZE_STUDIO_LLMTOOLPLANNER_H

#include "studio_core_global.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>

class QNetworkAccessManager;

namespace MNEANALYZESTUDIO
{

struct LlmPlannerConfig
{
    QString mode;
    QString providerName;
    QString endpoint;
    QString apiKey;
    QString model;
};

struct LlmPlanResult
{
    bool usedModel = false;
    bool success = false;
    int httpStatusCode = 0;
    QString providerName;
    QString summary;
    QString errorMessage;
    QString providerErrorType;
    QString rawResponse;
    QStringList plannedCommands;
};

/**
 * @brief OpenAI-compatible HTTP adapter that asks an LLM to plan tool calls.
 */
class STUDIOCORESHARED_EXPORT LlmToolPlanner : public QObject
{
    Q_OBJECT

public:
    explicit LlmToolPlanner(QObject* parent = nullptr);

    void setConfiguration(const LlmPlannerConfig& config);
    LlmPlannerConfig configuration() const;
    bool isConfigured() const;
    bool isMockMode() const;
    QString providerName() const;
    QString statusSummary() const;
    LlmPlanResult plan(const QString& userCommand,
                       const QJsonArray& toolDefinitions,
                       const QJsonObject& context) const;

private:
    QString configOrEnv(const QString& configuredValue, const char* envName) const;
    QString mode() const;
    bool isOpenAIResponsesMode() const;
    bool isGeminiOpenAICompatMode() const;
    bool isGitHubModelsMode() const;
    bool isAnthropicMessagesMode() const;
    QString endpoint() const;
    QString apiKey() const;
    QString model() const;
    QString extractApiErrorMessage(const QJsonObject& response, QString* errorType = nullptr) const;
    QString extractContentString(const QJsonObject& response) const;
    QString extractJsonPayload(const QString& text) const;

    LlmPlannerConfig m_config;
    mutable QNetworkAccessManager* m_networkAccessManager;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_LLMTOOLPLANNER_H
