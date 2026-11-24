//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2001
 * @brief    Implements mne_inverse_operator application.
 *           Port of the original MNE-C mne_inverse_operator by Matti Hamalainen.
 *
 *           Assembles the inverse operator from a forward solution and a noise
 *           covariance matrix.  Supports MEG-only, EEG-only, or combined
 *           MEG + EEG solutions with fixed, loose, or free source orientations
 *           and depth weighting.
 *
 *           Like the original MNE-C tool, no measurement file is needed.
 *           Channel information is obtained from the forward solution, and
 *           SSP projectors are read from the noise covariance file (or from
 *           explicit --proj files).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/mne/mne.h>
#include <mne/mne/mne_inverse_operator.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Build a FiffInfo from the forward solution's FiffInfoBase.
 * Copies all base-class fields (chs, ch_names, bads, coordinate transforms, etc.)
 * and populates SSP projectors from the noise covariance and/or --proj files.
 * This mirrors the original MNE-C approach where no measurement file is needed.
 */
static FiffInfo buildInfoFromForward(const MNEForwardSolution& forward,
                                     const FiffCov& noiseCov,
                                     const std::vector<FiffProj>& extraProjs)
{
    FiffInfo info;

    // Copy all FiffInfoBase members from the forward solution
    info.chs = forward.info.chs;
    info.nchan = forward.info.nchan;
    info.ch_names = forward.info.ch_names;
    info.bads = forward.info.bads;
    info.meas_id = forward.info.meas_id;
    info.dev_head_t = forward.info.dev_head_t;
    info.ctf_head_t = forward.info.ctf_head_t;

    // --- SSP projectors ---
    // Priority (matching SVN MNE-C):
    //   1. Projectors embedded in the noise covariance file
    //   2. Projectors from explicit --proj files (only if noise cov had none)
    if (!noiseCov.projs.empty()) {
        Log::info("  Using {} SSP projectors from noise covariance file.", static_cast<long long>(noiseCov.projs.size()));
        info.projs = noiseCov.projs;
        FiffProj::activate_projs(info.projs);
        if (!extraProjs.empty()) {
            Log::info("  NOTE: Noise covariance already contains projectors; ignoring --proj files.");
        }
    } else if (!extraProjs.empty()) {
        Log::info("  Using {} SSP projectors from --proj files.", static_cast<long long>(extraProjs.size()));
        info.projs = extraProjs;
        FiffProj::activate_projs(info.projs);
    }

    // Auto-add average EEG reference projector if EEG channels are present
    // and no EEG average ref projector was found (matches SVN MNE-C behavior)
    bool hasEeg = false;
    bool hasEegAvRef = false;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH)
            hasEeg = true;
    }
    for (int i = 0; i < std::ssize(info.projs); ++i) {
        if (info.projs[i].kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF)
            hasEegAvRef = true;
    }
    if (hasEeg && !hasEegAvRef) {
        Log::info("  Adding average EEG reference projector.");
        // Build EEG average reference projector
        StringList eegNames;
        for (int i = 0; i < info.nchan; ++i) {
            if (info.chs[i].kind == FIFFV_EEG_CH)
                eegNames.push_back(info.chs[i].ch_name);
        }
        if (eegNames.size() > 0) {
            RowVectorXd vecOnes = RowVectorXd::Ones(eegNames.size());
            MatrixXd data = vecOnes / sqrt(static_cast<double>(eegNames.size()));
            FiffNamedMatrix projData(1, static_cast<int>(eegNames.size()),
                                     {},
                                     eegNames,
                                     data);
            FiffProj eegAvRef;
            eegAvRef.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF;
            eegAvRef.active = true;
            eegAvRef.desc = "Average EEG reference";
            eegAvRef.data = FiffNamedMatrix(projData);
            info.projs.push_back(eegAvRef);
        }
    }

    return info;
}

/**
 * Read SSP projectors from a FIFF file.
 */
static std::vector<FiffProj> readProjFile(const std::string& fileName)
{
    std::vector<FiffProj> projs;
    FiffStream stream(path_from_utf8(fileName));
    if (!stream.open()) {
        Log::warning("  WARNING: Could not open projection file {}", fileName);
        return projs;
    }
    projs = stream.read_proj(stream.dirtree());
    stream.close();
    return projs;
}

