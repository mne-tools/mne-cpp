//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     httprequest.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     November, 2026
 * @version  dev
 * @brief    Implements HTTP requests through the system's curl program.
 */

#include "httprequest.h"

#include <mne/base/process.h>

#include <QDir>
#include <QEventLoop>
#include <QTemporaryFile>

#include <string>
#include <thread>
#include <vector>

using namespace MNEANALYZESTUDIO;

namespace
{

/** curl's exit code for a response with a status of 400 or more (--fail-with-body). */
constexpr int kCurlHttpError = 22;

/** @return A file only the user can read holding @p content, or nothing if it cannot be written. */
std::optional<QString> writePrivateFile(QTemporaryFile& file, const QByteArray& content)
{
    // QTemporaryFile creates its file readable and writable by the owner only
    if (!file.open() || file.write(content) != content.size() || !file.flush()) {
        return std::nullopt;
    }
    file.close();
    return file.fileName();
}

} // namespace

HttpResponse MNEANALYZESTUDIO::sendHttpRequest(const QString& url,
                                               const QList<QPair<QByteArray, QByteArray>>& headers,
                                               const std::optional<QByteArray>& body)
{
    HttpResponse response;

    const QString pattern = QDir(QDir::tempPath()).filePath(QStringLiteral("mne_studio_http_XXXXXX"));
    QTemporaryFile headerFile(pattern);
    QTemporaryFile bodyFile(pattern);

    // No "Expect: 100-continue": the body follows the headers without waiting for the server
    QByteArray headerLines("Expect:\r\n");
    for (const auto& [name, value] : headers) {
        headerLines += name + ": " + value + "\r\n";
    }
    const std::optional<QString> headerPath = writePrivateFile(headerFile, headerLines);
    const std::optional<QString> bodyPath = body ? writePrivateFile(bodyFile, *body) : std::optional<QString>(QString());
    if (!headerPath || !bodyPath) {
        response.errorString = QStringLiteral("Could not write the request to a temporary file.");
        return response;
    }

    // -q first: ignore ~/.curlrc. The status follows the body on a line of its own.
    std::vector<std::string> args{"-q", "-sS", "--fail-with-body", "--proto", "=http,https", "-H", "@" + headerPath->toStdString()};
    if (body) {
        args.insert(args.end(), {"--data-binary", "@" + bodyPath->toStdString()});
    }
    args.insert(args.end(), {"-w", "\\n%{http_code}", "--url", url.toStdString()});

    // curl runs in a thread while this one keeps handling its events, as a network reply would let it
    mne::ProcessResult result;
    QEventLoop loop;
    std::jthread worker([&result, &args, &loop]() {
        result = mne::run_process("curl", args);
        QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
    });
    loop.exec();
    worker.join();

    if (!result.started) {
        response.errorString = QStringLiteral("The curl program, which sends the requests, could not be started: %1")
                                   .arg(QString::fromStdString(result.error));
        return response;
    }

    QByteArray output = QByteArray::fromStdString(result.std_out);
    const qsizetype statusLine = output.lastIndexOf('\n');
    if (statusLine >= 0) {
        response.statusCode = output.mid(statusLine + 1).trimmed().toInt();
        output.truncate(statusLine);
    }
    response.body = output;

    if (result.ok()) {
        return response;
    }
    if (result.exit_code == kCurlHttpError && response.statusCode >= 400) {
        response.errorString = QStringLiteral("Error transferring %1 - server replied with HTTP status %2").arg(url).arg(response.statusCode);
    } else {
        const QString reason = QString::fromStdString(result.std_err).trimmed();
        response.errorString = reason.isEmpty() ? QStringLiteral("curl failed with exit code %1").arg(result.exit_code) : reason;
    }
    return response;
}
