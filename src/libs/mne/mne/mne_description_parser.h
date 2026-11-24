//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_description_parser.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Parser for the legacy MNE-C @c .ave / @c .cov plain-text description files.
 *
 * @ref mne::MNEDescriptionParser tokenises the @c "average { ... }"
 * / @c "cov { ... }" stanzas used by @c mne_process_raw and turns them
 * into @c mne::AverageDescription / @c mne::CovDescription records that the epoch /
 * covariance machinery downstream can act on.
 */

#ifndef MNE_DESCRIPTION_PARSER_H
#define MNE_DESCRIPTION_PARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_process_description.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Parses MNE-C style averaging (.ave) and covariance (.cov) description files.
 *
 * File format is a keyword-value text format with nested blocks:
 *   average { ... category { ... } ... }
 *   cov { ... def { ... } ... }
 *
 * Lines starting with '#' are comments.
 *
 * @snippet ex_mne_api/main.cpp mne_description_parser_usage
 */
class MNESHARED_EXPORT MNEDescriptionParser
{
public:
    /**
     * Reads an averaging description file.
     *
     * @param[in] path   The .ave file.
     *
     * @return The description, or no value if the file cannot be read or is malformed.
     */
    static std::optional<mne::AverageDescription> read_average(const std::filesystem::path& path);

    /**
     * Reads a covariance description file.
     *
     * @param[in] path   The .cov file.
     *
     * @return The description, or no value if the file cannot be read or is malformed.
     */
    static std::optional<CovDescription> read_covariance(const std::filesystem::path& path);

private:
    static bool parse_average(std::istream& in, const std::filesystem::path& path, mne::AverageDescription& desc);

    static bool parse_covariance(std::istream& in, const std::filesystem::path& path, CovDescription& desc);

    /**
     * Skip comment lines (starting with '#') and whitespace.
     */
    static void skip_comments(std::istream& in);

    /**
     * Read the next whitespace-delimited word, handling quoted strings.
     * Returns empty string on EOF.
     */
    static std::string next_word(std::istream& in);

    /**
     * Read and parse an integer value.
     */
    static bool get_int(std::istream& in, int& val);

    /**
     * Read and parse a float value.
     */
    static bool get_float(std::istream& in, float& val);

    /**
     * Parse rejection parameters common to both .ave and .cov files.
     * Returns true if the keyword was handled, false otherwise (unrecognized keyword).
     */
    static bool parse_rejection_param(std::string_view keyword, std::istream& in,
                                      mne::RejectionParams& rej, bool& ok);
};

} // namespace mne

#endif // MNE_DESCRIPTION_PARSER_H
