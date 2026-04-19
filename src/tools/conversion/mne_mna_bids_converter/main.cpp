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
 * @brief    MNA/MNX ↔ BIDS converter.
 *
 *  Commands:
 *    extract   Extract embedded files from an .mnx to a BIDS directory tree.
 *    pack      Import a BIDS directory tree into an .mna or .mnx project.
 *    convert   Convert between .mna (JSON) and .mnx (CBOR) formats.
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
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// BIDS HELPERS
//=============================================================================================================

/**
 * Map an MnaFileRole to a BIDS-like subdirectory.
 */
static QString bidsSubdirForRole(MnaFileRole role)
{
    switch (role) {
    case MnaFileRole::Surface:
    case MnaFileRole::Annotation:
        return QStringLiteral("anat");
    case MnaFileRole::Bem:
    case MnaFileRole::SourceSpace:
        return QStringLiteral("bem");
    case MnaFileRole::Digitizer:
    case MnaFileRole::Transform:
    case MnaFileRole::Evoked:
        return QStringLiteral("meg");
    case MnaFileRole::SourceEstimate:
        return QStringLiteral("source");
    default:
        return QStringLiteral("other");
    }
}

/**
 * Build a BIDS-like relative path: sub-<subj>/ses-<sess>/<modality>/<filename>
 */
static QString bidsBuildRelPath(const QString &subjId, const QString &sessId,
                                MnaFileRole role, const QString &fileName)
{
    return QStringLiteral("sub-%1/ses-%2/%3/%4")
        .arg(subjId, sessId, bidsSubdirForRole(role), QFileInfo(fileName).fileName());
}

/**
 * Guess MnaFileRole from a file name.
 */
static MnaFileRole guessRoleFromFileName(const QString &fileName)
{
    const QString fn = fileName.toLower();

    // Surface files
    if (fn.endsWith(QLatin1String(".pial")) || fn.endsWith(QLatin1String(".inflated"))
        || fn.endsWith(QLatin1String(".white")) || fn.endsWith(QLatin1String(".orig"))
        || fn.endsWith(QLatin1String(".sphere")))
        return MnaFileRole::Surface;

    // Annotations
    if (fn.endsWith(QLatin1String(".annot")))
        return MnaFileRole::Annotation;

    // Source estimates
    if (fn.endsWith(QLatin1String(".stc")) || fn.endsWith(QLatin1String(".w")))
        return MnaFileRole::SourceEstimate;

    // FIFF-based files — use name heuristics
    if (fn.contains(QLatin1String("-bem")) || fn.contains(QLatin1String("_bem")))
        return MnaFileRole::Bem;
    if (fn.contains(QLatin1String("-src")) || fn.contains(QLatin1String("_src"))
        || fn.contains(QLatin1String("-oct-")))
        return MnaFileRole::SourceSpace;
    if (fn.contains(QLatin1String("-trans")) || fn.contains(QLatin1String("_trans"))
        || fn.contains(QLatin1String("all-trans")))
        return MnaFileRole::Transform;
    if (fn.contains(QLatin1String("-ave")) || fn.contains(QLatin1String("_ave")))
        return MnaFileRole::Evoked;
    if (fn.contains(QLatin1String("-fwd")) || fn.contains(QLatin1String("_fwd")))
        return MnaFileRole::Forward;
    if (fn.contains(QLatin1String("-inv")) || fn.contains(QLatin1String("_inv")))
        return MnaFileRole::Inverse;
    if (fn.contains(QLatin1String("-cov")) || fn.contains(QLatin1String("_cov")))
        return MnaFileRole::Covariance;
    if (fn.contains(QLatin1String("_raw")) || fn.contains(QLatin1String("-raw")))
        return MnaFileRole::Raw;
    if (fn.contains(QLatin1String("_dig")) || fn.contains(QLatin1String("digitizer")))
        return MnaFileRole::Digitizer;

    return MnaFileRole::Custom;
}

/**
 * Infer BIDS modality directory from a subdirectory name.
 */
static MnaFileRole guessRoleFromBidsDir(const QString &dirName)
{
    if (dirName == QLatin1String("anat"))
        return MnaFileRole::Surface; // default; file name will disambiguate
    if (dirName == QLatin1String("bem"))
        return MnaFileRole::Bem;
    if (dirName == QLatin1String("meg") || dirName == QLatin1String("eeg"))
        return MnaFileRole::Raw;
    if (dirName == QLatin1String("source"))
        return MnaFileRole::SourceEstimate;
    return MnaFileRole::Custom;
}

