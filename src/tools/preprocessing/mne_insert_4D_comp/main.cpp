//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Implements mne_insert_4D_comp: merge 4D Magnes compensation channels with Procrustes alignment.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <limits>
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

// Parses a number as QString::toDouble: an infinity when it overflows, 0 with @p ok false when it is no number
static double toDouble(std::string_view text, bool& ok)
{
    if (const std::optional<double> value = to_double(text)) {
        ok = true;
        return *value;
    }
    ok = false;
    // to_double rejects a number beyond the double range, for which QString returns an infinity
    const std::string number(trimmed(text));
    char* end = nullptr;
    const double parsed = std::strtod(number.c_str(), &end);
    return !number.empty() && end == number.c_str() + number.size() && std::isinf(parsed) ? parsed : 0.0;
}

//=============================================================================================================

// Parses a number as QString::toFloat: an infinity when it overflows the float range, a signed 0 when it underflows
static float toFloat(std::string_view text, bool& ok)
{
    const double value = toDouble(text, ok);
    if (std::isfinite(value) && std::abs(value) > std::numeric_limits<float>::max()) {
        ok = false;
        return value > 0 ? std::numeric_limits<float>::infinity() : -std::numeric_limits<float>::infinity();
    }
    const auto single = static_cast<float>(value);
    if (value != 0.0 && single == 0.0f) {
        ok = false;
    }
    return single;
}

//=============================================================================================================

static float toFloat(std::string_view text)
{
    bool ok = false;
    return toFloat(text, ok);
}

//=============================================================================================================

static bool readSensorPositions(const std::string& filename, MatrixX3f& pos, StringList& names)
{
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open sensor position file: {}", filename);
        return false;
    }
    std::vector<Vector3f> posVec;
    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() >= 4) {
            names.emplace_back(parts[0]);
            // Convert from mm to meters
            posVec.emplace_back(toFloat(parts[1]) * 0.001f,
                                toFloat(parts[2]) * 0.001f,
                                toFloat(parts[3]) * 0.001f);
        }
    }
    pos.resize(std::ssize(posVec), 3);
    for (int i = 0; i < std::ssize(posVec); i++)
        pos.row(i) = posVec[i].transpose();
    return true;
}

//=============================================================================================================

static Matrix4f procrustes(const MatrixX3f& source, const MatrixX3f& target)
{
    // Procrustes alignment: find rotation + translation to map source onto target
    // Both matrices should have the same number of rows (corresponding points)

    // Centroids
    Vector3f centS = source.colwise().mean();
    Vector3f centT = target.colwise().mean();

    // Center the point sets
    MatrixX3f srcCentered = source.rowwise() - centS.transpose();
    MatrixX3f tgtCentered = target.rowwise() - centT.transpose();

    // Compute the cross-covariance matrix
    Matrix3f H = srcCentered.transpose() * tgtCentered;

    // SVD
    JacobiSVD<Matrix3f> svd(H, ComputeFullU | ComputeFullV);
    Matrix3f U = svd.matrixU();
    Matrix3f V = svd.matrixV();

    // Handle reflection
    float d = (V * U.transpose()).determinant();
    Matrix3f S = Matrix3f::Identity();
    if (d < 0)
        S(2, 2) = -1.0f;

    // Rotation
    Matrix3f R = V * S * U.transpose();

    // Translation
    Vector3f t = centT - R * centS;

    // Build 4x4 transform
    Matrix4f T = Matrix4f::Identity();
    T.block<3, 3>(0, 0) = R;
    T.block<3, 1>(0, 3) = t;
    return T;
}

//=============================================================================================================

