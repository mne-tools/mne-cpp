//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_library.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainFlow's BoardController library, loaded at run time through its C interface.
 *
 * No BrainFlow header or library is part of the build: the entry points are declared here as BrainFlow
 * documents them, and the library is found at run time.
 */

#ifndef BRAINFLOW_LIBRARY_H
#define BRAINFLOW_LIBRARY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"
#include "../../detail/acq_shared_library.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** A board as get_board_descr() describes its default preset. */
struct BrainFlowDescription
{
    std::string name;
    int sampling_rate = 0;
    std::size_t rows = 0; /**< Rows of a sample. */
    /** Rows by role, e.g. "eeg_channels" or "marker_channel" (one row), in the order of the description. */
    std::vector<std::pair<std::string, std::vector<int>>> roles;
    std::vector<std::string> eeg_names; /**< Names of the EEG rows, empty if BrainFlow has none. */

    /**
     * @param[in] role   A role key, e.g. "eeg_channels".
     * @return The rows of @p role, empty if the board has none.
     */
    [[nodiscard]] std::span<const int> role(std::string_view role) const noexcept;
};

/**
 * @param[in] json   The description, a flat JSON object.
 * @return The description, or DeviceIo if it is malformed.
 */
AcqExpected<BrainFlowDescription> parse_brainflow_description(std::string_view json);

/** The entry points of BoardController; shared by all sessions of a process. */
class BrainFlowLibrary
{
public:
    /**
     * Loads the first of @p candidates and checks its version.
     *
     * @param[in] candidates   Paths or names of BoardController to try, in order.
     * @return The library, BackendUnavailable if none loads or it lacks an entry point, UnsupportedVersion if it is
     *         not BrainFlow 5.
     */
    [[nodiscard]] static AcqExpected<std::shared_ptr<const BrainFlowLibrary>> load(const std::vector<std::string>& candidates);

    /** @return BrainFlow's version, e.g. "5.12.1". */
    [[nodiscard]] const std::string& version() const noexcept
    {
        return m_version;
    }

    /**
     * @param[in] board   A BrainFlow board id.
     * @return The description of @p board, or InvalidParameter if BrainFlow does not know the board.
     */
    [[nodiscard]] AcqExpected<BrainFlowDescription> description(int board) const;

    /**
     * @param[in] name   A board name, e.g. "synthetic", "Cyton", "CYTON_DAISY_BOARD".
     * @return The board id, or InvalidParameter listing the names BrainFlow knows.
     */
    [[nodiscard]] AcqExpected<int> find_board(std::string_view name) const;

    int (*prepare_session)(int, const char*) = nullptr;
    int (*start_stream)(int, const char*, int, const char*) = nullptr;
    int (*get_board_data_count)(int, int*, int, const char*) = nullptr;
    int (*get_board_data)(int, int, double*, int, const char*) = nullptr;
    int (*stop_stream)(int, const char*) = nullptr;
    int (*release_session)(int, const char*) = nullptr;

private:
    AcqSharedLibrary m_library;
    std::string m_version;
    int (*m_get_board_descr)(int, int, char*, int*, int) = nullptr;
    int (*m_get_version)(char*, int*, int) = nullptr;
};

/**
 * @param[in] name   A board name.
 * @return @p name in lower case without separators and a trailing "board": "CYTON_DAISY_BOARD" is "cytondaisy".
 */
std::string brainflow_board_key(std::string_view name);

/**
 * @param[in] code   A BrainFlow exit code other than 0.
 * @param[in] call   The call that returned it.
 * @return The error: DeviceNotFound for a port that does not open, InvalidParameter for arguments or boards
 *         BrainFlow refuses, Timeout for a board that does not answer, DeviceIo otherwise.
 */
AcqError brainflow_error(int code, std::string_view call);

} // namespace mne::detail

#endif // BRAINFLOW_LIBRARY_H
