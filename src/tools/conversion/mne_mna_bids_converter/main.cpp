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

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>
#include <mne/mna/mna_subject.h>
#include <mne/mna/mna_session.h>
#include <mne/mna/mna_recording.h>
#include <mne/mna/mna_file_ref.h>
#include <mne/mna/mna_types.h>

#include <mne/base/byte_stream.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// PATH HELPERS
//=============================================================================================================

/** @return @p path as a std::filesystem path. */
static std::filesystem::path fsPath(const std::string& path)
{
    return path_from_utf8(path);
}

/**
 * @return @p name inside the directory @p dir, joined with '/'; @p name itself if it is absolute.
 */
static std::string dirFilePath(std::string dir, const std::string& name)
{
    if (fsPath(name).is_absolute())
        return name;
    if (dir.empty())
        dir = ".";
    else if (dir.size() > 1 && dir.ends_with('/'))
        dir.pop_back();
    if (name.empty())
        return dir;
    return dir.ends_with('/') ? dir + name : dir + "/" + name;
}

/** @return The final component of @p path. */
static std::string fileNameOf(const std::string& path)
{
    return utf8_from_path(fsPath(path).filename());
}

/** @return The text after the last '.' of the file name of @p path, empty if there is none. */
static std::string suffixOf(const std::string& path)
{
    const std::string name = fileNameOf(path);
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? std::string() : name.substr(dot + 1);
}

/** @return The absolute path of the directory that contains @p path. */
static std::string absoluteDirOf(const std::string& path)
{
    std::error_code error;
    const std::filesystem::path absolute = std::filesystem::absolute(fsPath(path), error);
    return utf8_from_path(absolute.lexically_normal().parent_path());
}

/** @return @p path with "." and ".." resolved lexically, as written; no trailing separator. */
static std::string cleanPath(const std::string& path)
{
    std::string clean = utf8_from_path(fsPath(path).lexically_normal());
    if constexpr (std::filesystem::path::preferred_separator != '/')
        std::ranges::replace(clean, static_cast<char>(std::filesystem::path::preferred_separator), '/');
    if (clean.size() > 1 && clean.ends_with('/'))
        clean.pop_back();
    return clean;
}

/**
 * @return @p path relative to the directory @p dir (taken absolute), both cleaned and joined with '/'; a relative
 *         @p path is only cleaned.
 */
static std::string relativeFilePath(const std::string& dir, const std::string& path)
{
    if (!fsPath(path).is_absolute())
        return cleanPath(path);
    std::error_code error;
    const std::filesystem::path base = std::filesystem::absolute(fsPath(dir), error).lexically_normal();
    const std::filesystem::path target = std::filesystem::absolute(fsPath(path), error).lexically_normal();
    return cleanPath(utf8_from_path(target.lexically_relative(base)));
}

/** Creates @p dir and its parents. */
static void makePath(const std::string& dir)
{
    std::error_code error;
    std::filesystem::create_directories(fsPath(dir), error);
}

/**
 * @return The non-hidden subdirectories (or files) of @p dir whose names start with @p prefix ignoring case, in
 *         case-insensitive name order.
 */
static StringList entryList(const std::string& dir, bool directories, std::string_view prefix = {})
{
    StringList names;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(fsPath(dir), error)) {
        const std::string name = utf8_from_path(entry.path().filename());
        if (name.starts_with('.') || !to_lower(name).starts_with(to_lower(prefix)))
            continue;
        std::error_code typeError;
        if (directories ? entry.is_directory(typeError) : entry.is_regular_file(typeError))
            names.push_back(name);
    }
    std::ranges::sort(names, [](const std::string& a, const std::string& b) {
        const std::string lowerA = to_lower(a);
        const std::string lowerB = to_lower(b);
        return lowerA != lowerB ? lowerA < lowerB : a < b;
    });
    return names;
}

/** @return Whether @p path exists. */
static bool pathExists(const std::string& path)
{
    std::error_code error;
    return std::filesystem::exists(fsPath(path), error);
}

//=============================================================================================================
// BIDS HELPERS
//=============================================================================================================

/**
 * Map an MnaFileRole to a BIDS-like subdirectory.
 */
static std::string bidsSubdirForRole(MnaFileRole role)
{
    switch (role) {
        case MnaFileRole::Surface:
        case MnaFileRole::Annotation:
            return "anat";
        case MnaFileRole::Bem:
        case MnaFileRole::SourceSpace:
            return "bem";
        case MnaFileRole::Digitizer:
        case MnaFileRole::Transform:
        case MnaFileRole::Evoked:
            return "meg";
        case MnaFileRole::SourceEstimate:
            return "source";
        default:
            return "other";
    }
}

