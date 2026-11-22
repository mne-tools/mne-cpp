//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_event_list.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref mne::MNEEventList (MNE-C mne_events.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_event_list.h"

#include <mne/base/log.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <ranges>
#include <span>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

constexpr std::string_view whitespace = " \t\r";

//=============================================================================================================
/**
 * Parses the whitespace-delimited number at the start of @p text and drops it from @p text.
 */
template<typename T>
bool take_number(std::string_view& text, T& value)
{
    text = text.substr(std::min(text.find_first_not_of(whitespace), text.size()));
    const char* first = text.data();
    const char* end = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, end, value);
    if (ec != std::errc{} || (ptr != end && whitespace.find(*ptr) == std::string_view::npos)) {
        return false;
    }
    text.remove_prefix(static_cast<std::size_t>(ptr - first));
    return true;
}

//=============================================================================================================

std::string utf8_from_latin1(std::string_view bytes)
{
    std::string text;
    text.reserve(bytes.size());
    for (const char c : bytes) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x80) {
            text += c;
        } else {
            text += static_cast<char>(0xC0 | (u >> 6));
            text += static_cast<char>(0x80 | (u & 0x3F));
        }
    }
    return text;
}

//=============================================================================================================
/**
 * Characters outside Latin-1 become '?'.
 */
std::string latin1_from_utf8(std::string_view text)
{
    std::string bytes;
    bytes.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        const auto u = static_cast<unsigned char>(text[i]);
        const std::size_t length = u < 0x80 ? 1 : u >= 0xF0 ? 4
            : u >= 0xE0                                     ? 3
                                                            : 2;
        if (length == 1) {
            bytes += text[i];
        } else if (length == 2 && u <= 0xC3 && i + 1 < text.size()) {
            bytes += static_cast<char>(((u & 0x1F) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3F));
        } else {
            bytes += '?';
        }
        i += length;
    }
    return bytes;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<MNEEventList> MNEEventList::read_fif(const std::filesystem::path& path, int offset)
{
    FiffStream stream(path);
    if (!stream.open()) {
        return std::nullopt;
    }
    const std::vector<std::shared_ptr<FiffDirNode>> blocks = stream.dirtree()->dir_tree_find(FIFFB_MNE_EVENTS);
    const auto tag = blocks.empty() ? std::nullopt : blocks.front()->find_tag(stream, FIFF_MNE_EVENT_LIST);
    if (!tag) {
        mne::Log::warning("MNEEventList::read_fif - No event data in {}", mne::utf8_from_path(path));
        return std::nullopt;
    }
    // Written as signed or unsigned integers; both have the same bit pattern.
    const auto* values = reinterpret_cast<const std::int32_t*>(tag->data());
    const int nevent = static_cast<int>(tag->size() / (3 * sizeof(std::int32_t)));
    MNEEventList list;
    list.events.resize(nevent);
    for (int k = 0; k < nevent; ++k) {
        list.events[k].sample = values[3 * k] - offset;
        list.events[k].from = static_cast<unsigned int>(values[3 * k + 1]);
        list.events[k].to = static_cast<unsigned int>(values[3 * k + 2]);
    }
    // The comments are one NUL-separated string, one entry per event.
    if (const auto commentTag = blocks.front()->find_tag(stream, FIFF_MNE_EVENT_COMMENTS)) {
        const std::string_view comments(reinterpret_cast<const char*>(commentTag->data()), commentTag->size());
        int k = 0;
        for (auto&& comment : comments | std::views::split('\0')) {
            if (k >= nevent) {
                break;
            }
            list.events[k++].comment = utf8_from_latin1(std::string_view(comment.begin(), comment.end()));
        }
    }
    return list;
}

//=============================================================================================================

