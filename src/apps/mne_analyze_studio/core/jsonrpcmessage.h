//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     jsonrpcmessage.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares helper functions for JSON-RPC 2.0 message creation and parsing.
 */

#ifndef MNE_ANALYZE_STUDIO_JSONRPCMESSAGE_H
#define MNE_ANALYZE_STUDIO_JSONRPCMESSAGE_H

#include "studio_core_global.h"

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Utility namespace class for JSON-RPC 2.0 message handling.
 */
class STUDIOCORESHARED_EXPORT JsonRpcMessage
{
public:
    static QJsonObject createRequest(const QString& id,
                                     const QString& method,
                                     const QJsonObject& params = QJsonObject());

    static QJsonObject createResponse(const QJsonValue& id,
                                      const QJsonObject& result);

    static QJsonObject createError(const QJsonValue& id,
                                   int code,
                                   const QString& message);

    static QByteArray serialize(const QJsonObject& message);
    static bool deserialize(const QByteArray& payload, QJsonObject& message, QString& errorString);
    static bool isValid(const QJsonObject& message);
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_JSONRPCMESSAGE_H
