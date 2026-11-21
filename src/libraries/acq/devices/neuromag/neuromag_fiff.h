//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     neuromag_fiff.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    The FIFF tags of the Neuromag (MEGIN) acquisition stream: codes, byte order, channel records and the
 *           measurement-info file, without the FIFF library.
 */

#ifndef NEUROMAG_FIFF_H
#define NEUROMAG_FIFF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"
#include "../../acq_stream_info.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Tag kinds, block kinds and types of the FIFF format that the acquisition stream uses. */
namespace fiff
{
inline constexpr int kNewFile = 1;
inline constexpr int kCloseFile = 2;
inline constexpr int kErrorMessage = 4;
inline constexpr int kFileId = 100;
inline constexpr int kDirPointer = 101;
inline constexpr int kBlockStart = 104;
inline constexpr int kBlockEnd = 105;
inline constexpr int kNop = 108;
inline constexpr int kNchan = 200;
inline constexpr int kSfreq = 201;
inline constexpr int kChInfo = 203;
inline constexpr int kLowpass = 219;
inline constexpr int kHighpass = 223;
inline constexpr int kDataBuffer = 300;

inline constexpr int kBlockMeas = 100;
inline constexpr int kBlockMeasInfo = 101;
inline constexpr int kBlockRawData = 102;
inline constexpr int kBlockIsotrak = 107;

inline constexpr int kTypeVoid = 0;
inline constexpr int kTypeInt = 3;
inline constexpr int kTypeFloat = 4;
inline constexpr int kTypeString = 10;
inline constexpr int kTypeChInfo = 30;
inline constexpr int kTypeId = 31;

inline constexpr int kNextNone = -1;
} // namespace fiff

/** One tag in native byte order. */
struct NeuromagTag
{
    int kind = 0;
    int type = 0;
    std::vector<std::byte> data;

    /** @return The first int of the data, if it holds one. */
    [[nodiscard]] std::optional<std::int32_t> as_int() const noexcept;

    /** @return The first float of the data, if it holds one. */
    [[nodiscard]] std::optional<float> as_float() const noexcept;
};

/** A channel record (fiffChInfoRec), in native byte order. */
struct NeuromagChannel
{
    std::int32_t scan_no = 0;
    std::int32_t log_no = 0;
    std::int32_t kind = 0;
    float range = 1.0F;
    float cal = 1.0F;
    std::int32_t coil_type = 0;
    std::array<float, 12> loc{}; /**< r0, ex, ey, ez. */
    std::int32_t unit = 0;
    std::int32_t unit_mul = 0;
    std::string name;

    /** Size of the record in a tag. */
    static constexpr std::size_t kSize = 96;

    /**
     * @param[in] data   The data of a FIFF_CH_INFO tag, in native byte order.
     *
     * @return The channel, if @p data is one record.
     */
    [[nodiscard]] static std::optional<NeuromagChannel> parse(std::span<const std::byte> data);

    /** @return The record as the data of a FIFF_CH_INFO tag. */
    [[nodiscard]] std::vector<std::byte> serialize() const;
};

/**
 * Swaps the byte order of a tag's data in place, element by element of its type (strings stay, the name of a
 * channel record too). Unknown types are left alone.
 *
 * @param[in]      type      The tag type, including matrix bits.
 * @param[in, out] data      The data.
 * @param[in]      from_big  The data is big-endian now (it matters where the data says how long it is).
 */
void swap_tag_bytes(int type, std::span<std::byte> data, bool from_big) noexcept;

/**
 * Converts big-endian tag data, as in FIFF files, to the byte order of this machine.
 *
 * @param[in]      type   The tag type, including matrix bits.
 * @param[in, out] data   The data.
 */
void big_to_native(int type, std::span<std::byte> data) noexcept;

/**
 * @param[in] meas_info   The tags from FIFF_BLOCK_START to FIFF_BLOCK_END of FIFFB_MEAS_INFO, in native byte order.
 *
 * @return A FIFF file (big-endian) that holds the measurement info in a FIFFB_MEAS block, as FIFF readers expect.
 */
[[nodiscard]] std::vector<std::byte> meas_info_file(std::span<const NeuromagTag> meas_info);

/**
 * @param[in] file         Tags one after the other, each a header of kind, type, size and next, then its data.
 * @param[in] big_endian   The byte order of @p file: big-endian as in FIFF files, else little-endian.
 *
 * @return The tags in native byte order, up to the first truncated one.
 */
[[nodiscard]] std::vector<NeuromagTag> parse_tags(std::span<const std::byte> file, bool big_endian);

/**
 * @param[in] tags         Tags in native byte order.
 * @param[in] big_endian   Write big-endian as in FIFF files, else little-endian.
 *
 * @return The tags one after the other, after a file id and a directory pointer.
 */
[[nodiscard]] std::vector<std::byte> tag_file(std::span<const NeuromagTag> tags, bool big_endian);

/**
 * @param[in] tags    Tags in native byte order.
 * @param[in] block   Block kind, e.g. fiff::kBlockMeasInfo.
 *
 * @return The tags of the first such block, from its start to its end tag; empty if it is not complete.
 */
[[nodiscard]] std::vector<NeuromagTag> block_of(std::span<const NeuromagTag> tags, int block);

/** Schema of the AcqExtension that holds the measurement-info file. */
inline constexpr const char* kMeasInfoSchema = "fiff.MeasInfo/1";

/**
 * @param[in] meas_info   The tags of a FIFFB_MEAS_INFO block, in native byte order.
 * @param[in] source      Source id of the stream.
 * @param[in] format      Int32 for the acquisition's integers, scaled by calibration and range; a floating-point
 *                        format for values that are already physical.
 *
 * @return The stream "raw" with the channels and the measurement-info file as extension, or UnsupportedFormat.
 */
[[nodiscard]] AcqExpected<AcqStreamInfo> stream_info(std::span<const NeuromagTag> meas_info, const std::string& source,
                                                     AcqSampleFormat format);

/**
 * @param[in] info   A stream.
 *
 * @return The trigger channel: STI 014, else STI101, else the first stimulus channel.
 */
[[nodiscard]] std::optional<std::size_t> trigger_channel(const AcqStreamInfo& info);

} // namespace mne::detail

#endif // NEUROMAG_FIFF_H
