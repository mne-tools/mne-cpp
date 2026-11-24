//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     setupmri.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    SetupMri class definition.
 *           Ported from the original MNE C tools by Matti Hamalainen:
 *             - mne_setup_mri (shell script): directory orchestration
 *             - mne_make_cor_set (C program): COR/MGZ to COR.fif conversion
 *
 *           Cross-referenced with MNE-Python, which never ported this tool.
 *           In modern MNE-Python, .mgz files are read directly via nibabel,
 *           making the COR.fif conversion unnecessary. However, this tool
 *           maintains compatibility with the original MNE C workflow.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "setupmri.h"
#include "mne_setup_mri_settings.h"

#include <mne/mri/mri_mgh_io.h>
#include <mne/mri/mri_cor_io.h>
#include <mne/mri/mri_cor_fif_io.h>
#include <mne/mri/mri_vol_data.h>
#include <mne/mri/mri_types.h>

#include <mne/fiff/fiff_coord_trans.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;
using namespace mne;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{

bool fileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path_from_utf8(path), ec);
}

bool dirExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path_from_utf8(path), ec);
}

bool makePath(const std::string& path)
{
    std::error_code ec;
    std::filesystem::create_directories(path_from_utf8(path), ec);
    return std::filesystem::is_directory(path_from_utf8(path), ec);
}

void removeFile(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(path_from_utf8(path), ec))
        std::filesystem::remove(path_from_utf8(path), ec);
}

