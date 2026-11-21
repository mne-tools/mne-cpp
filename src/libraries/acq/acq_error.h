//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_error.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqError: catalogued error codes of the acquisition API, and AcqExpected, a value-or-error result.
 *
 * Every error the public API can return has a catalogue entry: a stable code, a message that
 * names the cause, the fix, and a documentation anchor. Catalogue strings are static, so an AcqError on
 * the data path (Timeout, Overrun, Late, ...) never allocates; only control-path errors attach a context
 * string. Neither type throws, so mne_acq builds with -fno-exceptions and -fno-rtti.
 */

#ifndef ACQ_ERROR_H
#define ACQ_ERROR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cassert>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Stable error codes. Values are part of the ABI and are never renumbered.
 */
enum class AcqErrorCode : std::uint16_t
{
    None = 0, /**< Success. */
    // Data path (allocation-free)
    Timeout = 1,         /**< No data (reader) or no space (output writer) within the timeout. */
    EndOfStream = 2,     /**< The producer finished and every committed sample has been read. */
    Overrun = 3,         /**< The producer overwrote samples while a view was held; discard the view's data. */
    DeviceStopped = 4,   /**< The source was stopped while waiting. */
    Late = 5,            /**< An output sample, event or segment targets a sample the device has already played. */
    NotSynchronized = 6, /**< The streams are not phase-locked in one clock domain, or the reference was lost. */
    BeforeStart = 7,     /**< The mapped instant precedes sample 0 of the target stream. */
    NotOnGrid = 8,       /**< The instant is not on the target's schedulable grid (sampleAlignment); check the clock setup. */
    // Control path
    InvalidUri = 100,          /**< The URI does not have the form scheme://target?key=value. */
    UnknownScheme = 101,       /**< No backend is registered for the URI scheme. */
    InvalidParameter = 102,    /**< A URI or configuration parameter is unknown or out of range. */
    BackendUnavailable = 103,  /**< A runtime-loaded library (BrainFlow, liblsl, vendor SDK) was not found. */
    UnsupportedVersion = 104,  /**< A runtime-loaded library has a version outside the supported range. */
    DeviceNotFound = 105,      /**< The device, file or network endpoint does not exist or does not answer. */
    DeviceIo = 106,            /**< The device or file failed during acquisition or output. */
    InvalidState = 107,        /**< The call is not allowed in the current state. */
    UnsupportedFormat = 108,   /**< The sample format or layout cannot be used for this operation. */
    ShapeMismatch = 109,       /**< A caller-owned buffer has the wrong shape. */
    TooManyReaders = 110,      /**< All reader slots of the stream are in use. */
    InvalidBuffer = 111,       /**< An adopted external buffer is misaligned or not a power of two in samples. */
    OutOfMemory = 112,         /**< The ring or its memory lock could not be allocated. */
    PlatformUnsupported = 113, /**< The backend or option is not available on this platform. */
    WrongDirection = 114,      /**< A writer was requested for an input stream, or a gating reader for an input stream. */
    WriterInUse = 115,         /**< The output stream already has its writer. */
    DeviceMemory = 116,        /**< The samples live in GPU memory, which the CPU must not read or write. */
    NotSupported = 117,        /**< The device does not offer this function (e.g. impedance measurement). */
    PermissionDenied = 118,    /**< The operating system refused the request (e.g. a realtime priority without the right). */
    Internal = 199             /**< Invariant violation inside mne_acq; please report it. */
};

//=============================================================================================================
/**
 * Catalogue entry of one error code. All strings have static storage duration.
 */
struct AcqErrorInfo
{
    AcqErrorCode code;          /**< The code this entry describes. */
    std::string_view name;      /**< Stable identifier, e.g. "UnknownScheme". */
    std::string_view message;   /**< What went wrong, naming the cause. */
    std::string_view fix;       /**< What the user can do about it. */
    std::string_view docAnchor; /**< Anchor on the acquisition error documentation page. */
};

//=============================================================================================================
/**
 * @param[in] code   The error code.
 *
 * @return The static catalogue entry of @p code; an unknown value maps to the Internal entry.
 */
ACQSHARED_EXPORT const AcqErrorInfo& acqErrorInfo(AcqErrorCode code) noexcept;

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * An error value: a catalogue code plus an optional context string.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_acquire
 */
class ACQSHARED_EXPORT AcqError
{
public:
    AcqError() noexcept = default;

    /**
     * Creates an error without context; allocation-free, usable on the data path.
     *
     * @param[in] code   The error code.
     */
    constexpr AcqError(AcqErrorCode code) noexcept
    : m_code(code)
    {
    }

    /**
     * Creates an error with context (control path, allocates).
     *
     * @param[in] code      The error code.
     * @param[in] context   What the error refers to, e.g. the URI or file path.
     */
    AcqError(AcqErrorCode code, std::string context);

