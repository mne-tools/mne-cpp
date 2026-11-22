//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_ecd_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::InvEcdSet including the @c .dip and @c .bdip readers/writers.
 *
 * Implements the dipole list management plus the file-format
 * serialisation that keeps round-trip compatibility with mrilab
 * (@c .dip text format) and xfit (@c .bdip binary format) so existing
 * inspection tools continue to work on output produced by mne-cpp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_ecd_set.h"
#include <mne/base/log.h>
#include <mne/fiff/fiff_byte_swap.h>
#include <mne/fiff/fiff_types.h>


//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <charconv>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINES
//=============================================================================================================

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

namespace mne
{

/**
 * @brief Binary-format dipole record for file I/O, storing fitted dipole parameters and error estimates.
 */
struct bdipEcdRec
{
    int dipole;               /* Which dipole in a multi-dipole set */
    float begin, end;         /* Fitting time range */
    float r0[3];              /* Sphere model origin */
    float rd[3];              /* Dipole location */
    float Q[3];               /* InvDipole amplitude */
    float goodness;           /* Goodness-of-fit */
    int errors_computed;      /* Have we computed the errors */
    float noise_level;        /* Noise level used for error computations */
    float single_errors[5];   /* Single parameter error limits */
    float error_matrix[5][5]; /* This fully describes the conf. ellipsoid */
    float conf_vol;           /* The xyz confidence volume */
    float khi2;               /* The khi^2 value */
    float prob;               /* Probability to exceed khi^2 by chance */
    float noise_est;          /* Total noise estimate */
};

} // Namespace

