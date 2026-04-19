//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Implements the mne_show_mna CLI tool for inspecting .mna/.mnx files.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_subject.h>
#include <mna/mna_session.h>
#include <mna/mna_recording.h>
#include <mna/mna_file_ref.h>
#include <mna/mna_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// HELPERS
//=============================================================================================================

static QString humanSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString::number(bytes) + QStringLiteral(" B");
    if (bytes < 1024 * 1024)
        return QString::number(bytes / 1024.0, 'f', 1) + QStringLiteral(" KB");
    if (bytes < 1024 * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + QStringLiteral(" MB");
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + QStringLiteral(" GB");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mne_show_mna"));
    app.setApplicationVersion(QStringLiteral(MNE_CPP_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Inspect MNA (.mna) and MNX (.mnx) project files.\n\n"
                       "Displays the project structure: subjects, sessions, recordings,\n"
                       "referenced/embedded files, pipeline nodes, and metadata."));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument(QStringLiteral("file"),
        QStringLiteral("Path to a .mna or .mnx project file."));

    QCommandLineOption verboseOpt(QStringList() << QStringLiteral("V") << QStringLiteral("verbose"),
        QStringLiteral("Show extended details (SHA-256, format, size)."));
    parser.addOption(verboseOpt);

    QCommandLineOption jsonOpt(QStringLiteral("json"),
        QStringLiteral("Output the project as formatted JSON to stdout."));
    parser.addOption(jsonOpt);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        fprintf(stderr, "Error: no input file specified.\n\n");
        parser.showHelp(1);
    }

    const QString filePath = args.first();
    if (!QFile::exists(filePath)) {
        fprintf(stderr, "Error: file not found: %s\n", qPrintable(filePath));
        return 1;
    }

    // ── Read the project ────────────────────────────────────────────────
    MnaProject proj = MnaIO::read(filePath);
    if (proj.name.isEmpty() && proj.subjects.isEmpty() && proj.pipeline.isEmpty()) {
        fprintf(stderr, "Error: failed to read or empty project: %s\n", qPrintable(filePath));
        return 1;
    }

    QTextStream out(stdout);

    // ── JSON dump mode ──────────────────────────────────────────────────
    if (parser.isSet(jsonOpt)) {
        QJsonDocument doc(proj.toJson());
        out << doc.toJson(QJsonDocument::Indented);
        return 0;
    }

    // ── Human-readable display ──────────────────────────────────────────
    const bool verbose = parser.isSet(verboseOpt);
    const QFileInfo fi(filePath);

    out << "═══════════════════════════════════════════════════════════════\n";
    out << " MNA Project: " << proj.name << "\n";
    out << "═══════════════════════════════════════════════════════════════\n";
    out << "  File:        " << fi.fileName() << " (" << humanSize(fi.size()) << ")\n";
    out << "  Format:      " << (fi.suffix().toLower() == QLatin1String("mnx") ? "MNX (CBOR)" : "MNA (JSON)") << "\n";
    out << "  MNA Version: " << proj.mnaVersion << "\n";
    if (!proj.description.isEmpty())
        out << "  Description: " << proj.description << "\n";
    if (proj.created.isValid())
        out << "  Created:     " << proj.created.toString(Qt::ISODate) << "\n";
    if (proj.modified.isValid())
        out << "  Modified:    " << proj.modified.toString(Qt::ISODate) << "\n";
    out << "\n";

    // ── Subjects / Sessions / Recordings / Files ────────────────────────
    int totalFiles = 0;
    qint64 totalEmbeddedSize = 0;
    int embeddedCount = 0;

    if (!proj.subjects.isEmpty()) {
        out << "  Subjects: " << proj.subjects.size() << "\n";
        out << "  ───────────────────────────────────────────────────────────\n";

        for (int si = 0; si < proj.subjects.size(); ++si) {
            const MnaSubject &subj = proj.subjects[si];
            out << "  [Subject " << (si + 1) << "] " << subj.id << "\n";

            for (int sei = 0; sei < subj.sessions.size(); ++sei) {
                const MnaSession &sess = subj.sessions[sei];
                out << "    [Session " << (sei + 1) << "] " << sess.id << "\n";

                for (int ri = 0; ri < sess.recordings.size(); ++ri) {
                    const MnaRecording &rec = sess.recordings[ri];
                    out << "      [Recording " << (ri + 1) << "] " << rec.id
                        << " ─ " << rec.files.size() << " file(s)\n";

                    for (int fi2 = 0; fi2 < rec.files.size(); ++fi2) {
                        const MnaFileRef &ref = rec.files[fi2];
                        ++totalFiles;

                        QString embTag;
                        if (ref.embedded) {
                            embTag = QStringLiteral(" [EMBEDDED]");
                            embeddedCount++;
                            totalEmbeddedSize += ref.sizeBytes;
                        }

                        out << "        " << (fi2 + 1) << ". "
                            << mnaFileRoleToString(ref.role) << "  "
                            << ref.path << embTag << "\n";

                        if (verbose) {
                            out << "           format: " << ref.format
                                << "  size: " << humanSize(ref.sizeBytes);
                            if (!ref.sha256.isEmpty())
                                out << "  sha256: " << ref.sha256.left(16) << "...";
                            out << "\n";
                        }
                    }
                }
            }
        }
        out << "\n";
    }

    // ── Pipeline ────────────────────────────────────────────────────────
    if (!proj.pipeline.isEmpty()) {
        out << "  Pipeline: " << proj.pipeline.size() << " node(s)\n";
        out << "  ───────────────────────────────────────────────────────────\n";

        for (int ni = 0; ni < proj.pipeline.size(); ++ni) {
            const MnaNode &node = proj.pipeline[ni];
            out << "  [" << (ni + 1) << "] " << node.id
                << "  op=" << node.opType;
            if (!node.toolVersion.isEmpty())
                out << "  v" << node.toolVersion;
            out << "\n";

            if (verbose) {
                for (const MnaPort &p : node.inputs)
                    out << "       IN  " << p.name << " (" << mnaDataKindToString(p.dataKind) << ")\n";
                for (const MnaPort &p : node.outputs)
                    out << "       OUT " << p.name << " (" << mnaDataKindToString(p.dataKind) << ")\n";
            }
        }
        out << "\n";
    }

    // ── Summary ─────────────────────────────────────────────────────────
    out << "  Summary\n";
    out << "  ───────────────────────────────────────────────────────────\n";
    out << "  Total files:    " << totalFiles << "\n";
    if (embeddedCount > 0)
        out << "  Embedded:       " << embeddedCount << " (" << humanSize(totalEmbeddedSize) << ")\n";
    out << "  Pipeline nodes: " << proj.pipeline.size() << "\n";
    out << "═══════════════════════════════════════════════════════════════\n";

    return 0;
}
