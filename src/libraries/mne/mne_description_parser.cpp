//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_description_parser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref mne::MNEDescriptionParser.
 *
 * Implements the tokeniser, the @c { ... } stanza state machine and the
 * per-keyword field assignment used to populate one
 * @c mne::AverageDescription or @c mne::CovDescription per stanza.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_description_parser.h"
#include <base/log.h>

#include <base/string_utils.h>


#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <optional>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

void MNEDescriptionParser::skip_comments(std::istream& in)
{
    // A '#' at a word start comments out the rest of the line
    for (int c = in.peek(); c != std::char_traits<char>::eof(); c = in.peek()) {
        if (c == '#') {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (std::isspace(c)) {
            in.get();
        } else {
            return;
        }
    }
}

//=============================================================================================================

std::string MNEDescriptionParser::next_word(std::istream& in)
{
    skip_comments(in);

    std::string word;
    char c = 0;
    if (!in.get(c)) {
        return word; // EOF
    }
    // Quoted string
    if (c == '"') {
        std::getline(in, word, '"');
        return word;
    }
    // Regular word
    word.push_back(c);
    while (in.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            break;
        }
        if (c == '#') {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        word.push_back(c);
    }
    return word;
}

//=============================================================================================================

bool MNEDescriptionParser::get_int(std::istream& in, int& val)
{
    const std::string word = next_word(in);
    if (word.empty()) {
        mne::Log::warning("MNEDescriptionParser: expected integer, got EOF");
        return false;
    }
    const std::optional<int> parsed = mne::toInt(word);
    if (!parsed) {
        mne::Log::warning("MNEDescriptionParser: bad integer: {}", word);
        return false;
    }
    val = *parsed;
    return true;
}

//=============================================================================================================

bool MNEDescriptionParser::get_float(std::istream& in, float& val)
{
    const std::string word = next_word(in);
    if (word.empty()) {
        mne::Log::warning("MNEDescriptionParser: expected float, got EOF");
        return false;
    }
    const std::optional<double> parsed = mne::toDouble(word);
    if (!parsed) {
        mne::Log::warning("MNEDescriptionParser: bad float: {}", word);
        return false;
    }
    val = static_cast<float>(*parsed);
    return true;
}

//=============================================================================================================

bool MNEDescriptionParser::parse_rejection_param(std::string_view keyword, std::istream& in,
                                                 mne::RejectionParams& rej, bool& ok)
{
    ok = true;
    float fval;

    if (mne::equalsIgnoreCase(keyword, "gradReject")) {
        ok = get_float(in, fval);
        if (ok)
            rej.megGradReject = fval;
    } else if (mne::equalsIgnoreCase(keyword, "magReject")) {
        ok = get_float(in, fval);
        if (ok)
            rej.megMagReject = fval;
    } else if (mne::equalsIgnoreCase(keyword, "eegReject")) {
        ok = get_float(in, fval);
        if (ok)
            rej.eegReject = fval;
    } else if (mne::equalsIgnoreCase(keyword, "eogReject")) {
        ok = get_float(in, fval);
        if (ok)
            rej.eogReject = fval;
    } else if (mne::equalsIgnoreCase(keyword, "ecgReject")) {
        ok = get_float(in, fval);
        if (ok)
            rej.ecgReject = fval;
    } else if (mne::equalsIgnoreCase(keyword, "gradFlat")) {
        ok = get_float(in, fval);
        if (ok)
            rej.megGradFlat = fval;
    } else if (mne::equalsIgnoreCase(keyword, "magFlat")) {
        ok = get_float(in, fval);
        if (ok)
            rej.megMagFlat = fval;
    } else if (mne::equalsIgnoreCase(keyword, "eegFlat")) {
        ok = get_float(in, fval);
        if (ok)
            rej.eegFlat = fval;
    } else if (mne::equalsIgnoreCase(keyword, "eogFlat")) {
        ok = get_float(in, fval);
        if (ok)
            rej.eogFlat = fval;
    } else if (mne::equalsIgnoreCase(keyword, "ecgFlat")) {
        ok = get_float(in, fval);
        if (ok)
            rej.ecgFlat = fval;
    } else if (mne::equalsIgnoreCase(keyword, "stimIgnore")) {
        ok = get_float(in, fval);
        if (ok)
            rej.stimIgnore = std::fabs(fval);
    } else {
        return false; // not a rejection keyword
    }
    return true; // was a rejection keyword (ok indicates parse success)
}

//=============================================================================================================
// AVERAGE DESCRIPTION PARSER
//=============================================================================================================

std::optional<mne::AverageDescription> MNEDescriptionParser::read_average(const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in) {
        mne::Log::warning("MNEDescriptionParser: cannot open {}", mne::utf8FromPath(path));
        return std::nullopt;
    }
    mne::AverageDescription desc;
    if (!parse_average(in, path, desc)) {
        return std::nullopt;
    }
    return desc;
}

