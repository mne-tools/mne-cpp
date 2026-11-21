//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_annotations.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref FiffAnnotation / @ref FiffAnnotations, including the JSON and CSV serializers.
 *
 * FiffStream reads and writes them under @c FIFFB_MNE_ANNOTATIONS. The JSON and
 * CSV paths exist so the same annotations can be exchanged with BIDS
 * sidecars and external scoring tools that do not parse FIFF.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_annotations.h"

#include "fiff_ch_name.h"
#include "fiff_annotations_json.h"
#include <base/byte_stream.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>
#include <limits>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using mne::ErrorCode;
using mne::Expected;
using mne::fail;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

mne::JsonArray mne::names_to_json(const std::vector<std::string>& names)
{
    mne::JsonArray array;
    array.reserve(names.size());
    for (const std::string& name : names) {
        array.push_back(name);
    }
    return array;
}

//=============================================================================================================

std::vector<std::string> mne::names_from_json(const mne::JsonArray& array)
{
    std::vector<std::string> names;
    names.reserve(array.size());
    for (const mne::JsonValue& name : array) {
        names.push_back(name.to_std_string());
    }
    return names;
}

//=============================================================================================================

mne::JsonObject mne::extras_to_json(const FiffAnnotation::Extras& extras)
{
    mne::JsonObject object;
    object.reserve(extras.size());
    for (const auto& [key, value] : extras) {
        object.insert(key, std::visit([](const auto& v) -> mne::JsonValue {
                          if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                              return nullptr;
                          } else {
                              return v;
                          }
                      },
                                      value));
    }
    return object;
}

//=============================================================================================================

FiffAnnotation::Extras mne::extras_from_json(const mne::JsonObject& object)
{
    FiffAnnotation::Extras extras;
    for (const auto& [key, value] : object) {
        FiffAnnotation::Extra extra;
        if (value.is_bool()) {
            extra = value.to_bool();
        } else if (value.is_integer()) {
            extra = value.to_integer();
        } else if (value.is_number()) {
            const double number = value.to_double();
            // JSON has one number type; integral values come back as int, as from Python's json
            if (std::trunc(number) == number && std::abs(number) < 9.007199254740992e15) {
                extra = static_cast<std::int64_t>(number);
            } else {
                extra = number;
            }
        } else if (value.is_string()) {
            extra = value.to_std_string();
        } else if (!value.is_null()) {
            continue;
        }
        extras.insert_or_assign(key, std::move(extra));
    }
    return extras;
}

//=============================================================================================================

