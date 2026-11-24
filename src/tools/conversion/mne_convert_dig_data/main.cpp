//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Convert Polhemus digitization data between FIFF, hpts, and elp formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>

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
#include <cstdlib>
#include <format>
#include <fstream>
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

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct DigPt
{
    int kind;
    int ident;
    float r[3]; // meters
};

//=============================================================================================================

/**
 * @param[in] value       Any value.
 * @param[in] format      'f', 'e' or 'g' as in QString::number.
 * @param[in] precision   Decimals for 'f' and 'e', significant digits for 'g'.
 *
 * @return @p value formatted like QString::number(value, format, precision), which rounds ties away from zero
 *         where std::format rounds them to even.
 */
static std::string digQtNumber(double value, char format, int precision)
{
    if (std::isnan(value))
        return "nan";
    if (std::isinf(value))
        return value < 0 ? "-inf" : "inf";
    const double magnitude = std::abs(value);
    // Decimal digits d0 d1 d2 ... of magnitude = d0.d1d2... * 10^exponent
    const auto expand = [magnitude](int decimals, std::string& digits, int& exponent) {
        const std::string text = std::format("{:.{}e}", magnitude, decimals);
        const std::size_t ePos = text.find('e');
        digits = text.substr(0, 1) + text.substr(2, ePos - 2);
        exponent = std::stoi(text.substr(ePos + 1));
    };
    std::string digits;
    int exponent = 0;
    expand(17, digits, exponent);
    const int significant = format == 'e' ? precision + 1 : format == 'g' ? std::max(precision, 1)
                                                                          : exponent + 1 + precision;
    expand(std::max(significant, 0) + 40, digits, exponent);
    const std::string_view tail = std::string_view(digits).substr(static_cast<std::size_t>(std::max(significant, 0)) + 1);
    if (tail.find_first_not_of('0') == std::string_view::npos || tail.find_first_not_of('9') == std::string_view::npos)
        expand(std::max(significant, 0) + 1100, digits, exponent); // exact for every double

    std::string rounded;
    if (significant >= 0) {
        rounded = digits.substr(0, static_cast<std::size_t>(significant));
        if (digits[static_cast<std::size_t>(significant)] >= '5') {
            int i = significant - 1;
            while (i >= 0 && rounded[static_cast<std::size_t>(i)] == '9')
                rounded[static_cast<std::size_t>(i--)] = '0';
            if (i >= 0) {
                ++rounded[static_cast<std::size_t>(i)];
            } else {
                rounded.insert(rounded.begin(), '1');
                ++exponent;
                if (format != 'f')
                    rounded.pop_back();
            }
        }
    }
    const bool zero = rounded.find_first_not_of('0') == std::string::npos;
    if (zero)
        exponent = 0;

    const auto fixed = [&](int decimals) {
        std::string intPart;
        std::string fraction;
        if (zero) {
            intPart = "0";
            fraction.assign(static_cast<std::size_t>(decimals), '0');
        } else if (exponent + 1 <= 0) {
            intPart = "0";
            fraction = std::string(static_cast<std::size_t>(-(exponent + 1)), '0') + rounded;
        } else {
            const std::size_t intDigits = static_cast<std::size_t>(exponent) + 1;
            intPart = rounded.substr(0, intDigits);
            fraction = rounded.substr(intDigits);
        }
        fraction.resize(static_cast<std::size_t>(decimals), '0');
        return decimals > 0 ? intPart + "." + fraction : intPart;
    };
    const auto scientific = [&](std::string mantissaDigits) {
        if (mantissaDigits.empty())
            mantissaDigits = "0";
        std::string text(1, mantissaDigits[0]);
        if (mantissaDigits.size() > 1)
            text += "." + mantissaDigits.substr(1);
        return text + std::format("e{}{:02}", exponent < 0 ? '-' : '+', std::abs(exponent));
    };

    std::string text;
    if (format == 'f') {
        text = fixed(precision);
    } else if (format == 'e') {
        text = scientific(zero ? std::string(static_cast<std::size_t>(precision + 1), '0') : rounded);
    } else {
        const int digitsWanted = std::max(precision, 1);
        if (!zero && (exponent < -4 || exponent >= digitsWanted)) {
            std::string mantissa = rounded;
            mantissa.erase(mantissa.find_last_not_of('0') + 1);
            text = scientific(mantissa);
        } else {
            text = fixed(std::max(digitsWanted - 1 - exponent, 0));
            if (text.find('.') != std::string::npos) {
                text.erase(text.find_last_not_of('0') + 1);
                if (text.back() == '.')
                    text.pop_back();
            }
        }
    }
    return value < 0 ? "-" + text : text;
}

//=============================================================================================================

