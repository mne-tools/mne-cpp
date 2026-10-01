//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
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
 * @brief    Entry point for the mne_doc_shots screenshot generator.
 */

#include "shot_capture.h"
#include "shot_runner.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#ifndef Q_OS_WIN
#include <QTimer>
#endif

int main(int argc, char* argv[])
{
    DOCSHOTS::prepareProcessEnvironment();
    DOCSHOTS::installDeduplicatingMessageHandler();
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mne_doc_shots"));
    QCoreApplication::setApplicationVersion(QStringLiteral("2.4.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Generate documentation screenshots from a JSON manifest."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("manifest"),
                                 QStringLiteral("Path to manifest JSON file."));

    const QCommandLineOption outOption(
        QStringList{QStringLiteral("o"), QStringLiteral("out")},
        QStringLiteral("Override the manifest's out_dir."),
        QStringLiteral("dir"));
    parser.addOption(outOption);

    const QCommandLineOption forceOption(
        QStringList{QStringLiteral("f"), QStringLiteral("force")},
        QStringLiteral("Re-generate every shot even if the PNG already exists."));
    parser.addOption(forceOption);

    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 1) {
        parser.showHelp(2);
    }

    QString themeErr;
    if (!DOCSHOTS::applyDeterministicTheme(app, themeErr)) {
        QTextStream(stderr) << "[mne_doc_shots] ERROR: " << themeErr << "\n";
        return 1;
    }
    QTextStream(stdout) << "[mne_doc_shots] environment: " << DOCSHOTS::environmentSummary() << "\n";

    DOCSHOTS::RunnerOptions opts;
    opts.manifestPath = QFileInfo(positional.first()).absoluteFilePath();
    opts.outDirOverride = parser.value(outOption);
    opts.force = parser.isSet(forceOption);

    DOCSHOTS::ShotRunner runner(opts);
#ifdef Q_OS_WIN
    return runner.run() ? 0 : 1;
#else
    QTimer::singleShot(0, &app, [&app, &runner]() {
        app.exit(runner.run() ? 0 : 1);
    });
    return app.exec();
#endif
}