namespace
{

constexpr std::string_view whitespace = " \t\r";

std::vector<std::string_view> split_words(std::string_view text)
{
    std::vector<std::string_view> words;
    while (true) {
        const std::size_t begin = text.find_first_not_of(whitespace);
        if (begin == std::string_view::npos) {
            return words;
        }
        text.remove_prefix(begin);
        const std::size_t end = text.find_first_of(whitespace);
        words.push_back(text.substr(0, end));
        text = (end == std::string_view::npos) ? std::string_view{} : text.substr(end);
    }
}

//=============================================================================================================

bool parse_floats(std::string_view line, std::vector<float>& values)
{
    values.clear();
    for (const std::string_view word : split_words(line)) {
        float value = 0.0f;
        const auto [ptr, ec] = std::from_chars(word.data(), word.data() + word.size(), value);
        if (ec != std::errc{} || ptr != word.data() + word.size()) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvEcdSet InvEcdSet::read_dipoles_dip(const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in) {
        mne::Log::critical("Not able to read from: {}", path.string());
        return {};
    }

    // begin end x y z Q Qx Qy Qz g/% come first; MNE-Python may append khi^2, free and confidence columns.
    constexpr std::size_t n_fixed = 10;
    std::ptrdiff_t khi2_column = -1;
    std::ptrdiff_t free_column = -1;
    bool data_seen = false;

    InvEcdSet set;
    std::string line;
    std::vector<float> values;
    while (std::getline(in, line)) {
        const std::size_t first = line.find_first_not_of(whitespace);
        if (first == std::string::npos) {
            continue;
        }
        if (line[first] == '#' || line[first] == '%') {
            if (const std::size_t gof = line.find("g/%"); !data_seen && gof != std::string::npos) {
                const std::vector<std::string_view> extra = split_words(std::string_view(line).substr(gof + 3));
                for (std::size_t k = 0; k < extra.size(); ++k) {
                    const auto column = static_cast<std::ptrdiff_t>(n_fixed + k);
                    if (extra[k] == "khi^2") {
                        khi2_column = column;
                    } else if (extra[k] == "free") {
                        free_column = column;
                    }
                }
            }
            continue;
        }
        data_seen = true;
        if (!parse_floats(line, values) || values.size() < n_fixed) {
            mne::Log::critical("Malformed dipole line in {}: {}", path.string(), line);
            return {};
        }
        InvEcd one;
        one.valid = true;
        one.time = values[0] / 1000.0f;
        one.rd = Eigen::Vector3f(values[2], values[3], values[4]) / 1000.0f;
        one.Q = Eigen::Vector3f(values[6], values[7], values[8]) / 1e9f;
        one.good = values[9] / 100.0f;
        const auto n_values = static_cast<std::ptrdiff_t>(values.size());
        if (khi2_column >= 0 && khi2_column < n_values) {
            one.khi2 = values[static_cast<std::size_t>(khi2_column)];
        }
        if (free_column >= 0 && free_column < n_values) {
            one.nfree = static_cast<int>(values[static_cast<std::size_t>(free_column)]);
        }
        set.push_back(one);
    }

    mne::Log::info("Read {} dipoles in dip format from {}", set.size(), path.string());
    return set;
}

//=============================================================================================================

bool InvEcdSet::save_dipoles_bdip(const std::filesystem::path& path) const
{
    if (path.empty() || this->size() == 0) {
        return true;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        mne::Log::critical("Not able to write to: {}", path.string());
        return false;
    }

    int nsave = 0;
    for (const InvEcd& one : m_dipoles) {
        if (!one.valid) {
            continue;
        }
        // Fields xfit fills from an error analysis stay zero.
        bdipEcdRec one_out{};
        one_out.dipole = mne::swap_int(1);
        one_out.begin = mne::swap_float(one.time);
        one_out.end = mne::swap_float(one.time);
        for (int p = 0; p < 3; p++) {
            one_out.r0[p] = mne::swap_float(0.0f);
            one_out.rd[p] = mne::swap_float(one.rd[p]);
            one_out.Q[p] = mne::swap_float(one.Q[p]);
        }
        one_out.goodness = mne::swap_float(one.good);
        one_out.errors_computed = mne::swap_int(0);
        one_out.khi2 = mne::swap_float(one.khi2);
        out.write(reinterpret_cast<const char*>(&one_out), sizeof(bdipEcdRec));
        nsave++;
    }
    if (!out.flush()) {
        mne::Log::critical("Failed to write the dipoles to {}", path.string());
        out.close();
        std::error_code ec;
        std::filesystem::remove(path, ec);
        return false;
    }
    mne::Log::info("Save {} dipoles in bdip format to {}", nsave, path.string());
    return true;
}

//=============================================================================================================

bool InvEcdSet::save_dipoles_dip(const std::filesystem::path& path) const
{
    if (path.empty() || this->size() == 0) {
        return true;
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        mne::Log::critical("Not able to write to: {}", path.string());
        return false;
    }
    // Column widths as in MNE-C save_dipoles_dip; other tools read this layout back.
    out << "# CoordinateSystem \"Head\"\n";
    out << std::format("# {:>7} {:>7} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>6}\n",
                       "begin", "end", "X (mm)", "Y (mm)", "Z (mm)", "Q(nAm)", "Qx(nAm)", "Qy(nAm)", "Qz(nAm)", "g/%");
    int nsave = 0;
    for (const InvEcd& one : m_dipoles) {
        if (one.valid) {
            const double time = 1000.0 * one.time;
            out << std::format("  {:7.1f} {:7.1f} {:8.2f} {:8.2f} {:8.2f} {:8.3f} {:8.3f} {:8.3f} {:8.3f} {:6.1f}\n",
                               time, time,
                               1000.0 * one.rd[X], 1000.0 * one.rd[Y], 1000.0 * one.rd[Z],
                               1e9 * one.Q.norm(), 1e9 * one.Q[X], 1e9 * one.Q[Y], 1e9 * one.Q[Z], 100.0 * one.good);
            nsave++;
        }
    }
    out << "## Name \"ALL dipoles\" Style \"Dipoles\"\n";
    if (!out.flush()) {
        mne::Log::critical("Failed to write the dipoles to {}", path.string());
        return false;
    }
    mne::Log::info("Save {} dipoles in dip format to {}", nsave, path.string());
    return true;
}