// Matches a QDir name filter: '*' and '?' wildcards, ASCII case-insensitive
bool wildcardMatch(std::string_view pattern, std::string_view name)
{
    std::size_t p = 0, n = 0, star = std::string_view::npos, mark = 0;
    const auto same = [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    };
    while (n < name.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || same(pattern[p], name[n]))) {
            ++p;
            ++n;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = n;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            n = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

// The names of the visible files in a directory that match any of the patterns (all if none), sorted by name
StringList listFiles(const std::string& dir, const StringList& patterns = {})
{
    StringList names;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(path_from_utf8(dir), ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec))
            continue;
        const std::string name = utf8_from_path(it->path().filename());
        if (name.starts_with('.'))
            continue;
        if (!patterns.empty() && std::ranges::none_of(patterns, [&](const std::string& pattern) { return wildcardMatch(pattern, name); }))
            continue;
        names.push_back(name);
    }
    std::ranges::sort(names);
    return names;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SetupMri::SetupMri(const MNESetupMriSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int SetupMri::run()
{
    // Validate settings
    if (m_settings.subjectsDir().empty()) {
        Log::critical("The subjects directory (SUBJECTS_DIR) is not set.\nUse --subjects-dir or set the SUBJECTS_DIR environment variable.");
        return 1;
    }
    if (m_settings.subject().empty()) {
        Log::critical("The subject name is not set.\nUse --subject or set the SUBJECT environment variable.");
        return 1;
    }

    std::string subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    std::string mriDir = subjectDir + "/mri";

    if (!dirExists(subjectDir)) {
        Log::critical("Could not find the MRI data directory {}", subjectDir);
        return 1;
    }
    if (!dirExists(mriDir)) {
        Log::critical("Could not find the MRI directory {}", mriDir);
        return 1;
    }

    // Process each MRI set
    const StringList mriSets = m_settings.mriSets();
    for (const std::string& mriName : mriSets) {
        Log::info("-----------------------------------------------------------------------------------------------");
        Log::info("Setting up {}...", mriName);

        if (!processMriSet(mriName)) {
            return 1;
        }
    }

    Log::info("-----------------------------------------------------------------------------------------------");
    Log::info("\nComplete.\n");
    return 0;
}

//=============================================================================================================

bool SetupMri::processMriSet(const std::string& mriName)
{
    std::string mriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

    // Determine paths, handling absolute paths with '/'
    // Ported from the shell script logic: if mri name contains '/', use as-is
    std::string thisDir;
    std::string thisMgz;
    std::string neuromagDir;

    if (mriName.find('/') != std::string::npos) {
        std::error_code ec;
        const std::filesystem::path absolute = std::filesystem::absolute(path_from_utf8(mriName), ec).lexically_normal();
        thisDir = utf8_from_path(absolute);
        thisMgz = thisDir + ".mgz";
        neuromagDir = utf8_from_path(absolute.parent_path()) + "/" + utf8_from_path(absolute.filename()) + "-neuromag";
    } else {
        thisDir = mriDir + "/" + mriName;
        thisMgz = mriDir + "/" + mriName + ".mgz";
        neuromagDir = mriDir + "/" + mriName + "-neuromag";
    }

    // Check that MRI data exists (either directory or .mgz file)
    bool mgzExists = fileExists(thisMgz);
    if (!dirExists(thisDir) && !mgzExists) {
        // Also try .mgh extension
        std::string thisMgh = thisMgz;
        for (std::size_t pos = thisMgh.find(".mgz"); pos != std::string::npos; pos = thisMgh.find(".mgz", pos + 4))
            thisMgh.replace(pos, 4, ".mgh");
        if (fileExists(thisMgh)) {
            thisMgz = thisMgh;
            mgzExists = true;
        } else {
            Log::critical("Could not find the MRI data for {}", mriName);
            Log::critical("  Looked for directory: {}", thisDir);
            Log::critical("  Looked for file: {}", thisMgz);
            return false;
        }
    }

    // Check for existing output
    if (dirExists(neuromagDir) && !m_settings.overwrite()) {
        Log::critical("Destination directory {} already exists.", neuromagDir);
        Log::critical("Use --overwrite option to overwrite existing data.");
        return false;
    }

    // Create directory structure
    // Ported from shell script: mkdir -p $neuromag_dir/sets, $neuromag_dir/slices
    std::string setsDir = neuromagDir + "/sets";
    std::string slicesDir = neuromagDir + "/slices";

    if (!makePath(setsDir)) {
        Log::critical("Could not create sets directory {}", setsDir);
        return false;
    }
    if (!makePath(slicesDir)) {
        Log::critical("Could not create slices directory {}", slicesDir);
        return false;
    }

    // Read MRI data and create COR.fif
    std::vector<MriSlice> slices;
    std::vector<FiffCoordTrans> additionalTrans;

    if (mgzExists) {
        // MGZ/MGH path — use mne::MriMghIO
        Log::info("Reading {}...", thisMgz);

        MriVolData volData;
        std::string subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

        if (!MriMghIO::read(thisMgz, volData, additionalTrans, subjectMriDir, m_settings.verbose())) {
            Log::critical("Failed to read MGZ/MGH file {}", thisMgz);
            return false;
        }
        slices = volData.slices;
    } else {
        // COR slices path — use mne::MriCorIO
        Log::info("Reading COR files from {}...", thisDir);

        // Create symbolic links (like the shell script does)
        const StringList corFiles = listFiles(thisDir, {"COR-*"});
        for (const std::string& corFile : corFiles) {
            std::string src = std::format("{}/{}", thisDir, corFile);
            std::string dst = std::format("{}/{}", slicesDir, corFile);
            removeFile(dst);
            std::error_code ec;
            std::filesystem::create_symlink(path_from_utf8(src), path_from_utf8(dst), ec);
            if (ec) {
                Log::warning("Could not create symbolic link: {} -> {}", dst, src);
            }
        }

        if (!MriCorIO::read(thisDir, slices, m_settings.verbose())) {
            Log::critical("Failed to read COR slices from {}", thisDir);
            return false;
        }
    }

    // Write COR.fif — use mne::MriCorFifIO
    std::string corFifPath = setsDir + "/COR.fif";
    removeFile(corFifPath);

    Log::info("Creating {}...", corFifPath);
    if (!MriCorFifIO::write(corFifPath, slices, additionalTrans)) {
        Log::critical("Failed to write {}", corFifPath);
        return false;
    }
    Log::info("Created {}", corFifPath);

    return true;
}
