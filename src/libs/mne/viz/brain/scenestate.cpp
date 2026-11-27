//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scenestate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Ordered scene-layer iteration, draw orchestration and picking routing across all mne_viz_qt modalities.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scenestate.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

// Relative comparison with the tolerances of qFuzzyCompare (5 decimal digits for float, 12 for double)
bool fuzzyEqual(float a, float b)
{
    return std::abs(a - b) * 100000.0f <= std::min(std::abs(a), std::abs(b));
}

bool fuzzyEqual(double a, double b)
{
    return std::abs(a - b) * 1000000000000.0 <= std::min(std::abs(a), std::abs(b));
}

} // namespace

//=============================================================================================================

SceneState::SceneState()
{
}

//=============================================================================================================

SceneState::~SceneState() = default;

//=============================================================================================================

void SceneState::addLayer(SceneLayer layer)
{
    if (layer.id.empty()) {
        mne::Log::warning("SceneState::addLayer: refusing to add layer with empty id");
        return;
    }

    const auto it = m_indexById.find(layer.id);
    if (it != m_indexById.end()) {
        m_layers[it->second] = std::move(layer);
    } else {
        m_indexById.insert_or_assign(layer.id, m_layers.size());
        m_layers.push_back(std::move(layer));
    }
    rebuildOrder();
    if (m_layersChangedCallback) {
        m_layersChangedCallback();
    }
}

//=============================================================================================================

bool SceneState::removeLayer(const std::string& id)
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end()) {
        return false;
    }
    m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(it->second));
    rebuildOrder();
    if (m_layersChangedCallback) {
        m_layersChangedCallback();
    }
    return true;
}

//=============================================================================================================

void SceneState::clear()
{
    if (m_layers.empty()) {
        return;
    }
    m_layers.clear();
    m_indexById.clear();
    if (m_layersChangedCallback) {
        m_layersChangedCallback();
    }
}

//=============================================================================================================

std::vector<SceneLayer> SceneState::layers() const
{
    return m_layers;
}

//=============================================================================================================

const SceneLayer* SceneState::findLayer(const std::string& id) const
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end()) {
        return nullptr;
    }
    return &m_layers.at(it->second);
}

//=============================================================================================================

void SceneState::setLayerVisible(const std::string& id, bool visible)
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end()) {
        return;
    }
    if (m_layers[it->second].visible == visible) {
        return;
    }
    m_layers[it->second].visible = visible;
    if (m_layersChangedCallback) {
        m_layersChangedCallback();
    }
}

//=============================================================================================================

void SceneState::setLayerOpacity(const std::string& id, float opacity)
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end()) {
        return;
    }
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (fuzzyEqual(m_layers[it->second].opacity + 1.0f, opacity + 1.0f)) {
        return;
    }
    m_layers[it->second].opacity = opacity;
    if (m_layersChangedCallback) {
        m_layersChangedCallback();
    }
}

//=============================================================================================================

int SceneState::currentTimeSample() const
{
    return m_currentTimeSample;
}

//=============================================================================================================

void SceneState::setCurrentTimeSample(int sample)
{
    if (sample < 0) {
        sample = -1;
    }
    if (sample == m_currentTimeSample) {
        return;
    }
    m_currentTimeSample = sample;
    if (m_timeSampleChangedCallback) {
        m_timeSampleChangedCallback(sample);
    }
}

//=============================================================================================================

double SceneState::timeCursor() const
{
    return m_timeCursor;
}

//=============================================================================================================

void SceneState::setTimeCursor(double seconds)
{
    if (fuzzyEqual(seconds + 1.0, m_timeCursor + 1.0)) {
        return;
    }
    m_timeCursor = seconds;
    if (m_timeCursorChangedCallback) {
        m_timeCursorChangedCallback(seconds);
    }
}

//=============================================================================================================

float SceneState::overlayFmin() const
{
    return m_overlayFmin;
}

//=============================================================================================================

float SceneState::overlayFmid() const
{
    return m_overlayFmid;
}

//=============================================================================================================

float SceneState::overlayFmax() const
{
    return m_overlayFmax;
}

//=============================================================================================================

