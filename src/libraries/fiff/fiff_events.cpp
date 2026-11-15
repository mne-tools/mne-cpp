//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_events.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FiffEvents: FIFFB_MNE_EVENTS read / write and the stim-channel event-detection path.
 *
 * Detection follows @c mne.find_events; the (sample, prev, new) triples
 * are written under @c FIFFB_MNE_EVENTS so ``-eve.fif'' files round-trip
 * through MNE-Python unchanged.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_events.h"
#include "fiff_ch_name.h"
#include <memory>

#include "fiff_evoked_set.h"
#include "fiff_raw_data.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_constants.h"
#include "fiff_file.h"
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

using mne::ErrorCode;
using mne::Expected;
using mne::fail;

namespace
{

// Pads to |width|, right-aligned for a positive width and left-aligned for a negative one.
std::string padded(const std::string& text, int width)
{
    const std::size_t size = static_cast<std::size_t>(std::abs(width));
    if (text.size() >= size)
        return text;
    const std::string fill(size - text.size(), ' ');
    return width < 0 ? text + fill : fill + text;
}

bool isFif(const std::filesystem::path& path)
{
    return mne::utf8_from_path(path.filename()).find(".fif") != std::string::npos;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvents::FiffEvents(const std::filesystem::path& path)
: FiffEvents(read(path).value())
{
}

//=============================================================================================================

Expected<FiffEvents> FiffEvents::read(const std::filesystem::path& path)
{
    return isFif(path) ? read_fif(path) : read_ascii(path);
}

//=============================================================================================================

Expected<FiffEvents> FiffEvents::read_fif(const std::filesystem::path& path)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    const std::vector<std::shared_ptr<FiffDirNode>> blocks = stream->dirtree()->dir_tree_find(FIFFB_MNE_EVENTS);
    if (blocks.empty()) {
        return fail(ErrorCode::NotFound, "{} holds no events", mne::utf8_from_path(path));
    }
    for (const FiffDirEntry& entry : blocks.front()->dir) {
        if (entry.kind != FIFF_MNE_EVENT_LIST) {
            continue;
        }
        const std::optional<FiffTag> tag = stream->read_tag(entry.pos);
        if (!tag || (tag->type != FIFFT_INT && tag->type != FIFFT_UINT)) {
            break;
        }
        // Stored row by row (sample, before, after); unsigned lists are read as their bit patterns
        const Eigen::Index rows = tag->size() / 12;
        return FiffEvents(Eigen::Map<const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>>(reinterpret_cast<const int*>(tag->data()), rows, 3));
    }
    return fail(ErrorCode::NotFound, "{} holds no event list", mne::utf8_from_path(path));
}

//=============================================================================================================

Expected<FiffEvents> FiffEvents::read_ascii(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        return fail(ErrorCode::Io, "cannot read {}", mne::utf8_from_path(path));
    }
    std::vector<Eigen::RowVector3i> rows;
    std::string line;
    for (int number = 1; std::getline(file, line); ++number) {
        // "sample time before after" as written by write_ascii, or just "sample before after"
        std::vector<std::string> fields;
        std::istringstream words(line);
        for (std::string word; words >> word;)
            fields.push_back(word);
        if (fields.empty() || fields.front().starts_with('%') || fields.front().starts_with('#'))
            continue;
        const std::size_t offset = fields.size() >= 4 ? 1 : 0;
        const auto field = [&fields](std::size_t i) {
            return i < fields.size() ? mne::to_int(fields[i]) : std::optional<int>(0);
        };
        const std::optional<int> sample = field(0), before = field(1 + offset), after = field(2 + offset);
        if (!sample || !before || !after) {
            return fail(ErrorCode::Format, "{} line {} is not 'sample [time] before after': {}", mne::utf8_from_path(path), number, line);
        }
        rows.emplace_back(*sample, *before, *after);
    }
    FiffEvents out;
    out.events.resize(static_cast<Eigen::Index>(rows.size()), 3);
    for (std::size_t i = 0; i < rows.size(); ++i) {
        out.events.row(static_cast<Eigen::Index>(i)) = rows[i];
    }
    return out;
}

