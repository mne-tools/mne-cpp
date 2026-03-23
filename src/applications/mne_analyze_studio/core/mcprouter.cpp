//=============================================================================================================
/**
 * @file     mcprouter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the lightweight MCP/JSON-RPC method router.
 */

#include "mcprouter.h"

#include "jsonrpcmessage.h"

using namespace MNEANALYZESTUDIO;

McpRouter::McpRouter(QObject* parent)
: QObject(parent)
{
}

void McpRouter::registerMethod(const QString& method, Handler handler)
{
    m_handlers.insert(method, std::move(handler));
}

QJsonObject McpRouter::route(const QJsonObject& request) const
{
    if(!JsonRpcMessage::isValid(request)) {
        return JsonRpcMessage::createError(request.value("id"), -32600, "Invalid JSON-RPC envelope.");
    }

    const QString method = request.value("method").toString();
    const auto it = m_handlers.constFind(method);
    if(it == m_handlers.constEnd()) {
        return JsonRpcMessage::createError(request.value("id"), -32601, QString("No handler registered for %1").arg(method));
    }

    return JsonRpcMessage::createResponse(request.value("id"), it.value()(request.value("params").toObject()));
}