//=============================================================================================================
// COMMAND: extract
//=============================================================================================================

static int cmdExtract(const QString &inputPath, const QString &outputDir)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    MnaProject proj = MnaIO::read(inputPath);
    if (proj.subjects.isEmpty()) {
        err << "Error: failed to read or empty project: " << inputPath << "\n";
        return 1;
    }

    int extracted = 0;
    int skipped = 0;

    for (const MnaSubject &subj : proj.subjects) {
        for (const MnaSession &sess : subj.sessions) {
            for (const MnaRecording &rec : sess.recordings) {
                for (const MnaFileRef &ref : rec.files) {
                    // Build BIDS output path
                    QString relPath = ref.path;
                    if (relPath.isEmpty())
                        relPath = bidsBuildRelPath(subj.id, sess.id, ref.role,
                                                   QStringLiteral("file_") + QString::number(extracted));

                    const QString fullPath = QDir(outputDir).filePath(relPath);

                    if (ref.embedded && !ref.data.isEmpty()) {
                        QDir().mkpath(QFileInfo(fullPath).absolutePath());
                        QFile f(fullPath);
                        if (f.open(QIODevice::WriteOnly)) {
                            f.write(ref.data);
                            f.close();
                            out << "  extracted: " << relPath << "\n";
                            ++extracted;
                        } else {
                            err << "  FAILED:    " << relPath << "\n";
                        }
                    } else if (!ref.embedded) {
                        // Copy from source location relative to the project file
                        const QString srcDir = QFileInfo(inputPath).absolutePath();
                        const QString srcPath = QDir(srcDir).filePath(ref.path);
                        if (QFile::exists(srcPath)) {
                            QDir().mkpath(QFileInfo(fullPath).absolutePath());
                            if (QFile::copy(srcPath, fullPath)) {
                                out << "  copied:    " << relPath << "\n";
                                ++extracted;
                            } else {
                                err << "  FAILED:    " << relPath << " (copy error)\n";
                            }
                        } else {
                            err << "  SKIPPED:   " << ref.path << " (source not found)\n";
                            ++skipped;
                        }
                    } else {
                        err << "  SKIPPED:   " << ref.path << " (empty embedded data)\n";
                        ++skipped;
                    }
                }
            }
        }
    }

    // Write a sidecar .mna with non-embedded references pointing into BIDS tree
    MnaProject outProj = proj;
    for (MnaSubject &subj : outProj.subjects) {
        for (MnaSession &sess : subj.sessions) {
            for (MnaRecording &rec : sess.recordings) {
                for (MnaFileRef &ref : rec.files) {
                    ref.embedded = false;
                    ref.data.clear();
                }
            }
        }
    }
    outProj.modified = QDateTime::currentDateTimeUtc();
    const QString sidecarPath = QDir(outputDir).filePath(QStringLiteral("project.mna"));
    MnaIO::write(outProj, sidecarPath);
    out << "\n  Sidecar written: " << sidecarPath << "\n";

    out << "\n  Done. Extracted: " << extracted << "  Skipped: " << skipped << "\n";
    return 0;
}

//=============================================================================================================
// COMMAND: pack
//=============================================================================================================

