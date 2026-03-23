//=============================================================================================================
/**
 * @file     jsonrpcmessage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements JSON-RPC 2.0 message helpers for MNE Analyze Studio.
 */

#include "jsonrpcmessage.h"

#include <QJsonDocument>
#include <QJsonParseError>

using namespace MNEANALYZESTUDIO;

QJsonObject JsonRpcMessage::createRequest(const QString& id,
                                          const QString& method,
                                          const QJsonObject& params)
{
    return QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method},
        {"params", params}
    };
}

QJsonObject JsonRpcMessage::createResponse(const QJsonValue& id,
                                           const QJsonObject& result)
{
    return QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
}

QJsonObject JsonRpcMessage::createError(const QJsonValue& id,
                                        int code,
                                        const QString& message)
{
    return QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", QJsonObject{
            {"code", code},
            {"message", message}
        }}
    };
}

QByteArray JsonRpcMessage::serialize(const QJsonObject& message)
{
    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');
    return payload;
}

bool JsonRpcMessage::deserialize(const QByteArray& payload, QJsonObject& message, QString& errorString)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload.trimmed(), &error);
    if(error.error != QJsonParseError::NoError || !document.isObject()) {
        errorString = error.errorString();
        return false;
    }

    message = document.object();
    return isValid(message);
}

bool JsonRpcMessage::isValid(const QJsonObject& message)
{
    return message.value("jsonrpc").toString() == "2.0";
}
