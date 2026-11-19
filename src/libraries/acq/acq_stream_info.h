//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_stream_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqStreamInfo: direction, format, layout, rate and per-channel description of one stream.
 *
 * A stream is either an input (device -> host: EEG, MEG, MR receive coils) or an output (host ->
 * device: electrical or magnetic stimulation waveforms, audio/visual stimuli, trigger lines, RF
 * transmit pulses). Both are described by the same structure and carried by the same ring.
 *
 * The description can be converted to and from mne::FiffInfo without loss (fiff/acq_fiff.h) and
 * also describes what FIFF cannot, such as segmented complex-valued MR data. Nothing is guessed:
 * an unknown unit is AcqUnit::Unknown, an unknown range is NaN, an irregular stream has sfreq 0.
 * Metadata changes are versioned at a sample index (@c version, @c validFromSample).
 *
 * MR receive (32 coils, 512-sample readouts, ISMRMRD header per readout) and RF transmit:
 * @code
 * mne::AcqStreamInfo rx;
 * rx.name = "rx";
 * rx.direction = mne::AcqDirection::Input;
 * rx.format = mne::AcqSampleFormat::ComplexInt16;
 * rx.layout = mne::AcqLayout::Planar;                    // one DMA buffer per coil
 * rx.sfreq = 1.0e6;                                         // 1 us dwell time
 * mne::AcqChannelInfo coil;
 * coil.kind = mne::AcqChannelKind::MrReceive;
 * coil.unit = mne::AcqUnit::Volt;
 * coil.scale = 1.0 / 32768.0;                               // ADC full scale -> 1 V
 * for (int c = 0; c < 32; ++c) {
 *     coil.name = "coil" + std::to_string(c);
 *     rx.channels.push_back(coil);
 * }
 * rx.segmentation.mode = mne::AcqSegmentation::Mode::Triggered;
 * rx.segmentation.samplesPerSegment = 512;
 * rx.segmentation.headerBytes = 340;                        // sizeof(ISMRMRD_AcquisitionHeader)
 * rx.segmentation.headerSchema = "ismrmrd.AcquisitionHeader/1";
 * rx.clockDomain = "spectrum:12345";                        // set by the backend: the clock-master card
 *
 * mne::AcqStreamInfo tx = rx;                              // same card, transmit path
 * tx.name = "tx";
 * tx.direction = mne::AcqDirection::Output;
 * tx.format = mne::AcqSampleFormat::ComplexInt16;
 * tx.channels[0].digitalBits = 1;                           // bit 15 of the I word drives the T/R switch
 * tx.bitFields.push_back({"tr_switch", 0, 15, 1, mne::AcqChannelKind::RfSwitch});
 * tx.channels.resize(1);
 * tx.channels[0].name = "rf0";
 * tx.channels[0].kind = mne::AcqChannelKind::MrTransmit;
 * tx.channels[0].scale = 1.0;
 * tx.segmentation.samplesPerSegment = 0;                    // pulses of variable length
 * tx.underrunFill = mne::AcqUnderrunFill::Zero;          // never hold an RF amplitude
 * @endcode
 */

#ifndef ACQ_STREAM_INFO_H
#define ACQ_STREAM_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_sample_format.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Direction of a stream as seen from the host.
 */
enum class AcqDirection : std::uint8_t
{
    Input = 0, /**< Device produces, host readers consume. The producer never waits. */
    Output = 1 /**< Host writer produces, the device consumes. The writer waits for space, never overwrites unplayed samples. */
};

//=============================================================================================================
/**
 * What an output device emits when the writer has not committed the sample it has to play.
 */
enum class AcqUnderrunFill : std::uint8_t
{
    Zero = 0,    /**< Emit the zero raw value (safe default for stimulation and RF). */
    HoldLast = 1 /**< Repeat the last committed sample per channel (DC levels, trigger lines). */
};

//=============================================================================================================
/**
 * Signal kind of a channel. Mapped to and from FIFF channel kinds by fiff/acq_fiff.h.
 */
