//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     filterio.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     April 2015
 * @brief    Definition of the FilterIO class
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterio.h"

#include <base/byte_stream.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <iterator>
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
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<FilterKernel> FilterIO::read_filter(const std::filesystem::path& path)
{
    const Expected<std::string> text = read_text_file(path);
    if (!text) {
        return std::unexpected(text.error());
    }
    FilterKernel filter;
    std::vector<double> coefficients;
    std::string_view rest = *text;
    while (!rest.empty()) {
        const std::size_t end = rest.find('\n');
        const std::string_view line = trimmed(rest.substr(0, end));
        rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
        if (line.empty()) {
            continue;
        }
        if (!line.starts_with('#')) {
            const std::optional<double> value = to_double(line);
            if (!value) {
                return fail(ErrorCode::Format, "{} has a filter coefficient that is no number: \"{}\"",
                            utf8_from_path(path), line);
            }
            coefficients.push_back(*value);
            continue;
        }
        const std::string_view entry = line.substr(1);
        const std::size_t space = entry.find_first_of(" \t");
        const std::string_view key = entry.substr(0, space);
        const std::string_view value = space == std::string_view::npos ? std::string_view() : trimmed(entry.substr(space));
        if (key == "name") {
            filter.set_name(std::string(value));
        } else if (key == "type") {
            filter.set_filter_type(FilterKernel::filter_type_index(value));
        } else if (key == "DesignMethod") {
            filter.set_design_method(FilterKernel::design_method_index(value));
        } else if (key == "order") {
            continue; // the coefficients give the order
        } else {
            const auto setter = key == "sFreq" ? &FilterKernel::set_sampling_frequency
                : key == "HPFreq"              ? &FilterKernel::set_highpass_freq
                : key == "LPFreq"              ? &FilterKernel::set_lowpass_freq
                : key == "CenterFreq"          ? &FilterKernel::set_center_frequency
                : key == "ParksWidth"          ? &FilterKernel::set_parks_width
                                               : nullptr;
            if (!setter) {
                continue;
            }
            const std::optional<double> number = to_double(value);
            if (!number) {
                return fail(ErrorCode::Format, "{} has no number for {}: \"{}\"", utf8_from_path(path), key, value);
            }
            (filter.*setter)(*number);
        }
    }
    filter.set_filter_order(static_cast<int>(coefficients.size()));
    filter.set_coefficients(Map<const RowVectorXd>(coefficients.data(), std::ssize(coefficients)));
    return filter;
}

//=============================================================================================================

Expected<void> FilterIO::write_filter(const std::filesystem::path& path, const FilterKernel& filter)
{
    std::string text = std::format("#sFreq {}\n#name {}\n#type {}\n#order {}\n#HPFreq {}\n#LPFreq {}\n#CenterFreq {}\n"
                                   "#DesignMethod {}\n#ParksWidth {}\n",
                                   filter.sampling_frequency(), filter.name(), filter.filter_type().name(),
                                   filter.filter_order(), filter.highpass_freq(), filter.lowpass_freq(),
                                   filter.center_frequency(), filter.design_method().name(), filter.parks_width());
    for (const double coefficient : filter.coefficients()) {
        std::format_to(std::back_inserter(text), "{}\n", coefficient);
    }
    return write_text_file(path, text);
}
