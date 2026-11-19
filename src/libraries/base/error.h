//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     error.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Error and Expected: why a reader or writer failed, as a value or as an exception.
 *
 * Like std::filesystem, every reader comes in two forms: one that throws, for compact code that cannot
 * continue without the data (FiffCov cov(path)), and one that returns Expected<T>, for code that handles the
 * failure (if (auto cov = FiffCov::read(path)) ...). Both carry the same Error, whose message names the file.
 */

#ifndef BASE_ERROR_H
#define BASE_ERROR_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <expected>
#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * What went wrong, so callers can react without parsing the message.
 */
enum class ErrorCode
{
    Io,      /**< The file cannot be opened, read or written. */
    Format,  /**< The file is not of the expected kind or is damaged. */
    NotFound /**< The file is valid but does not hold what was asked for. */
};

//=============================================================================================================
/**
 * @brief Why a reader or writer failed; what() names the cause and the file.
 *
 * @snippet ex_fiff_api/main.cpp fiff_coord_trans_usage
 */
class Error : public std::runtime_error
{
public:
    Error(ErrorCode code, const std::string& message)
    : std::runtime_error(message)
    , code(code)
    {
    }

    ErrorCode code; /**< The kind of failure. */
};

//=============================================================================================================
/**
 * @brief A value or the Error why there is none; std::expected whose value() throws that Error.
 *
 * @tparam T   The value type, or void for writers.
 *
 * @snippet ex_fiff_api/main.cpp expected_usage
 */
template<class T>
class Expected : public std::expected<T, Error>
{
public:
    using std::expected<T, Error>::expected;

    Expected(std::expected<T, Error> other)
    : std::expected<T, Error>(std::move(other))
    {
    }

    /** @return The value; throws the Error, rather than std::bad_expected_access, if there is none. */
    decltype(auto) value() &
    {
        return checked(*this);
    }

    /** @copydoc value() */
    decltype(auto) value() const&
    {
        return checked(*this);
    }

    /** @copydoc value() */
    decltype(auto) value() &&
    {
        return checked(std::move(*this));
    }

private:
    template<class Self>
    static decltype(auto) checked(Self&& self)
    {
        if (!self.has_value()) {
            throw Error(self.error());
        }
        if constexpr (!std::is_void_v<T>) {
            return *std::forward<Self>(self);
        }
    }
};

//=============================================================================================================
/**
 * @param[in] code     The kind of failure.
 * @param[in] format   A std::format string for the message, checked at compile time.
 * @param[in] args     The values it formats.
 *
 * @return The error, ready to return from a function that returns Expected.
 */
template<class... Args>
std::unexpected<Error> fail(ErrorCode code, std::format_string<Args...> format, Args&&... args)
{
    return std::unexpected(Error(code, std::format(format, std::forward<Args>(args)...)));
}

} // namespace mne

#endif // BASE_ERROR_H