static std::vector<DigPt> readFifDig(const std::string& filename)
{
    std::vector<DigPt> points;
    FiffStream stream(mne::path_from_utf8(filename));
    if (!stream.open()) {
        Log::critical("Cannot open FIFF file: {}", filename);
        return points;
    }

    for (int k = 0; k < stream.nent(); k++) {
        if (stream.dir()[k].kind == FIFF_DIG_POINT) {
            std::optional<FiffTag> tag;
            tag = stream.read_tag(stream.dir()[k].pos);
            FiffDigPoint dp = tag->to_dig_point();
            DigPt p;
            p.kind = dp.kind;
            p.ident = dp.ident;
            p.r[0] = dp.r[0];
            p.r[1] = dp.r[1];
            p.r[2] = dp.r[2];
            points.push_back(p);
        }
    }
    stream.close();
    return points;
}

//=============================================================================================================

static std::vector<DigPt> readHpts(const std::string& filename)
{
    std::vector<DigPt> points;
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open hpts file: {}", filename);
        return points;
    }

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() < 5)
            continue;

        DigPt p;
        const std::string cat = mne::to_lower(parts[0]);
        if (cat == "cardinal" || cat == "fiducial")
            p.kind = FIFFV_POINT_CARDINAL;
        else if (cat == "hpi")
            p.kind = FIFFV_POINT_HPI;
        else if (cat == "eeg")
            p.kind = FIFFV_POINT_EEG;
        else
            p.kind = FIFFV_POINT_EXTRA;

        p.ident = mne::to_int(parts[1]).value_or(0);
        // hpts coordinates are in mm
        p.r[0] = static_cast<float>(mne::to_double(parts[2]).value_or(0.0)) / 1000.0f;
        p.r[1] = static_cast<float>(mne::to_double(parts[3]).value_or(0.0)) / 1000.0f;
        p.r[2] = static_cast<float>(mne::to_double(parts[4]).value_or(0.0)) / 1000.0f;

        points.push_back(p);
    }
    return points;
}

//=============================================================================================================

static bool writeFifDig(const std::string& filename, const std::vector<DigPt>& points)
{
    auto outStream = FiffStream::start_file(mne::path_from_utf8(filename));
    if (!outStream) {
        Log::critical("Cannot open output FIFF file: {}", filename);
        return false;
    }

    outStream->start_block(FIFFB_ISOTRAK);
    for (const DigPt& p : points) {
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

    return true;
}

//=============================================================================================================

static bool writeHpts(const std::string& filename, const std::vector<DigPt>& points)
{
    std::ofstream out(mne::path_from_utf8(filename));
    if (!out) {
        Log::critical("Cannot open hpts output file: {}", filename);
        return false;
    }

    for (const DigPt& p : points) {
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
                   digQtNumber(static_cast<double>(1000.0f * p.r[0]), 'f', 1),
                   digQtNumber(static_cast<double>(1000.0f * p.r[1]), 'f', 1),
                   digQtNumber(static_cast<double>(1000.0f * p.r[2]), 'f', 1));
    }

    out.close();
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert Polhemus digitization data between FIFF and hpts formats.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"fif"}, "Input FIFF file with digitization data.", "name", {}});
    parser.add_option({{"hpts"}, "Input hpts text file.", "name", {}});
    parser.add_option({{"fifout"}, "Output FIFF file.", "name", {}});
    parser.add_option({{"hptsout"}, "Output hpts file.", "name", {}});
    parser.process(argc, argv, "mne_convert_dig_data", PROGRAM_VERSION);

    const std::string fifIn = parser.value("fif");
    const std::string hptsIn = parser.value("hpts");
    const std::string fifOut = parser.value("fifout");
    const std::string hptsOut = parser.value("hptsout");

    if (fifIn.empty() && hptsIn.empty()) {
        Log::critical("Specify input with --fif or --hpts.");
        std::print("{}", parser.help_text("mne_convert_dig_data"));
        return 1;
    }
    if (fifOut.empty() && hptsOut.empty()) {
        Log::critical("Specify output with --fifout or --hptsout.");
        std::print("{}", parser.help_text("mne_convert_dig_data"));
        return 1;
    }

    // Read input
    std::vector<DigPt> points;
    if (!fifIn.empty()) {
        points = readFifDig(fifIn);
        Log::info("Read {} points from {}", static_cast<long long>(points.size()), fifIn);
    } else {
        points = readHpts(hptsIn);
        Log::info("Read {} points from {}", static_cast<long long>(points.size()), hptsIn);
    }

    if (points.empty()) {
        Log::critical("No digitization points read.");
        return 1;
    }
    const auto pointCount = static_cast<long long>(points.size());

    // Write output(s)
    if (!fifOut.empty()) {
        if (!writeFifDig(fifOut, points))
            return 1;
        Log::info("Wrote {} points to {}", pointCount, fifOut);
    }
    if (!hptsOut.empty()) {
        if (!writeHpts(hptsOut, points))
            return 1;
        Log::info("Wrote {} points to {}", pointCount, hptsOut);
    }

    Log::info("done.");
    return 0;
}