void SceneState::setOverlayThresholds(float fmin, float fmid, float fmax)
{
    // Clamp to monotone fmin <= fmid <= fmax.
    if (fmid < fmin) {
        fmid = fmin;
    }
    if (fmax < fmid) {
        fmax = fmid;
    }
    const bool same = fuzzyEqual(fmin + 1.0f, m_overlayFmin + 1.0f) && fuzzyEqual(fmid + 1.0f, m_overlayFmid + 1.0f) && fuzzyEqual(fmax + 1.0f, m_overlayFmax + 1.0f);
    if (same) {
        return;
    }
    m_overlayFmin = fmin;
    m_overlayFmid = fmid;
    m_overlayFmax = fmax;
    if (m_overlayThresholdsChangedCallback) {
        m_overlayThresholdsChangedCallback(fmin, fmid, fmax);
    }
}

//=============================================================================================================

const PickResult& SceneState::lastPick() const
{
    return m_lastPick;
}

//=============================================================================================================

void SceneState::reportPick(const PickResult& pick)
{
    m_lastPick = pick;
    if (m_pickedCallback) {
        m_pickedCallback(pick);
    }
}

//=============================================================================================================

void SceneState::worldBounds(Eigen::Vector3f& bbMin, Eigen::Vector3f& bbMax) const
{
    constexpr float fmax = std::numeric_limits<float>::max();
    constexpr float flow = std::numeric_limits<float>::lowest();
    bbMin = Eigen::Vector3f::Constant(fmax);
    bbMax = Eigen::Vector3f::Constant(flow);

    bool anyHit = false;
    for (const auto& layer : m_layers) {
        if (!layer.visible) {
            continue;
        }
        const auto it = m_boundsFns.find(static_cast<int>(layer.kind));
        if (it == m_boundsFns.end() || !it->second) {
            continue;
        }
        Eigen::Vector3f mn = Eigen::Vector3f::Zero();
        Eigen::Vector3f mx = Eigen::Vector3f::Zero();
        if (!it->second(layer, mn, mx)) {
            continue;
        }
        bbMin = bbMin.cwiseMin(mn);
        bbMax = bbMax.cwiseMax(mx);
        anyHit = true;
    }

    if (!anyHit) {
        bbMin = Eigen::Vector3f::Constant(-1.0f);
        bbMax = Eigen::Vector3f::Constant(1.0f);
    }
}

//=============================================================================================================

void SceneState::registerBoundsFn(SceneLayerKind kind, BoundsFn fn)
{
    m_boundsFns.insert_or_assign(static_cast<int>(kind), std::move(fn));
}

//=============================================================================================================

void SceneState::rebuildOrder()
{
    // Stable sort by (kind, drawOrder); insertion order is preserved as tie-breaker.
    std::stable_sort(m_layers.begin(), m_layers.end(),
                     [](const SceneLayer& a, const SceneLayer& b) {
                         if (a.kind != b.kind) {
                             return static_cast<int>(a.kind) < static_cast<int>(b.kind);
                         }
                         return a.drawOrder < b.drawOrder;
                     });
    m_indexById.clear();
    m_indexById.reserve(m_layers.size());
    for (std::size_t i = 0; i < m_layers.size(); ++i) {
        m_indexById.insert_or_assign(m_layers[i].id, i);
    }
}

//=============================================================================================================

void SceneState::setLayersChangedCallback(std::function<void()> callback)
{
    m_layersChangedCallback = std::move(callback);
}

//=============================================================================================================

void SceneState::setTimeSampleChangedCallback(std::function<void(int)> callback)
{
    m_timeSampleChangedCallback = std::move(callback);
}

//=============================================================================================================

void SceneState::setTimeCursorChangedCallback(std::function<void(double)> callback)
{
    m_timeCursorChangedCallback = std::move(callback);
}

//=============================================================================================================

void SceneState::setOverlayThresholdsChangedCallback(std::function<void(float, float, float)> callback)
{
    m_overlayThresholdsChangedCallback = std::move(callback);
}

//=============================================================================================================

void SceneState::setPickedCallback(std::function<void(const mne::viz::PickResult&)> callback)
{
    m_pickedCallback = std::move(callback);
}
