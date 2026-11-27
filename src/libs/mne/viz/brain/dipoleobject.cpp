//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipoleobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Instanced-arrow geometry and per-dipole instance buffer construction with goodness-of-fit colour mapping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipoleobject.h"

#include "instancing.h"
#include <mne/viz/views/ray_picking.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numbers>
#include <optional>
#include <random>
#include <vector>

namespace mne::viz
{

//=============================================================================================================
// PIMPL
//=============================================================================================================

DipoleObject::DipoleObject() = default;

DipoleObject::~DipoleObject() = default;

InstancedStreams DipoleObject::streams() const
{
    return {{m_vertexData.data(), static_cast<std::uint32_t>(m_vertexData.size()), m_geometryGeneration},
            {m_indexData.data(), static_cast<std::uint32_t>(m_indexData.size()), m_geometryGeneration},
            {m_instanceData.data(), static_cast<std::uint32_t>(m_instanceCount * sizeof(InstanceData)), m_instanceGeneration},
            m_indexCount,
            m_instanceCount};
}

void DipoleObject::load(const mne::InvEcdSet& ecdSet)
{
    createGeometry();

    m_instanceCount = ecdSet.size();
    m_instanceData.resize(m_instanceCount * sizeof(InstanceData));
    m_loadedModels.resize(m_instanceCount);
    InstanceData* data = reinterpret_cast<InstanceData*>(m_instanceData.data());

    const Eigen::Vector3f from = Eigen::Vector3f::UnitY(); // Cone points up Y axis

    float maxMag = 0.0f;
    for (int i = 0; i < ecdSet.size(); ++i) {
        const float mag = ecdSet[i].Q.norm();
        maxMag = std::max(maxMag, mag);
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

    for (int i = 0; i < ecdSet.size(); ++i) {
        const auto& dip = ecdSet[i];

        const Eigen::Vector3f pos(dip.rd(0), dip.rd(1), dip.rd(2));
        const Eigen::Vector3f Q(dip.Q(0), dip.Q(1), dip.Q(2));
        const float mag = Q.norm();

        const Eigen::Vector3f to = Q.normalized();
        // Handle case where 'to' is parallel to 'from'
        Eigen::Quaternionf rot = Eigen::Quaternionf::Identity();
        if (from.dot(to) < -0.99f) {
            rot = Eigen::Quaternionf(Eigen::AngleAxisf(std::numbers::pi_v<float>, Eigen::Vector3f::UnitX()));
        } else if (from.dot(to) <= 0.99f) {
            rot = rotationTo(from, to);
        }

        // Scale based on magnitude relative to max, with a minimum size of 20%
        float scaleFactor = (maxMag > 0.0f) ? (0.2f + 0.8f * (mag / maxMag)) : 1.0f;

        const Eigen::Matrix4f m = modelMatrix(pos, rot, Eigen::Vector3f::Constant(scaleFactor));
        m_loadedModels[i] = m;
        storeMatrix(m, data[i].model);

        // Random Color
        data[i].color[0] = colorDist(rng);
        data[i].color[1] = colorDist(rng);
        data[i].color[2] = colorDist(rng);
        data[i].color[3] = 1.0f;

        // Selection state
        data[i].isSelected = 0.0f;
    }

    ++m_instanceGeneration;
}

void DipoleObject::applyTransform(const Eigen::Matrix4f& trans)
{
    if (m_instanceCount == 0)
        return;

    InstanceData* data = reinterpret_cast<InstanceData*>(m_instanceData.data());

    for (int i = 0; i < m_instanceCount; ++i) {
        storeMatrix(trans * m_loadedModels[i], data[i].model);
    }

    ++m_instanceGeneration;
}

bool DipoleObject::boundingBox(Eigen::Vector3f& min, Eigen::Vector3f& max) const
{
    if (m_instanceCount == 0)
        return false;
    const InstanceData* data = reinterpret_cast<const InstanceData*>(m_instanceData.data());
    for (int i = 0; i < m_instanceCount; ++i) {
        // Column-major model matrix: the translation is elements 12..14
        const Eigen::Vector3f pos(data[i].model[12], data[i].model[13], data[i].model[14]);
        min = i == 0 ? pos : Eigen::Vector3f(min.cwiseMin(pos));
        max = i == 0 ? pos : Eigen::Vector3f(max.cwiseMax(pos));
    }
    return true;
}

void DipoleObject::createGeometry()
{
    if (!m_vertexData.empty())
        return;

    // Create a simple cone
    // Radius 0.001, Height 0.003
    // 32 segments

    float radius = 0.005f; // Slightly larger for visibility
    float height = 0.01f;
    int segments = 16;

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // Tip
    VertexData tip = {0, height, 0, 0, 1, 0};
    vertices.push_back(tip);

    // Base center
    VertexData baseCenter = {0, 0, 0, 0, -1, 0};
    vertices.push_back(baseCenter); // Index 1

    // Rim vertices
    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        // Side normal
        // Slope vector: (cos, -h/r, sin) -> normalized
        const Eigen::Vector3f n = Eigen::Vector3f(x, radius / height, z).normalized();

        VertexData vSide = {x, 0, z, n.x(), n.y(), n.z()};
        vertices.push_back(vSide);

        VertexData vBase = {x, 0, z, 0, -1, 0};
        vertices.push_back(vBase);
    }

    // Indices
    for (int i = 0; i < segments; ++i) {
        // Cone sides
        // Tip (0), current(i), next(next)
        // Side vertices start at firstRimIdx.
        // Layout: Tip, BaseCenter, Side0, Base0, Side1, Base1...
        // Wait, easier to keep separate lists or just flat

        // Re-do layout for ease:
        // 0: Tip
        // 1: Base Center
        // 2..2+segments-1: Side rim vertices
        // 2+segments..2+2*segments-1: Base rim vertices
    }

    vertices.clear();
    vertices.push_back(tip);        // 0
    vertices.push_back(baseCenter); // 1

    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        // Normal perpendicular to the slope down the side
        const Eigen::Vector3f slope(x, -height, z);
        const Eigen::Vector3f tangent(-std::sin(angle), 0.0f, std::cos(angle));
        const Eigen::Vector3f sideNormal = tangent.cross(slope).normalized();
        // Actually simple: normal y component is radius/height ratio related.

        vertices.push_back({x, 0, z, sideNormal.x(), sideNormal.y(), sideNormal.z()}); // Side rim
    }

    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back({x, 0, z, 0, -1, 0}); // Base rim
    }

    int sideStart = 2;
    int baseStart = 2 + segments;

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;

        // Cone face
        indices.push_back(0);
        indices.push_back(sideStart + next);
        indices.push_back(sideStart + i);

        // Base face
        indices.push_back(1);
        indices.push_back(baseStart + i);
        indices.push_back(baseStart + next);
    }

    m_indexCount = static_cast<int>(indices.size());

    m_vertexData.resize(vertices.size() * sizeof(VertexData));
    memcpy(m_vertexData.data(), vertices.data(), m_vertexData.size());

    m_indexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_indexData.data(), indices.data(), m_indexData.size());

    ++m_geometryGeneration;
}

