//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2005
 * @brief    Create topographical EEG layout file from electrode positions in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
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

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <print>
#include <string>
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

/**
 * Azimuthal equidistant projection from 3D electrode positions on a sphere.
 * Projects to a 2D plane preserving angles from the center.
 */
static void azimuthalProjection(const Vector3f& pos, const Vector3f& center,
                                [[maybe_unused]] float radius, float& x, float& y)
{
    Vector3f d = pos - center;
    d.normalize();

    // Convert to spherical
    float theta = acos(std::max(-1.0f, std::min(1.0f, d(2)))); // polar angle from z-axis
    float phi = atan2(d(1), d(0));                             // azimuthal angle

    // Azimuthal equidistant projection
    float r = theta;
    x = r * std::cos(static_cast<double>(phi));
    y = r * std::sin(static_cast<double>(phi));
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Create a topographical EEG layout file from electrode positions in a FIFF file.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"fif"}, "FIFF file with EEG channel info.", "name", {}});
    parser.add_option({{"out"}, "Output layout file (.lout).", "name", {}});
    parser.add_option({{"width"}, "Channel box width.", "val", "5.0"});
    parser.add_option({{"height"}, "Channel box height.", "val", "4.0"});
    parser.process(argc, argv, "mne_make_eeg_layout", PROGRAM_VERSION);

    const std::string fifName = parser.value("fif");
    const std::string outName = parser.value("out");
    float width = static_cast<float>(to_double(parser.value("width")).value_or(0.0));
    float height = static_cast<float>(to_double(parser.value("height")).value_or(0.0));

    if (fifName.empty() || outName.empty()) {
        Log::critical("Both --fif and --out are required.");
        std::print("{}", parser.help_text("mne_make_eeg_layout"));
        return 1;
    }

    // Read the FIFF file to get channel info
    FiffRawData raw(path_from_utf8(fifName));
    if (raw.info.empty()) {
        Log::critical("Cannot read FIFF file: {}", fifName);
        return 1;
    }

    // Collect EEG channel positions
    struct EegCh
    {
        std::string name;
        Vector3f pos;
    };
    std::vector<EegCh> eegChannels;

    for (int i = 0; i < raw.info.nchan; i++) {
        if (raw.info.chs[i].kind == FIFFV_EEG_CH) {
            EegCh ch;
            ch.name = raw.info.chs[i].ch_name;
            ch.pos = Vector3f(raw.info.chs[i].chpos.r0(0),
                              raw.info.chs[i].chpos.r0(1),
                              raw.info.chs[i].chpos.r0(2));
            eegChannels.push_back(ch);
        }
    }

    if (eegChannels.empty()) {
        Log::critical("No EEG channels found in {}", fifName);
        return 1;
    }
    const auto eegChannelCount = static_cast<long long>(eegChannels.size());

    Log::info("{} EEG channels found.", eegChannelCount);

    // Compute center of mass for a simple sphere fit
    Vector3f center = Vector3f::Zero();
    for (const EegCh& ch : eegChannels) {
        center += ch.pos;
    }
    center /= static_cast<float>(eegChannels.size());

    // Compute average radius
    float radius = 0.0f;
    for (const EegCh& ch : eegChannels) {
        radius += (ch.pos - center).norm();
    }
    radius /= static_cast<float>(eegChannels.size());

    Log::info("Sphere center: {:.3f} {:.3f} {:.3f} m, radius: {:.3f} m", center(0), center(1), center(2), radius);

    // Project to 2D
    std::vector<float> xs, ys;
    for (const EegCh& ch : eegChannels) {
        float x, y;
        azimuthalProjection(ch.pos, center, radius, x, y);
        xs.push_back(x);
        ys.push_back(y);
    }

    // Normalize to layout coordinates
    float xmin = *std::min_element(xs.begin(), xs.end());
    float xmax = *std::max_element(xs.begin(), xs.end());
    float ymin = *std::min_element(ys.begin(), ys.end());
    float ymax = *std::max_element(ys.begin(), ys.end());

    float xrange = xmax - xmin;
    float yrange = ymax - ymin;
    if (xrange < 1e-10f)
        xrange = 1.0f;
    if (yrange < 1e-10f)
        yrange = 1.0f;

    // Write layout file
    std::ofstream out(path_from_utf8(outName));
    if (!out) {
        Log::critical("Cannot open output file: {}", outName);
        return 1;
    }

    for (std::size_t i = 0; i < eegChannels.size(); i++) {
        // Normalize to 0..100 range and center
        float nx = 5.0f + 90.0f * (xs[i] - xmin) / xrange;
        float ny = 5.0f + 90.0f * (ys[i] - ymin) / yrange;

        // Layout format: channel_number x y width height name
        out << std::format("{} {:.2f} {:.2f} {:.2f} {:.2f} {}\n", i + 1, mne::tie_away_from_zero(nx, 2), mne::tie_away_from_zero(ny, 2), mne::tie_away_from_zero(width, 2), mne::tie_away_from_zero(height, 2), eegChannels[i].name);
    }

    out.close();
    Log::info("Layout written to {} ({} channels)", outName, eegChannelCount);

    return 0;
}
