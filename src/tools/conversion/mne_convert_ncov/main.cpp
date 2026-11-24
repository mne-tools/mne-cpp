//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert old MNE ASCII noise covariance (ncov) format to FIFF.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "Convert old MNE ASCII noise covariance (ncov) format to FIFF.\n\n"
        "The ncov format uses a header line '#!ascii' followed by a dimensions\n"
        "line 'nmeg neeg nfree' and a full symmetric matrix in row-major order.\n"
        "The tool can selectively include MEG-only, EEG-only, or combined\n"
        "channels.\n\n"
        "Port of the MNE-C mne_convert_ncov utility.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"ncov"}, "Input ncov ASCII covariance file.", "file", {}});
    parser.add_option({{"cov"}, "Output FIFF covariance file (default: <ncov-stem>.fif).", "file", {}});
    parser.add_option({{"meas"}, "Measurement FIFF file (for channel names).", "file", {}});
    parser.add_option({{"meg"}, "Include MEG channels.", {}, {}});
    parser.add_option({{"eeg"}, "Include EEG channels.", {}, {}});
    parser.process(argc, argv, "mne_convert_ncov", PROGRAM_VERSION);

    const std::string ncovFile = parser.value("ncov");
    std::string covFile = parser.value("cov");
    const std::string measFile = parser.value("meas");
    bool includeMeg = parser.is_set("meg");
    bool includeEeg = parser.is_set("eeg");

    if (ncovFile.empty()) {
        Log::critical("--ncov is required.");
        return 1;
    }
    if (measFile.empty()) {
        Log::critical("--meas is required.");
        return 1;
    }

    // Default: include both if neither specified
    if (!includeMeg && !includeEeg) {
        includeMeg = true;
        includeEeg = true;
    }

    // Default output name
    if (covFile.empty()) {
        const std::filesystem::path ncovPath = mne::path_from_utf8(ncovFile);
        const std::string dir = ncovPath.has_parent_path() ? mne::utf8_from_path(ncovPath.parent_path()) : std::string(".");
        covFile = dir + "/" + mne::utf8_from_path(ncovPath.stem()) + ".fif";
    }

    //=========================================================================
    // Read measurement info for channel names and types
    //=========================================================================
    FiffRawData raw(mne::path_from_utf8(measFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read measurement info from: {}", measFile);
        return 1;
    }
    const FiffInfo& info = raw.info;

    // Classify channels
    std::vector<int> megIdx, eegIdx;
    for (int i = 0; i < info.nchan; ++i) {
        int kind = info.chs[i].kind;
        if (kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH) {
            megIdx.push_back(i);
        } else if (kind == FIFFV_EEG_CH) {
            eegIdx.push_back(i);
        }
    }

    Log::info("Measurement: {} MEG channels, {} EEG channels, {} total", static_cast<long long>(megIdx.size()), static_cast<long long>(eegIdx.size()), info.nchan);

    //=========================================================================
    // Read ncov ASCII file
    //=========================================================================
    std::ifstream in(mne::path_from_utf8(ncovFile));
    if (!in) {
        Log::critical("Cannot open ncov file: {}", ncovFile);
        return 1;
    }

    // Check header
    std::string line;
    std::getline(in, line);
    const std::string header(mne::trimmed(line));
    if (header != "#!ascii") {
        Log::critical("Invalid ncov header: expected '#!ascii', got '{}'.", header);
        return 1;
    }

    // Read dimensions: nmeg neeg nfree
    line.clear();
    std::getline(in, line);
    const std::string dimLine(mne::trimmed(line));
    const std::vector<std::string_view> dims = mne::split_whitespace(dimLine);
    if (dims.size() < 3) {
        Log::critical("Invalid dimension line: '{}'.", dimLine);
        return 1;
    }

    int nMegFile = mne::to_int(dims[0]).value_or(0);
    int nEegFile = mne::to_int(dims[1]).value_or(0);
    int nFree = mne::to_int(dims[2]).value_or(0);
    int nTotal = nMegFile + nEegFile;

    Log::info("Ncov file: {} MEG + {} EEG = {} channels, nfree = {}", nMegFile, nEegFile, nTotal, nFree);

    // Validate dimensions
    if (nMegFile != std::ssize(megIdx)) {
        Log::critical("MEG channel count mismatch: file says {}, measurement has {}.", nMegFile, static_cast<long long>(megIdx.size()));
        return 1;
    }
    if (nEegFile != std::ssize(eegIdx)) {
        Log::critical("EEG channel count mismatch: file says {}, measurement has {}.", nEegFile, static_cast<long long>(eegIdx.size()));
        return 1;
    }

    // Read full covariance matrix
    const std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    const auto isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    };
    std::size_t pos = 0;
    MatrixXd fullCov(nTotal, nTotal);
    for (int i = 0; i < nTotal; ++i) {
        for (int j = 0; j < nTotal; ++j) {
            if (pos >= body.size()) {
                Log::critical("Unexpected end of file at element ({}, {}).", i, j);
                return 1;
            }
            // Skip whitespace and read one number; a missing or malformed one reads as 0
            while (pos < body.size() && isSpace(body[pos]))
                ++pos;
            const std::size_t start = pos;
            while (pos < body.size() && !isSpace(body[pos]))
                ++pos;
            fullCov(i, j) = mne::to_double(std::string_view(body).substr(start, pos - start)).value_or(0.0);
        }
    }

    Log::info("Read {}x{} covariance matrix", nTotal, nTotal);

    //=========================================================================
    // Extract requested submatrix
    //=========================================================================
    std::vector<int> selectedFileIdx; // Indices into the file's matrix
    std::vector<int> selectedMeasIdx; // Indices into the measurement info

    if (includeMeg) {
        for (int i = 0; i < nMegFile; ++i) {
            selectedFileIdx.push_back(i);
            selectedMeasIdx.push_back(megIdx[i]);
        }
    }
    if (includeEeg) {
        for (int i = 0; i < nEegFile; ++i) {
            selectedFileIdx.push_back(nMegFile + i);
            selectedMeasIdx.push_back(eegIdx[i]);
        }
    }

    const int nSel = static_cast<int>(selectedFileIdx.size());
    MatrixXd selCov(nSel, nSel);
    for (int i = 0; i < nSel; ++i) {
        for (int j = 0; j < nSel; ++j) {
            selCov(i, j) = fullCov(selectedFileIdx[i], selectedFileIdx[j]);
        }
    }

    // Symmetrize
    selCov = (selCov + selCov.transpose()) / 2.0;

    Log::info("Selected {} channels ({}{})", nSel, includeMeg ? "MEG" : "", (includeMeg && includeEeg) ? "+EEG" : (includeEeg ? "EEG" : ""));

    //=========================================================================
    // Build and write FIFF covariance
    //=========================================================================
    StringList chNames;
    for (int i = 0; i < nSel; ++i) {
        chNames.push_back(info.chs[selectedMeasIdx[i]].ch_name);
    }

    FiffCov cov;
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.diag = false;
    cov.dim = nSel;
    cov.names = chNames;
    cov.data = selCov;
    cov.nfree = nFree;

    if (!cov.save(mne::path_from_utf8(covFile))) {
        Log::critical("Cannot write output file: {}", covFile);
        return 1;
    }

    Log::info("Written FIFF covariance ({} channels) to: {}", nSel, covFile);
    return 0;
}
