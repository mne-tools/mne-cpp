//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     httprequest.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     November, 2026
 * @version  dev
 * @brief    Declares HTTP requests through the system's curl program.
 */

#ifndef MNE_ANALYZE_STUDIO_HTTPREQUEST_H
#define MNE_ANALYZE_STUDIO_HTTPREQUEST_H

#include "studio_core_global.h"

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

#include <optional>

namespace MNEANALYZESTUDIO
{

struct HttpResponse
{
    int statusCode = 0;  /**< HTTP status of the response, 0 if none arrived. */
    QByteArray body;     /**< Body of the response, also of an error response. */
    QString errorString; /**< Why the request failed (no response or a status of 400 or more); empty on success. */
};

/**
 * Sends a request with the system's curl program while the calling thread keeps processing its events. The
 * headers and the body go to curl in files only the user can read, so no secret appears in the process list.
 *
 * @param[in] url        Target, http or https.
 * @param[in] headers    Request headers.
 * @param[in] body       Body of a POST request; none sends a GET request.
 *
 * @return The response.
 */
STUDIOCORESHARED_EXPORT HttpResponse sendHttpRequest(const QString& url,
                                                     const QList<QPair<QByteArray, QByteArray>>& headers,
                                                     const std::optional<QByteArray>& body = std::nullopt);

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_HTTPREQUEST_H