enum class AcqChannelKind : std::uint8_t
{
    Unknown = 0,
    Eeg,
    Meg, /**< Magnetometer or gradiometer; FIFF coil type distinguishes them. */
    Opm, /**< Optically pumped magnetometer. */
    Ecg,
    Eog,
    Erg, /**< Electroretinogram. */
    Emg,
    Seeg,
    Ecog,
    Dbs,
    Fnirs,
    Respiration,
    Gsr,
    Stim, /**< Trigger / digital input channel carrying event codes. */
    Accelerometer,
    Gyroscope,
    Temperature,
    Timestamp, /**< Device timestamp row (e.g. BrainFlow timestamp channel). */
    Marker,    /**< Device marker row; also delivered as AcqEvent. */
    MrReceive, /**< MR receive-coil channel. */
    // Output kinds
    StimCurrent,  /**< Electrical stimulation current (tACS/tDCS, DBS, intracortical). */
    StimVoltage,  /**< Electrical stimulation voltage. */
    MagneticStim, /**< TMS coil drive or amplitude command. */
    Audio,        /**< Acoustic stimulus. */
    Light,        /**< Optical / optogenetic stimulus intensity. */
    TriggerOut,   /**< Digital output line(s). */
    MrTransmit,   /**< RF transmit channel. */
    MrGradient,   /**< Gradient waveform channel. */
    RfSwitch,     /**< T/R switch, RF blanking or ADC gate line. */
    ClockSync,    /**< Recorded sync / reference marker used to verify a clock domain (verify()). */
    Misc,
    Position,   /**< Coordinate of a tracked sensor, e.g. a digitizer stylus (metres). */
    Orientation /**< Quaternion component of a tracked sensor's orientation. */
};

//=============================================================================================================
/**
 * SI unit of the physical value of a channel (value = raw * scale + offset, times 10^unitExponent).
 */
enum class AcqUnit : std::uint8_t
{
    Unknown = 0, /**< Not stated by the source; never guessed. */
    None,        /**< Dimensionless (counts, trigger codes). */
    Volt,
    Ampere,
    Tesla,
    TeslaPerMeter,
    AmpereMeter,
    MeterPerSecondSquared,
    RadianPerSecond,
    DegreeCelsius,
    Second,
    Ohm,
    Watt,
    Pascal, /**< Sound pressure. */
    Molar,  /**< fNIRS concentrations. */
    Meter
};

//=============================================================================================================
/**
 * FIFF-specific channel fields, kept so that FiffChInfo round-trips without loss. Plain integers and
 * floats with FIFF semantics (FIFFV_* constants), so no FIFF header is needed.
 */
struct AcqFiffChannel
{
    int kind = 0;                /**< FIFFV_*_CH. */
    int coilType = 0;            /**< FIFFV_COIL_*. */
    int logicalNo = 0;           /**< logno. */
    int scannerNo = 0;           /**< scanno. */
    int unit = 0;                /**< FIFF_UNIT_*. */
    int unitMul = 0;             /**< FIFF_UNITM_*. */
    int coordFrame = 0;          /**< FIFFV_COORD_*. */
    float cal = 1.0f;            /**< FIFF cal. */
    float range = 1.0f;          /**< FIFF range. */
    std::array<float, 12> loc{}; /**< Position r0 and orientation ex, ey, ez. */
};

//=============================================================================================================
/**
 * Description of one channel. For output channels, rangeMin/rangeMax are the safety limits the encoder
 * saturates to.
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_info
 */
struct AcqChannelInfo
{
    std::string name;                                           /**< Unique within the stream. */
    AcqChannelKind kind = AcqChannelKind::Unknown;              /**< Signal kind. */
    AcqUnit unit = AcqUnit::Unknown;                            /**< SI unit of the physical value. */
    int unitExponent = 0;                                       /**< Power of ten of the unit (e.g. -6 for uV). */
    double scale = 1.0;                                         /**< Physical = raw * scale + offset. */
    double offset = 0.0;                                        /**< See scale. */
    double rangeMin = std::numeric_limits<double>::quiet_NaN(); /**< Physical range, NaN if unknown. */
    double rangeMax = std::numeric_limits<double>::quiet_NaN(); /**< Physical range, NaN if unknown. */
    std::optional<AcqFiffChannel> fiff;                         /**< Present for FIFF-described channels. */
    std::uint8_t digitalBits = 0;                               /**< Integer formats: top bits of the word used by AcqBitFields; the analog value is the remaining low bits. */
};

//=============================================================================================================
/**
 * Segmentation of a stream: continuous, or segments (triggered readouts, RF pulses, epochs) each with
 * a header delivered alongside the samples.
 */