//=============================================================================================================

Expected<void> FiffEvents::write(const std::filesystem::path& path, float sfreq) const
{
    return isFif(path) ? write_fif(path) : write_ascii(path, sfreq);
}

//=============================================================================================================

Expected<void> FiffEvents::write_fif(const std::filesystem::path& path) const
{
    if (events.rows() == 0 || events.cols() < 3) {
        return fail(ErrorCode::Format, "no events to write to {}", mne::utf8_from_path(path));
    }
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8_from_path(path));
    }
    // FIFF stores the list row by row (sample, before, after).
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> rows = events.leftCols(3);
    stream->start_block(FIFFB_MNE_EVENTS);
    stream->write_int(FIFF_MNE_EVENT_LIST, rows.data(), rows.rows() * 3);
    stream->end_block(FIFFB_MNE_EVENTS);
    stream->end_file();
    stream->close();
    return {};
}

//=============================================================================================================

Expected<void> FiffEvents::write_ascii(const std::filesystem::path& path, float sfreq) const
{
    std::ofstream out(path);
    if (!out) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8_from_path(path));
    }
    for (Eigen::Index k = 0; k < events.rows(); ++k) {
        const int sample = events(k, 0);
        const int before = events.cols() > 1 ? events(k, 1) : 0;
        const int after = events.cols() > 2 ? events(k, 2) : 0;
        const float time = sfreq > 0.0f ? static_cast<float>(sample) / sfreq : 0.0f;
        char digits[64];
        const auto result = std::to_chars(digits, digits + sizeof(digits), static_cast<double>(time), std::chars_format::fixed, 3);
        out << padded(std::to_string(sample), 6) << ' '
            << padded(std::string(digits, result.ptr), -10) << ' '
            << padded(std::to_string(before), 3) << ' '
            << padded(std::to_string(after), 3) << '\n';
    }
    if (!out.flush()) {
        return fail(ErrorCode::Io, "cannot write {}", mne::utf8_from_path(path));
    }
    return {};
}

//=============================================================================================================

Expected<FiffEvents> FiffEvents::find_events(const FiffRawData& raw, std::string_view stim_channel, unsigned int mask, bool leading_edge)
{
    const int index = channel_index(raw.info.ch_names, stim_channel);
    if (index < 0) {
        return fail(ErrorCode::NotFound, "no stimulus channel {}", stim_channel);
    }

    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp, RowVectorXi::Constant(1, index))) {
        return fail(ErrorCode::Io, "cannot read stimulus channel {}", stim_channel);
    }

    std::vector<Eigen::RowVector3i> rows;
    const auto value = [&data, mask](Eigen::Index s) {
        return static_cast<int>(static_cast<unsigned int>(static_cast<int>(data(0, s))) & mask);
    };
    int previous = value(0);
    for (Eigen::Index s = 1; s < data.cols(); ++s) {
        const int current = value(s);
        if (current != previous && (!leading_edge || (previous == 0 && current != 0))) {
            rows.emplace_back(static_cast<int>(raw.first_samp + s), previous, current);
        }
        previous = current;
    }
    FiffEvents out;
    out.events.resize(static_cast<Eigen::Index>(rows.size()), 3);
    for (std::size_t i = 0; i < rows.size(); ++i) {
        out.events.row(static_cast<Eigen::Index>(i)) = rows[i];
    }
    return out;
}

//=============================================================================================================