int DipoleObject::intersect(const Eigen::Vector3f& rayOrigin, const Eigen::Vector3f& rayDir, float& dist) const
{
    if (m_instanceCount == 0)
        return -1;

    int closestIdx = -1;
    float closestDist = std::numeric_limits<float>::max();

    const InstanceData* data = reinterpret_cast<const InstanceData*>(m_instanceData.data());

    // Geometry radius ~ 0.005 (base) to 0.01 (height).
    // Let's use a slightly larger hit radius to make selection easier and more accurate.
    const float baseRadius = 0.02f;
    const Ray ray{rayOrigin, rayDir};

    for (int i = 0; i < m_instanceCount; ++i) {
        // Translation (last column) and scale (length of the first column, roughly uniform)
        const Eigen::Vector3f center(data[i].model[12], data[i].model[13], data[i].model[14]);
        const float scale = Eigen::Vector3f(data[i].model[0], data[i].model[1], data[i].model[2]).norm();
        const float radius = baseRadius * scale;

        // Spheres behind the ray origin do not count; from inside a sphere the exit point is hit
        const std::optional<SphereCrossing> crossing = intersect_ray_sphere(ray, center, radius);
        if (!crossing || crossing->entry + crossing->exit < 0.0f)
            continue;
        const float t = crossing->entry >= 0.0f ? crossing->entry : crossing->exit;
        if (t < 0.0f)
            continue;

        if (t < closestDist) {
            closestDist = t;
            closestIdx = i;
        }
    }

    if (closestIdx != -1) {
        dist = closestDist;
        return closestIdx;
    }

    return -1;
}

void DipoleObject::setSelected(int index, bool selected)
{
    if (index < 0 || index >= m_instanceCount)
        return;

    InstanceData* data = reinterpret_cast<InstanceData*>(m_instanceData.data());

    data[index].isSelected = selected ? 1.0f : 0.0f;

    ++m_instanceGeneration;
}

} // namespace mne::viz
