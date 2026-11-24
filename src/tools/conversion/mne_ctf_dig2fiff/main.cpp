//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2006
 * @brief    Convert CTF Polhemus digitization data to FIFF and hpts formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_coord_trans.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

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
#include <ostream>
#include <print>
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

struct DigPoint
{
    int kind; // FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA, FIFFV_POINT_EEG
    int ident;
    float r[3]; // position in meters
};

//=============================================================================================================

/**
 * @return @p text as a float, or nothing if it is not a number or out of float range.
 */
static std::optional<float> ctfDigToFloat(std::string_view text)
{
    const std::optional<double> value = mne::to_double(text);
    if (!value)
        return std::nullopt;
    if (std::isinf(*value) || std::isnan(*value))
        return static_cast<float>(*value);
    if (std::abs(*value) > static_cast<double>(std::numeric_limits<float>::max()))
        return std::nullopt;
    const float result = static_cast<float>(*value);
    if (*value != 0.0 && result == 0.0f)
        return std::nullopt;
    return result;
}

//=============================================================================================================

/**
 * Read CTF digitization text file.
 * Format: one point per line with x y z coordinates in cm.
 * First three points are fiducials (nasion, left ear, right ear).
 */
static std::vector<DigPoint> readCtfDig(const std::string& filename, bool numFids)
{
    std::vector<DigPoint> points;
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open CTF digitization file: {}", filename);
        return points;
    }

    int lineNo = 0;
    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() < 3)
            continue;

        const std::optional<float> x = ctfDigToFloat(parts[0]);
        const std::optional<float> y = ctfDigToFloat(parts[1]);
        const std::optional<float> z = ctfDigToFloat(parts[2]);
        if (!x || !y || !z)
            continue;

        DigPoint p;
        // Convert from cm to meters
        p.r[0] = *x / 100.0f;
        p.r[1] = *y / 100.0f;
        p.r[2] = *z / 100.0f;

        if (lineNo < 3) {
            // First three points are fiducials
            p.kind = FIFFV_POINT_CARDINAL;
            if (numFids) {
                // 1=nasion, 2=left auricular, 3=right auricular
                p.ident = lineNo + 1;
            } else {
                // Standard order: nasion, left ear, right ear
                if (lineNo == 0)
                    p.ident = FIFFV_POINT_NASION;
                else if (lineNo == 1)
                    p.ident = FIFFV_POINT_LPA;
                else
                    p.ident = FIFFV_POINT_RPA;
            }
        } else {
            p.kind = FIFFV_POINT_EXTRA;
            p.ident = lineNo - 3 + 1;
        }

        points.push_back(p);
        lineNo++;
    }

    return points;
}

//=============================================================================================================

/**
 * Compute head coordinate transform from fiducial points (nasion, LPA, RPA).
 * The Neuromag head coordinate system:
 *   Origin at midpoint between LPA and RPA
 *   X axis toward nasion
 *   Z axis up (y x z = right hand)
 *   Y axis toward LPA
 */
