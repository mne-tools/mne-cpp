//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mcprouter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the lightweight MCP/JSON-RPC method router used by the studio processes.
 */

#ifndef MNE_ANALYZE_STUDIO_MCPROUTER_H
#define MNE_ANALYZE_STUDIO_MCPROUTER_H

#include "studio_core_global.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <functional>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Simple request router that dispatches JSON-RPC method calls to registered handlers.
 */
class STUDIOCORESHARED_EXPORT McpRouter : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<QJsonObject(const QJsonObject&)>;

    explicit McpRouter(QObject* parent = nullptr);

    void registerMethod(const QString& method, Handler handler);
    QJsonObject route(const QJsonObject& request) const;

private:
    QHash<QString, Handler> m_handlers;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_MCPROUTER_H
