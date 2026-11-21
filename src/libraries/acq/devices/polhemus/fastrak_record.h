//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fastrak_record.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May, 2026
 * @brief    ASCII output records of Polhemus Fastrak and FastSCAN digitizers.
 *
 * A record is one line: the station, x y z, and either the Euler angles azimuth elevation roll in degrees
 * (output list 2,4) or a quaternion w x y z (output list 2,11). Fixed-width fields run together when a value
 * is negative ("0.8829-0.3055").
 */

#ifndef ACQ_FASTRAK_RECORD_H
#define ACQ_FASTRAK_RECORD_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Linear unit the digitizer reports positions in. */
enum class FastrakUnits : std::uint8_t
{
    Inches,     /**< Fastrak factory default. */
    Centimetres /**< FastSCAN and Fastrak after the "u" command. */
};

/** One decoded record. */
struct FastrakRecord
{
    int station = 0;                                 /**< Station 1..4 as reported. */
    std::array<float, 3> position{};                 /**< Metres. */
    std::array<float, 4> orientation{1.0F, 0, 0, 0}; /**< Unit quaternion w, x, y, z; identity without angles. */
    bool has_orientation = false;                    /**< The record carried Euler angles or a quaternion. */
};

/**
 * @param[in] line    One record without its line end.
 * @param[in] units   Unit of the positions in @p line.
 *
 * @return The record with its position in metres, or nothing for a malformed line.
 */
[[nodiscard]] std::optional<FastrakRecord> parse_fastrak_record(std::string_view line, FastrakUnits units);

} // namespace mne::detail

#endif // ACQ_FASTRAK_RECORD_H
