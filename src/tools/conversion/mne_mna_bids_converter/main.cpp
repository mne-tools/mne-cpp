//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    MNA/MNX ↔ BIDS converter.
 *
 *  Commands:
 *    extract   Extract embedded files from an .mnx to a BIDS directory tree.
 *    pack      Import a BIDS directory tree into an .mna or .mnx project.
 *    convert   Convert between .mna (JSON) and .mnx (CBOR) formats.
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

#include <base/byte_stream.h>
#include <base/string_utils.h>
#include <base/log.h>

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
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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
static QString bidsBuildRelPath(const QString& subjId, const QString& sessId,
                                MnaFileRole role, const QString& fileName)
{
    return QStringLiteral("sub-%1/ses-%2/%3/%4")
        .arg(subjId, sessId, bidsSubdirForRole(role), QFileInfo(fileName).fileName());
}

/**
 * Guess MnaFileRole from a file name.
 */
static MnaFileRole guessRoleFromFileName(const QString& fileName)
{
    const QString fn = fileName.toLower();

    // Surface files
    if (fn.endsWith(QLatin1String(".pial")) || fn.endsWith(QLatin1String(".inflated")) || fn.endsWith(QLatin1String(".white")) || fn.endsWith(QLatin1String(".orig")) || fn.endsWith(QLatin1String(".sphere")))
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
    if (fn.contains(QLatin1String("-src")) || fn.contains(QLatin1String("_src")) || fn.contains(QLatin1String("-oct-")))
        return MnaFileRole::SourceSpace;
    if (fn.contains(QLatin1String("-trans")) || fn.contains(QLatin1String("_trans")) || fn.contains(QLatin1String("all-trans")))
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
static MnaFileRole guessRoleFromBidsDir(const QString& dirName)
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

/** @return @p path as a std::filesystem path. */
static std::filesystem::path fsPath(const QString& path)
{
    return path_from_utf8(path.toStdString());
}

/** @return The project at @p path, or nothing after reporting why it cannot be read. */
static std::optional<MnaProject> readProject(const QString& path)
{
    Expected<MnaProject> project = MnaIO::read(fsPath(path));
    if (!project) {
        qCritical().noquote().nospace() << "failed to read project: " << project.error().what();
        return std::nullopt;
    }
    return std::move(*project);
}

/** Writes @p project to @p path and reports the outcome; returns the exit code. */
static int writeProject(const MnaProject& project, const QString& path, const QString& done)
{
    if (const Expected<void> written = MnaIO::write(project, fsPath(path)); !written) {
        qCritical().noquote().nospace() << "failed to write " << written.error().what();
        return 1;
    }
    qInfo().noquote().nospace() << done;
    return 0;
}

static UtcTime now()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

static int cmdExtract(const QString& inputPath, const QString& outputDir)
{
    std::optional<MnaProject> read = readProject(inputPath);
    if (!read || read->subjects.empty()) {
        qCritical().noquote().nospace() << "empty project: " << inputPath;
        return 1;
    }
    const MnaProject& proj = *read;

    int extracted = 0;
    int skipped = 0;

    for (const MnaSubject& subj : proj.subjects) {
        for (const MnaSession& sess : subj.sessions) {
            for (const MnaRecording& rec : sess.recordings) {
                for (const MnaFileRef& ref : rec.files) {
                    // Build BIDS output path
                    const QString refPath = QString::fromStdString(ref.path);
                    QString relPath = refPath;
                    if (relPath.isEmpty())
                        relPath = bidsBuildRelPath(QString::fromStdString(subj.id), QString::fromStdString(sess.id), ref.role,
                                                   QStringLiteral("file_") + QString::number(extracted));

                    const QString fullPath = QDir(outputDir).filePath(relPath);

                    if (ref.embedded && !ref.data.empty()) {
                        QDir().mkpath(QFileInfo(fullPath).absolutePath());
                        if (write_file_bytes(fsPath(fullPath), ref.data)) {
                            qInfo().noquote().nospace() << "  extracted: " << relPath;
                            ++extracted;
                        } else {
                            qCritical().noquote().nospace() << "Failed: " << relPath;
                        }
                    } else if (!ref.embedded) {
                        // Copy from source location relative to the project file
                        const QString srcDir = QFileInfo(inputPath).absolutePath();
                        // Resolve ".." lexically, as written; the OS would follow symlinks out of the project folder
                        const QString srcPath = QDir::cleanPath(QDir(srcDir).filePath(refPath));
                        if (QFile::exists(srcPath)) {
                            QDir().mkpath(QFileInfo(fullPath).absolutePath());
                            if (QFile::copy(srcPath, fullPath)) {
                                qInfo().noquote().nospace() << "  copied:    " << relPath;
                                ++extracted;
                            } else {
                                qCritical().noquote().nospace() << "Failed: " << relPath << " (copy error)";
                            }
                        } else {
                            qWarning().noquote().nospace() << "Skipped: " << refPath << " (source not found)";
                            ++skipped;
                        }
                    } else {
                        qWarning().noquote().nospace() << "Skipped: " << refPath << " (empty embedded data)";
                        ++skipped;
                    }
                }
            }
        }
    }

    // Write a sidecar .mna with non-embedded references pointing into BIDS tree
    MnaProject outProj = proj;
    for (MnaSubject& subj : outProj.subjects) {
        for (MnaSession& sess : subj.sessions) {
            for (MnaRecording& rec : sess.recordings) {
                for (MnaFileRef& ref : rec.files) {
                    ref.embedded = false;
                    ref.data.clear();
                }
            }
        }
    }
    outProj.modified = now();
    const QString sidecarPath = QDir(outputDir).filePath(QStringLiteral("project.mna"));
    if (writeProject(outProj, sidecarPath, QStringLiteral("  Sidecar written: ") + sidecarPath) != 0) {
        return 1;
    }

    qInfo().noquote().nospace() << "  Done. Extracted: " << extracted << "  Skipped: " << skipped;
    return 0;
}

//=============================================================================================================
// COMMAND: pack
//=============================================================================================================

static int cmdPack(const QString& inputDir, const QString& outputPath, bool embed)
{
    // Check if there's an existing project.mna sidecar to use as template
    const QString sidecarPath = QDir(inputDir).filePath(QStringLiteral("project.mna"));
    MnaProject proj;

    if (QFile::exists(sidecarPath)) {
        std::optional<MnaProject> sidecar = readProject(sidecarPath);
        if (!sidecar) {
            return 1;
        }
        proj = std::move(*sidecar);
        qInfo().noquote().nospace() << "  Using existing sidecar: " << sidecarPath;

        if (embed) {
            // Read file data into each ref
            for (MnaSubject& subj : proj.subjects) {
                for (MnaSession& sess : subj.sessions) {
                    for (MnaRecording& rec : sess.recordings) {
                        for (MnaFileRef& ref : rec.files) {
                            const QString filePath = QDir(inputDir).filePath(QString::fromStdString(ref.path));
                            if (!ref.embed(fsPath(filePath))) {
                                qWarning().noquote().nospace() << "cannot read " << filePath;
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Scan the directory for BIDS structure: sub-*/ses-*/<modality>/*
        qInfo().noquote().nospace() << "  Scanning BIDS directory: " << inputDir;

        proj.name = "BIDS Import";
        proj.description = "Imported from BIDS directory";

        // Find sub-* directories
        QDir rootDir(inputDir);
        QStringList subDirs = rootDir.entryList(QStringList() << QStringLiteral("sub-*"),
                                                QDir::Dirs | QDir::NoDotAndDotDot);

        if (subDirs.isEmpty()) {
            // Flat structure — treat all files as belonging to one subject
            subDirs << QStringLiteral(".");
        }

        for (const QString& subDirName : subDirs) {
            MnaSubject subj;
            subj.id = (subDirName.startsWith(QLatin1String("sub-")) ? subDirName.mid(4) : subDirName).toStdString();

            QDir subDir(rootDir.filePath(subDirName));
            QStringList sesDirs = subDir.entryList(QStringList() << QStringLiteral("ses-*"),
                                                   QDir::Dirs | QDir::NoDotAndDotDot);
            if (sesDirs.isEmpty())
                sesDirs << QStringLiteral(".");

            for (const QString& sesDirName : sesDirs) {
                MnaSession sess;
                sess.id = sesDirName.startsWith(QLatin1String("ses-")) ? sesDirName.mid(4).toStdString() : std::string("01");

                MnaRecording rec;
                rec.id = "recording-01";

                QDir sesDir(subDir.filePath(sesDirName));
                // Iterate modality dirs: anat, meg, bem, source, etc.
                QStringList modDirs = sesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (modDirs.isEmpty())
                    modDirs << QStringLiteral(".");

                for (const QString& modDirName : modDirs) {
                    QDir modDir(sesDir.filePath(modDirName));
                    MnaFileRole defaultRole = guessRoleFromBidsDir(modDirName);

                    QStringList files = modDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
                    for (const QString& fileName : files) {
                        MnaFileRef ref;
                        // Prefer file-name heuristic; fall back to directory-based guess
                        ref.role = guessRoleFromFileName(fileName);
                        if (ref.role == MnaFileRole::Custom && defaultRole != MnaFileRole::Custom)
                            ref.role = defaultRole;

                        const QString absPath = modDir.filePath(fileName);
                        ref.path = rootDir.relativeFilePath(absPath).toStdString();
                        ref.format = QFileInfo(fileName).suffix().toStdString();

                        if (embed) {
                            if (!ref.embed(fsPath(absPath))) {
                                qWarning().noquote().nospace() << "cannot read " << absPath;
                            }
                        } else {
                            ref.embedded = false;
                            ref.size_bytes = QFileInfo(absPath).size();
                        }

                        qInfo().noquote().nospace() << "  + " << QString::fromUtf8(to_string(ref.role)) << "  " << QString::fromStdString(ref.path);
                        rec.files.push_back(std::move(ref));
                    }
                }

                if (!rec.files.empty()) {
                    sess.recordings.push_back(std::move(rec));
                }
            }

            if (!subj.sessions.empty()) {
                proj.subjects.push_back(std::move(subj));
            }
        }
    }

    proj.modified = now();
    return writeProject(proj, outputPath, QStringLiteral("  Written: ") + outputPath);
}

//=============================================================================================================
// COMMAND: convert
//=============================================================================================================

static int cmdConvert(const QString& inputPath, const QString& outputPath)
{
    std::optional<MnaProject> read = readProject(inputPath);
    if (!read) {
        return 1;
    }
    MnaProject& proj = *read;

    const QString outSuffix = QFileInfo(outputPath).suffix().toLower();
    const QString inSuffix = QFileInfo(inputPath).suffix().toLower();

    // When converting .mna → .mnx, embed file data automatically
    if (outSuffix == QLatin1String("mnx") && inSuffix == QLatin1String("mna")) {
        const QString projDir = QFileInfo(inputPath).absolutePath();
        for (MnaSubject& subj : proj.subjects) {
            for (MnaSession& sess : subj.sessions) {
                for (MnaRecording& rec : sess.recordings) {
                    for (MnaFileRef& ref : rec.files) {
                        if (!ref.embedded) {
                            const QString filePath = QDir::cleanPath(QDir(projDir).filePath(QString::fromStdString(ref.path)));
                            if (ref.embed(fsPath(filePath))) {
                                ref.path = QFileInfo(filePath).fileName().toStdString();
                                qInfo().noquote().nospace() << "  embedded: " << filePath;
                            } else {
                                qWarning().noquote().nospace() << "cannot read " << filePath << ", keeping as reference";
                            }
                        }
                    }
                }
            }
        }
    }

    // When converting .mnx → .mna, strip embedded data (files must be extracted first)
    if (outSuffix == QLatin1String("mna") && inSuffix == QLatin1String("mnx")) {
        qInfo().noquote().nospace() << "  Note: embedded data will be dropped. Use 'extract' first to write files to disk.";
        for (MnaSubject& subj : proj.subjects) {
            for (MnaSession& sess : subj.sessions) {
                for (MnaRecording& rec : sess.recordings) {
                    for (MnaFileRef& ref : rec.files) {
                        ref.embedded = false;
                        ref.data.clear();
                    }
                }
            }
        }
    }

    proj.modified = now();
    return writeProject(proj, outputPath, QStringLiteral("  Converted: ") + inputPath + QStringLiteral(" -> ") + outputPath);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

static void printUsage(QTextStream& out)
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

int main(int argc, char* argv[])
{
    mne::Log::install_qt_message_handler();
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
            qCritical().noquote().nospace() << "extract requires <input> <output_dir>";
            return 1;
        }
        return cmdExtract(args[0], args[1]);
    }

    if (cmd == QLatin1String("pack")) {
        if (args.size() < 2) {
            qCritical().noquote().nospace() << "pack requires <bids_dir> <output.mnx|mna>";
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
            qCritical().noquote().nospace() << "convert requires <input> <output>";
            return 1;
        }
        return cmdConvert(args[0], args[1]);
    }

    qCritical().noquote().nospace() << "unknown command '" << cmd << "'";
    printUsage(err);
    return 1;
}