/**
 * Read bad channel names from a text file (one channel name per line).
 */
static StringList readBadFile(const std::string& fileName)
{
    StringList bads;
    std::ifstream file(path_from_utf8(fileName));
    if (!file) {
        Log::warning("  WARNING: Could not open bad channel file {}", fileName);
        return bads;
    }
    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string_view line = trimmed(rawLine);
        if (!line.empty() && !line.starts_with('#'))
            bads.emplace_back(line);
    }
    file.close();
    return bads;
}

/**
 * Parse a number like QString::toFloat(): 0 if the text is not a number.
 */
static float toFloat(const std::string& text)
{
    return static_cast<float>(to_double(text).value_or(0.0));
}

/**
 * The file name without its last extension, like QFileInfo::completeBaseName().
 */
static std::string completeBaseName(const std::string& file)
{
    const std::string name = utf8_from_path(path_from_utf8(file).filename());
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

/**
 * The directory of a file, "." if it has none, like QFileInfo::path().
 */
static std::string dirPath(const std::string& file)
{
    const std::filesystem::path parent = path_from_utf8(file).parent_path();
    return parent.empty() ? std::string(".") : utf8_from_path(parent);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_inverse_operator application.
 * This is a port of the original MNE-C mne_inverse_operator by Matti Hamalainen.
 *
 * It reads a forward solution and a noise covariance matrix, then assembles
 * the inverse operator decomposition and writes it to a FIFF file.
 *
 * Like the original MNE-C tool, no measurement file is required. Channel
 * information is obtained from the forward solution, SSP projectors from the
 * noise covariance file or explicit --proj files.
 *
 * @param[in] argc  (argument count)
 * @param[in] argv  (argument vector)
 * @return exit code (0 on success).
 */
int main(int argc, char* argv[])
{
    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    CommandLineParser parser(
        "Assemble an inverse operator from a forward solution and noise covariance.\n"
        "Port of the original MNE-C mne_inverse_operator by Matti Hamalainen.\n\n"
        "Supports MEG-only, EEG-only, or combined MEG+EEG solutions with\n"
        "fixed, loose, or free source orientations and depth weighting.\n\n"
        "Channel information is obtained from the forward solution.\n"
        "SSP projectors are read from the noise covariance file (or --proj files).\n"
        "No measurement file is required (same as the original MNE-C tool).");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"fwd"}, "The forward solution file.", "file", {}});
    parser.add_option({{"noisecov", "senscov"}, "The noise (sensor) covariance matrix file.", "file", {}});
    parser.add_option({{"srccov"}, "Source covariance matrix file (default: identity).", "file", {}});
    parser.add_option({{"meg"}, "Use MEG channels.", {}, {}});
    parser.add_option({{"eeg"}, "Use EEG channels (specify both --meg and --eeg for a combined solution).", {}, {}});
    parser.add_option({{"fixed"}, "Use fixed source orientations normal to the cortical surface.", {}, {}});
    parser.add_option({{"loose"}, "Amount of loose orientation constraint (0.0-1.0, default: 0.2).", "amount", "0.2"});
    parser.add_option({{"depth"}, "Enable depth weighting.", {}, {}});
    parser.add_option({{"weightexp"}, "Depth weighting exponent (0.0-1.0, default: 0.8).", "value", "0.8"});
    parser.add_option({{"weightlimit"}, "Depth weighting upper limit (default: 10.0).", "value", "10.0"});
    parser.add_option({{"nodepth"}, "Disable depth weighting.", {}, {}});
    parser.add_option({{"reg"}, "Regularization factor for all channel types.", "amount", {}});
    parser.add_option({{"magreg"}, "Regularization factor for magnetometers (default: 0.1).", "amount", "0.1"});
    parser.add_option({{"gradreg"}, "Regularization factor for gradiometers (default: 0.1).", "amount", "0.1"});
    parser.add_option({{"eegreg"}, "Regularization factor for EEG channels (default: 0.1).", "amount", "0.1"});
    parser.add_option({{"diagnoise"}, "Omit off-diagonal terms from the noise covariance matrix.", {}, {}});
    parser.add_option({{"proj"}, "Load SSP projectors from file (can be repeated).", "file", {}});
    parser.add_option({{"bad"}, "Bad channels file (one name per line, can be repeated).", "file", {}});
    parser.add_option({{"inv"}, "Output file for the inverse operator (default: <fwd>-inv.fif).", "file", {}});

    parser.process(argc, argv, "mne_inverse_operator", PROGRAM_VERSION);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.is_set("fwd")) {
        Log::critical("Error: --fwd option is required.");
        std::print("{}", parser.help_text("mne_inverse_operator"));
        return 1;
    }
    if (!parser.is_set("noisecov")) {
        Log::critical("Error: --noisecov option is required.");
        std::print("{}", parser.help_text("mne_inverse_operator"));
        return 1;
    }

    // Default to MEG+EEG if neither --meg nor --eeg is specified
    bool useMeg = parser.is_set("meg");
    bool useEeg = parser.is_set("eeg");
    if (!useMeg && !useEeg) {
        Log::info("Neither --meg nor --eeg specified; using all available channels.");
        useMeg = true;
        useEeg = true;
    }

    bool useFixed = parser.is_set("fixed");
    float loose = toFloat(parser.value("loose"));
    float depth = parser.is_set("nodepth") ? 0.0f
        : parser.is_set("depth")           ? toFloat(parser.value("weightexp"))
                                           : 0.8f;
    bool diagNoise = parser.is_set("diagnoise");

    // Regularization parameters
    float magReg = toFloat(parser.value("magreg"));
    float gradReg = toFloat(parser.value("gradreg"));
    float eegReg = toFloat(parser.value("eegreg"));
    if (parser.is_set("reg")) {
        float regAll = toFloat(parser.value("reg"));
        magReg = regAll;
        gradReg = regAll;
        eegReg = regAll;
    }

    // Output file name
    const std::string fwdName = parser.value("fwd");
    std::string invName;
    if (parser.is_set("inv")) {
        invName = parser.value("inv");
    } else {
        // Derive from forward solution file name
        std::string base = completeBaseName(fwdName);
        if (base.ends_with("-fwd")) {
            base.resize(base.size() - 4);
        }
        invName = dirPath(fwdName) + "/" + base + "-inv.fif";
    }

    //=========================================================================================================
    // Read forward solution
    //=========================================================================================================

    Log::info("{}", "");
    Log::info("========================================");
    Log::info("Reading forward solution from {}...", fwdName);

    auto forwardRead = MNEForwardSolution::read(path_from_utf8(fwdName), {.surf_ori = true});
    if (!forwardRead) {
        Log::critical("{}", forwardRead.error().what());
        return 1;
    }
    MNEForwardSolution& forward = *forwardRead;

    Log::info("  Forward solution: {} sources, {} channels", forward.nsource, forward.nchan);
    Log::info("  Source orientation: {}", forward.isFixedOrient() ? "fixed" : "free");
    Log::info("  Source space type: {} hemispheres", forward.src.size());

    // Pick channel types according to --meg/--eeg options
    if (!(useMeg && useEeg)) {
        Log::info("Restricting forward solution to {} channels...", useMeg ? "MEG" : "EEG");
        forward = forward.pick_types(useMeg, useEeg);
        if (forward.empty()) {
            Log::critical("Error: No channels remaining after channel type restriction.");
            return 1;
        }
        Log::info("  After restriction: {} channels", forward.nchan);
    }

    //=========================================================================================================
    // Read noise covariance
    //=========================================================================================================

    Log::info("\nReading noise covariance from {}...", parser.value("noisecov"));

    FiffCov noiseCov(path_from_utf8(parser.value("noisecov")));

    if (noiseCov.data.rows() == 0) {
        Log::critical("Error: Could not read noise covariance from {}", parser.value("noisecov"));
        return 1;
    }

    Log::info("  Noise covariance: {} x {}", static_cast<int>(noiseCov.data.rows()), static_cast<int>(noiseCov.data.cols()));
    if (!noiseCov.projs.empty()) {
        Log::info("  Noise covariance contains {} SSP projectors.", static_cast<long long>(noiseCov.projs.size()));
    }

    //=========================================================================================================
    // Read optional SSP projection files
    //=========================================================================================================

    std::vector<FiffProj> extraProjs;
    const StringList projFiles = parser.values("proj");
    for (const std::string& projFileName : projFiles) {
        Log::info("Reading SSP projectors from {}...", projFileName);
        std::vector<FiffProj> fileProjs = readProjFile(projFileName);
        Log::info("  Found {} projectors.", static_cast<long long>(fileProjs.size()));
        std::ranges::move(fileProjs, std::back_inserter(extraProjs));
    }

    //=========================================================================================================
    // Build FiffInfo from forward solution + noise covariance projectors
    // (same approach as the original MNE-C tool — no measurement file needed)
    //=========================================================================================================

    Log::info("\nBuilding measurement info from forward solution...");
    FiffInfo info = buildInfoFromForward(forward, noiseCov, extraProjs);
    Log::info("  Info: {} channels, {} projectors", info.nchan, static_cast<long long>(info.projs.size()));

    //=========================================================================================================
    // Merge bad channels from --bad files
    //=========================================================================================================

    const StringList badFiles = parser.values("bad");
    for (const std::string& badFileName : badFiles) {
        Log::info("Reading bad channels from {}...", badFileName);
        const StringList fileBads = readBadFile(badFileName);
        for (const std::string& bad : fileBads) {
            if (!std::ranges::contains(info.bads, bad))
                info.bads.push_back(bad);
        }
    }
    // Also merge bad channels from the noise covariance
    for (const std::string& bad : noiseCov.bads) {
        if (!std::ranges::contains(info.bads, bad))
            info.bads.push_back(bad);
    }
    if (!info.bads.empty()) {
        Log::info("  Bad channels ({}): {}", static_cast<long long>(info.bads.size()), mne::join(info.bads, " "));
    }

    //=========================================================================================================
    // Regularize noise covariance
    //=========================================================================================================

    Log::info("\nRegularizing noise covariance (mag={:.3f}, grad={:.3f}, eeg={:.3f})...", magReg, gradReg, eegReg);
    noiseCov = noiseCov.regularize(info, magReg, gradReg, eegReg, true);

    // If --diagnoise, zero out off-diagonal elements
    if (diagNoise) {
        Log::info("Using only diagonal noise covariance.");
        MatrixXd diagCov = MatrixXd::Zero(noiseCov.data.rows(), noiseCov.data.cols());
        diagCov.diagonal() = noiseCov.data.diagonal();
        noiseCov.data = diagCov;
    }

    //=========================================================================================================
    // Assemble the inverse operator
    //=========================================================================================================

    Log::info("\nAssembling inverse operator...");
    Log::info("  Fixed orientation: {}", useFixed ? "yes" : "no");
    if (!useFixed) {
        Log::info("  Loose constraint: {:.2f}", loose);
    }
    if (depth > 0.0f) {
        Log::info("  Depth weighting:  yes ({:.2f})", depth);
    } else {
        Log::info("  Depth weighting:  no");
    }

    // The forward solution is not needed afterwards: moving it saves copying its source spaces
    MNEInverseOperator invOp =
        MNEInverseOperator::make_inverse_operator(info, std::move(forward), noiseCov, loose, depth, useFixed, true);

    if (invOp.nsource <= 0) {
        Log::critical("Error: Failed to assemble inverse operator.");
        return 1;
    }

    Log::info("  Inverse operator: {} sources, {} channels", invOp.nsource, invOp.nchan);

    //=========================================================================================================
    // Write the inverse operator
    //=========================================================================================================

    Log::info("\nWriting inverse operator to {}...", invName);

    if (auto written = invOp.write(path_from_utf8(invName)); !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }

    Log::info("Done.\n");
    Log::info("The inverse operator is ready.");
    Log::info("You can now use mne_compute_mne or mne_compute_raw_inverse");
    Log::info("to compute source estimates.\n");

    return 0;
}