/**
 * Build a BIDS-like relative path: sub-<subj>/ses-<sess>/<modality>/<filename>
 */
static std::string bidsBuildRelPath(const std::string& subjId, const std::string& sessId,
                                    MnaFileRole role, const std::string& fileName)
{
    return "sub-" + subjId + "/ses-" + sessId + "/" + bidsSubdirForRole(role) + "/" + fileNameOf(fileName);
}

/**
 * Guess MnaFileRole from a file name.
 */
static MnaFileRole guessRoleFromFileName(const std::string& fileName)
{
    const std::string fn = to_lower(fileName);

    // Surface files
    if (fn.ends_with(".pial") || fn.ends_with(".inflated") || fn.ends_with(".white") || fn.ends_with(".orig") || fn.ends_with(".sphere"))
        return MnaFileRole::Surface;

    // Annotations
    if (fn.ends_with(".annot"))
        return MnaFileRole::Annotation;

    // Source estimates
    if (fn.ends_with(".stc") || fn.ends_with(".w"))
        return MnaFileRole::SourceEstimate;

    // FIFF-based files — use name heuristics
    if (fn.contains("-bem") || fn.contains("_bem"))
        return MnaFileRole::Bem;
    if (fn.contains("-src") || fn.contains("_src") || fn.contains("-oct-"))
        return MnaFileRole::SourceSpace;
    if (fn.contains("-trans") || fn.contains("_trans") || fn.contains("all-trans"))
        return MnaFileRole::Transform;
    if (fn.contains("-ave") || fn.contains("_ave"))
        return MnaFileRole::Evoked;
    if (fn.contains("-fwd") || fn.contains("_fwd"))
        return MnaFileRole::Forward;
    if (fn.contains("-inv") || fn.contains("_inv"))
        return MnaFileRole::Inverse;
    if (fn.contains("-cov") || fn.contains("_cov"))
        return MnaFileRole::Covariance;
    if (fn.contains("_raw") || fn.contains("-raw"))
        return MnaFileRole::Raw;
    if (fn.contains("_dig") || fn.contains("digitizer"))
        return MnaFileRole::Digitizer;

    return MnaFileRole::Custom;
}

/**
 * Infer BIDS modality directory from a subdirectory name.
 */
static MnaFileRole guessRoleFromBidsDir(const std::string& dirName)
{
    if (dirName == "anat")
        return MnaFileRole::Surface; // default; file name will disambiguate
    if (dirName == "bem")
        return MnaFileRole::Bem;
    if (dirName == "meg" || dirName == "eeg")
        return MnaFileRole::Raw;
    if (dirName == "source")
        return MnaFileRole::SourceEstimate;
    return MnaFileRole::Custom;
}

//=============================================================================================================
// COMMAND: extract
//=============================================================================================================

/** @return The project at @p path, or nothing after reporting why it cannot be read. */
static std::optional<MnaProject> readProject(const std::string& path)
{
    Expected<MnaProject> project = MnaIO::read(fsPath(path));
    if (!project) {
        Log::critical("failed to read project: {}", project.error().what());
        return std::nullopt;
    }
    return std::move(*project);
}

/** Writes @p project to @p path and reports the outcome; returns the exit code. */
static int writeProject(const MnaProject& project, const std::string& path, const std::string& done)
{
    if (const Expected<void> written = MnaIO::write(project, fsPath(path)); !written) {
        Log::critical("failed to write {}", written.error().what());
        return 1;
    }
    Log::info("{}", done);
    return 0;
}

static UtcTime now()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