namespace
{

std::string lowerExtension(const std::filesystem::path& path)
{
    return mne::to_lower(mne::utf8_from_path(path.extension()));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void FiffAnnotations::append(double onset, double duration, std::string description, std::vector<std::string> ch_names)
{
    m_annotations.push_back({onset, duration, std::move(description), std::move(ch_names), {}, {}});
}

//=============================================================================================================

int FiffAnnotations::onset_to_sample(std::size_t index, double sfreq, int first_sample) const
{
    return static_cast<int>(m_annotations[index].onset * sfreq) + first_sample;
}

//=============================================================================================================

int FiffAnnotations::end_to_sample(std::size_t index, double sfreq, int first_sample) const
{
    const FiffAnnotation& a = m_annotations[index];
    return static_cast<int>((a.onset + a.duration) * sfreq) + first_sample;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::select(std::string_view prefix) const
{
    FiffAnnotations result;
    std::ranges::copy_if(m_annotations, std::back_inserter(result.m_annotations), [prefix](const FiffAnnotation& a) {
        return a.description.starts_with(prefix);
    });
    return result;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::select_by_channel(std::string_view ch_name) const
{
    FiffAnnotations result;
    std::ranges::copy_if(m_annotations, std::back_inserter(result.m_annotations), [ch_name](const FiffAnnotation& a) {
        return a.ch_names.empty() || has_channel(a.ch_names, ch_name);
    });
    return result;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::crop(double tmin, double tmax) const
{
    FiffAnnotations result;
    for (const FiffAnnotation& a : m_annotations) {
        if (a.onset > tmax || a.onset + a.duration < tmin) {
            continue;
        }
        FiffAnnotation clipped = a;
        if (clipped.onset < tmin) {
            clipped.duration -= tmin - clipped.onset;
            clipped.onset = tmin;
        }
        clipped.duration = std::min(clipped.duration, tmax - clipped.onset);
        result.m_annotations.push_back(std::move(clipped));
    }
    return result;
}

//=============================================================================================================

Expected<FiffAnnotations> FiffAnnotations::read_json(const std::filesystem::path& path)
{
    const Expected<std::string> bytes = mne::read_text_file(path);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    mne::JsonParseError parseError;
    const mne::JsonDocument doc = mne::JsonDocument::from_json(*bytes, &parseError);
    if (parseError.error != mne::JsonParseError::NoError || !doc.is_object()) {
        return fail(ErrorCode::Format, "{} is not an annotation JSON file: {}", mne::utf8_from_path(path), parseError.error_string());
    }

    FiffAnnotations annot;
    for (const mne::JsonValue& entry : doc.object().value("annotations").to_array()) {
        const mne::JsonObject& obj = entry.to_object();
        annot.m_annotations.push_back({obj.value("onset").to_double(0.0),
                                       obj.value("duration").to_double(0.0),
                                       obj.value("description").to_std_string(),
                                       names_from_json(obj.value("channel_names").to_array()),
                                       obj.value("comment").to_std_string(),
                                       extras_from_json(obj.value("extras").to_object())});
    }
    return annot;
}

//=============================================================================================================

Expected<void> FiffAnnotations::write_json(const std::filesystem::path& path) const
{
    mne::JsonArray arr;
    arr.reserve(m_annotations.size());
    for (const FiffAnnotation& a : m_annotations) {
        arr.push_back(mne::JsonObject{{"onset", a.onset},
                                      {"duration", a.duration},
                                      {"description", a.description},
                                      {"channel_names", names_to_json(a.ch_names)},
                                      {"comment", a.comment},
                                      {"extras", extras_to_json(a.extras)}});
    }
    return mne::write_text_file(path, mne::JsonDocument(mne::JsonObject{{"annotations", std::move(arr)}}).to_json());
}

//=============================================================================================================

Expected<FiffAnnotations> FiffAnnotations::read_csv(const std::filesystem::path& path)
{
    const Expected<std::string> bytes = mne::read_text_file(path);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    FiffAnnotations annot;
    const mne::StringList lines = mne::split(*bytes, '\n');
    for (std::size_t i = 1; i < lines.size(); ++i) {
        const std::string_view line = mne::trimmed(lines[i]);
        // The description may hold commas, so only the first two separate fields
        const std::size_t first = line.find(',');
        const std::size_t second = first == std::string_view::npos ? first : line.find(',', first + 1);
        if (second == std::string_view::npos) {
            continue;
        }
        const std::optional<double> onset = mne::to_double(line.substr(0, first));
        const std::optional<double> duration = mne::to_double(line.substr(first + 1, second - first - 1));
        if (onset && duration) {
            annot.append(*onset, *duration, std::string(line.substr(second + 1)));
        }
    }
    return annot;
}

//=============================================================================================================

Expected<void> FiffAnnotations::write_csv(const std::filesystem::path& path) const
{
    // std::format writes the shortest text that reads back to the same double
    std::string text = "onset,duration,description\n";
    for (const FiffAnnotation& a : m_annotations) {
        std::format_to(std::back_inserter(text), "{},{},{}\n", a.onset, a.duration, a.description);
    }
    return mne::write_text_file(path, text);
}

//=============================================================================================================

Expected<FiffAnnotations> FiffAnnotations::read(const std::filesystem::path& path)
{
    const std::string extension = lowerExtension(path);
    if (extension == ".json") {
        return read_json(path);
    }
    if (extension == ".csv") {
        return read_csv(path);
    }
    if (Expected<FiffAnnotations> json = read_json(path); json || json.error().code == ErrorCode::Io) {
        return json;
    }
    return read_csv(path);
}

//=============================================================================================================

Expected<void> FiffAnnotations::write(const std::filesystem::path& path) const
{
    return lowerExtension(path) == ".csv" ? write_csv(path) : write_json(path);
}
