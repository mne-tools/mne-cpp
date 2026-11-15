//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_library.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    BrainFlow's BoardController library, loaded at run time through its C interface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflow_library.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <map>
#include <mutex>
#include <optional>
#include <type_traits>

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

constexpr int kSupportedMajor = 5;
constexpr int kDefaultPreset = 0;
constexpr int kFirstBoard = -3; // BrainFlow's playback file board
constexpr int kLastBoard = 255;
constexpr std::size_t kMaxDescription = 16384;

/** Reads the flat JSON object of a board description. */
class JsonCursor
{
public:
    explicit JsonCursor(std::string_view text)
    : m_text(text)
    {
    }

    bool eat(char c)
    {
        skip_space();
        if (m_at < m_text.size() && m_text[m_at] == c) {
            ++m_at;
            return true;
        }
        return false;
    }

    [[nodiscard]] char peek()
    {
        skip_space();
        return m_at < m_text.size() ? m_text[m_at] : '\0';
    }

    std::optional<std::string> string()
    {
        if (!eat('"')) {
            return std::nullopt;
        }
        std::string out;
        while (m_at < m_text.size() && m_text[m_at] != '"') {
            if (m_text[m_at] == '\\' && m_at + 1 < m_text.size()) {
                ++m_at; // board names are ASCII; an escape stands for the character after it
            }
            out += m_text[m_at++];
        }
        if (m_at == m_text.size()) {
            return std::nullopt;
        }
        ++m_at;
        return out;
    }

    /** @return An integer; the descriptions have no other numbers. */
    std::optional<int> integer()
    {
        skip_space();
        int value = 0;
        const char* begin = m_text.data() + m_at;
        const auto [end, error] = std::from_chars(begin, m_text.data() + m_text.size(), value);
        if (error != std::errc()) {
            return std::nullopt;
        }
        m_at += static_cast<std::size_t>(end - begin);
        return value;
    }

    /** Skips any value, nested or not. @return false if it is malformed. */
    bool skip_value()
    {
        const char c = peek();
        if (c == '"') {
            return string().has_value();
        }
        if (c == '[' || c == '{') {
            const char close = c == '[' ? ']' : '}';
            ++m_at;
            if (eat(close)) {
                return true;
            }
            do {
                if (close == '}' && (!string() || !eat(':'))) {
                    return false;
                }
                if (!skip_value()) {
                    return false;
                }
            } while (eat(','));
            return eat(close);
        }
        const std::size_t begin = m_at;
        while (m_at < m_text.size() && std::string_view(",]} \t\r\n").find(m_text[m_at]) == std::string_view::npos) {
            ++m_at;
        }
        return m_at > begin;
    }

private:
    void skip_space()
    {
        while (m_at < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_at])) != 0) {
            ++m_at;
        }
    }

    std::string_view m_text;
    std::size_t m_at = 0;
};