struct AcqSegmentation
{
    /** Continuous or segmented. */
    enum class Mode : std::uint8_t
    {
        Continuous = 0, /**< No segment headers. */
        Triggered = 1   /**< Samples come in segments started by a trigger; each has an AcqSegmentHeader. */
    };

    Mode mode = Mode::Continuous;        /**< Continuous or segmented. */
    std::uint32_t samplesPerSegment = 0; /**< Fixed segment length, 0 for variable length. */
    std::uint32_t headerBytes = 0;       /**< Size of the device-specific header payload per segment. */
    std::string headerSchema;            /**< Payload format, e.g. "ismrmrd.AcquisitionHeader/1". */
};

//=============================================================================================================
/**
 * A digital line carried inside the sample word of an integer stream, sample-synchronous with the analog
 * value (marker outputs of AWG cards, T/R switch and blanking lines, recorded sync markers).
 *
 * Packing follows the hardware (Spectrum SPCM_XMODE_DIGOUTSRC_BIT15..BIT12 for output, SPC_DIGMODE for
 * input): the lines occupy the top AcqChannelInfo::digitalBits bits of the channel's word, allocated
 * from bit 15 downwards; the analog value is the remaining low bits in two's complement, i.e. the
 * full-scale value shifted right by digitalBits. decode()/encode() undo/apply that shift, so
 * AcqChannelInfo::scale stays the full-word scale; decodeBits()/encodeBits() touch only the fields.
 * For complex int16 the field lives in the I component.
 */
struct AcqBitField
{
    std::string name;                                 /**< e.g. "tr_switch", "sync". */
    std::uint32_t channel = 0;                        /**< Channel whose sample word carries the field. */
    std::uint8_t firstBit = 0;                        /**< Least significant bit of the field. */
    std::uint8_t width = 1;                           /**< Bits in the field. */
    AcqChannelKind kind = AcqChannelKind::TriggerOut; /**< RfSwitch, ClockSync, Stim, TriggerOut, ... */
};

//=============================================================================================================
/**
 * Opaque, schema-tagged metadata block that a bridge needs for a lossless round trip
 * (e.g. "fiff.MeasInfo/1" written by fiff/acq_fiff.h, "ismrmrd.Header/1").
 */
struct AcqExtension
{
    std::string schema;          /**< Identifier and version of the payload format. */
    std::vector<std::byte> data; /**< Serialized payload. */
};

//=============================================================================================================
/**
 * Complete description of one stream at one metadata version.
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_info
 */
struct AcqStreamInfo
{
    std::string name;                                            /**< Unique within its source (e.g. "eeg", "stim"). */
    std::string sourceId;                                        /**< Device serial, file path or LSL source id. */
    AcqDirection direction = AcqDirection::Input;                /**< Input or output. */
    AcqSampleFormat format = AcqSampleFormat::Float32;           /**< Native sample format stored in the ring. */
    AcqLayout layout = AcqLayout::Interleaved;                   /**< Ring layout. */
    double sfreq = 0.0;                                          /**< Nominal rate in Hz; 0 for irregular streams. */
    std::vector<AcqChannelInfo> channels;                        /**< One entry per channel, in ring order; may be empty for event-only streams. */
    AcqSegmentation segmentation;                                /**< Continuous unless stated otherwise. */
    AcqUnderrunFill underrunFill = AcqUnderrunFill::Zero;        /**< Output streams: what the device emits on underrun. */
    std::vector<AcqBitField> bitFields;                          /**< Digital lines packed into the top bits of channel words. */
    std::string clockDomain;                                     /**< Clock domain name, e.g. "spectrum:12345"; empty if free-running. */
    std::vector<std::string> eventLabels;                        /**< Optional names of AcqEvent::code values, index = code. */
    std::vector<std::pair<std::string, std::string>> properties; /**< Free-form device properties (firmware, ...). */
    std::vector<AcqExtension> extensions;                        /**< Bridge payloads for lossless round trips. */

    std::uint32_t version = 0;         /**< Metadata version; incremented on every change. */
    std::uint64_t validFromSample = 0; /**< First sample index this version describes. */

    /** @return Number of channels. */
    [[nodiscard]] std::size_t channelCount() const noexcept
    {
        return channels.size();
    }

    /** @return Bytes of one frame (all channels of one sample index). */
    [[nodiscard]] std::size_t frameBytes() const noexcept
    {
        return channels.size() * bytesPerSample(format);
    }
};

} // namespace mne

#endif // ACQ_STREAM_INFO_H
