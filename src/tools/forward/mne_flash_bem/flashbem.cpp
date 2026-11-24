//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     flashbem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    FlashBem class definition.
 *
 *           Ported from the original MNE shell script mne_flash_bem
 *           by Matti Hamalainen (SVN $Id: mne_flash_bem 3255 2010-11-15 18:34:59Z msh $).
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_flash_bem().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "flashbem.h"
#include "mne_flash_bem_settings.h"

#include <mne/fs/fs_surface.h>

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>

#include <mne/mri/mri_mgh_io.h>
#include <mne/mri/mri_vol_data.h>

#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/base/process.h>
#include <mne/base/byte_stream.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEFLASHBEM;
using namespace mne;
using namespace Eigen;

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

void removeRecursively(const std::string& path)
{
    std::error_code ec;
    std::filesystem::remove_all(path_from_utf8(path), ec);
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

// The local time in the format of QDateTime::toString(), e.g. "Tue Nov 24 08:32:48 2026"
std::string currentDateTimeText()
{
    static constexpr std::array<const char*, 7> days = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static constexpr std::array<const char*, 12> months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return std::format("{} {} {} {:02}:{:02}:{:02} {}",
                       days[static_cast<std::size_t>(local.tm_wday)],
                       months[static_cast<std::size_t>(local.tm_mon)],
                       local.tm_mday,
                       local.tm_hour,
                       local.tm_min,
                       local.tm_sec,
                       local.tm_year + 1900);
}

// Runs a program with its output forwarded to this process's standard output and error
ProcessResult runForwarded(const std::string& program, const StringList& args, ProcessOptions options)
{
    options.on_output = [](ProcessChannel channel, std::string_view data) {
        std::FILE* stream = channel == ProcessChannel::StdOut ? stdout : stderr;
        std::print(stream, "{}", data);
        std::fflush(stream);
    };
    return run_process(path_from_utf8(program), args, options);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FlashBem::FlashBem(const MNEFlashBemSettings& settings)
: m_settings(settings)
, m_step(0)
{
}

//=============================================================================================================

int FlashBem::run()
{
    //
    // Validate environment — ported from mne_flash_bem shell script
    //
    if (m_settings.freeSurferHome().empty()) {
        Log::critical("The FreeSurfer environment needs to be set up for this script.");
        Log::critical("Please set FREESURFER_HOME.");
        return 1;
    }
    if (m_settings.subjectsDir().empty()) {
        Log::critical("SUBJECTS_DIR has not been set.");
        return 1;
    }
    if (m_settings.subject().empty()) {
        Log::critical("SUBJECT has not been set.");
        return 1;
    }

    //
    // Derive paths
    //
    std::string subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    std::string mriDir = subjectDir + "/mri";
    std::string mriFlashDir = mriDir + "/flash";
    std::string paramDir = mriFlashDir + "/parameter_maps";
    std::string bemDir = subjectDir + "/bem";
    std::string flashBemDir = bemDir + "/flash";
    std::string flashDir = m_settings.flashDir();

    const std::string startTime = currentDateTimeText();

    Log::info("{}", "");
    Log::info("Processing the flash MRI data for subject {} to produce", m_settings.subject());
    Log::info("BEM meshes under {}", flashBemDir);
    Log::info("{}", "");

    //
    // Step 1: Convert DICOM images to MGZ format
    //
    if (!makePath(paramDir)) {
        Log::critical("Could not create directory {}", paramDir);
        return 1;
    }

    int echosConverted = 0;
    if (!m_settings.noConvert()) {
        if (!convertImages(flashDir, mriFlashDir, echosConverted)) {
            return 1;
        }
    }

    //
    // Change to flash directory
    //
    if (!dirExists(mriFlashDir)) {
        Log::critical("Could not find directory {}", mriFlashDir);
        return 1;
    }

    //
    // Optional gradient unwarping
    //
    if (!m_settings.unwarp().empty()) {
        if (!unwarpImages(mriFlashDir)) {
            return 1;
        }
    }

    //
    // Clear parameter maps if some data were reconverted
    //
    if (echosConverted > 0) {
        removeRecursively(paramDir);
        makePath(paramDir);
        Log::info("Parameter maps directory cleared");
    }

    //
    // Step 2: Create parameter maps (only if flash30 available)
    //
    if (!m_settings.noFlash30()) {
        if (!createParameterMaps(mriFlashDir, paramDir)) {
            return 1;
        }
    }

    //
    // Step 3: Synthesize or average flash-5 volume
    //
    if (!createFlash5Volume(mriFlashDir, paramDir)) {
        return 1;
    }

    //
    // Step 4: Register flash5 with MPRAGE
    //
    if (!registerWithMprage(paramDir, mriDir)) {
        return 1;
    }

    //
    // Step 5: Convert to COR format
    //
    bool convertedT1 = false;
    bool convertedBrain = false;
    if (!convertToCor(paramDir, mriDir, convertedT1, convertedBrain)) {
        return 1;
    }

    //
    // Step 6: Create BEM surfaces
    //
    if (!createBemSurfaces()) {
        return 1;
    }

    //
    // Step 7: Convert tri files to surf files
    //
    if (!convertTriToSurf(bemDir, paramDir)) {
        return 1;
    }

    //
    // Cleanup
    //
    cleanup(bemDir, mriDir, convertedT1, convertedBrain);

    Log::info("\nThank you for waiting.");
    Log::info("The BEM triangulations for this subject are now available at {}", flashBemDir);
    Log::info("We hope the BEM meshes created will facilitate your MEG and EEG data analyses.");
    Log::info("\nProcessing started at {}", startTime);
    Log::info("Processing finished at {}\n", currentDateTimeText());

    return 0;
}

//=============================================================================================================

bool FlashBem::runCommand(const std::string& program, const StringList& args,
                          const std::string& workDir)
{
    // Print the command being run
    Log::info("Running: {} {}", program, join(args, " "));

    // Set up FreeSurfer environment
    ProcessOptions options;
    options.environment = {{"FREESURFER_HOME", m_settings.freeSurferHome()},
                           {"SUBJECTS_DIR", m_settings.subjectsDir()},
                           {"SUBJECT", m_settings.subject()}};

    if (!workDir.empty()) {
        options.working_dir = path_from_utf8(workDir);
    }

    const ProcessResult result = runForwarded(program, args, options);

    if (!result.started) {
        Log::critical("Failed to start {} : {}", program, result.error);
        return false;
    }

    if (result.crashed || result.timed_out) {
        Log::critical("{} did not finish: {}", program, result.crashed ? "Process crashed" : "Process timed out");
        return false;
    }

    if (result.exit_code != 0) {
        Log::critical("{} failed with exit code {}", program, result.exit_code);
        return false;
    }

    return true;
}

//=============================================================================================================

bool FlashBem::convertImages(const std::string& flashDir, const std::string& mriFlashDir,
                             int& echosConverted)
{
    m_step++;
    Log::info("{}", "");
    Log::info("Step {} : Converting images...\n", m_step);

    //
    // Determine which flash angles to process
    //
    StringList flashes = {"05"};
    if (!m_settings.noFlash30()) {
        flashes.push_back("30");
    }

    //
    // Determine echo numbering: try 001-008 first, then 002-009
    //   (matching the original shell script behavior)
    //
    StringList echos = {"001", "002", "003", "004", "005", "006", "007", "008"};

    bool missing = false;
    for (const std::string& flash : flashes) {
        for (const std::string& echo : echos) {
            std::string dir = std::format("{}/flash{}/{}", flashDir, flash, echo);
            if (!dirExists(dir)) {
                missing = true;
                break;
            }
        }
        if (missing)
            break;
    }

    if (missing) {
        echos = {"002", "003", "004", "005", "006", "007", "008", "009"};
    }

    //
    // Verify all directories exist
    //
    for (const std::string& flash : flashes) {
        for (const std::string& echo : echos) {
            std::string dir = std::format("{}/flash{}/{}", flashDir, flash, echo);
            if (!dirExists(dir)) {
                Log::critical("Directory {} is missing", dir);
                return false;
            }
        }
    }

    //
    // Convert each echo
    //
    std::string mriConvert = m_settings.freeSurferHome() + "/bin/mri_convert";

    echosConverted = 0;
    for (const std::string& flash : flashes) {
        for (const std::string& echo : echos) {
            std::string dir = std::format("{}/flash{}/{}", flashDir, flash, echo);
            std::string destFile = std::format("{}/mef{}_{}.mgz", mriFlashDir, flash, echo);

            // Skip if already converted
            if (fileExists(destFile)) {
                Log::info("{} is already there", destFile);
                continue;
            }

            // Pick the first file in the directory
            const StringList entries = listFiles(dir);
            if (entries.empty()) {
                Log::critical("No files found in {}", dir);
                return false;
            }

            std::string sampleFile = dir + "/" + entries.front();
            if (!runCommand(mriConvert, {sampleFile, destFile})) {
                return false;
            }
            echosConverted++;
        }
    }

    return true;
}

//=============================================================================================================

bool FlashBem::unwarpImages(const std::string& mriFlashDir)
{
    Log::info("\nApplying gradient unwarping...\n");

    const StringList files = listFiles(mriFlashDir, {"mef*.mgz"});

    for (const std::string& file : files) {
        // Skip files already unwarped (ending in u.mgz)
        if (file.ends_with("u.mgz"))
            continue;

        std::string inFile = std::format("{}/{}", mriFlashDir, file);
        const std::string baseName = file.substr(0, file.size() - 4); // Remove .mgz
        std::string outFile = std::format("{}/{}u.mgz", mriFlashDir, baseName);

        if (!runCommand("grad_unwarp",
                        {"-i", inFile, "-o", outFile, "-unwarp", m_settings.unwarp()})) {
            Log::critical("Could not unwarp with option {}", m_settings.unwarp());
            return false;
        }
    }

    return true;
}

//=============================================================================================================

bool FlashBem::createParameterMaps(const std::string& mriFlashDir, const std::string& paramDir)
{
    m_step++;
    Log::info("{}", "");
    Log::info("Step {} : Creating the parameter maps...\n", m_step);

    //
    // Check if parameter maps already exist
    //
    if (!listFiles(paramDir).empty()) {
        Log::info("Parameter maps were already computed");
        return true;
    }

    //
    // Collect all echo files
    //
    StringList pattern;
    if (!m_settings.unwarp().empty()) {
        pattern = {"mef05*u.mgz", "mef30*u.mgz"};
    } else {
        pattern = {"mef05*.mgz", "mef30*.mgz"};
    }
    const StringList files = listFiles(mriFlashDir, pattern);

    StringList fullPaths;
    for (const std::string& f : files) {
        fullPaths.push_back(std::format("{}/{}", mriFlashDir, f));
    }

    // mri_ms_fitparms <files> <output_dir>
    StringList args = fullPaths;
    args.push_back(paramDir);

    return runCommand(m_settings.freeSurferHome() + "/bin/mri_ms_fitparms", args);
}

//=============================================================================================================

bool FlashBem::createFlash5Volume(const std::string& mriFlashDir, const std::string& paramDir)
{
    m_step++;

    std::string flash5File = paramDir + "/flash5.mgz";

    if (!m_settings.noFlash30()) {
        //
        // With flash30: synthesize from T1 and PD parameter maps
        //
        Log::info("{}", "");
        Log::info("Step {} : Synthesizing flash 5...\n", m_step);

        if (fileExists(flash5File)) {
            Log::info("Synthesized flash 5 volume is already there");
            return true;
        }

        // Remove old registered version
        removeFile(paramDir + "/flash5_reg.mgz");

        // mri_synthesize 20 5 5 T1.mgz PD.mgz flash5.mgz
        return runCommand(m_settings.freeSurferHome() + "/bin/mri_synthesize",
                          {"20", "5", "5",
                           paramDir + "/T1.mgz",
                           paramDir + "/PD.mgz",
                           flash5File});
    } else {
        //
        // Without flash30: average all flash-5 echoes
        //
        Log::info("{}", "");
        Log::info("Step {} : Averaging flash5 echoes...\n", m_step);

        const StringList files = listFiles(mriFlashDir, {m_settings.unwarp().empty() ? "mef05*.mgz" : "mef05*u.mgz"});

        StringList args = {"-noconform"};
        for (const std::string& f : files) {
            args.push_back(std::format("{}/{}", mriFlashDir, f));
        }
        args.push_back(flash5File);

        // Remove old registered version
        removeFile(paramDir + "/flash5_reg.mgz");

        return runCommand(m_settings.freeSurferHome() + "/bin/mri_average", args);
    }
}

//=============================================================================================================

bool FlashBem::registerWithMprage(const std::string& paramDir, const std::string& mriDir)
{
    m_step++;
    Log::info("{}", "");
    Log::info("Step {} : Registering flash 5 with MPRAGE...\n", m_step);

    std::string flash5Reg = paramDir + "/flash5_reg.mgz";
    if (fileExists(flash5Reg)) {
        Log::info("Registered flash 5 image is already there");
        return true;
    }

    // Determine reference volume
    std::string refVolume;
    if (fileExists(mriDir + "/T1.mgz")) {
        refVolume = mriDir + "/T1.mgz";
    } else if (dirExists(mriDir + "/T1")) {
        refVolume = mriDir + "/T1";
    } else {
        Log::critical("Could not find T1 reference volume in {}", mriDir);
        return false;
    }

    return runCommand(m_settings.freeSurferHome() + "/bin/fsl_rigid_register",
                      {"-r", refVolume,
                       "-i", paramDir + "/flash5.mgz",
                       "-o", flash5Reg});
}

//=============================================================================================================

bool FlashBem::convertToCor(const std::string& paramDir, const std::string& mriDir,
                            bool& convertedT1, bool& convertedBrain)
{
    m_step++;
    std::string mriConvert = m_settings.freeSurferHome() + "/bin/mri_convert";

    //
    // Step 5a: Convert flash5_reg.mgz to COR format
    //
    Log::info("{}", "");
    Log::info("Step {}a: Converting flash5 volume into COR format...\n", m_step);

    std::string flash5Dir = mriDir + "/flash5";
    makePath(flash5Dir);

    // Remove old COR files
    for (const std::string& f : listFiles(flash5Dir)) {
        removeFile(std::format("{}/{}", flash5Dir, f));
    }

    if (!runCommand(mriConvert, {paramDir + "/flash5_reg.mgz", flash5Dir})) {
        Log::critical("flash5 volume conversion to COR failed");
        return false;
    }

    //
    // Step 5b: Convert T1 to COR if needed
    //
    convertedT1 = false;
    std::string t1Dir = mriDir + "/T1";
    bool needT1 = false;

    if (dirExists(t1Dir)) {
        // Check if COR files exist
        if (listFiles(t1Dir, {"COR*"}).empty()) {
            needT1 = true;
        }
    } else {
        needT1 = true;
    }

    if (needT1) {
        Log::info("{}", "");
        Log::info("Step {}b : Converting T1 volume into COR format...\n", m_step);

        std::string t1Mgz = mriDir + "/T1.mgz";
        if (!fileExists(t1Mgz)) {
            Log::critical("Both T1 mgz and T1 COR volumes missing");
            return false;
        }

        makePath(t1Dir);
        if (!runCommand(mriConvert, {t1Mgz, t1Dir})) {
            Log::critical("T1 volume conversion to COR failed");
            return false;
        }
        convertedT1 = true;
    } else {
        Log::info("{}", "");
        Log::info("Step {}b : T1 volume is already in COR format\n", m_step);
    }

    //
    // Step 5c: Convert brain to COR if needed
    //
    convertedBrain = false;
    std::string brainDir = mriDir + "/brain";
    bool needBrain = false;

    if (dirExists(brainDir)) {
        if (listFiles(brainDir, {"COR*"}).empty()) {
            needBrain = true;
        }
    } else {
        needBrain = true;
    }

    if (needBrain) {
        Log::info("{}", "");
        Log::info("Step {}c : Converting brain volume into COR format...\n", m_step);

        std::string brainMgz = mriDir + "/brain.mgz";
        if (!fileExists(brainMgz)) {
            Log::critical("Both brain mgz and brain COR volumes missing");
            return false;
        }

        makePath(brainDir);
        if (!runCommand(mriConvert, {brainMgz, brainDir})) {
            Log::critical("brain volume conversion to COR failed");
            return false;
        }
        convertedBrain = true;
    } else {
        Log::info("{}", "");
        Log::info("Step {}c : brain volume is already in COR format\n", m_step);
    }

    return true;
}

//=============================================================================================================

bool FlashBem::createBemSurfaces()
{
    m_step++;
    Log::info("{}", "");
    Log::info("Step {} : Creating the BEM surfaces...\n", m_step);

    return runCommand(m_settings.freeSurferHome() + "/bin/mri_make_bem_surfaces",
                      {m_settings.subject()});
}

//=============================================================================================================

bool FlashBem::convertTriToSurf(const std::string& bemDir, const std::string& paramDir)
{
    m_step++;
    Log::info("{}", "");
    Log::info("Step {} : Converting the tri files into surf files...", m_step);

    //
    // Create flash output directory
    //
    std::string flashBemDir = bemDir + "/flash";
    makePath(flashBemDir);

    //
    // Read the flash5_reg.mgz volume to get the vox-to-RAS transform
    //   This is needed for the coordinate transform when converting
    //   from the .tri file (which uses voxel-based coordinates from
    //   mri_make_bem_surfaces) to surface RAS coordinates.
    //
    std::string flash5RegFile = paramDir + "/flash5_reg.mgz";

    //
    // Convert each surface using mne_convert_surface equivalent:
    //   mne_convert_surface --tri ${surf}.tri --surfout ${surf}.surf --swap --mghmri flash5_reg.mgz
    //
    // In the original script, the .tri files are produced by mri_make_bem_surfaces
    // and placed in the bem/ directory. We move them to bem/flash/ and convert.
    //
    const StringList surfNames = {"inner_skull", "outer_skull", "outer_skin"};

    for (const std::string& surfName : surfNames) {
        Log::info("\n{} ...\n", surfName);

        // Move .tri file from bem/ to bem/flash/
        std::string triSrc = std::format("{}/{}.tri", bemDir, surfName);
        std::string triDst = std::format("{}/{}.tri", flashBemDir, surfName);

        if (fileExists(triSrc)) {
            // Remove existing destination if present
            removeFile(triDst);
            std::error_code ec;
            std::filesystem::rename(path_from_utf8(triSrc), path_from_utf8(triDst), ec);
            if (ec) {
                Log::critical("Could not move {} to {}", triSrc, triDst);
                return false;
            }
        } else if (!fileExists(triDst)) {
            Log::critical("Could not find {} .tri file", surfName);
            return false;
        }

        //
        // Use mne_convert_surface to do the conversion, as in the original script:
        //   mne_convert_surface --tri inner_skull.tri --surfout inner_skull.surf
        //                       --swap --mghmri $SUBJECTS_DIR/$SUBJECT/mri/flash/parameter_maps/flash5_reg.mgz
        //
        // We look for mne_convert_surface in the MNE bin directory.
        // If not available, we try the FreeSurfer bin directory.
        //
        // Note: The original mne_convert_surface C tool reads the .tri file,
        //       applies the MGH vox-to-surface-RAS transform from the mghmri
        //       volume, swaps triangle winding, and writes a FreeSurfer binary
        //       surface file.
        //
        std::string mneConvertSurface;

        // Try to find mne_convert_surface in PATH or known locations
        StringList searchPaths;
        if (const char* mneBinDir = std::getenv("MNE_BIN_DIR"); mneBinDir && *mneBinDir) {
            searchPaths.emplace_back(mneBinDir);
        }
        if (const char* mneRoot = std::getenv("MNE_ROOT"); mneRoot && *mneRoot) {
            searchPaths.push_back(std::string(mneRoot) + "/bin");
        }
        searchPaths.push_back(m_settings.freeSurferHome() + "/bin");

        for (const std::string& path : searchPaths) {
            std::string candidate = path + "/mne_convert_surface";
            if (fileExists(candidate)) {
                mneConvertSurface = candidate;
                break;
            }
        }

        if (mneConvertSurface.empty()) {
            //
            // mne_convert_surface not found — perform the conversion
            // directly using mne-cpp libraries.
            //
            // This is the equivalent of:
            //   mne_convert_surface --tri ${surf}.tri --surfout ${surf}.surf
            //                       --swap --mghmri flash5_reg.mgz
            //
            // Steps:
            //   1. Read the .tri file (ASCII triangle format)
            //   2. Swap triangle winding order
            //   3. Read the MGH volume for vox-to-RAS transform
            //   4. Apply the transform to vertex coordinates
            //   5. Write as FreeSurfer binary surface
            //
            Log::info("mne_convert_surface not found, using built-in conversion.");

            // Read .tri file
            std::ifstream in(path_from_utf8(triDst));
            if (!in) {
                Log::critical("Could not open {}", triDst);
                return false;
            }

            int nvert = 0;
            in >> nvert;
            if (nvert <= 0) {
                Log::critical("Invalid vertex count in {}", triDst);
                return false;
            }

            MatrixX3f rr(nvert, 3);
            for (int k = 0; k < nvert; ++k) {
                // Read as double and round to float, as QTextStream did
                double x = 0.0, y = 0.0, z = 0.0;
                in >> x >> y >> z;
                rr(k, 0) = static_cast<float>(x);
                rr(k, 1) = static_cast<float>(y);
                rr(k, 2) = static_cast<float>(z);
            }

            int ntri = 0;
            in >> ntri;
            if (ntri <= 0) {
                Log::critical("Invalid triangle count in {}", triDst);
                return false;
            }

            MatrixX3i tris(ntri, 3);
            for (int k = 0; k < ntri; ++k) {
                int v1 = 0, v2 = 0, v3 = 0;
                in >> v1 >> v2 >> v3;
                // Swap winding order (--swap) and convert from 1-based to 0-based
                tris(k, 0) = v1 - 1;
                tris(k, 1) = v3 - 1; // Swapped
                tris(k, 2) = v2 - 1; // Swapped
            }
            in.close();

            Log::info("Read {} vertices, {} triangles from {}", nvert, ntri, triDst);

            //
            // Read the MGH volume to get vox-to-RAS transform
            //
            MriVolData volData;
            std::vector<FiffCoordTrans> additionalTrans;
            std::string subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

            if (!MriMghIO::read(flash5RegFile, volData, additionalTrans, subjectMriDir, false)) {
                Log::critical("Could not read MGH file {}", flash5RegFile);
                return false;
            }

            // mri_make_bem_surfaces already writes the .tri vertices in
            // surface RAS coordinates, in millimetres, so no vox-to-surface-RAS
            // transform is applied here; only the unit conversion is needed.
            rr /= 1000.0f;

            //
            // Write as FreeSurfer binary surface (big-endian triangle format)
            //
            std::string surfOut = std::format("{}/{}.surf", flashBemDir, surfName);

            auto surfFile = FileByteStream::open(path_from_utf8(surfOut), FileByteStream::Mode::Write);
            if (!surfFile) {
                Log::critical("Could not open {} for writing", surfOut);
                return false;
            }

            // Magic number for triangle surface: 0xFFFFFE
            const std::array<std::uint8_t, 3> magic = {0xFF, 0xFF, 0xFE};
            surfFile->write(magic.data(), magic.size());

            // Two lines of text (created_by + newline)
            const std::string_view comment = "created by mne_flash_bem\n\n";
            surfFile->write(comment.data(), comment.size());

            // Number of vertices and faces
            surfFile->write_big_endian(static_cast<std::int32_t>(nvert));
            surfFile->write_big_endian(static_cast<std::int32_t>(ntri));

            // Vertex coordinates (as float32)
            for (int k = 0; k < nvert; ++k) {
                surfFile->write_big_endian(rr(k, 0));
                surfFile->write_big_endian(rr(k, 1));
                surfFile->write_big_endian(rr(k, 2));
            }

            // Triangle vertex indices (as int32, 0-based)
            for (int k = 0; k < ntri; ++k) {
                surfFile->write_big_endian(static_cast<std::int32_t>(tris(k, 0)));
                surfFile->write_big_endian(static_cast<std::int32_t>(tris(k, 1)));
                surfFile->write_big_endian(static_cast<std::int32_t>(tris(k, 2)));
            }

            surfFile->close();
            Log::info("Written {}", surfOut);
        } else {
            //
            // mne_convert_surface is available — use it directly
            //
            std::string surfOut = std::format("{}/{}.surf", flashBemDir, surfName);
            if (!runCommand(mneConvertSurface,
                            {"--tri", triDst,
                             "--surfout", surfOut,
                             "--swap",
                             "--mghmri", flash5RegFile})) {
                return false;
            }
        }
    }

    return true;
}

//=============================================================================================================

void FlashBem::cleanup(const std::string& bemDir, const std::string& mriDir,
                       bool convertedT1, bool convertedBrain)
{
    Log::info("{}", "");
    Log::info("Final step : Cleaning up...\n");

    // Remove inner_skull_tmp.tri
    removeFile(bemDir + "/inner_skull_tmp.tri");

    // Remove COR volumes created during this run
    if (convertedT1) {
        removeRecursively(mriDir + "/T1");
        Log::info("Deleted the T1 COR volume");
    }
    if (convertedBrain) {
        removeRecursively(mriDir + "/brain");
        Log::info("Deleted the brain COR volume");
    }

    // Always remove flash5 COR volume (created fresh each run)
    removeRecursively(mriDir + "/flash5");
    Log::info("Deleted the flash5 COR volume");
}