bool FiffEvents::match_event(const AverageCategory& cat, const MatrixXi& events, int event_idx)
{
    if (event_idx < 0 || event_idx >= events.rows())
        return false;

    int evFrom = events(event_idx, 1);
    int evTo = events(event_idx, 2);

    // Check if any of the category's event codes match
    bool match = false;
    for (const auto event : cat.events) {
        if ((evFrom & ~cat.ignore) == 0 &&
            (evTo & ~cat.ignore) == event) {
            match = true;
            break;
        }
    }
    if (!match)
        return false;

    // Check previous event constraint
    if (cat.prevEvent != 0) {
        bool found = false;
        for (int j = event_idx - 1; j >= 0; --j) {
            if ((events(j, 1) & ~cat.prevIgnore) == 0) {
                found = true;
                match = match && ((events(j, 2) & ~cat.prevIgnore) == cat.prevEvent);
                break;
            }
        }
        if (!found)
            match = false;
    }

    // Check next event constraint
    if (cat.nextEvent != 0) {
        bool found = false;
        for (int j = event_idx + 1; j < events.rows(); ++j) {
            if ((events(j, 1) & ~cat.nextIgnore) == 0) {
                found = true;
                match = match && ((events(j, 2) & ~cat.nextIgnore) == cat.nextEvent);
                break;
            }
        }
        if (!found)
            match = false;
    }

    return match;
}

//=============================================================================================================

namespace
{

/** The rows of @p events whose flag is set, in order. */
MatrixXi selectRows(const MatrixXi& events, const std::vector<bool>& keep)
{
    MatrixXi out(std::count(keep.cbegin(), keep.cend(), true), events.cols());
    Index r = 0;
    for (Index i = 0; i < events.rows(); ++i) {
        if (keep[i])
            out.row(r++) = events.row(i);
    }
    return out;
}

} // namespace

//=============================================================================================================

