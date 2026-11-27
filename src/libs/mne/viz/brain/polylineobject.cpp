//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polylineobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    PolylineObject class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "polylineobject.h"

#include "instancing.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

PolylineObject::PolylineObject()
: m_iIndexCount(0)
, m_iInstanceCount(0)
, m_fRadius(0.0015f)
, m_startColor(Color{60, 120, 255, 255})
, m_endColor(Color{255, 80, 40, 255})
, m_bVisible(true)
{
    createSegmentGeometry();
}

//=============================================================================================================

PolylineObject::~PolylineObject() = default;

//=============================================================================================================

void PolylineObject::setPoints(const std::vector<Eigen::Vector3f>& vecPoints)
{
    // A single point has no segment, so it is treated the same as no data at
    // all rather than drawn as a degenerate line.
    if (vecPoints.size() < 2) {
        clear();
        return;
    }

    m_vecPoints = vecPoints;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::clear()
{
    m_vecPoints.clear();
    m_instanceData.clear();
    m_iInstanceCount = 0;
    ++m_instanceGeneration;
}

//=============================================================================================================

void PolylineObject::setRadius(float fRadius)
{
    m_fRadius = fRadius;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::setGradient(const Color& startColor, const Color& endColor)
{
    m_startColor = startColor;
    m_endColor = endColor;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::createSegmentGeometry()
{
    if (!m_vertexData.empty()) {
        return;
    }

    // Unit cylinder along Y, height 1 and radius 1, scaled and rotated per
    // instance. Eight segments is enough for a thin line and keeps the vertex
    // count low, since a long path can have thousands of instances.
    const int iSegments = 8;
    const float fHalfHeight = 0.5f;

    std::vector<VertexData> vertices;
    std::vector<std::uint32_t> indices;

    for (int i = 0; i < iSegments; ++i) {
        const float fAngle = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(iSegments);
        const float x = std::cos(fAngle);
        const float z = std::sin(fAngle);

        const Eigen::Vector3f normal = Eigen::Vector3f(x, 0.0f, z).normalized();

        vertices.push_back({x, fHalfHeight, z, normal.x(), normal.y(), normal.z()});
        vertices.push_back({x, -fHalfHeight, z, normal.x(), normal.y(), normal.z()});
    }

    for (int i = 0; i < iSegments; ++i) {
        const std::uint32_t uTop = static_cast<std::uint32_t>(i * 2);
        const std::uint32_t uBottom = uTop + 1;
        const std::uint32_t uNextTop = static_cast<std::uint32_t>(((i + 1) % iSegments) * 2);
        const std::uint32_t uNextBottom = uNextTop + 1;

        indices.push_back(uTop);
        indices.push_back(uBottom);
        indices.push_back(uNextTop);

        indices.push_back(uNextTop);
        indices.push_back(uBottom);
        indices.push_back(uNextBottom);
    }

    m_vertexData = toByteBuffer(vertices);
    m_indexData = toByteBuffer(indices);
    m_iIndexCount = static_cast<int>(indices.size());
    ++m_geometryGeneration;
}

//=============================================================================================================

void PolylineObject::buildInstances()
{
    if (m_vecPoints.size() < 2) {
        m_instanceData.clear();
        m_iInstanceCount = 0;
        return;
    }

    const int iSegmentCount = static_cast<int>(m_vecPoints.size()) - 1;
    std::vector<InstanceData> instances;
    instances.reserve(static_cast<size_t>(iSegmentCount));

    const float fLastStep = static_cast<float>(iSegmentCount - 1);

    for (int i = 0; i < iSegmentCount; ++i) {
        const Eigen::Vector3f& vecStart = m_vecPoints.at(i);
        const Eigen::Vector3f& vecEnd = m_vecPoints.at(i + 1);

        const float fLength = (vecEnd - vecStart).norm();

        // Two consecutive fits can land on the same position when the subject
        // does not move. Such a segment has no direction to orient a cylinder
        // by, so it is skipped instead of producing a NaN rotation.
        //
        // The threshold has to be far below anything physical rather than
        // qFuzzyIsNull, whose tolerance for float is 1e-5. Positions are in
        // metres, so that would discard every step under ten micrometres and
        // punch gaps into the path of a slowly drifting subject. A picometre
        // is small enough that only genuinely coincident points are dropped,
        // and large enough to keep the division below well conditioned.
        constexpr float fMinSegmentLength = 1.0e-12f;
        if (fLength < fMinSegmentLength) {
            continue;
        }

        InstanceData instance{};
        storeMatrix(segmentMatrix(vecStart, vecEnd, m_fRadius), instance.model);

        // Grade from the start colour to the end colour so the direction the
        // path was travelled in is visible without extra decoration.
        const float fRatio = (fLastStep > 0.0f) ? (static_cast<float>(i) / fLastStep) : 0.0f;
        std::array<float, 4> start{};
        std::array<float, 4> end{};
        storeColor(m_startColor, start.data());
        storeColor(m_endColor, end.data());
        for (int k = 0; k < 3; ++k) {
            instance.color[k] = start[k] + (end[k] - start[k]) * fRatio;
        }
        instance.color[3] = 1.0f;
        instance.isSelected = 0.0f;

        instances.push_back(instance);
    }

    m_instanceData = toByteBuffer(instances);
    m_iInstanceCount = static_cast<int>(instances.size());
    ++m_instanceGeneration;
}

//=============================================================================================================

InstancedStreams PolylineObject::streams() const
{
    return {{m_vertexData.data(), std::uint32_t(m_vertexData.size()), m_geometryGeneration},
            {m_indexData.data(), std::uint32_t(m_indexData.size()), m_geometryGeneration},
            {m_instanceData.data(), std::uint32_t(m_iInstanceCount * sizeof(InstanceData)), m_instanceGeneration},
            m_iIndexCount,
            m_iInstanceCount};
}

} // namespace mne::viz
