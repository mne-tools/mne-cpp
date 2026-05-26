//=============================================================================================================
/**
 * @file     shot_runner.h
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
 * @brief    Declaration of DOCSHOTS::ShotRunner and the manifest data types.
 */

#ifndef DOCSHOTS_SHOT_RUNNER_H
#define DOCSHOTS_SHOT_RUNNER_H

#include <QJsonObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

namespace DOCSHOTS
{

//=============================================================================================================
/** Per-shot specification parsed from the manifest. */
struct ShotSpec
{
    QString     id;        ///< Output stem; joined with out_dir → out_dir/<id>.png.
    QString     kind;      ///< Shot kind, see shot_widget_mockup.cpp / shot_scene_only.cpp.
    QSize       size{1280, 800};
    QJsonObject setup;     ///< Free-form per-kind configuration block.
    bool        requiresSampleData{false};
};

//=============================================================================================================
/** Top-level runner configuration. */
struct RunnerOptions
{
    QString manifestPath;
    QString outDirOverride;
    bool    force{false};
};

//=============================================================================================================
/**
 * Loads a manifest JSON file and produces a PNG per shot under <out_dir>.
 *
 * @brief    Top-level orchestrator for the mne_doc_shots tool.
 */
class ShotRunner
{
public:
    explicit ShotRunner(RunnerOptions options);

    /** Returns true if all shots succeeded (or were intentionally skipped). */
    bool run();

private:
    bool loadManifest(QString& outDir, QVector<ShotSpec>& shots, QString& err) const;

    /** Dispatch on `spec.kind`. Returns true on success or graceful skip. */
    bool renderShot(const ShotSpec& spec, const QString& outPath, bool& skipped, QString& err) const;

    RunnerOptions m_options;
};

}  // namespace DOCSHOTS

#endif  // DOCSHOTS_SHOT_RUNNER_H
