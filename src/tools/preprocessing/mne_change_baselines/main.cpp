//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     2003
 * @brief    Change baseline in evoked data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Change baseline in evoked data.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Input evoked FIFF file.", "file", {}});
    parser.add_option({{"bmin"}, "Baseline start in seconds.", "seconds", {}});
    parser.add_option({{"bmax"}, "Baseline end in seconds.", "seconds", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_change_baselines", PROGRAM_VERSION);

    const std::string measFile = parser.value("meas");
    const std::string outFile = parser.value("out");

    if (measFile.empty()) {
        Log::critical("--meas is required.");
        return 1;
    }
    if (!parser.is_set("bmin")) {
        Log::critical("--bmin is required.");
        return 1;
    }
    if (!parser.is_set("bmax")) {
        Log::critical("--bmax is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    float bmin = toFloat(parser.value("bmin"));
    float bmax = toFloat(parser.value("bmax"));

    // Read evoked data without baseline correction
    auto read = FiffEvokedSet::read(path_from_utf8(measFile), {}, {}, false);
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    FiffEvokedSet& evokedSet = *read;

    Log::info("Read {} evoked data set(s) from {}", static_cast<long long>(evokedSet.evoked.size()), measFile);
    Log::info("Applying baseline correction [{:g}, {:g}] s", bmin, bmax);

    // Apply baseline correction to each evoked dataset
    std::pair<float, float> baseline(bmin, bmax);
    for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
        evokedSet.evoked[i].applyBaselineCorrection(baseline);
        Log::info("  Set {} ({}): baseline corrected", i, evokedSet.evoked[i].comment);
    }

    // Save
    if (!evokedSet.save(path_from_utf8(outFile))) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }

    Log::info("Written baseline-corrected evoked data to: {}", outFile);
    return 0;
}