static bool computeHeadTransform(const DigPoint& nasion, const DigPoint& lpa, const DigPoint& rpa,
                                 Matrix4f& trans)
{
    Vector3f n(nasion.r[0], nasion.r[1], nasion.r[2]);
    Vector3f l(lpa.r[0], lpa.r[1], lpa.r[2]);
    Vector3f r(rpa.r[0], rpa.r[1], rpa.r[2]);

    Vector3f origin = 0.5f * (l + r);
    Vector3f ex = (n - origin).normalized();
    Vector3f ez_tmp = (r - l).normalized();
    Vector3f ey = ez_tmp.cross(ex).normalized();
    Vector3f ez = ex.cross(ey).normalized();

    trans = Matrix4f::Identity();
    trans(0, 0) = ex(0);
    trans(0, 1) = ex(1);
    trans(0, 2) = ex(2);
    trans(1, 0) = ey(0);
    trans(1, 1) = ey(1);
    trans(1, 2) = ey(2);
    trans(2, 0) = ez(0);
    trans(2, 1) = ez(1);
    trans(2, 2) = ez(2);
    trans(0, 3) = -ex.dot(origin);
    trans(1, 3) = -ey.dot(origin);
    trans(2, 3) = -ez.dot(origin);

    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert CTF Polhemus digitization data to FIFF and hpts formats.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"ctf"}, "CTF digitization text file.", "name", {}});
    parser.add_option({{"fif"}, "Output FIFF file.", "name", {}});
    parser.add_option({{"hpts"}, "Output hpts file.", "name", {}});
    parser.add_option({{"numfids"}, "Fiducials are numbered (1=nasion, 2=left, 3=right).", {}, {}});
    parser.process(argc, argv, "mne_ctf_dig2fiff", PROGRAM_VERSION);

    const std::string ctfName = parser.value("ctf");
    const std::string fifName = parser.value("fif");
    const std::string hptsName = parser.value("hpts");
    bool numFids = parser.is_set("numfids");

    if (ctfName.empty()) {
        Log::critical("--ctf is required.");
        std::print("{}", parser.help_text("mne_ctf_dig2fiff"));
        return 1;
    }
    if (fifName.empty() && hptsName.empty()) {
        Log::critical("At least one of --fif or --hpts must be specified.");
        std::print("{}", parser.help_text("mne_ctf_dig2fiff"));
        return 1;
    }

    std::vector<DigPoint> points = readCtfDig(ctfName, numFids);
    const auto pointCount = static_cast<long long>(points.size());
    if (points.size() < 3) {
        Log::critical("Need at least 3 fiducial points, got {}", pointCount);
        return 1;
    }

    Log::info("Read {} digitization points from {}", pointCount, ctfName);

    // Compute head coordinate transform from fiducials
    Matrix4f headTrans;
    computeHeadTransform(points[0], points[1], points[2], headTrans);

    // Transform all points to head coordinates
    for (DigPoint& p : points) {
        Vector3f r(p.r[0], p.r[1], p.r[2]);
        Vector3f rt = headTrans.block<3, 3>(0, 0) * r + headTrans.block<3, 1>(0, 3);
        p.r[0] = rt(0);
        p.r[1] = rt(1);
        p.r[2] = rt(2);
    }

    // Write FIFF output
    if (!fifName.empty()) {
        auto outStream = FiffStream::start_file(mne::path_from_utf8(fifName));
        if (!outStream) {
            Log::critical("Cannot open output file: {}", fifName);
            return 1;
        }

        outStream->start_block(FIFFB_ISOTRAK);
        for (const DigPoint& p : points) {
            FiffDigPoint dp;
            dp.kind = p.kind;
            dp.ident = p.ident;
            dp.r[0] = p.r[0];
            dp.r[1] = p.r[1];
            dp.r[2] = p.r[2];
            dp.coord_frame = FIFFV_COORD_HEAD;
            outStream->write_dig_point(dp);
        }
        outStream->end_block(FIFFB_ISOTRAK);
        outStream->end_file();
        Log::info("Wrote {} points to {}", pointCount, fifName);
    }

    // Write hpts output
    if (!hptsName.empty()) {
        std::ofstream out(mne::path_from_utf8(hptsName));
        if (!out) {
            Log::critical("Cannot open hpts file: {}", hptsName);
            return 1;
        }
        for (const DigPoint& p : points) {
            std::string cat;
            if (p.kind == FIFFV_POINT_CARDINAL)
                cat = "cardinal";
            else if (p.kind == FIFFV_POINT_HPI)
                cat = "hpi";
            else if (p.kind == FIFFV_POINT_EEG)
                cat = "eeg";
            else
                cat = "extra";

            std::print(out, "{} {} {} {} {}\n", cat, p.ident,
                       mne::format_number(static_cast<double>(1000.0f * p.r[0]), 'f', 1),
                       mne::format_number(static_cast<double>(1000.0f * p.r[1]), 'f', 1),
                       mne::format_number(static_cast<double>(1000.0f * p.r[2]), 'f', 1));
        }
        out.close();
        Log::info("Wrote {} points to {}", pointCount, hptsName);
    }

    return 0;
}