static bool readRefData(const std::string& filename, MatrixXf& refData, int& nChannels, int& nSamples)
{
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open reference data file: {}", filename);
        return false;
    }

    // First pass: count dimensions
    std::vector<std::vector<float>> rows;
    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;
        std::vector<float> row;
        for (const std::string_view p : split_whitespace(line)) {
            bool ok = false;
            const float val = toFloat(p, ok);
            if (ok)
                row.push_back(val);
        }
        if (!row.empty())
            rows.push_back(std::move(row));
    }

    if (rows.empty()) {
        Log::critical("No data in reference file");
        return false;
    }

    nSamples = static_cast<int>(rows.size());
    nChannels = static_cast<int>(rows[0].size());

    refData.resize(nChannels, nSamples);
    for (int s = 0; s < nSamples; s++) {
        for (int c = 0; c < std::min(nChannels, static_cast<int>(rows[s].size())); c++) {
            refData(c, s) = rows[s][c];
        }
    }

    Log::info("Read reference data: {} channels x {} samples", nChannels, nSamples);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Merge 4D Magnes WH3600 compensation data with a FIFF file.\n\nSupports Procrustes alignment of sensor positions.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"in"}, "Input FIFF file with MEG data.", "file", {}});
    parser.add_option({{"ref"}, "Reference channel data file (text).", "file", {}});
    parser.add_option({{"refpos"}, "Reference sensor positions file.", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.add_option({{"megpos"}, "MEG sensor positions file (for alignment).", "file", {}});
    parser.process(argc, argv, "mne_insert_4D_comp", PROGRAM_VERSION);

    const std::string inFile = parser.value("in");
    const std::string refFile = parser.value("ref");
    const std::string refPosFile = parser.value("refpos");
    const std::string megPosFile = parser.value("megpos");
    const std::string outFile = parser.value("out");

    if (inFile.empty() || refFile.empty() || outFile.empty()) {
        Log::info("Required: --in, --ref, --out");
        return 1;
    }

    //
    // Read input FIFF
    //
    FiffRawData raw(path_from_utf8(inFile));
    if (!raw.info.empty()) {
        Log::info("Read input: {} channels at {:.1f} Hz", raw.info.nchan, raw.info.sfreq);
    } else {
        Log::critical("Failed to read input FIFF: {}", inFile);
        return 1;
    }

    //
    // Read reference data
    //
    MatrixXf refData;
    int nRefCh = 0, nRefSamp = 0;
    if (!readRefData(refFile, refData, nRefCh, nRefSamp))
        return 1;

    //
    // Read sensor positions for alignment (optional)
    //
    Matrix4f alignTransform = Matrix4f::Identity();
    if (!refPosFile.empty() && !megPosFile.empty()) {
        MatrixX3f refPos, megPos;
        StringList refNames, megNames;
        readSensorPositions(refPosFile, refPos, refNames);
        readSensorPositions(megPosFile, megPos, megNames);

        // Use matching positions for Procrustes alignment
        int nMatch = std::min(static_cast<int>(refPos.rows()),
                              static_cast<int>(megPos.rows()));
        if (nMatch >= 3) {
            alignTransform = procrustes(refPos.topRows(nMatch), megPos.topRows(nMatch));
            Log::info("Computed Procrustes alignment from {} matching points", nMatch);
        }
    }

    //
    // Create reference channel info
    //
    std::vector<FiffChInfo> refChs;
    for (int k = 0; k < nRefCh; k++) {
        FiffChInfo ch;
        ch.scanNo = raw.info.nchan + k + 1;
        ch.logNo = k + 1;
        ch.kind = FIFFV_REF_MEG_CH;
        ch.range = 1.0f;
        ch.cal = 1.0f;
        ch.chpos.coil_type = 4003; // FIFFV_COIL_MAGNES_REF_MAG
        ch.unit = FIFF_UNIT_T;
        ch.unit_mul = FIFF_UNITM_NONE;
        ch.ch_name = std::format("REF{:03}", k + 1);
        refChs.push_back(ch);
    }

    //
    // Write output FIFF with merged channels
    //
    auto outStream = FiffStream::start_file(path_from_utf8(outFile));
    if (!outStream) {
        Log::critical("Cannot create output file.");
        return 1;
    }

    // Write measurement info with original + reference channels
    int totalCh = raw.info.nchan + nRefCh;

    outStream->start_block(FIFFB_MEAS);
    outStream->start_block(FIFFB_MEAS_INFO);

    fiff_int_t nch = totalCh;
    outStream->write_int(FIFF_NCHAN, &nch);
    float sfreq = raw.info.sfreq;
    outStream->write_float(FIFF_SFREQ, &sfreq);

    // Write original channel info
    for (int k = 0; k < raw.info.nchan; k++) {
        outStream->write_ch_info(raw.info.chs[k]);
    }

    // Write reference channel info
    for (int k = 0; k < nRefCh; k++) {
        outStream->write_ch_info(refChs[k]);
    }

    // Copy digitization data if present
    if (!raw.info.dig.empty()) {
        outStream->start_block(FIFFB_ISOTRAK);
        for (int k = 0; k < std::ssize(raw.info.dig); k++) {
            outStream->write_dig_point(raw.info.dig[k]);
        }
        outStream->end_block(FIFFB_ISOTRAK);
    }

    // Write coordinate transforms if present
    if (raw.info.dev_head_t.from != -1) {
        outStream->write_coord_trans(raw.info.dev_head_t);
    }

    outStream->end_block(FIFFB_MEAS_INFO);

    // Write raw data with merged reference channels
    outStream->start_block(FIFFB_RAW_DATA);

    fiff_int_t firstSample = raw.first_samp;
    outStream->write_int(FIFF_FIRST_SAMPLE, &firstSample);
    RowVectorXd cals(totalCh);
    for (int channel = 0; channel < raw.info.nchan; ++channel) {
        cals[channel] = static_cast<double>(raw.info.chs[channel].range) * raw.info.chs[channel].cal;
    }
    cals.tail(nRefCh).setOnes();

    int bufSize = static_cast<int>(10.0f * raw.info.sfreq);
    MatrixXd readData;
    MatrixXd times;

    for (int start = raw.first_samp; start <= raw.last_samp; start += bufSize) {
        int end = std::min(start + bufSize - 1, static_cast<int>(raw.last_samp));
        if (!raw.read_raw_segment(readData, times, start, end))
            break;

        int nSamp = static_cast<int>(readData.cols());

        // Build merged data matrix
        MatrixXf merged(totalCh, nSamp);
        merged.topRows(raw.info.nchan) = readData.cast<float>();

        // Insert reference data (aligned to this segment)
        int refStart = start - static_cast<int>(raw.first_samp);
        for (int c = 0; c < nRefCh; c++) {
            for (int s = 0; s < nSamp; s++) {
                int refIdx = refStart + s;
                if (refIdx >= 0 && refIdx < nRefSamp)
                    merged(raw.info.nchan + c, s) = refData(c, refIdx);
                else
                    merged(raw.info.nchan + c, s) = 0.0f;
            }
        }

        outStream->write_raw_buffer(merged.cast<double>(), cals);
    }

    outStream->end_block(FIFFB_RAW_DATA);
    outStream->end_block(FIFFB_MEAS);
    outStream->end_file();

    Log::info("Wrote merged FIFF: {} channels ({} original + {} reference) to {}", totalCh, raw.info.nchan, nRefCh, outFile);

    return 0;
}
