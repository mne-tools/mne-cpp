//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     niceticks.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Axis tick positions and labels shared by the DISPLIB plot widgets; not installed.
 */

#ifndef DISPLIB_DETAIL_NICETICKS_H
#define DISPLIB_DETAIL_NICETICKS_H

#include <QString>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace DISPLIB
{
namespace detail
{

/** Ticks at 1, 2 or 5 times a power of ten, about @p target of them across [lo, hi], at least @p minStep apart. */
inline QVector<double> niceTicks(double lo, double hi, int target, double minStep = 0.0)
{
    QVector<double> ticks;
    if (!(hi > lo) || target < 1) {
        return ticks;
    }
    const double raw = std::max((hi - lo) / target, minStep);
    const int exponent = static_cast<int>(std::floor(std::log10(raw)));
    const double norm = raw / std::pow(10.0, exponent);
    const double multiple = norm < 1.5 ? 1.0 : norm < 3.0 ? 2.0
        : norm < 7.0                                      ? 5.0
                                                          : 10.0;
    // tick k is k * multiple * 10^exponent; dividing by an exact 10^-exponent gives the nearest double (0.6, not 0.6000000000000001)
    const double scale = std::pow(10.0, std::abs(exponent));
    const auto tick = [&](double k) {
        return exponent < 0 ? k * multiple / scale : k * multiple * scale;
    };
    const double step = tick(1.0);
    for (double k = std::ceil(lo / step - 1e-9); tick(k) <= hi + step * 1e-9; k += 1.0) {
        ticks.append(k == 0.0 ? 0.0 : tick(k));
    }
    return ticks;
}

/** Shortest label of a tick value. */
inline QString tickLabel(double value)
{
    return QString::number(value, 'g', 6);
}

} // namespace detail
} // namespace DISPLIB

#endif // DISPLIB_DETAIL_NICETICKS_H
