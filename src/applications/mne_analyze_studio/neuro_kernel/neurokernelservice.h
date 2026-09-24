//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     neurokernelservice.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the local Neuro-Kernel service endpoint for MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_NEUROKERNELSERVICE_H
#define MNE_ANALYZE_STUDIO_NEUROKERNELSERVICE_H

#include <mcprouter.h>

#include <QJsonArray>
#include <QLocalServer>
#include <QObject>

class QLocalSocket;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Local socket service that exposes Neuro-Kernel analysis tools over JSON-RPC.
 */
class NeuroKernelService : public QObject
{
    Q_OBJECT

public:
    explicit NeuroKernelService(QObject* parent = nullptr);
    bool start(const QString& socketName);

private:
    QJsonObject handleToolCall(const QJsonObject& params) const;
    QJsonObject handleToolCallUnchecked(const QJsonObject& params) const;
    QJsonObject handleToolsList() const;
    QJsonArray toolDefinitions() const;

    QLocalServer m_server;
    McpRouter m_router;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_NEUROKERNELSERVICE_H