//=============================================================================================================

bool MNEDescriptionParser::parse_average(std::istream& in, const std::filesystem::path& path, mne::AverageDescription& desc)
{
    bool expectBrace = false;
    bool bminSet = false, bmaxSet = false;
    bool inAverage = false;
    bool inCategory = false;
    mne::AverageCategory currentCat;

    std::string word;
    while (!(word = next_word(in)).empty()) {
        if (expectBrace) {
            if (word == "{") {
                expectBrace = false;
            } else {
                mne::Log::warning("MNEDescriptionParser: expected '{{', got {}", word);
                return false;
            }
            continue;
        }

        if (word == "{") {
            mne::Log::warning("MNEDescriptionParser: unexpected '{{'");
            return false;
        }

        if (word == "}") {
            if (inCategory) {
                currentCat.doBaseline = bminSet && bmaxSet;
                // Validate category
                if (currentCat.comment.empty()) {
                    mne::Log::warning("MNEDescriptionParser: category name missing");
                    return false;
                }
                if (currentCat.tmin >= currentCat.tmax) {
                    mne::Log::warning("MNEDescriptionParser: illegal time range for {}", currentCat.comment);
                    return false;
                }
                if (currentCat.events.empty()) {
                    mne::Log::warning("MNEDescriptionParser: no events for {}", currentCat.comment);
                    return false;
                }
                if (currentCat.doBaseline && currentCat.bmin > currentCat.bmax) {
                    mne::Log::warning("MNEDescriptionParser: illegal baseline for {}", currentCat.comment);
                    return false;
                }
                if (!currentCat.prevIgnore)
                    currentCat.prevIgnore = currentCat.ignore;
                if (!currentCat.nextIgnore)
                    currentCat.nextIgnore = currentCat.ignore;
                desc.categories.push_back(currentCat);
                currentCat = mne::AverageCategory();
                inCategory = false;
                bminSet = bmaxSet = false;
            } else if (inAverage) {
                if (desc.comment.empty())
                    desc.comment = "Average";
                inAverage = false;
                break; // done
            }
            continue;
        }

        // Top-level: "average" keyword
        if (mne::equalsIgnoreCase(word, "average")) {
            if (inAverage || inCategory) {
                mne::Log::warning("MNEDescriptionParser: nested average");
                return false;
            }
            inAverage = true;
            expectBrace = true;
            continue;
        }

        // Category start
        if (mne::equalsIgnoreCase(word, "category") ||
            mne::equalsIgnoreCase(word, "condition")) {
            if (!inAverage || inCategory) {
                mne::Log::warning("MNEDescriptionParser: misplaced category");
                return false;
            }
            inCategory = true;
            expectBrace = true;
            bminSet = bmaxSet = false;
            continue;
        }

        // "name" keyword
        if (mne::equalsIgnoreCase(word, "name")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: name requires a value");
                return false;
            }
            if (inCategory)
                currentCat.comment = val;
            else if (inAverage)
                desc.comment = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced name");
                return false;
            }
            continue;
        }

        // "outfile" keyword
        if (mne::equalsIgnoreCase(word, "outfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: outfile requires a value");
                return false;
            }
            if (inAverage && !inCategory)
                desc.filename = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced outfile");
                return false;
            }
            continue;
        }

        // "eventfile" keyword
        if (mne::equalsIgnoreCase(word, "eventfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: eventfile requires a value");
                return false;
            }
            if (inAverage && !inCategory)
                desc.eventFile = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced eventfile");
                return false;
            }
            continue;
        }

        // "logfile" keyword
        if (mne::equalsIgnoreCase(word, "logfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: logfile requires a value");
                return false;
            }
            if (inAverage && !inCategory)
                desc.logFile = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced logfile");
                return false;
            }
            continue;
        }

        // "fixskew" keyword
        if (mne::equalsIgnoreCase(word, "fixskew")) {
            if (inAverage && !inCategory)
                desc.fixSkew = true;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced fixskew");
                return false;
            }
            continue;
        }

        // --- Category-level keywords ---
        if (mne::equalsIgnoreCase(word, "tmin")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inCategory)
                currentCat.tmin = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced tmin");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "tmax")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inCategory)
                currentCat.tmax = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced tmax");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "basemin") ||
            mne::equalsIgnoreCase(word, "bmin")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inCategory) {
                currentCat.bmin = fval;
                bminSet = true;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced basemin");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "basemax") ||
            mne::equalsIgnoreCase(word, "bmax")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inCategory) {
                currentCat.bmax = fval;
                bmaxSet = true;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced basemax");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "event")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: event numbers must be positive");
                return false;
            }
            if (inCategory)
                currentCat.events.push_back(static_cast<unsigned int>(ival));
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced event");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "nextevent")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: nextevent must be positive");
                return false;
            }
            if (inCategory)
                currentCat.nextEvent = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced nextevent");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "prevevent")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: prevevent must be positive");
                return false;
            }
            if (inCategory)
                currentCat.prevEvent = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced prevevent");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "ignore")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival < 0) {
                mne::Log::warning("MNEDescriptionParser: ignore numbers must be positive or zero");
                return false;
            }
            if (inCategory)
                currentCat.ignore = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced ignore");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "prevignore")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival < 0) {
                mne::Log::warning("MNEDescriptionParser: ignore numbers must be positive or zero");
                return false;
            }
            if (inCategory)
                currentCat.prevIgnore = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced prevignore");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "nextignore")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival < 0) {
                mne::Log::warning("MNEDescriptionParser: ignore numbers must be positive or zero");
                return false;
            }
            if (inCategory)
                currentCat.nextIgnore = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced nextignore");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "mask")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: mask must be positive");
                return false;
            }
            if (inCategory) {
                currentCat.ignore = static_cast<unsigned int>(ival);
                currentCat.ignore = ~currentCat.ignore;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced mask");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "prevmask")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: mask numbers must be positive");
                return false;
            }
            if (inCategory) {
                currentCat.prevIgnore = static_cast<unsigned int>(ival);
                currentCat.prevIgnore = ~currentCat.prevIgnore;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced prevmask");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "nextmask")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: mask numbers must be positive");
                return false;
            }
            if (inCategory) {
                currentCat.nextIgnore = static_cast<unsigned int>(ival);
                currentCat.nextIgnore = ~currentCat.nextIgnore;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced nextmask");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "delay")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inCategory)
                currentCat.delay = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced delay");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "stderr")) {
            if (inCategory)
                currentCat.doStdErr = true;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced stderr");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "abs")) {
            if (inCategory)
                currentCat.doAbs = true;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced abs");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "color")) {
            float r, g, b;
            if (!get_float(in, r) || !get_float(in, g) || !get_float(in, b))
                return false;
            if (r < 0.0f || r > 1.0f || g < 0.0f || g > 1.0f || b < 0.0f || b > 1.0f) {
                mne::Log::warning("MNEDescriptionParser: color components must be within 0 ... 1");
                return false;
            }
            if (inCategory) {
                currentCat.color[0] = r;
                currentCat.color[1] = g;
                currentCat.color[2] = b;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced color");
                return false;
            }
            continue;
        }

        // Try rejection parameters (average-level only)
        bool parseOk;
        if (parse_rejection_param(word, in, desc.rej, parseOk)) {
            if (!(inAverage && !inCategory)) {
                mne::Log::warning("MNEDescriptionParser: misplaced {}", word);
                return false;
            }
            if (!parseOk)
                return false;
            continue;
        }

        mne::Log::warning("MNEDescriptionParser: unknown keyword {} in {}", word, mne::utf8FromPath(path));
    }

    if (desc.categories.empty()) {
        mne::Log::warning("MNEDescriptionParser: no categories found in {}", mne::utf8FromPath(path));
        return false;
    }

    return true;
}

