//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2001
 * @brief    Export raw FIFF data to MATLAB Level 5 MAT file format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/math/mat_file.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <fstream>
#include <ostream>
#include <print>
#include <string>

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
    CommandLineParser parser("Export raw FIFF data to MATLAB Level 5 MAT file format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Input raw FIFF file.", "file", {}});
    parser.add_option({{"out"}, "Output .mat file.", "file", {}});
    parser.add_option({{"from"}, "First sample.", "samp", "-1"});
    parser.add_option({{"to"}, "Last sample.", "samp", "-1"});
    parser.process(argc, argv, "mne_raw2mat", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    const std::string outFile = parser.value("out");
    int fromSamp = mne::to_int(parser.value("from")).value_or(0);
    int toSamp = mne::to_int(parser.value("to")).value_or(0);

    if (rawFile.empty()) {
        Log::critical("--raw is required.");
        std::print("{}", parser.help_text("mne_raw2mat"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_raw2mat"));
        return 1;
    }

    // Load raw data
    FiffRawData raw(mne::path_from_utf8(rawFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read raw: {}", rawFile);
        return 1;
    }

    int nChan = raw.info.nchan;
    double sfreq = raw.info.sfreq;
    int first = (fromSamp >= 0) ? fromSamp : raw.first_samp;
    int last = (toSamp >= 0) ? toSamp : raw.last_samp;

    Log::info("Raw: {} channels, {:.1f} Hz, samples {}..{}", nChan, sfreq, first, last);

    // Read data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, first, last)) {
        Log::critical("Cannot read data segment.");
        return 1;
    }

    const auto rowCount = static_cast<long long>(data.rows());
    const auto sampleCount = static_cast<long long>(data.cols());
    Log::info("Read {} channels x {} samples", rowCount, sampleCount);

    // Build channel names
    StringList chNames;
    for (int c = 0; c < nChan; ++c)
        chNames.push_back(raw.info.chs[c].ch_name);

    MatrixXd sfreqMat(1, 1);
    sfreqMat(0, 0) = sfreq;
    if (auto written = write_mat_file(mne::path_from_utf8(outFile), "mne_raw2mat", {{"data", data}, {"sfreq", sfreqMat}, {"times", times}});
        !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }
    Log::info("Written MAT file: {}", outFile);
    Log::info("Variables: data ({} x {}), sfreq (1x1), times (1 x {})", nChan, sampleCount, static_cast<long long>(times.cols()));

    return 0;
}