std::optional<MNEEventList> MNEEventList::read_text(const std::filesystem::path& path, int offset, int old_offset, float sfreq)
{
    std::ifstream in(path);
    if (!in) {
        mne::Log::warning("MNEEventList::read_text - Cannot open {}", mne::utf8_from_path(path));
        return std::nullopt;
    }
    MNEEventList list;
    bool first = true;
    std::string line;
    while (std::getline(in, line)) {
        std::string_view rest = trimmed(line);
        if (rest.empty() || rest.front() == '#') {
            continue;
        }
        int sample = 0;
        float time = 0.0f;
        unsigned int from = 0;
        unsigned int to = 0;
        if (!take_number(rest, sample) || !take_number(rest, time) || !take_number(rest, from) || !take_number(rest, to)) {
            break;
        }
        if (sample < 0) {
            sample = static_cast<int>(time * sfreq);
        }
        if (first) {
            first = false;
            if (from == 0 && to == 0) {
                if (offset != sample) {
                    mne::Log::warning("MNEEventList::read_text - First sample of the data ({}) and of {} ({}) differ", offset, mne::utf8_from_path(path), sample);
                }
                continue;
            }
            offset -= old_offset;
        }
        list.events.push_back(MNEEvent{sample - offset, from, to, false, false, std::string(trimmed(rest))});
    }
    if (list.events.empty()) {
        mne::Log::warning("MNEEventList::read_text - No events read from {}", mne::utf8_from_path(path));
        return std::nullopt;
    }
    return list;
}

//=============================================================================================================

bool MNEEventList::write_fif(const std::filesystem::path& path, int offset) const
{
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return false;
    }
    std::vector<std::int32_t> values;
    values.reserve(3 * events.size());
    std::string comments;
    bool anyComment = false;
    for (const MNEEvent& event : events) {
        values.push_back(event.sample + offset);
        values.push_back(static_cast<std::int32_t>(event.from));
        values.push_back(static_cast<std::int32_t>(event.to));
        comments += latin1_from_utf8(event.comment);
        comments += '\0';
        anyComment = anyComment || !event.comment.empty();
    }
    stream->start_block(FIFFB_MNE_EVENTS);
    // Unsigned like MNE-C; trigger values are bit masks.
    FiffTag list;
    list.kind = FIFF_MNE_EVENT_LIST;
    list.type = FIFFT_UINT;
    list.next = FIFFV_NEXT_SEQ;
    list.assign(std::as_bytes(std::span(values)));
    stream->write_tag(list);
    if (anyComment) {
        // Latin-1 bytes, one NUL-terminated comment per event, as MNE-C writes and mne-python reads them.
        FiffTag tag;
        tag.kind = FIFF_MNE_EVENT_COMMENTS;
        tag.type = FIFFT_BYTE;
        tag.next = FIFFV_NEXT_SEQ;
        tag.assign(std::as_bytes(std::span(comments)));
        stream->write_tag(tag);
    }
    stream->end_block(FIFFB_MNE_EVENTS);
    stream->end_file();
    return true;
}

//=============================================================================================================

void MNEEventList::sort()
{
    std::stable_sort(events.begin(), events.end(), [](const MNEEvent& a, const MNEEvent& b) {
        return a.sample < b.sample;
    });
}

//=============================================================================================================

void MNEEventList::append(const MNEEventList& other)
{
    events.insert(events.end(), other.events.cbegin(), other.events.cend());
}

//=============================================================================================================

MNEEventList MNEEventList::select_onsets(unsigned int value) const
{
    MNEEventList selected;
    std::copy_if(events.cbegin(), events.cend(), std::back_inserter(selected.events), [value](const MNEEvent& event) {
        return event.from == 0 && (value == 0 || event.to == value);
    });
    return selected;
}

//=============================================================================================================

unsigned int MNEEventList::max_onset() const
{
    unsigned int maximum = 0;
    for (const MNEEvent& event : events) {
        if (event.from == 0) {
            maximum = std::max(maximum, event.to);
        }
    }
    return maximum;
}

//=============================================================================================================

Eigen::MatrixXi MNEEventList::to_matrix() const
{
    Eigen::MatrixXi matrix(nevent(), 3);
    for (int k = 0; k < nevent(); ++k) {
        matrix.row(k) << events[k].sample, static_cast<int>(events[k].from), static_cast<int>(events[k].to);
    }
    return matrix;
}

//=============================================================================================================

MNEEventList MNEEventList::from_matrix(const Eigen::MatrixXi& matrix)
{
    MNEEventList list;
    list.events.resize(matrix.rows());
    for (Eigen::Index k = 0; k < matrix.rows(); ++k) {
        list.events[k].sample = matrix(k, 0);
        list.events[k].from = static_cast<unsigned int>(matrix(k, 1));
        list.events[k].to = static_cast<unsigned int>(matrix(k, 2));
    }
    return list;
}