//=============================================================================================================
// COVARIANCE DESCRIPTION PARSER
//=============================================================================================================

std::optional<CovDescription> MNEDescriptionParser::read_covariance(const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in) {
        mne::Log::warning("MNEDescriptionParser: cannot open {}", mne::utf8FromPath(path));
        return std::nullopt;
    }
    CovDescription desc;
    if (!parse_covariance(in, path, desc)) {
        return std::nullopt;
    }
    return desc;
}

//=============================================================================================================

bool MNEDescriptionParser::parse_covariance(std::istream& in, const std::filesystem::path& path, CovDescription& desc)
{
    bool expectBrace = false;
    bool bminSet = false, bmaxSet = false;
    bool inCov = false;
    bool seenCov = false;
    bool inDef = false;
    CovDefinition currentDef;

    std::string word;
    while (!(word = next_word(in)).empty()) {
        if (expectBrace) {
            if (word == "{") {
                expectBrace = false;
            } else {
                mne::Log::warning("MNEDescriptionParser: expected '{{', got {}", word);
                return false;
            }
            continue;
        }

        if (word == "{") {
            mne::Log::warning("MNEDescriptionParser: unexpected '{{'");
            return false;
        }

        if (word == "}") {
            if (inDef) {
                currentDef.doBaseline = bminSet && bmaxSet;
                if (currentDef.tmin >= currentDef.tmax) {
                    mne::Log::warning("MNEDescriptionParser: illegal time range in def");
                    return false;
                }
                if (currentDef.doBaseline && currentDef.bmin > currentDef.bmax) {
                    mne::Log::warning("MNEDescriptionParser: illegal baseline in def");
                    return false;
                }
                desc.defs.push_back(currentDef);
                currentDef = CovDefinition();
                inDef = false;
                bminSet = bmaxSet = false;
            } else if (inCov) {
                inCov = false;
                break;
            }
            continue;
        }

        if (mne::equalsIgnoreCase(word, "cov")) {
            if (inCov || inDef) {
                mne::Log::warning("MNEDescriptionParser: nested cov");
                return false;
            }
            inCov = true;
            seenCov = true;
            expectBrace = true;
            continue;
        }

        if (mne::equalsIgnoreCase(word, "def")) {
            if (!inCov || inDef) {
                mne::Log::warning("MNEDescriptionParser: misplaced def");
                return false;
            }
            inDef = true;
            expectBrace = true;
            bminSet = bmaxSet = false;
            continue;
        }

        // Cov-level keywords
        if (mne::equalsIgnoreCase(word, "outfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: outfile requires a value");
                return false;
            }
            if (inCov && !inDef)
                desc.filename = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced outfile");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "eventfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: eventfile requires a value");
                return false;
            }
            if (inCov && !inDef)
                desc.eventFile = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced eventfile");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "logfile")) {
            const std::string val = next_word(in);
            if (val.empty()) {
                mne::Log::warning("MNEDescriptionParser: logfile requires a value");
                return false;
            }
            if (inCov && !inDef)
                desc.logFile = val;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced logfile");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "keepsamplemean")) {
            if (inCov && !inDef)
                desc.removeSampleMean = false;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced keepsamplemean");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "fixskew")) {
            if (inCov && !inDef)
                desc.fixSkew = true;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced fixskew");
                return false;
            }
            continue;
        }

        // Def-level keywords
        if (mne::equalsIgnoreCase(word, "tmin")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inDef)
                currentDef.tmin = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced tmin");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "tmax")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inDef)
                currentDef.tmax = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced tmax");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "basemin") ||
            mne::equalsIgnoreCase(word, "bmin")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inDef) {
                currentDef.bmin = fval;
                bminSet = true;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced basemin");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "basemax") ||
            mne::equalsIgnoreCase(word, "bmax")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inDef) {
                currentDef.bmax = fval;
                bmaxSet = true;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced basemax");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "event")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: event must be positive");
                return false;
            }
            if (inDef)
                currentDef.events.push_back(static_cast<unsigned int>(ival));
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced event");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "ignore")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival < 0) {
                mne::Log::warning("MNEDescriptionParser: ignore numbers must be positive or zero");
                return false;
            }
            if (inDef)
                currentDef.ignore = static_cast<unsigned int>(ival);
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced ignore");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "mask")) {
            int ival;
            if (!get_int(in, ival))
                return false;
            if (ival <= 0) {
                mne::Log::warning("MNEDescriptionParser: mask numbers must be positive");
                return false;
            }
            if (inDef) {
                currentDef.ignore = static_cast<unsigned int>(ival);
                currentDef.ignore = ~currentDef.ignore;
            } else {
                mne::Log::warning("MNEDescriptionParser: misplaced mask");
                return false;
            }
            continue;
        }
        if (mne::equalsIgnoreCase(word, "delay")) {
            float fval;
            if (!get_float(in, fval))
                return false;
            if (inDef)
                currentDef.delay = fval;
            else {
                mne::Log::warning("MNEDescriptionParser: misplaced delay");
                return false;
            }
            continue;
        }

        // Rejection parameters (cov-level only)
        bool parseOk;
        if (parse_rejection_param(word, in, desc.rej, parseOk)) {
            if (!(inCov && !inDef)) {
                mne::Log::warning("MNEDescriptionParser: misplaced {}", word);
                return false;
            }
            if (!parseOk)
                return false;
            continue;
        }

        mne::Log::warning("MNEDescriptionParser: unknown keyword {} in {}", word, mne::utf8FromPath(path));
    }

    if (!seenCov) {
        mne::Log::warning("MNEDescriptionParser: nothing useful read from {}", mne::utf8FromPath(path));
        return false;
    }

    return true;
}
