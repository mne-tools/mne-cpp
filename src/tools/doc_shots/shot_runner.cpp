//=============================================================================================================
/**
 * @file     shot_runner.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Implementation of DOCSHOTS::ShotRunner.
 */

#include "shot_runner.h"
#include "shot_kinds.h"
#include "shot_mne_align_app.h"
#include "shot_mne_inspect_app.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace DOCSHOTS
{

ShotRunner::ShotRunner(RunnerOptions options)
    : m_options(std::move(options))
{
}

bool ShotRunner::loadManifest(QString& outDir, QVector<ShotSpec>& shots, QString& err) const
{
    QFile f(m_options.manifestPath);
    if (!f.open(QIODevice::ReadOnly)) {
        err = QStringLiteral("Cannot open manifest: %1").arg(m_options.manifestPath);
        return false;
    }
    QJsonParseError je{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &je);
    if (je.error != QJsonParseError::NoError) {
        err = QStringLiteral("Manifest JSON parse error: %1").arg(je.errorString());
        return false;
    }
    if (!doc.isObject()) {
        err = QStringLiteral("Manifest root must be a JSON object");
        return false;
    }
    const QJsonObject root = doc.object();
    outDir = m_options.outDirOverride.isEmpty()
                 ? root.value(QStringLiteral("out_dir")).toString()
                 : m_options.outDirOverride;
    if (outDir.isEmpty()) {
        err = QStringLiteral("Manifest is missing required key 'out_dir'");
        return false;
    }
    // Resolve relative out_dir relative to the manifest file's directory.
    if (QFileInfo(outDir).isRelative()) {
        outDir = QDir(QFileInfo(m_options.manifestPath).absolutePath())
                     .absoluteFilePath(outDir);
    }

    const QJsonArray arr = root.value(QStringLiteral("shots")).toArray();
    if (arr.isEmpty()) {
        err = QStringLiteral("Manifest has no shots");
        return false;
    }
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        ShotSpec s;
        s.id = o.value(QStringLiteral("id")).toString();
        s.kind = o.value(QStringLiteral("kind")).toString();
        const QJsonArray sz = o.value(QStringLiteral("size")).toArray();
        if (sz.size() == 2) {
            s.size = QSize(sz.at(0).toInt(1280), sz.at(1).toInt(800));
        }
        s.setup = o.value(QStringLiteral("setup")).toObject();
        s.requiresSampleData = o.value(QStringLiteral("requires_sample_data")).toBool(false);
        if (s.id.isEmpty() || s.kind.isEmpty()) {
            err = QStringLiteral("Manifest entry missing 'id' or 'kind'");
            return false;
        }
        shots.append(s);
    }
    return true;
}

bool ShotRunner::renderShot(const ShotSpec& spec, const QString& outPath,
                            bool& skipped, QString& err) const
{
    skipped = false;
    if (spec.requiresSampleData
        && qEnvironmentVariableIsEmpty("MNE_DOC_SHOTS_HAVE_SAMPLE_DATA")) {
        skipped = true;
        return true;
    }

    if (spec.kind == QLatin1String("widget_mockup")) {
        return DOCSHOTS::renderWidgetMockup(spec, outPath, err);
    }
    if (spec.kind == QLatin1String("scene_only")) {
        return DOCSHOTS::renderSceneOnly(spec, outPath, skipped, err);
    }
    if (spec.kind == QLatin1String("mne_inspect_app")) {
        return DOCSHOTS::renderMneInspectApp(spec, outPath, skipped, err);
    }
    if (spec.kind == QLatin1String("mne_align_app")) {
        return DOCSHOTS::renderMneAlignApp(spec, outPath, skipped, err);
    }
    err = QStringLiteral("Unknown shot kind: %1").arg(spec.kind);
    return false;
}

bool ShotRunner::run()
{
    QTextStream out(stdout);
    QTextStream errOut(stderr);

    QString outDir;
    QVector<ShotSpec> shots;
    QString loadErr;
    if (!loadManifest(outDir, shots, loadErr)) {
        errOut << "[mne_doc_shots] ERROR: " << loadErr << "\n";
        return false;
    }

    if (!QDir().mkpath(outDir)) {
        errOut << "[mne_doc_shots] ERROR: cannot create out_dir: " << outDir << "\n";
        return false;
    }

    int generated = 0;
    int alreadyPresent = 0;
    int skipped = 0;
    int failed = 0;

    for (const ShotSpec& spec : shots) {
        const QString outPath = QDir(outDir).absoluteFilePath(spec.id + QStringLiteral(".png"));
        QDir().mkpath(QFileInfo(outPath).absolutePath());

        if (!m_options.force && QFileInfo::exists(outPath)) {
            alreadyPresent++;
            out << "[mne_doc_shots] CACHED " << spec.id << "\n";
            continue;
        }

        bool wasSkipped = false;
        QString err;
        if (renderShot(spec, outPath, wasSkipped, err)) {
            if (wasSkipped) {
                skipped++;
                out << "[mne_doc_shots] SKIP    " << spec.id
                    << " (" << spec.kind << ")\n";
            } else {
                generated++;
                out << "[mne_doc_shots] OK      " << spec.id
                    << " -> " << QDir().relativeFilePath(outPath) << "\n";
            }
        } else {
            failed++;
            errOut << "[mne_doc_shots] FAIL    " << spec.id << ": " << err << "\n";
        }
    }

    out << "[mne_doc_shots] summary: " << generated << " generated, "
        << alreadyPresent << " cached, " << skipped << " skipped, "
        << failed << " failed\n";
    return failed == 0;
}

}  // namespace DOCSHOTS
