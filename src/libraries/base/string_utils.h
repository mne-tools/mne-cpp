//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     string_utils.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Text and path helpers in standard C++ for the QString calls the libraries use most.
 *
 * Strings are UTF-8; case and whitespace handling covers ASCII only, which is what file formats and channel
 * names need.
 */

#ifndef BASE_STRING_UTILS_H
#define BASE_STRING_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/** Ordered list of names, e.g. channel names; the standard-library counterpart of QStringList. */
using StringList = std::vector<std::string>;

/**
 * @param[in] text   Any text.
 *
 * @return @p text without leading and trailing ASCII whitespace, like QString::trimmed().
 */
BASESHARED_EXPORT std::string_view trimmed(std::string_view text);

/**
 * @param[in] text        Any text.
 * @param[in] separator   Field separator.
 * @param[in] skipEmpty   Whether to drop empty fields, like Qt::SkipEmptyParts.
 *
 * @return The fields of @p text, like QString::split(); empty text gives one empty field unless skipped.
 */
BASESHARED_EXPORT StringList split(std::string_view text, char separator, bool skipEmpty = false);

/**
 * @param[in] text   Any text, e.g. a line of a whitespace-separated table.
 *
 * @return The fields of @p text between runs of ASCII whitespace, like
 *         QString::split(QRegularExpression("\\s+"), Qt::SkipEmptyParts); they view @p text.
 */
BASESHARED_EXPORT std::vector<std::string_view> split_whitespace(std::string_view text);

//=============================================================================================================
/**
 * @param[in] list   A list of names, e.g. channel names.
 * @param[in] name   The name to look for, compared exactly.
 *
 * @return Index of the first occurrence of @p name, or -1, like QStringList::indexOf().
 */
BASESHARED_EXPORT int index_of(const StringList& list, std::string_view name);

//=============================================================================================================
/**
 * @param[in] list        A list of names.
 * @param[in] separator   Text put between two names.
 *
 * @return The names joined by @p separator, like QStringList::join().
 */
BASESHARED_EXPORT std::string join(const StringList& list, std::string_view separator);

/**
 * @param[in] text   UTF-8 text.
 *
 * @return @p text with ASCII letters in lower case; other bytes unchanged.
 */
BASESHARED_EXPORT std::string to_lower(std::string_view text);

/**
 * @param[in] a   UTF-8 text.
 * @param[in] b   UTF-8 text.
 *
 * @return Whether @p a and @p b are equal when ASCII letters are compared without case.
 */
BASESHARED_EXPORT bool iequals(std::string_view a, std::string_view b);

/**
 * @param[in] text   A decimal number with optional surrounding whitespace, independent of the locale.
 *
 * @return The value, or nothing if @p text is not entirely a finite or infinite number, like QString::toDouble().
 */
BASESHARED_EXPORT std::optional<double> to_double(std::string_view text);

/**
 * @param[in] text   A decimal integer with optional surrounding whitespace.
 *
 * @return The value, or nothing if @p text is not entirely an int in range, like QString::toInt().
 */
BASESHARED_EXPORT std::optional<int> to_int(std::string_view text);

/**
 * @param[in] utf8   A path in UTF-8.
 *
 * @return The path; unlike std::filesystem::path(std::string), correct for non-ASCII names on Windows too.
 */
BASESHARED_EXPORT std::filesystem::path path_from_utf8(std::string_view utf8);

/**
 * @param[in] path   Any path.
 *
 * @return @p path in UTF-8 on every platform.
 */
BASESHARED_EXPORT std::string utf8_from_path(const std::filesystem::path& path);

/**
 * @param[in] latin1   ISO 8859-1 text, one byte per character.
 *
 * @return The same characters in UTF-8.
 */
BASESHARED_EXPORT std::string latin1_to_utf8(std::string_view latin1);

/**
 * @param[in] text   Any bytes.
 *
 * @return Whether @p text is well-formed UTF-8 (no overlong forms, surrogates or code points above U+10FFFF).
 */
BASESHARED_EXPORT bool is_utf8(std::string_view text);

/**
 * @param[in] data   Any bytes.
 *
 * @return @p data as lowercase hex digits, two per byte, like QByteArray::toHex().
 */
BASESHARED_EXPORT std::string to_hex(std::span<const std::byte> data);

/**
 * @param[in] data   Any bytes.
 * @param[in] url    Whether to use the URL-safe alphabet without padding (RFC 4648 section 5).
 *
 * @return @p data in base64, like QByteArray::toBase64().
 */
BASESHARED_EXPORT std::string to_base64(std::span<const std::byte> data, bool url = false);

/**
 * @param[in] text   Base64 in the standard or the URL-safe alphabet, padded or not; whitespace is skipped.
 *
 * @return The bytes, or nothing if @p text holds another character or is cut short.
 */
BASESHARED_EXPORT std::optional<std::vector<std::byte>> from_base64(std::string_view text);

/** A UTC time point with millisecond resolution, e.g. a file's creation time. */
using UtcTime = std::chrono::sys_time<std::chrono::milliseconds>;

/**
 * @param[in] time           A UTC time.
 * @param[in] milliseconds   Whether to append the milliseconds.
 *
 * @return @p time as ISO 8601 text, "2026-11-15T09:30:00Z" or "2026-11-15T09:30:00.250Z", like
 *         QDateTime::toString(Qt::ISODate) and Qt::ISODateWithMs.
 */
BASESHARED_EXPORT std::string format_iso8601(UtcTime time, bool milliseconds = false);

/**
 * @param[in] text   ISO 8601 date YYYY-MM-DD, optionally followed by 'T' or ' ' and hh:mm[:ss[.fraction]] and 'Z',
 *                   an offset like +02:00, or nothing for UTC.
 *
 * @return The UTC time, or nothing if @p text is not such a date and time.
 */
BASESHARED_EXPORT std::optional<UtcTime> parse_iso8601(std::string_view text);

/**
 * @return The directory of the running executable with symbolic links resolved, like
 *         QCoreApplication::applicationDirPath() but without an application object; empty if unknown.
 */
BASESHARED_EXPORT std::filesystem::path executable_dir();

/**
 * @param[in] relative   A path below the resources directory in UTF-8, e.g. "general/coilDefinitions/coil_def.dat".
 *
 * @return The file in the resources directory installed next to the executable's directory
 *         (executable_dir() / ".." / "resources"), or the same path relative to the working directory if that does not exist.
 */
BASESHARED_EXPORT std::filesystem::path resource_path(std::string_view relative);

} // namespace mne

#endif // BASE_STRING_UTILS_H
