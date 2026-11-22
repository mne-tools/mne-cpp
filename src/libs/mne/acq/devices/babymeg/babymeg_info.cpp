//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg_info.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    The text of the BabyMEG server's channel description.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_transport.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <charconv>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::size_t kFields = 16; // scale, 12 location values, coil type, cal, gain

AcqError broken(std::string what)
{
    return {AcqErrorCode::DeviceIo, "the BabyMEG channel description " + std::move(what)};
}

/** @return The text up to @p separator, removing it and the separator from @p text. */
std::string_view take(std::string_view& text, char separator)
{
    const std::size_t end = text.find(separator);
    const std::string_view field = text.substr(0, end);
    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
    return field;
}

template<typename T>
bool number(std::string_view text, T& value)
{
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc() && end == text.data() + text.size();
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<BabyMegInfo> mne::detail::parse_babymeg_info(std::string_view text)
{
    if (!text.starts_with("INFO:")) {
        return broken("does not start with INFO:");
    }
    text.remove_prefix(5);
    BabyMegInfo info;
    std::size_t channels = 0;
    if (!number(take(text, ':'), channels) || !number(take(text, ':'), info.samples) || !number(take(text, ':'), info.rate)) {
        return broken("has no channel count, package size and sampling rate");
    }
    while (!text.empty()) {
        std::string_view entry = take(text, ';');
        if (entry.empty()) {
            continue;
        }
        BabyMegChannel& channel = info.channels.emplace_back();
        channel.name = take(entry, '|');
        std::array<std::string_view, kFields> fields{};
        for (std::string_view& field : fields) {
            field = take(entry, ',');
        }
        bool ok = !channel.name.empty();
        for (std::size_t i = 0; i < channel.loc.size(); ++i) {
            ok = ok && number(fields[1 + i], channel.loc[i]);
        }
        ok = ok && number(fields[13], channel.coil_type) && number(fields[14], channel.cal) && number(fields[15], channel.gain);
        if (!ok || channel.gain == 0.0F) {
            return broken(std::format("has a malformed channel {}", info.channels.size()));
        }
    }
    if (info.channels.size() != channels || channels == 0 || !(info.rate > 0.0) || info.samples == 0) {
        return broken(std::format("announces {} channels at {} Hz in packages of {} and describes {}", channels, info.rate, info.samples,
                                  info.channels.size()));
    }
    return info;
}

//=============================================================================================================

std::string mne::detail::babymeg_info_text(const BabyMegInfo& info)
{
    std::string text = std::format("INFO:{}:{}:{}:", info.channels.size(), info.samples, info.rate);
    for (const BabyMegChannel& channel : info.channels) {
        text += channel.name + "|1";
        for (const float value : channel.loc) {
            text += std::format(",{}", value);
        }
        text += std::format(",{},{},{};", channel.coil_type, channel.cal, channel.gain);
    }
    return text;
}
