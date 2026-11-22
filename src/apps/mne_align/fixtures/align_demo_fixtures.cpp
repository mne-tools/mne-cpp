//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     align_demo_fixtures.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the mne_align synthetic demo fixtures.
 */

#include "align_demo_fixtures.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>

namespace MNEALIGN
{

using mne::AcquiredPoints;
using mne::DigitizedPoint;
using mne::FiducialId;
using mne::PointKind;

std::vector<DigitizedPoint> demoFiducials()
{
    std::vector<DigitizedPoint> out;
    out.reserve(3);

    DigitizedPoint nas;
    nas.kind = PointKind::Fiducial;
    nas.label = "NAS";
    nas.ident_number = static_cast<int>(FiducialId::NAS);
    nas.position = Eigen::Vector3f(0.000f, 0.095f, 0.005f);
    out.push_back(nas);

    DigitizedPoint lpa;
    lpa.kind = PointKind::Fiducial;
    lpa.label = "LPA";
    lpa.ident_number = static_cast<int>(FiducialId::LPA);
    lpa.position = Eigen::Vector3f(-0.080f, 0.000f, 0.000f);
    out.push_back(lpa);

    DigitizedPoint rpa;
    rpa.kind = PointKind::Fiducial;
    rpa.label = "RPA";
    rpa.ident_number = static_cast<int>(FiducialId::RPA);
    rpa.position = Eigen::Vector3f(0.080f, 0.000f, 0.000f);
    out.push_back(rpa);

    return out;
}

std::vector<DigitizedPoint> demoEegCap(int count)
{
    // Eight cardinal 10-20 positions on a 95 mm sphere centred at the
    // head origin. Values are loosely realistic; exact geometry is not
    // important for documentation screenshots.
    struct Entry
    {
        const char* label;
        Eigen::Vector3f pos;
    };
    static const Entry kEntries[] = {
        {"Fz", Eigen::Vector3f(0.000f, 0.070f, 0.060f)},
        {"Cz", Eigen::Vector3f(0.000f, 0.000f, 0.095f)},
        {"Pz", Eigen::Vector3f(0.000f, -0.070f, 0.060f)},
        {"Oz", Eigen::Vector3f(0.000f, -0.095f, 0.005f)},
        {"T7", Eigen::Vector3f(-0.075f, 0.000f, 0.040f)},
        {"T8", Eigen::Vector3f(0.075f, 0.000f, 0.040f)},
        {"O1", Eigen::Vector3f(-0.035f, -0.085f, 0.025f)},
        {"O2", Eigen::Vector3f(0.035f, -0.085f, 0.025f)},
    };

    const int n = std::clamp(count, 0, static_cast<int>(std::size(kEntries)));
    std::vector<DigitizedPoint> out;
    out.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        DigitizedPoint p;
        p.kind = PointKind::Eeg;
        p.label = kEntries[i].label;
        p.ident_number = i + 1;
        p.position = kEntries[i].pos;
        out.push_back(p);
    }
    return out;
}

std::vector<DigitizedPoint> demoHeadShape(int count)
{
    std::vector<DigitizedPoint> out;
    if (count <= 0)
        return out;
    out.reserve(static_cast<std::size_t>(count));

    constexpr float kRadius = 0.095f; // 95 mm head sphere
    for (int i = 0; i < count; ++i) {
        // Spiral over the upper hemisphere using a Fibonacci-like layout.
        const float t = (i + 0.5f) / static_cast<float>(count);
        const float phi = std::acos(1.0f - t);             // 0 .. π/2 (upper hemisphere)
        const float th = static_cast<float>(i) * 2.39996f; // golden angle

        DigitizedPoint p;
        p.kind = PointKind::HeadShape;
        p.label = std::format("HSP-{}", i + 1);
        p.ident_number = i + 1;
        p.position = Eigen::Vector3f(
            kRadius * std::sin(phi) * std::cos(th),
            kRadius * std::sin(phi) * std::sin(th),
            kRadius * std::cos(phi));
        out.push_back(p);
    }
    return out;
}

std::vector<DigitizedPoint> demoFullDigitisation()
{
    std::vector<DigitizedPoint> out;
    for (const std::vector<DigitizedPoint>& part : {demoFiducials(), demoEegCap(8), demoHeadShape(40)})
        out.insert(out.end(), part.begin(), part.end());
    return out;
}

void applyTo(AcquiredPoints* store, const std::vector<DigitizedPoint>& points)
{
    if (!store)
        return;
    store->clear();
    for (const DigitizedPoint& p : points) {
        store->append(p);
    }
}

} // namespace MNEALIGN
