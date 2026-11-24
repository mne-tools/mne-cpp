//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2004
 * @brief    Implements mne_volume_data2mri: convert source estimates to MRI overlay volumes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne.h>
#include <mne/mne/mne_source_space.h>
#include <mne/base/byte_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static bool readWFile(const std::string& filename, VectorXd& data, VectorXi& vertices)
{
    const std::unique_ptr<FileByteStream> file = FileByteStream::open(path_from_utf8(filename), FileByteStream::Mode::Read);
    if (!file) {
        Log::critical("Cannot open w file: {}", filename);
        return false;
    }

    // Like a QDataStream: big endian, and once a read runs past the end every value reads as zero
    bool pastEnd = false;
    auto read = [&file, &pastEnd]<class T>(T& value) {
        if (pastEnd || !file->read_big_endian(value)) {
            pastEnd = true;
            value = T{};
        }
    };

    // Skip 2-byte header
    std::uint16_t header = 0;
    read(header);

    // Number of vertices (3 bytes)
    std::uint8_t b1 = 0;
    std::uint8_t b2 = 0;
    std::uint8_t b3 = 0;
    read(b1);
    read(b2);
    read(b3);
    std::int32_t nvert = (b1 << 16) | (b2 << 8) | b3;

    vertices.resize(nvert);
    data.resize(nvert);

    for (int i = 0; i < nvert; i++) {
        // Vertex number (3 bytes)
        read(b1);
        read(b2);
        read(b3);
        vertices(i) = (b1 << 16) | (b2 << 8) | b3;

        // Value: a 4-byte float
        float val = 0.0f;
        read(val);
        data(i) = val;
    }
    return true;
}

//=============================================================================================================

static bool readStcFile(const std::string& filename, MatrixXd& data, VectorXi& vertices,
                        float& tmin, float& tstep)
{
    auto read = InvSourceEstimate::read(path_from_utf8(filename));
    if (!read) {
        Log::critical("{}", read.error().what());
        return false;
    }
    const InvSourceEstimate& stc = *read;

    data = stc.data;
    vertices = stc.vertices;
    tmin = stc.tmin;
    tstep = stc.tstep;
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Map source estimate data to MRI vertex space.\n\nReads volume source space and STC/W estimates.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"src"}, "Source space FIFF file.", "file", {}});
    parser.add_option({{"stc"}, "STC source estimate file.", "file", {}});
    parser.add_option({{"w"}, "W source estimate file.", "file", {}});
    parser.add_option({{"out"}, "Output MRI file (mgh format).", "file", {}});
    parser.add_option({{"tpoint"}, "Time point index for STC files.", "index", "0"});

    parser.process(argc, argv, "mne_volume_data2mri", PROGRAM_VERSION);

    const std::string srcFile = parser.value("src");
    const std::string stcFile = parser.value("stc");
    const std::string wFile = parser.value("w");
    const std::string outFile = parser.value("out");
    const int tpoint = to_int(parser.value("tpoint")).value_or(0);

    if (srcFile.empty()) {
        Log::critical("Source space file (--src) is required.");
        return 1;
    }
    if (stcFile.empty() && wFile.empty()) {
        Log::info("Either --stc or --w must be specified.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    //
    // Read source space
    //
    std::vector<std::unique_ptr<MNESourceSpace>> spaces;
    if (MNESourceSpace::read_source_spaces(path_from_utf8(srcFile), spaces) != FIFF_OK || spaces.empty()) {
        Log::critical("Failed to read source space from: {}", srcFile);
        return 1;
    }

    Log::info("Read {} source spaces", static_cast<long long>(spaces.size()));

    //
    // Read source estimate data
    //
    VectorXd values;
    VectorXi vertices;

    if (!stcFile.empty()) {
        MatrixXd stcData;
        float tmin;
        float tstep;
        if (!readStcFile(stcFile, stcData, vertices, tmin, tstep)) {
            Log::critical("Failed to read stc file.");
            return 1;
        }
        if (tpoint < 0 || tpoint >= static_cast<int>(stcData.cols())) {
            Log::info("Time point {} out of range (0-{})", tpoint, static_cast<long>(stcData.cols() - 1));
            return 1;
        }
        values = stcData.col(tpoint);
        Log::info("Read STC: {} vertices, {} time points, using tpoint {}", static_cast<long>(stcData.rows()), static_cast<long>(stcData.cols()), tpoint);
    } else {
        if (!readWFile(wFile, values, vertices)) {
            Log::critical("Failed to read w file.");
            return 1;
        }
        Log::info("Read W: {} vertices", static_cast<long>(values.size()));
    }

    //
    // For volume source spaces, map source data to vertex space
    //
    bool found = false;
    int offset = 0;
    for (size_t s = 0; s < spaces.size(); s++) {
        const auto& sp = *spaces[s];

        // Build vertex value map for this source space
        VectorXd vertValues = VectorXd::Zero(sp.np);

        for (int i = 0; i < vertices.size(); i++) {
            int vi = vertices(i) - offset;
            if (vi >= 0 && vi < sp.np)
                vertValues(vi) = values(i);
        }
        offset += sp.nuse;

        // Check for non-zero values
        int nonZero = static_cast<int>((vertValues.array() != 0.0).count());
        if (nonZero == 0)
            continue;

        found = true;

        // Write vertex values as text (vertex index and value)
        std::ofstream out(path_from_utf8(outFile));
        if (!out) {
            Log::critical("Cannot write output file: {}", outFile);
            return 1;
        }
        out << "# MNE volume data overlay\n";
        out << "# Source space: " << srcFile << "\n";
        out << "# Vertices: " << sp.np << "\n";
        out << "# Non-zero: " << nonZero << "\n";
        for (int i = 0; i < sp.np; i++) {
            if (vertValues(i) != 0.0) {
                out << std::format("{} {:g} {:g} {:g} {:g}\n", i, sp.rr(i, 0), sp.rr(i, 1), sp.rr(i, 2), vertValues(i));
            }
        }
        out.close();
        Log::info("Wrote {} non-zero vertex values to {}", nonZero, outFile);
        break;
    }

    if (!found) {
        Log::info("No matching source data found in any source space.");
        return 1;
    }

    return 0;
}