FiffEvents FiffEvents::pick(const std::vector<int>& include, const std::vector<int>& exclude, bool step) const
{
    std::vector<bool> keep(static_cast<std::size_t>(events.rows()));
    for (Index i = 0; i < events.rows(); ++i) {
        keep[static_cast<std::size_t>(i)] = !include.empty()
            ? std::ranges::contains(include, events(i, 2)) || (step && std::ranges::contains(include, events(i, 1)))
            : !(std::ranges::contains(exclude, events(i, 2)) || (step && std::ranges::contains(exclude, events(i, 1))));
    }
    FiffEvents out;
    out.events = selectRows(events, keep);
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::merge(const std::vector<int>& ids, int new_id, bool replace_events) const
{
    FiffEvents out;
    out.events = events;
    std::vector<bool> touched(events.rows(), false);
    for (Index i = 0; i < events.rows(); ++i) {
        for (int col = 1; col <= 2; ++col) {
            if (std::ranges::find(ids, events(i, col)) != ids.end()) {
                out.events(i, col) = new_id;
                touched[i] = true;
            }
        }
    }
    if (!replace_events) {
        const MatrixXi originals = selectRows(events, touched);
        MatrixXi all(out.events.rows() + originals.rows(), 3);
        all << out.events, originals;
        std::vector<Index> order(all.rows());
        std::iota(order.begin(), order.end(), 0);
        std::stable_sort(order.begin(), order.end(), [&all](Index a, Index b) {
            return std::lexicographical_compare(all.row(a).begin(), all.row(a).end(), all.row(b).begin(), all.row(b).end());
        });
        out.events.resize(all.rows(), 3);
        for (Index i = 0; i < all.rows(); ++i)
            out.events.row(i) = all.row(order[i]);
    }
    return out;
}

//=============================================================================================================

std::map<int, int> FiffEvents::count(const std::vector<int>& ids) const
{
    std::map<int, int> counts;
    for (const int id : ids)
        counts.emplace(id, 0);
    for (Index i = 0; i < events.rows(); ++i) {
        if (ids.empty() || counts.contains(events(i, 2)))
            ++counts[events(i, 2)];
    }
    return counts;
}

//=============================================================================================================

FiffEvents FiffEvents::concatenate(const std::vector<FiffEvents>& events, const std::vector<int>& first_samps, const std::vector<int>& last_samps)
{
    FiffEvents out;
    if (events.empty() || events.size() != first_samps.size() || events.size() != last_samps.size())
        return out;
    Index rows = 0;
    for (const FiffEvents& e : events)
        rows += e.events.rows();
    out.events.resize(rows, 3);
    Index r = 0;
    int offset = 0;
    for (std::size_t k = 0; k < events.size(); ++k) {
        MatrixXi shifted = events[k].events;
        if (k > 0)
            shifted.col(0).array() += offset + first_samps[0] - first_samps[k];
        out.events.middleRows(r, shifted.rows()) = shifted;
        r += shifted.rows();
        offset += last_samps[k] - first_samps[k] + 1;
    }
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::find_stim_steps(const MatrixXi& stim_data, int first_samp, std::optional<int> pad_start, std::optional<int> pad_stop, int merge)
{
    std::vector<RowVector3i> steps;
    for (Index t = 0; t + 1 < stim_data.cols(); ++t) {
        if ((stim_data.col(t + 1).array() != stim_data.col(t).array()).all())
            steps.emplace_back(static_cast<int>(t + 1) + first_samp, stim_data(0, t), stim_data(0, t + 1));
    }
    FiffEvents out;
    if (steps.empty())
        return out;
    if (pad_start && steps.front()(1) != *pad_start)
        steps.insert(steps.begin(), RowVector3i(0, *pad_start, steps.front()(1)));
    if (pad_stop && steps.back()(2) != *pad_stop)
        steps.emplace_back(static_cast<int>(stim_data.cols()) + first_samp, steps.back()(2), *pad_stop);
    if (merge != 0) {
        const size_t n = steps.size();
        std::vector<bool> close(n > 0 ? n - 1 : 0);
        for (size_t i = 0; i + 1 < n; ++i)
            close[i] = steps[i + 1](0) - steps[i](0) <= std::abs(merge);
        if (std::find(close.cbegin(), close.cend(), true) != close.cend()) {
            // Merged steps take the outer values; the step that is merged away is dropped
            std::vector<bool> keep(n, true);
            const std::vector<RowVector3i> original = steps;
            for (size_t i = 0; i + 1 < n; ++i) {
                if (!close[i])
                    continue;
                if (merge > 0) {
                    steps[i + 1](1) = original[i](1);
                    keep[i] = false;
                } else {
                    steps[i](2) = original[i + 1](2);
                    keep[i + 1] = false;
                }
            }
            std::vector<RowVector3i> kept;
            for (size_t i = 0; i < n; ++i) {
                if (keep[i] && steps[i](1) != steps[i](2))
                    kept.push_back(steps[i]);
            }
            steps.swap(kept);
        }
    }
    out.events.resize(static_cast<Index>(steps.size()), 3);
    for (size_t i = 0; i < steps.size(); ++i)
        out.events.row(static_cast<Index>(i)) = steps[i];
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::make_fixed_length(const FiffRawData& raw, int id, double start, double stop, double duration, bool first_samp, double overlap)
{
    FiffEvents out;
    const double sfreq = raw.info.sfreq;
    if (overlap < 0.0 || overlap >= duration || sfreq <= 0.0)
        return out;
    double first = std::nearbyint(start * sfreq);
    double last = stop >= 0.0 ? std::nearbyint(stop * sfreq) : raw.last_samp + 1.0;
    if (first_samp) {
        first += raw.first_samp;
        last = std::min(last + (stop >= 0.0 ? raw.first_samp : 0), raw.last_samp + 1.0);
    } else {
        last = std::min(last - (stop >= 0.0 ? 0 : raw.first_samp), static_cast<double>(raw.last_samp - raw.first_samp + 1));
    }
    last -= std::nearbyint(sfreq * duration);
    const double stepSamples = sfreq * (duration - overlap);
    const auto count = static_cast<Index>(std::max(0.0, std::ceil((last + 1.0 - first) / stepSamples)));
    out.events = MatrixXi::Zero(count, 3);
    for (Index i = 0; i < count; ++i) {
        out.events(i, 0) = static_cast<int>(first + static_cast<double>(i) * stepSamples);
        out.events(i, 2) = id;
    }
    return out;
}