/** @return The major version of "major.minor.patch", or nullopt. */
std::optional<int> major_of(std::string_view version)
{
    int major = 0;
    const auto [end, error] = std::from_chars(version.data(), version.data() + version.size(), major);
    return error == std::errc() && end != version.data() ? std::optional<int>(major) : std::nullopt;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::span<const int> BrainFlowDescription::role(std::string_view role) const noexcept
{
    const auto it = std::find_if(roles.begin(), roles.end(), [&](const auto& r) { return r.first == role; });
    return it == roles.end() ? std::span<const int>() : std::span<const int>(it->second);
}

//=============================================================================================================

AcqExpected<std::shared_ptr<const BrainFlowLibrary>> BrainFlowLibrary::load(const std::vector<std::string>& candidates)
{
    // One library per candidate list while any session uses it
    static std::mutex mutex;
    static std::map<std::vector<std::string>, std::weak_ptr<const BrainFlowLibrary>> loaded;
    const std::lock_guard<std::mutex> lock(mutex);
    if (auto existing = loaded[candidates].lock()) {
        return existing;
    }
    auto shared = AcqSharedLibrary::load(candidates);
    if (!shared) {
        return AcqError(AcqErrorCode::BackendUnavailable,
                        "BrainFlow not found: " + shared.error().text() + "; pip install brainflow, set MNE_CPP_BRAINFLOW_LIB or pass library=");
    }
    auto library = std::make_shared<BrainFlowLibrary>();
    library->m_library = std::move(*shared);
    std::string missing;
    const auto bind = [&](auto& function, const char* name) {
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(library->m_library.symbol(name));
        if (function == nullptr) {
            missing += (missing.empty() ? "" : ", ") + std::string(name);
        }
    };
    bind(library->prepare_session, "prepare_session");
    bind(library->start_stream, "start_stream");
    bind(library->get_board_data_count, "get_board_data_count");
    bind(library->get_board_data, "get_board_data");
    bind(library->stop_stream, "stop_stream");
    bind(library->release_session, "release_session");
    bind(library->m_get_board_descr, "get_board_descr");
    bind(library->m_get_version, "get_version_board_controller");
    if (!missing.empty()) {
        return AcqError(AcqErrorCode::BackendUnavailable, library->m_library.path() + " is not BrainFlow's BoardController: it lacks " + missing);
    }
    std::array<char, 64> version{};
    int length = 0;
    if (const int code = library->m_get_version(version.data(), &length, static_cast<int>(version.size())); code != 0) {
        return brainflow_error(code, "get_version_board_controller");
    }
    library->m_version.assign(version.data(), static_cast<std::size_t>(std::clamp(length, 0, static_cast<int>(version.size()))));
    if (major_of(library->m_version) != kSupportedMajor) {
        return AcqError(AcqErrorCode::UnsupportedVersion, std::format("BrainFlow {} at {} is not supported; brainflow:// needs BrainFlow {}.x", library->m_version, library->m_library.path(), kSupportedMajor));
    }
    loaded[candidates] = library;
    return std::shared_ptr<const BrainFlowLibrary>(std::move(library));
}

//=============================================================================================================

AcqExpected<BrainFlowDescription> BrainFlowLibrary::description(int board) const
{
    std::string json(kMaxDescription, '\0');
    int length = 0;
    if (const int code = m_get_board_descr(board, kDefaultPreset, json.data(), &length, static_cast<int>(json.size())); code != 0) {
        AcqError error = brainflow_error(code, "get_board_descr");
        return AcqError(error.code(), std::format("BrainFlow does not know board {}: {}", board, error.text()));
    }
    json.resize(static_cast<std::size_t>(std::clamp(length, 0, static_cast<int>(json.size()))));
    return parse_brainflow_description(json);
}

//=============================================================================================================

AcqExpected<int> BrainFlowLibrary::find_board(std::string_view name) const
{
    const std::string wanted = brainflow_board_key(name);
    std::string known;
    for (int board = kFirstBoard; board <= kLastBoard; ++board) {
        auto description = this->description(board);
        if (!description) {
            continue;
        }
        if (brainflow_board_key(description->name) == wanted) {
            return board;
        }
        known += (known.empty() ? "" : ", ") + brainflow_board_key(description->name);
    }
    return AcqError(AcqErrorCode::InvalidParameter, std::format("BrainFlow {} has no board '{}'; pass a board id or one of: {}", m_version, name, known));
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<BrainFlowDescription> mne::detail::parse_brainflow_description(std::string_view json)
{
    const auto malformed = [&] {
        return AcqError(AcqErrorCode::DeviceIo, "malformed BrainFlow board description: " + std::string(json.substr(0, 200)));
    };
    JsonCursor cursor(json);
    BrainFlowDescription out;
    if (!cursor.eat('{')) {
        return malformed();
    }
    if (cursor.eat('}')) {
        return out;
    }
    do {
        auto key = cursor.string();
        if (!key || !cursor.eat(':')) {
            return malformed();
        }
        if (*key == "name" || *key == "eeg_names") {
            auto text = cursor.string();
            if (!text) {
                return malformed();
            }
            if (*key == "name") {
                out.name = std::move(*text);
            } else {
                for (std::size_t begin = 0; begin <= text->size();) {
                    const std::size_t end = std::min(text->find(',', begin), text->size());
                    out.eeg_names.push_back(text->substr(begin, end - begin));
                    begin = end + 1;
                }
            }
        } else if (cursor.peek() == '[' && key->ends_with("_channels")) {
            cursor.eat('[');
            std::vector<int> rows;
            if (!cursor.eat(']')) {
                do {
                    const auto row = cursor.integer();
                    if (!row) {
                        return malformed();
                    }
                    rows.push_back(*row);
                } while (cursor.eat(','));
                if (!cursor.eat(']')) {
                    return malformed();
                }
            }
            out.roles.emplace_back(std::move(*key), std::move(rows));
        } else if (*key == "sampling_rate" || *key == "num_rows" || key->ends_with("_channel")) {
            const auto value = cursor.integer();
            if (!value) {
                return malformed();
            }
            if (*key == "sampling_rate") {
                out.sampling_rate = *value;
            } else if (*key == "num_rows") {
                out.rows = static_cast<std::size_t>(std::max(*value, 0));
            } else {
                out.roles.emplace_back(std::move(*key), std::vector<int>{*value});
            }
        } else if (!cursor.skip_value()) {
            return malformed();
        }
    } while (cursor.eat(','));
    if (!cursor.eat('}')) {
        return malformed();
    }
    for (const auto& [role, rows] : out.roles) {
        if (std::any_of(rows.begin(), rows.end(), [&](int row) { return row < 0 || static_cast<std::size_t>(row) >= out.rows; })) {
            return AcqError(AcqErrorCode::DeviceIo, std::format("BrainFlow board {}: {} lies outside its {} rows", out.name, role, out.rows));
        }
    }
    return out;
}

//=============================================================================================================

AcqError mne::detail::brainflow_error(int code, std::string_view call)
{
    // BrainFlow's exit codes
    static constexpr std::array<std::string_view, 24> kNames = {
        "STATUS_OK", "PORT_ALREADY_OPEN_ERROR", "UNABLE_TO_OPEN_PORT_ERROR",
        "SET_PORT_ERROR", "BOARD_WRITE_ERROR", "INCOMMING_MSG_ERROR",
        "INITIAL_MSG_ERROR", "BOARD_NOT_READY_ERROR", "STREAM_ALREADY_RUN_ERROR",
        "INVALID_BUFFER_SIZE_ERROR", "STREAM_THREAD_ERROR", "STREAM_THREAD_IS_NOT_RUNNING",
        "EMPTY_BUFFER_ERROR", "INVALID_ARGUMENTS_ERROR", "UNSUPPORTED_BOARD_ERROR",
        "BOARD_NOT_CREATED_ERROR", "ANOTHER_BOARD_IS_CREATED_ERROR", "GENERAL_ERROR",
        "SYNC_TIMEOUT_ERROR", "JSON_NOT_FOUND_ERROR", "NO_SUCH_DATA_IN_JSON_ERROR",
        "CLASSIFIER_IS_NOT_PREPARED_ERROR", "ANOTHER_CLASSIFIER_IS_PREPARED_ERROR",
        "UNSUPPORTED_CLASSIFIER_AND_METRIC_COMBINATION_ERROR"};
    AcqErrorCode kind = AcqErrorCode::DeviceIo;
    switch (code) {
        case 2:
        case 3:
            kind = AcqErrorCode::DeviceNotFound;
            break;
        case 13:
        case 14:
            kind = AcqErrorCode::InvalidParameter;
            break;
        case 18:
            kind = AcqErrorCode::Timeout;
            break;
        default:
            break;
    }
    const std::string_view name = code >= 0 && static_cast<std::size_t>(code) < kNames.size() ? kNames[static_cast<std::size_t>(code)] : "unknown";
    return {kind, std::format("BrainFlow {} failed: {} ({})", call, name, code)};
}

//=============================================================================================================

std::string mne::detail::brainflow_board_key(std::string_view name)
{
    std::string out;
    for (const char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    if (out.size() > 5 && out.ends_with("board")) {
        out.resize(out.size() - 5);
    }
    return out;
}
