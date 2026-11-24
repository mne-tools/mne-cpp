//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March, 2026
 * @brief    Implements mne_create_comp_data: create CTF compensation data from ASCII matrices.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_ctf_comp.h>
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
#include <cstddef>
#include <cstdlib>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
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

// Parses a matrix entry as QString::toDouble: an infinity when it overflows, 0 when it is no number
static double matrixValue(std::string_view text)
{
    if (const std::optional<double> value = to_double(text)) {
        return *value;
    }
    // to_double rejects a number beyond the double range, for which QString returns an infinity
    const std::string number(trimmed(text));
    char* end = nullptr;
    const double parsed = std::strtod(number.c_str(), &end);
    return !number.empty() && end == number.c_str() + number.size() && std::isinf(parsed) ? parsed : 0.0;
}

//=============================================================================================================

// Splits a trimmed line at whitespace; an empty line gives one empty field, as QString::split does
static StringList splitFields(std::string_view line)
{
    StringList fields;
    for (const std::string_view field : split_whitespace(line))
        fields.emplace_back(field);
    if (fields.empty())
        fields.emplace_back();
    return fields;
}

//=============================================================================================================

static bool readAsciiMatrix(const std::string& filename,
                            MatrixXd& data,
                            StringList& rowNames,
                            StringList& colNames)
{
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open matrix file: {}", filename);
        return false;
    }
    StringList lines;
    for (std::string line; std::getline(file, line);)
        lines.push_back(std::move(line));
    std::size_t next = 0;
    const auto readLine = [&]() -> std::string_view {
        return next < lines.size() ? trimmed(lines[next++]) : std::string_view();
    };

    // First line: nrow ncol
    int nrow = 0, ncol = 0;
    const StringList dimParts = splitFields(readLine());
    if (dimParts.size() >= 2) {
        nrow = to_int(dimParts[0]).value_or(0);
        ncol = to_int(dimParts[1]).value_or(0);
    }
    if (nrow <= 0 || ncol <= 0) {
        Log::critical("Invalid dimensions in matrix file: {} x {}", nrow, ncol);
        return false;
    }

    // Second line: column names
    colNames = splitFields(readLine());
    while (std::ssize(colNames) < ncol) {
        colNames.push_back(std::format("col{}", colNames.size() + 1));
    }
    if (std::ssize(colNames) > ncol)
        colNames.resize(static_cast<std::size_t>(ncol));

    // Data lines: row_name val1 val2 ...
    data.resize(nrow, ncol);
    rowNames.clear();
    for (int r = 0; r < nrow; r++) {
        if (next >= lines.size()) {
            Log::critical("Unexpected end of matrix file at row {}", r);
            return false;
        }
        const StringList parts = splitFields(readLine());
        if (std::ssize(parts) >= ncol + 1) {
            rowNames.push_back(parts[0]);
            for (int c = 0; c < ncol; c++) {
                data(r, c) = matrixValue(parts[c + 1]);
            }
        } else if (std::ssize(parts) >= ncol) {
            // No row name, just values
            rowNames.push_back(std::format("row{}", r + 1));
            for (int c = 0; c < ncol; c++) {
                data(r, c) = matrixValue(parts[c]);
            }
        }
    }

    Log::info("Read matrix: {} x {}", nrow, ncol);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Create CTF compensation data from ASCII matrix files.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"in"}, "Input ASCII matrix file.", "file", {}});
    parser.add_option({{"kind"}, "CTF compensation kind (e.g. 101, 201, 301).", "kind", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.add_option({{"calibrated"}, "Matrix data is already calibrated.", {}, {}});
    parser.process(argc, argv, "mne_create_comp_data", PROGRAM_VERSION);

    const std::string inFile = parser.value("in");
    const std::string outFile = parser.value("out");
    int ctfKind = to_int(parser.value("kind")).value_or(0);
    bool calibrated = parser.is_set("calibrated");

    if (inFile.empty()) {
        Log::critical("Input matrix file (--in) is required.");
        return 1;
    }
    if (!parser.is_set("kind")) {
        Log::critical("Compensation kind (--kind) is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    //
    // Read the ASCII matrix
    //
    MatrixXd data;
    StringList rowNames, colNames;
    if (!readAsciiMatrix(inFile, data, rowNames, colNames))
        return 1;

    int nrow = static_cast<int>(data.rows());
    int ncol = static_cast<int>(data.cols());

    //
    // Write FIFF output with CTF compensation data
    //
    auto stream = FiffStream::start_file(path_from_utf8(outFile));
    if (!stream) {
        Log::critical("Cannot create output FIFF file.");
        return 1;
    }

    // CTF compensation block
    stream->start_block(FIFFB_MNE_CTF_COMP);
    stream->start_block(FIFFB_MNE_CTF_COMP_DATA);

    // Compensation kind
    fiff_int_t kind = ctfKind;
    stream->write_int(FIFF_MNE_CTF_COMP_KIND, &kind);

    // Calibrated flag
    fiff_int_t cal = calibrated ? 1 : 0;
    stream->write_int(FIFF_MNE_CTF_COMP_CALIBRATED, &cal);

    // Write the named matrix
    FiffNamedMatrix namedMat(nrow, ncol, rowNames, colNames, data);
    stream->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, namedMat);

    stream->end_block(FIFFB_MNE_CTF_COMP_DATA);
    stream->end_block(FIFFB_MNE_CTF_COMP);
    stream->end_file();

    Log::info("Wrote CTF compensation data (kind={}, {}x{}, {}) to {}", ctfKind, nrow, ncol, calibrated ? "calibrated" : "uncalibrated", outFile);

    return 0;
}
