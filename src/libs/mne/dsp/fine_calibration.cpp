//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fine_calibration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Fine calibration implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fine_calibration.h"

#include <mne/base/byte_stream.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<FineCalibration> FineCalibration::read(const std::filesystem::path& path)
{
    const Expected<std::string> text = read_text_file(path);
    if (!text) {
        return std::unexpected(text.error());
    }
    FineCalibration cal;
    std::string_view rest = *text;
    while (!rest.empty()) {
        const std::size_t end = rest.find('\n');
        const std::string_view line = rest.substr(0, end);
        rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.empty() || parts.front().starts_with('#')) {
            continue;
        }
        std::string_view name = parts.front();
        if (name.starts_with("MEG")) {
            name.remove_prefix(3);
        }
        FineCalEntry entry;
        VectorXd values(std::ssize(parts) - 1);
        std::optional<int> number = to_int(name);
        bool ok = number.has_value() && (parts.size() == 14 || parts.size() == 16);
        for (Index k = 0; ok && k < values.size(); ++k) {
            const std::optional<double> value = to_double(parts[static_cast<std::size_t>(k) + 1]);
            ok = value.has_value();
            values(k) = value.value_or(0.0);
        }
        if (!ok) {
            return fail(ErrorCode::Format, "{} has no fine calibration of 14 or 16 values in line \"{}\"",
                        utf8_from_path(path), line);
        }
        entry.chNumber = *number;
        entry.position = values.head<3>();
        entry.orientation = Map<const Matrix<double, 3, 3, RowMajor>>(values.data() + 3);
        entry.imbalance = values.tail(values.size() - 12);
        cal.push_back(entry);
    }
    return cal;
}

//=============================================================================================================

Expected<void> FineCalibration::write(const std::filesystem::path& path) const
{
    std::string text;
    for (const FineCalEntry& entry : m_entries) {
        std::format_to(std::back_inserter(text), "{}", entry.chNumber);
        const Matrix<double, 3, 3, RowMajor> axes = entry.orientation;
        const auto put = [&text](double value) {
            std::format_to(std::back_inserter(text), " {:.6f}", value);
        };
        for (Index k = 0; k < 3; ++k) {
            put(entry.position(k));
        }
        for (Index k = 0; k < 9; ++k) {
            put(axes.data()[k]);
        }
        for (Index k = 0; k < entry.imbalance.size(); ++k) {
            put(entry.imbalance(k));
        }
        text += '\n';
    }
    return write_text_file(path, text);
}

//=============================================================================================================

const FineCalEntry* FineCalibration::find(int chNumber) const
{
    const auto it = std::ranges::find(m_entries, chNumber, &FineCalEntry::chNumber);
    return it == m_entries.end() ? nullptr : &*it;
}

//=============================================================================================================

VectorXd FineCalibration::gain_vector() const
{
    VectorXd gains = VectorXd::Ones(m_entries.size());
    for (int i = 0; i < std::ssize(m_entries); ++i) {
        if (m_entries[i].chNumber % 10 == 1) {
            gains(i) = m_entries[i].imbalance(0);
        }
    }
    return gains;
}

//=============================================================================================================

MatrixXd FineCalibration::imbalance_matrix() const
{
    MatrixXd imb = MatrixXd::Zero(m_entries.size(), 3);
    for (int i = 0; i < std::ssize(m_entries); ++i) {
        if (m_entries[i].chNumber % 10 != 1) {
            imb.row(i).head(m_entries[i].imbalance.size()) = m_entries[i].imbalance.transpose();
        }
    }
    return imb;
}
