//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Export sensor locations from a measurement FIFF file to text.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_dir_node.h>
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

#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
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
/**
 * Qt's asprintf, QString::number and QTextStream round exact decimal ties away from zero where std::format rounds
 * them to even. Moves a tie at @p decimals to the neighbouring double away from zero, so that std::format writes
 * the digits Qt wrote.
 */
static double sensorQtTie(double value, int decimals)
{
    const double scaled = std::ldexp(value, decimals + 1);
    // A tie at decimals is an odd multiple of 2^-(decimals + 1)
    if (std::isfinite(scaled) && scaled == std::trunc(scaled) && std::fmod(scaled, 2.0) != 0.0) {
        return std::nextafter(value, value < 0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());
    }
    return value;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Export sensor locations to text.\n\n"
                             "Reads channel locations from a measurement FIFF file and writes\n"
                             "channel_name x y z (one per line) for MEG and EEG channels.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Measurement FIFF file.", "file", {}});
    parser.add_option({{"out"}, "Output text file.", "file", {}});
    parser.add_option({{"frame"}, "Coordinate frame: \"head\" (default) or \"device\".", "frame", "head"});
    parser.process(argc, argv, "mne_sensor_locations", PROGRAM_VERSION);

    const std::string measFile = parser.value("meas");
    const std::string outFile = parser.value("out");
    const std::string frame = to_lower(parser.value("frame"));

    if (measFile.empty() || outFile.empty()) {
        Log::critical("Both --meas and --out are required.");
        std::print("{}", parser.help_text("mne_sensor_locations"));
        return 1;
    }
    if (frame != "head" && frame != "device") {
        Log::critical("--frame must be \"head\" or \"device\".");
        return 1;
    }

    // Read measurement info
    FiffStream stream(path_from_utf8(measFile));
    if (!stream.open()) {
        Log::critical("Cannot open FIFF file: {}", measFile);
        return 1;
    }

    FiffInfo info;
    std::shared_ptr<FiffDirNode> infoNode;
    if (!stream.read_meas_info(stream.dirtree(), info, infoNode)) {
        Log::critical("Cannot read measurement info from: {}", measFile);
        return 1;
    }
    stream.close();

    // Prepare coordinate transform if device frame requested
    // Channel positions for MEG are in device coords by default;
    // EEG positions are in head coords by default.
    // dev_head_t: device -> head transform
    bool useDevice = (frame == "device");
    FiffCoordTrans headToDev;
    if (useDevice) {
        headToDev = info.dev_head_t.inverted();
    }

    // Open output file
    std::ofstream out(path_from_utf8(outFile));
    if (!out) {
        Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }

    int nWritten = 0;
    for (int k = 0; k < std::ssize(info.chs); ++k) {
        const FiffChInfo& ch = info.chs[k];

        float x, y, z;

        if (ch.kind == FIFFV_MEG_CH || ch.kind == FIFFV_REF_MEG_CH) {
            // MEG coil location (device coords)
            x = ch.chpos.r0[0];
            y = ch.chpos.r0[1];
            z = ch.chpos.r0[2];

            if (!useDevice) {
                // Transform device -> head
                Vector3f pos(x, y, z);
                MatrixX3f posMat(1, 3);
                posMat.row(0) = pos.transpose();
                MatrixX3f result = info.dev_head_t.apply_trans(posMat);
                x = result(0, 0);
                y = result(0, 1);
                z = result(0, 2);
            }
        } else if (ch.kind == FIFFV_EEG_CH) {
            // EEG electrode location (head coords)
            x = ch.eeg_loc(0, 0);
            y = ch.eeg_loc(1, 0);
            z = ch.eeg_loc(2, 0);

            if (useDevice) {
                // Transform head -> device
                Vector3f pos(x, y, z);
                MatrixX3f posMat(1, 3);
                posMat.row(0) = pos.transpose();
                MatrixX3f result = headToDev.apply_trans(posMat);
                x = result(0, 0);
                y = result(0, 1);
                z = result(0, 2);
            }
        } else {
            continue;
        }

        out << std::format("{:<20} {:12.6f} {:12.6f} {:12.6f}\n", ch.ch_name, sensorQtTie(x, 6), sensorQtTie(y, 6), sensorQtTie(z, 6));
        ++nWritten;
    }

    out.close();
    Log::info("Wrote {} sensor locations to {} (frame: {})", nWritten, outFile, frame);

    return 0;
}