static int cmdExtract(const std::string& inputPath, const std::string& outputDir)
{
    std::optional<MnaProject> read = readProject(inputPath);
    if (!read || read->subjects.empty()) {
        Log::critical("empty project: {}", inputPath);
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
                    const std::string& refPath = ref.path;
                    std::string relPath = refPath;
                    if (relPath.empty())
                        relPath = bidsBuildRelPath(subj.id, sess.id, ref.role, "file_" + std::to_string(extracted));

                    const std::string fullPath = dirFilePath(outputDir, relPath);

                    if (ref.embedded && !ref.data.empty()) {
                        makePath(absoluteDirOf(fullPath));
                        if (write_file_bytes(fsPath(fullPath), ref.data)) {
                            Log::info("  extracted: {}", relPath);
                            ++extracted;
                        } else {
                            Log::critical("Failed: {}", relPath);
                        }
                    } else if (!ref.embedded) {
                        // Copy from source location relative to the project file
                        const std::string srcDir = absoluteDirOf(inputPath);
                        // Resolve ".." lexically, as written; the OS would follow symlinks out of the project folder
                        const std::string srcPath = cleanPath(dirFilePath(srcDir, refPath));
                        if (pathExists(srcPath)) {
                            makePath(absoluteDirOf(fullPath));
                            std::error_code copyError;
                            if (std::filesystem::copy_file(fsPath(srcPath), fsPath(fullPath), copyError)) {
                                Log::info("  copied:    {}", relPath);
                                ++extracted;
                            } else {
                                Log::critical("Failed: {} (copy error)", relPath);
                            }
                        } else {
                            Log::warning("Skipped: {} (source not found)", refPath);
                            ++skipped;
                        }
                    } else {
                        Log::warning("Skipped: {} (empty embedded data)", refPath);
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
    const std::string sidecarPath = dirFilePath(outputDir, "project.mna");
    if (writeProject(outProj, sidecarPath, "  Sidecar written: " + sidecarPath) != 0) {
        return 1;
    }

    Log::info("  Done. Extracted: {}  Skipped: {}", extracted, skipped);
    return 0;
}

//=============================================================================================================
// COMMAND: pack
//=============================================================================================================

static int cmdPack(const std::string& inputDir, const std::string& outputPath, bool embed)
{
    // Check if there's an existing project.mna sidecar to use as template
    const std::string sidecarPath = dirFilePath(inputDir, "project.mna");
    MnaProject proj;

    if (pathExists(sidecarPath)) {
        std::optional<MnaProject> sidecar = readProject(sidecarPath);
        if (!sidecar) {
            return 1;
        }
        proj = std::move(*sidecar);
        Log::info("  Using existing sidecar: {}", sidecarPath);

        if (embed) {
            // Read file data into each ref
            for (MnaSubject& subj : proj.subjects) {
                for (MnaSession& sess : subj.sessions) {
                    for (MnaRecording& rec : sess.recordings) {
                        for (MnaFileRef& ref : rec.files) {
                            const std::string filePath = dirFilePath(inputDir, ref.path);
                            if (!ref.embed(fsPath(filePath))) {
                                Log::warning("cannot read {}", filePath);
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Scan the directory for BIDS structure: sub-*/ses-*/<modality>/*
        Log::info("  Scanning BIDS directory: {}", inputDir);

        proj.name = "BIDS Import";
        proj.description = "Imported from BIDS directory";

        // Find sub-* directories
        const std::string& rootDir = inputDir;
        StringList subDirs = entryList(rootDir, true, "sub-");

        if (subDirs.empty()) {
            // Flat structure — treat all files as belonging to one subject
            subDirs.push_back(".");
        }

        for (const std::string& subDirName : subDirs) {
            MnaSubject subj;
            subj.id = subDirName.starts_with("sub-") ? subDirName.substr(4) : subDirName;

            const std::string subDir = dirFilePath(rootDir, subDirName);
            StringList sesDirs = entryList(subDir, true, "ses-");
            if (sesDirs.empty())
                sesDirs.push_back(".");

            for (const std::string& sesDirName : sesDirs) {
                MnaSession sess;
                sess.id = sesDirName.starts_with("ses-") ? sesDirName.substr(4) : std::string("01");

                MnaRecording rec;
                rec.id = "recording-01";

                const std::string sesDir = dirFilePath(subDir, sesDirName);
                // Iterate modality dirs: anat, meg, bem, source, etc.
                StringList modDirs = entryList(sesDir, true);
                if (modDirs.empty())
                    modDirs.push_back(".");

                for (const std::string& modDirName : modDirs) {
                    const std::string modDir = dirFilePath(sesDir, modDirName);
                    MnaFileRole defaultRole = guessRoleFromBidsDir(modDirName);

                    const StringList files = entryList(modDir, false);
                    for (const std::string& fileName : files) {
                        MnaFileRef ref;
                        // Prefer file-name heuristic; fall back to directory-based guess
                        ref.role = guessRoleFromFileName(fileName);
                        if (ref.role == MnaFileRole::Custom && defaultRole != MnaFileRole::Custom)
                            ref.role = defaultRole;

                        const std::string absPath = dirFilePath(modDir, fileName);
                        ref.path = relativeFilePath(rootDir, absPath);
                        ref.format = suffixOf(fileName);

                        if (embed) {
                            if (!ref.embed(fsPath(absPath))) {
                                Log::warning("cannot read {}", absPath);
                            }
                        } else {
                            ref.embedded = false;
                            std::error_code sizeError;
                            const std::uintmax_t size = std::filesystem::file_size(fsPath(absPath), sizeError);
                            ref.size_bytes = sizeError ? 0 : static_cast<decltype(ref.size_bytes)>(size);
                        }

                        Log::info("  + {}  {}", to_string(ref.role), ref.path);
                        rec.files.push_back(std::move(ref));
                    }
                }

                if (!rec.files.empty()) {
                    sess.recordings.push_back(std::move(rec));
                }
                if (!sess.recordings.empty()) {
                    subj.sessions.push_back(std::move(sess));
                }
            }

            if (!subj.sessions.empty()) {
                proj.subjects.push_back(std::move(subj));
            }
        }
    }

    proj.modified = now();
    return writeProject(proj, outputPath, "  Written: " + outputPath);
}

//=============================================================================================================
// COMMAND: convert
//=============================================================================================================

static int cmdConvert(const std::string& inputPath, const std::string& outputPath)
{
    std::optional<MnaProject> read = readProject(inputPath);
    if (!read) {
        return 1;
    }
    MnaProject& proj = *read;

    const std::string outSuffix = to_lower(suffixOf(outputPath));
    const std::string inSuffix = to_lower(suffixOf(inputPath));

    // When converting .mna → .mnx, embed file data automatically
    if (outSuffix == "mnx" && inSuffix == "mna") {
        const std::string projDir = absoluteDirOf(inputPath);
        for (MnaSubject& subj : proj.subjects) {
            for (MnaSession& sess : subj.sessions) {
                for (MnaRecording& rec : sess.recordings) {
                    for (MnaFileRef& ref : rec.files) {
                        if (!ref.embedded) {
                            const std::string filePath = cleanPath(dirFilePath(projDir, ref.path));
                            if (ref.embed(fsPath(filePath))) {
                                ref.path = fileNameOf(filePath);
                                Log::info("  embedded: {}", filePath);
                            } else {
                                Log::warning("cannot read {}, keeping as reference", filePath);
                            }
                        }
                    }
                }
            }
        }
    }

    // When converting .mnx → .mna, strip embedded data (files must be extracted first)
    if (outSuffix == "mna" && inSuffix == "mnx") {
        Log::info("  Note: embedded data will be dropped. Use 'extract' first to write files to disk.");
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
    return writeProject(proj, outputPath, "  Converted: " + inputPath + " -> " + outputPath);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

static void printUsage(std::FILE* out)
{
    std::print(out,
               "Usage: mne_mna_bids_converter <command> [options]\n\n"
               "Commands:\n"
               "  extract  <input.mnx|mna> <output_dir>    Extract files to BIDS directory tree\n"
               "  pack     <bids_dir> <output.mnx|mna>     Import BIDS directory into MNA/MNX project\n"
               "  convert  <input.mna|mnx> <output.mnx|mna>  Convert between MNA (JSON) and MNX (CBOR)\n"
               "\nOptions:\n"
               "  --embed     (pack) Embed file data into the output (default for .mnx)\n"
               "  --no-embed  (pack) Store file references only (default for .mna)\n"
               "  -h, --help  Show this help\n"
               "  --version   Show version\n");
}

int main(int argc, char* argv[])
{
    StringList args(argv + std::min(argc, 1), argv + argc); // without the program name

    const auto contains = [&args](std::string_view arg) {
        return std::ranges::find(args, arg) != args.end();
    };

    // Check for help/version flags
    if (args.empty() || contains("-h") || contains("--help")) {
        printUsage(stdout);
        return args.empty() ? 1 : 0;
    }
    if (contains("--version")) {
        std::println("mne_mna_bids_converter {}", MNE_CPP_VERSION);
        return 0;
    }

    const std::string cmd = args.front();
    args.erase(args.begin());

    // Parse optional flags
    bool forceEmbed = std::erase(args, "--embed") > 0;
    bool forceNoEmbed = std::erase(args, "--no-embed") > 0;

    if (cmd == "extract") {
        if (args.size() < 2) {
            Log::critical("extract requires <input> <output_dir>");
            return 1;
        }
        return cmdExtract(args[0], args[1]);
    }

    if (cmd == "pack") {
        if (args.size() < 2) {
            Log::critical("pack requires <bids_dir> <output.mnx|mna>");
            return 1;
        }
        bool embed;
        if (forceEmbed)
            embed = true;
        else if (forceNoEmbed)
            embed = false;
        else
            embed = to_lower(args[1]).ends_with(".mnx");
        return cmdPack(args[0], args[1], embed);
    }

    if (cmd == "convert") {
        if (args.size() < 2) {
            Log::critical("convert requires <input> <output>");
            return 1;
        }
        return cmdConvert(args[0], args[1]);
    }

    Log::critical("unknown command '{}'", cmd);
    printUsage(stderr);
    return 1;
}