static int cmdPack(const QString &inputDir, const QString &outputPath, bool embed)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    // Check if there's an existing project.mna sidecar to use as template
    const QString sidecarPath = QDir(inputDir).filePath(QStringLiteral("project.mna"));
    MnaProject proj;

    if (QFile::exists(sidecarPath)) {
        proj = MnaIO::read(sidecarPath);
        out << "  Using existing sidecar: " << sidecarPath << "\n";

        if (embed) {
            // Read file data into each ref
            for (MnaSubject &subj : proj.subjects) {
                for (MnaSession &sess : subj.sessions) {
                    for (MnaRecording &rec : sess.recordings) {
                        for (MnaFileRef &ref : rec.files) {
                            const QString filePath = QDir(inputDir).filePath(ref.path);
                            QFile f(filePath);
                            if (f.open(QIODevice::ReadOnly)) {
                                ref.data = f.readAll();
                                ref.sizeBytes = ref.data.size();
                                ref.embedded = true;
                                f.close();
                            } else {
                                err << "  WARNING: cannot read " << filePath << "\n";
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Scan the directory for BIDS structure: sub-*/ses-*/<modality>/*
        out << "  Scanning BIDS directory: " << inputDir << "\n";

        proj.name = QStringLiteral("BIDS Import");
        proj.description = QStringLiteral("Imported from BIDS directory");
        proj.modified = QDateTime::currentDateTimeUtc();

        // Find sub-* directories
        QDir rootDir(inputDir);
        QStringList subDirs = rootDir.entryList(QStringList() << QStringLiteral("sub-*"),
                                                 QDir::Dirs | QDir::NoDotAndDotDot);

        if (subDirs.isEmpty()) {
            // Flat structure — treat all files as belonging to one subject
            subDirs << QStringLiteral(".");
        }

        for (const QString &subDirName : subDirs) {
            MnaSubject subj;
            subj.id = subDirName.startsWith(QLatin1String("sub-"))
                      ? subDirName.mid(4) : subDirName;

            QDir subDir(rootDir.filePath(subDirName));
            QStringList sesDirs = subDir.entryList(QStringList() << QStringLiteral("ses-*"),
                                                    QDir::Dirs | QDir::NoDotAndDotDot);
            if (sesDirs.isEmpty())
                sesDirs << QStringLiteral(".");

            for (const QString &sesDirName : sesDirs) {
                MnaSession sess;
                sess.id = sesDirName.startsWith(QLatin1String("ses-"))
                          ? sesDirName.mid(4) : QStringLiteral("01");

                MnaRecording rec;
                rec.id = QStringLiteral("recording-01");

                QDir sesDir(subDir.filePath(sesDirName));
                // Iterate modality dirs: anat, meg, bem, source, etc.
                QStringList modDirs = sesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (modDirs.isEmpty())
                    modDirs << QStringLiteral(".");

                for (const QString &modDirName : modDirs) {
                    QDir modDir(sesDir.filePath(modDirName));
                    MnaFileRole defaultRole = guessRoleFromBidsDir(modDirName);

                    QStringList files = modDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
                    for (const QString &fileName : files) {
                        MnaFileRef ref;
                        // Prefer file-name heuristic; fall back to directory-based guess
                        ref.role = guessRoleFromFileName(fileName);
                        if (ref.role == MnaFileRole::Custom && defaultRole != MnaFileRole::Custom)
                            ref.role = defaultRole;

                        const QString absPath = modDir.filePath(fileName);
                        ref.path = rootDir.relativeFilePath(absPath);
                        ref.format = QFileInfo(fileName).suffix();

                        if (embed) {
                            QFile f(absPath);
                            if (f.open(QIODevice::ReadOnly)) {
                                ref.data = f.readAll();
                                ref.sizeBytes = ref.data.size();
                                ref.embedded = true;
                                f.close();
                            }
                        } else {
                            ref.embedded = false;
                            ref.sizeBytes = QFileInfo(absPath).size();
                        }

                        rec.files.append(ref);
                        out << "  + " << mnaFileRoleToString(ref.role) << "  " << ref.path << "\n";
                    }
                }

                if (!rec.files.isEmpty()) {
                    sess.recordings.append(rec);
                }
            }

            if (!subj.sessions.isEmpty()) {
                proj.subjects.append(subj);
            }
        }
    }

    proj.modified = QDateTime::currentDateTimeUtc();

    if (MnaIO::write(proj, outputPath)) {
        out << "\n  Written: " << outputPath << "\n";
        return 0;
    } else {
        err << "Error: failed to write " << outputPath << "\n";
        return 1;
    }
}

//=============================================================================================================
// COMMAND: convert
//=============================================================================================================

static int cmdConvert(const QString &inputPath, const QString &outputPath)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    MnaProject proj = MnaIO::read(inputPath);
    if (proj.name.isEmpty() && proj.subjects.isEmpty() && proj.pipeline.isEmpty()) {
        err << "Error: failed to read project: " << inputPath << "\n";
        return 1;
    }

    const QString outSuffix = QFileInfo(outputPath).suffix().toLower();
    const QString inSuffix = QFileInfo(inputPath).suffix().toLower();

    // When converting .mna → .mnx, embed file data automatically
    if (outSuffix == QLatin1String("mnx") && inSuffix == QLatin1String("mna")) {
        const QString projDir = QFileInfo(inputPath).absolutePath();
        for (MnaSubject &subj : proj.subjects) {
            for (MnaSession &sess : subj.sessions) {
                for (MnaRecording &rec : sess.recordings) {
                    for (MnaFileRef &ref : rec.files) {
                        if (!ref.embedded) {
                            const QString filePath = QDir(projDir).filePath(ref.path);
                            QFile f(filePath);
                            if (f.open(QIODevice::ReadOnly)) {
                                ref.data = f.readAll();
                                ref.sizeBytes = ref.data.size();
                                ref.embedded = true;
                                ref.path = QFileInfo(ref.path).fileName();
                                f.close();
                                out << "  embedded: " << filePath << "\n";
                            } else {
                                err << "  WARNING: cannot read " << filePath << ", keeping as reference\n";
                            }
                        }
                    }
                }
            }
        }
    }

    // When converting .mnx → .mna, strip embedded data (files must be extracted first)
    if (outSuffix == QLatin1String("mna") && inSuffix == QLatin1String("mnx")) {
        out << "  Note: embedded data will be dropped. Use 'extract' first to write files to disk.\n";
        for (MnaSubject &subj : proj.subjects) {
            for (MnaSession &sess : subj.sessions) {
                for (MnaRecording &rec : sess.recordings) {
                    for (MnaFileRef &ref : rec.files) {
                        ref.embedded = false;
                        ref.data.clear();
                    }
                }
            }
        }
    }

    proj.modified = QDateTime::currentDateTimeUtc();

    if (MnaIO::write(proj, outputPath)) {
        out << "  Converted: " << inputPath << " -> " << outputPath << "\n";
        return 0;
    } else {
        err << "Error: failed to write " << outputPath << "\n";
        return 1;
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

static void printUsage(QTextStream &out)
{
    out << "Usage: mne_mna_bids_converter <command> [options]\n\n"
        << "Commands:\n"
        << "  extract  <input.mnx|mna> <output_dir>    Extract files to BIDS directory tree\n"
        << "  pack     <bids_dir> <output.mnx|mna>     Import BIDS directory into MNA/MNX project\n"
        << "  convert  <input.mna|mnx> <output.mnx|mna>  Convert between MNA (JSON) and MNX (CBOR)\n"
        << "\nOptions:\n"
        << "  --embed     (pack) Embed file data into the output (default for .mnx)\n"
        << "  --no-embed  (pack) Store file references only (default for .mna)\n"
        << "  -h, --help  Show this help\n"
        << "  --version   Show version\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mne_mna_bids_converter"));
    app.setApplicationVersion(QStringLiteral(MNE_CPP_VERSION));

    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = app.arguments();
    args.removeFirst(); // program name

    // Check for help/version flags
    if (args.isEmpty() || args.contains(QStringLiteral("-h")) || args.contains(QStringLiteral("--help"))) {
        printUsage(out);
        return args.isEmpty() ? 1 : 0;
    }
    if (args.contains(QStringLiteral("--version"))) {
        out << "mne_mna_bids_converter " << MNE_CPP_VERSION << "\n";
        return 0;
    }

    const QString cmd = args.takeFirst();

    // Parse optional flags
    bool forceEmbed = args.removeAll(QStringLiteral("--embed")) > 0;
    bool forceNoEmbed = args.removeAll(QStringLiteral("--no-embed")) > 0;

    if (cmd == QLatin1String("extract")) {
        if (args.size() < 2) {
            err << "Error: extract requires <input> <output_dir>\n";
            return 1;
        }
        return cmdExtract(args[0], args[1]);
    }

    if (cmd == QLatin1String("pack")) {
        if (args.size() < 2) {
            err << "Error: pack requires <bids_dir> <output.mnx|mna>\n";
            return 1;
        }
        bool embed;
        if (forceEmbed)
            embed = true;
        else if (forceNoEmbed)
            embed = false;
        else
            embed = args[1].endsWith(QLatin1String(".mnx"), Qt::CaseInsensitive);
        return cmdPack(args[0], args[1], embed);
    }

    if (cmd == QLatin1String("convert")) {
        if (args.size() < 2) {
            err << "Error: convert requires <input> <output>\n";
            return 1;
        }
        return cmdConvert(args[0], args[1]);
    }

    err << "Error: unknown command '" << cmd << "'\n\n";
    printUsage(err);
    return 1;
}
