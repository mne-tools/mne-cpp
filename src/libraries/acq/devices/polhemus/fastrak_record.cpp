//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fastrak_record.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May, 2026
 * @brief    Decoding of Fastrak / FastSCAN ASCII records.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fastrak_record.h"

#include <cmath>
#include <locale>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::optional<mne::detail::FastrakRecord> mne::detail::parse_fastrak_record(std::string_view line, FastrakUnits units)
{
    // A minus sign that follows a digit starts the next field
    std::string text;
    text.reserve(line.size() + 8);
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '-' && i > 0 && line[i - 1] >= '0' && line[i - 1] <= '9') {
            text.push_back(' ');
        }
        text.push_back(line[i]);
    }
    std::istringstream stream(std::move(text));
    stream.imbue(std::locale::classic());

    FastrakRecord record;
    if (!(stream >> record.station)) {
        return std::nullopt;
    }
    std::vector<float> values;
    values.reserve(7);
    std::string token;
    while (stream >> token) {
        std::istringstream number(token);
        number.imbue(std::locale::classic());
        float value = 0.0F;
        if (!(number >> value) || number.peek() != std::char_traits<char>::eof()) {
            values.push_back(std::nanf(""));
        } else {
            values.push_back(value);
        }
    }
    if (values.size() < 3 || !std::isfinite(values[0]) || !std::isfinite(values[1]) || !std::isfinite(values[2])) {
        return std::nullopt;
    }
    const float scale = units == FastrakUnits::Inches ? 0.0254F : 0.01F;
    record.position = {values[0] * scale, values[1] * scale, values[2] * scale};

    const auto finite = [&](std::size_t from, std::size_t count) {
        for (std::size_t i = from; i < from + count; ++i) {
            if (!std::isfinite(values[i])) {
                return false;
            }
        }
        return true;
    };
    if (values.size() >= 7 && finite(3, 4)) {
        const float norm = std::sqrt(values[3] * values[3] + values[4] * values[4] + values[5] * values[5] + values[6] * values[6]);
        if (norm > 0.0F) {
            record.orientation = {values[3] / norm, values[4] / norm, values[5] / norm, values[6] / norm};
            record.has_orientation = true;
        }
    } else if (values.size() >= 6 && finite(3, 3)) {
        // Azimuth about z, then elevation about y, then roll about x
        constexpr float kHalfRad = std::numbers::pi_v<float> / 360.0F;
        const float cy = std::cos(values[3] * kHalfRad);
        const float sy = std::sin(values[3] * kHalfRad);
        const float cp = std::cos(values[4] * kHalfRad);
        const float sp = std::sin(values[4] * kHalfRad);
        const float cr = std::cos(values[5] * kHalfRad);
        const float sr = std::sin(values[5] * kHalfRad);
        record.orientation = {cr * cp * cy + sr * sp * sy, sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy};
        record.has_orientation = true;
    }
    return record;
}
