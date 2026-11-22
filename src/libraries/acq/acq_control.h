//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_control.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Device control: the parameters, commands and status values a device publishes, and their events.
 *
 * Every device describes what it lets a program set, run and watch while it is open, in one list of
 * AcqControlInfo: parameters (a typed value, readable, optionally writable), commands (an operation with typed
 * arguments and a result) and status values (read-only telemetry). A program reads, writes and invokes them by id
 * through AcqDevice and learns about changes from AcqControlEvent records, which the backend publishes from any
 * thread, the acquisition thread included, without locks or allocation. A change that alters how samples are
 * scaled or named is also a new metadata version of the stream, valid from the sample it applies to
 * (AcqStream::info(sample)), so data and their description never disagree.
 *
 * The model is generic on purpose: an amplifier's range, a MEG system's SQUID tuning, a file simulator's file and
 * an MR console's sequence are all parameters and commands of the same kind, so one user interface serves every
 * device.
 *
 * @snippet ex_acq_oddball/main.cpp acq_control_usage
 */

#ifndef ACQ_CONTROL_H
#define ACQ_CONTROL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * A control's value: nothing, a flag, an integer, a number or a text (enum values are texts).
 */
using AcqValue = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

//=============================================================================================================
/**
 * What a control is.
 */
enum class AcqControlKind : std::uint8_t
{
    Parameter = 0, /**< A value that can be read and, if writable, set. */
    Command = 1,   /**< An operation that is invoked with arguments and returns a value. */
    Status = 2     /**< A read-only value the device updates by itself, e.g. a temperature or a buffer fill. */
};

//=============================================================================================================
/**
 * Type of a control's value, or of a command's result and arguments.
 */
enum class AcqValueType : std::uint8_t
{
    None = 0,   /**< No value (a command without a result). */
    Bool = 1,   /**< bool. */
    Int = 2,    /**< std::int64_t. */
    Double = 3, /**< double. */
    String = 4, /**< std::string. */
    Enum = 5    /**< std::string, one of AcqControlInfo::options. */
};

//=============================================================================================================
/**
 * Device states, as a bit mask of the states in which a control may be written or invoked.
 */
enum AcqControlWhen : std::uint8_t
{
    AcqWhenOpened = 1,                                              /**< Opened, before the first start(). */
    AcqWhenRunning = 2,                                             /**< Running. */
    AcqWhenStopped = 4,                                             /**< Stopped. */
    AcqWhenAlways = AcqWhenOpened | AcqWhenRunning | AcqWhenStopped /**< Whenever the device is open. */
};

//=============================================================================================================
/**
 * Description of one control.
 *
 * @snippet ex_acq_oddball/main.cpp acq_control_usage
 */
struct AcqControlInfo
{
    std::string id;                                            /**< Unique within the device, e.g. "squid/fll/mode". */
    std::string label;                                         /**< Short name for a user interface. */
    std::string group;                                         /**< User-interface group, e.g. "Amplifier". */
    std::string description;                                   /**< One sentence. */
    AcqControlKind kind = AcqControlKind::Parameter;           /**< Parameter, command or status. */
    AcqValueType type = AcqValueType::None;                    /**< Type of the value or of the command's result. */
    bool writable = false;                                     /**< Parameters: whether write() may set it. */
    std::uint8_t when = AcqWhenAlways;                         /**< AcqControlWhen states in which write/invoke is allowed. */
    std::string unit;                                          /**< Unit of a number, e.g. "uV", "Hz"; empty if none. */
    double minimum = -std::numeric_limits<double>::infinity(); /**< Smallest valid number. */
    double maximum = std::numeric_limits<double>::infinity();  /**< Largest valid number. */
    std::vector<std::string> options;                          /**< Valid values of an Enum. */
    std::vector<AcqValueType> arguments;                       /**< Commands: types of the arguments, in order. */
    bool changesStreamInfo = false;                            /**< Setting or invoking it publishes a new stream metadata version. */
};

//=============================================================================================================
/**
 * What an AcqControlEvent reports.
 */
enum class AcqControlEventKind : std::uint8_t
{
    ValueChanged = 0, /**< A parameter took a new value, from @c sample on if it applies to samples. */
    Status = 1,       /**< A status value was updated. */
    Alert = 2,        /**< Something the operator should see, with a severity and a text. */
    CommandDone = 3   /**< A command finished; @c value is its result, @c text its error if it failed. */
};

//=============================================================================================================
/**
 * Severity of an alert.
 */
enum class AcqSeverity : std::uint8_t
{
    Info = 0,    /**< For the record. */
    Warning = 1, /**< Data may be affected; acquisition continues. */
    Error = 2    /**< Data are affected or a command failed. */
};

//=============================================================================================================
/**
 * A change or message of a device's control state. Trivially copyable, so the backend can publish it from the
 * acquisition thread without allocation; texts longer than @c text are cut.
 *
 * @snippet ex_acq_oddball/main.cpp acq_control_usage
 */
struct AcqControlEvent
{
    static constexpr std::uint64_t kNoSample = std::numeric_limits<std::uint64_t>::max();  /**< @c sample when none applies. */
    static constexpr std::uint32_t kNoControl = std::numeric_limits<std::uint32_t>::max(); /**< @c control of a device-wide alert. */

    AcqControlEventKind kind = AcqControlEventKind::Alert;          /**< What happened. */
    std::uint32_t control = kNoControl;                             /**< Index into AcqDevice::controls(), or kNoControl. */
    std::variant<std::monostate, bool, std::int64_t, double> value; /**< The new value or result; texts are read with read(). */
    AcqSeverity severity = AcqSeverity::Info;                       /**< Severity of an alert or a failed command. */
    std::uint64_t sample = kNoSample;                               /**< First sample of stream 0 the change applies to. */
    std::int64_t hostNs = 0;                                        /**< steady_clock time of the event in nanoseconds. */
    std::array<char, 120> text{};                                   /**< Null-terminated message. */

    /** @return The message. */
    [[nodiscard]] std::string_view message() const noexcept
    {
        std::size_t n = 0;
        while (n < text.size() && text[n] != '\0') {
            ++n;
        }
        return {text.data(), n};
    }

    /**
     * Sets the message, cut to fit.
     *
     * @param[in] message   The text.
     */
    void setMessage(std::string_view message) noexcept
    {
        const std::size_t n = message.size() < text.size() - 1 ? message.size() : text.size() - 1;
        for (std::size_t i = 0; i < n; ++i) {
            text[i] = message[i];
        }
        text[n] = '\0';
    }
};

} // namespace mne

#endif // ACQ_CONTROL_H
