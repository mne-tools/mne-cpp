//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     April, 2019
 * @brief    Converts EDF/EDF+ and BDF recordings to FIFF via mne::EDFReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/bids/readers/bids_edf_reader.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <filesystem>
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
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("EDF/EDF+/BDF to FIFF conversion. Samples are stored in SI units like mne.io.read_raw_edf; "
                             "channels sampled below the highest rate are skipped.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"fileIn"}, "The input EDF or BDF file. Needs to be specified.", "in", {}});
    parser.add_option({{"fileOut"}, "The output file. If not specified, the input file name with a .fif extension.", "out", {}});
    parser.process(argc, argv, "mne_edf2fiff", PROGRAM_VERSION);

    const std::string sInputFile = parser.value("fileIn");
    std::string sOutputFile = parser.value("fileOut");
    if (sInputFile.empty()) {
        std::print("{}", parser.help_text("mne_edf2fiff"));
        return 1;
    }

    const std::filesystem::path inputPath = mne::path_from_utf8(sInputFile);
    EDFReader reader;
    if (!reader.supports_extension(mne::utf8_from_path(inputPath.extension()))) {
        Log::critical("Not an EDF or BDF file: {}", sInputFile);
        return 1;
    }
    if (!reader.open(inputPath) || reader.sample_count() <= 0) {
        Log::critical("Could not read {}", sInputFile);
        return 1;
    }
    if (sOutputFile.empty()) {
        sOutputFile = sInputFile.substr(0, sInputFile.rfind('.')) + ".fif";
    }

    const FiffRawData fiffRaw = reader.to_fiff_raw_data();
    RowVectorXd cals;
    auto outfid = FiffStream::start_writing_raw(mne::path_from_utf8(sOutputFile), fiffRaw.info, cals);
    if (!outfid) {
        Log::critical("Could not write {}", sOutputFile);
        return 1;
    }
    fiff_int_t first = 0;
    outfid->write_int(FIFF_FIRST_SAMPLE, &first);

    // 10 s buffers
    const int iBufferSamples = static_cast<int>(std::ceil(10.0f * fiffRaw.info.sfreq));
    const int iSampleCount = static_cast<int>(reader.sample_count());
    for (int iStart = 0; iStart < iSampleCount; iStart += iBufferSamples) {
        const int iEnd = std::min(iStart + iBufferSamples, iSampleCount);
        outfid->write_raw_buffer(reader.read_raw_segment(iStart, iEnd).cast<double>(), cals);
    }
    outfid->finish_writing_raw();

    Log::info("Wrote {}", sOutputFile);
    return 0;
}