    /** @return true when this value represents success. */
    [[nodiscard]] constexpr bool ok() const noexcept
    {
        return m_code == AcqErrorCode::None;
    }

    /** @return The error code. */
    [[nodiscard]] constexpr AcqErrorCode code() const noexcept
    {
        return m_code;
    }

    /** @return The context string, empty if none was attached. */
    [[nodiscard]] std::string_view context() const noexcept;

    /** @return The catalogue entry of the code. */
    [[nodiscard]] const AcqErrorInfo& info() const noexcept
    {
        return acqErrorInfo(m_code);
    }

    /** @return "message (context). fix" ready for printing (allocates). */
    [[nodiscard]] std::string text() const;

    friend constexpr bool operator==(const AcqError& lhs, AcqErrorCode rhs) noexcept
    {
        return lhs.m_code == rhs;
    }

private:
    AcqErrorCode m_code = AcqErrorCode::None;
    std::shared_ptr<const std::string> m_context;
};
ACQ_STD_MEMBERS_END

//=============================================================================================================
/**
 * Value-or-error result, the subset of C++23 std::expected that mne_acq needs, without exceptions:
 * accessing the wrong alternative is a precondition violation (assert), never a throw.
 *
 * @tparam T   The value type; may be move-only.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_acquire
 */
template<typename T>
class AcqExpected
{
public:
    /**
     * Holds a value.
     *
     * @param[in] value   The value.
     */
    AcqExpected(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    : m_hasValue(true)
    {
        ::new (static_cast<void*>(&m_value)) T(std::move(value));
    }

    /** @copydoc AcqExpected(T&&) */
    AcqExpected(const T& value)
        requires std::is_copy_constructible_v<T>
    : m_hasValue(true)
    {
        ::new (static_cast<void*>(&m_value)) T(value);
    }

    /**
     * Holds an error.
     *
     * @param[in] error   The error. @pre !error.ok()
     */
    AcqExpected(AcqError error) noexcept
    : m_hasValue(false)
    {
        assert(!error.ok());
        ::new (static_cast<void*>(&m_error)) AcqError(std::move(error));
    }

    /**
     * Holds an error without context.
     *
     * @param[in] code   The error code. @pre code != AcqErrorCode::None
     */
    AcqExpected(AcqErrorCode code) noexcept
    : AcqExpected(AcqError(code))
    {
    }

    AcqExpected(const AcqExpected& other)
        requires std::is_copy_constructible_v<T>
    : m_hasValue(other.m_hasValue)
    {
        if (m_hasValue) {
            ::new (static_cast<void*>(&m_value)) T(other.m_value);
        } else {
            ::new (static_cast<void*>(&m_error)) AcqError(other.m_error);
        }
    }

    AcqExpected(AcqExpected&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    : m_hasValue(other.m_hasValue)
    {
        if (m_hasValue) {
            ::new (static_cast<void*>(&m_value)) T(std::move(other.m_value));
        } else {
            ::new (static_cast<void*>(&m_error)) AcqError(std::move(other.m_error));
        }
    }

    AcqExpected& operator=(AcqExpected other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        destroy();
        m_hasValue = other.m_hasValue;
        if (m_hasValue) {
            ::new (static_cast<void*>(&m_value)) T(std::move(other.m_value));
        } else {
            ::new (static_cast<void*>(&m_error)) AcqError(std::move(other.m_error));
        }
        return *this;
    }

    ~AcqExpected()
    {
        destroy();
    }

    /** @return true when a value is held. */
    [[nodiscard]] bool hasValue() const noexcept
    {
        return m_hasValue;
    }
    explicit operator bool() const noexcept
    {
        return m_hasValue;
    }

    /** @return The held value. @pre hasValue() */
    [[nodiscard]] T& value() & noexcept
    {
        assert(m_hasValue);
        return m_value;
    }
    [[nodiscard]] const T& value() const& noexcept
    {
        assert(m_hasValue);
        return m_value;
    }
    [[nodiscard]] T&& value() && noexcept
    {
        assert(m_hasValue);
        return std::move(m_value);
    }

    T& operator*() & noexcept
    {
        return value();
    }
    const T& operator*() const& noexcept
    {
        return value();
    }

    /** @return The value, moved out of a temporary result, e.g. *stream.reader(mode). @pre hasValue() */
    T&& operator*() && noexcept
    {
        return std::move(*this).value();
    }
    T* operator->() noexcept
    {
        return &value();
    }
    const T* operator->() const noexcept
    {
        return &value();
    }

    /** @return The held error, or success when a value is held. */
    [[nodiscard]] AcqError error() const noexcept
    {
        return m_hasValue ? AcqError() : m_error;
    }

private:
    void destroy() noexcept
    {
        if (m_hasValue) {
            m_value.~T();
        } else {
            m_error.~AcqError();
        }
    }

    union
    {
        T m_value;
        AcqError m_error;
    };
    bool m_hasValue;
};

} // namespace mne

#endif // ACQ_ERROR_H
