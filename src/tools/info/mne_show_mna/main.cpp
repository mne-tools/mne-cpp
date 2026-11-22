//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Implements the mne_show_mna CLI tool for inspecting .mna/.mnx files.
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
#include <base/json.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <format>
#include <print>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// HELPERS
//=============================================================================================================

static std::string humanSize(std::uintmax_t bytes)
{
    const double b = static_cast<double>(bytes);
    if (bytes < 1024)
        return std::format("{} B", bytes);
    if (bytes < 1024 * 1024)
        return std::format("{:.1f} KB", b / 1024.0);
    if (bytes < 1024 * 1024 * 1024)
        return std::format("{:.1f} MB", b / (1024.0 * 1024.0));
    return std::format("{:.2f} GB", b / (1024.0 * 1024.0 * 1024.0));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
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
        Log::critical("Error: no input file specified.");
        parser.showHelp(1);
    }

    const std::filesystem::path filePath = path_from_utf8(args.first().toStdString());
    const Expected<MnaProject> read = MnaIO::read(filePath);
    if (!read) {
        std::println(stderr, "Error: {}", read.error().what());
        return 1;
    }
    const MnaProject& proj = *read;

    // ── JSON dump mode ──────────────────────────────────────────────────
    if (parser.isSet(jsonOpt)) {
        std::print("{}", JsonDocument(proj.to_json()).to_json(JsonDocument::Indented));
        return 0;
    }

    // ── Human-readable display ──────────────────────────────────────────
    const bool verbose = parser.isSet(verboseOpt);
    std::error_code ec;

    std::println("═══════════════════════════════════════════════════════════════");
    std::println(" MNA Project: {}", proj.name);
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("  File:        {} ({})", utf8_from_path(filePath.filename()), humanSize(std::filesystem::file_size(filePath, ec)));
    std::println("  Format:      {}", to_lower(utf8_from_path(filePath.extension())) == ".mnx" ? "MNX (CBOR)" : "MNA (JSON)");
    std::println("  MNA Version: {}", proj.mna_version);
    if (!proj.description.empty())
        std::println("  Description: {}", proj.description);
    if (proj.created)
        std::println("  Created:     {}", format_iso8601(*proj.created));
    if (proj.modified)
        std::println("  Modified:    {}", format_iso8601(*proj.modified));
    std::println("");

    // ── Subjects / Sessions / Recordings / Files ────────────────────────
    int totalFiles = 0;
    std::int64_t totalEmbeddedSize = 0;
    int embeddedCount = 0;

    if (!proj.subjects.empty()) {
        std::println("  Subjects: {}", proj.subjects.size());
        std::println("  ───────────────────────────────────────────────────────────");
        for (std::size_t si = 0; si < proj.subjects.size(); ++si) {
            const MnaSubject& subj = proj.subjects[si];
            std::println("  [Subject {}] {}", si + 1, subj.id);
            for (std::size_t sei = 0; sei < subj.sessions.size(); ++sei) {
                const MnaSession& sess = subj.sessions[sei];
                std::println("    [Session {}] {}", sei + 1, sess.id);
                for (std::size_t ri = 0; ri < sess.recordings.size(); ++ri) {
                    const MnaRecording& rec = sess.recordings[ri];
                    std::println("      [Recording {}] {} ─ {} file(s)", ri + 1, rec.id, rec.files.size());
                    for (std::size_t f = 0; f < rec.files.size(); ++f) {
                        const MnaFileRef& ref = rec.files[f];
                        ++totalFiles;
                        if (ref.embedded) {
                            embeddedCount++;
                            totalEmbeddedSize += ref.size_bytes;
                        }
                        std::println("        {}. {}  {}{}", f + 1, to_string(ref.role), ref.path, ref.embedded ? " [EMBEDDED]" : "");
                        if (verbose) {
                            std::print("           format: {}  size: {}", ref.format, humanSize(static_cast<std::uintmax_t>(ref.size_bytes)));
                            if (!ref.sha256.empty())
                                std::print("  sha256: {}...", ref.sha256.substr(0, 16));
                            std::println("");
                        }
                    }
                }
            }
        }
        std::println("");
    }

    // ── Pipeline ────────────────────────────────────────────────────────
    if (!proj.pipeline.empty()) {
        std::println("  Pipeline: {} node(s)", proj.pipeline.size());
        std::println("  ───────────────────────────────────────────────────────────");
        for (std::size_t ni = 0; ni < proj.pipeline.size(); ++ni) {
            const MnaNode& node = proj.pipeline[ni];
            std::print("  [{}] {}  op={}", ni + 1, node.id, node.op_type);
            if (!node.tool_version.empty())
                std::print("  v{}", node.tool_version);
            std::println("");
            if (verbose) {
                for (const MnaPort& p : node.inputs)
                    std::println("       IN  {} ({})", p.name, to_string(p.data_kind));
                for (const MnaPort& p : node.outputs)
                    std::println("       OUT {} ({})", p.name, to_string(p.data_kind));
            }
        }
        std::println("");
    }

    // ── Summary ─────────────────────────────────────────────────────────
    std::println("  Summary");
    std::println("  ───────────────────────────────────────────────────────────");
    std::println("  Total files:    {}", totalFiles);
    if (embeddedCount > 0)
        std::println("  Embedded:       {} ({})", embeddedCount, humanSize(static_cast<std::uintmax_t>(totalEmbeddedSize)));
    std::println("  Pipeline nodes: {}", proj.pipeline.size());
    std::println("═══════════════════════════════════════════════════════════════");
    return 0;
}
