//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multimodalscene.cpp
 * @since 2026
 * @date  May 2026
 * @brief Ordered scene-layer iteration, draw orchestration and picking routing across all disp3D modalities.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multimodalscene.h"

#include <QtGlobal>

#include <algorithm>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================

MultimodalScene::MultimodalScene(QObject* parent)
    : QObject(parent)
{
}

//=============================================================================================================

MultimodalScene::~MultimodalScene() = default;

//=============================================================================================================

void MultimodalScene::addLayer(SceneLayer layer)
{
    if (layer.id.isEmpty()) {
        qWarning("MultimodalScene::addLayer: refusing to add layer with empty id");
        return;
    }

    const auto it = m_indexById.constFind(layer.id);
    if (it != m_indexById.constEnd()) {
        m_layers[*it] = std::move(layer);
    } else {
        m_indexById.insert(layer.id, m_layers.size());
        m_layers.append(std::move(layer));
    }
    rebuildOrder();
    emit layersChanged();
}

//=============================================================================================================

bool MultimodalScene::removeLayer(const QString& id)
{
    const auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) {
        return false;
    }
    m_layers.removeAt(*it);
    rebuildOrder();
    emit layersChanged();
    return true;
}

//=============================================================================================================

void MultimodalScene::clear()
{
    if (m_layers.isEmpty()) {
        return;
    }
    m_layers.clear();
    m_indexById.clear();
    emit layersChanged();
}

//=============================================================================================================

QVector<SceneLayer> MultimodalScene::layers() const
{
    return m_layers;
}

//=============================================================================================================

const SceneLayer* MultimodalScene::findLayer(const QString& id) const
{
    const auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) {
        return nullptr;
    }
    return &m_layers.at(*it);
}

//=============================================================================================================

void MultimodalScene::setLayerVisible(const QString& id, bool visible)
{
    const auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) {
        return;
    }
    if (m_layers[*it].visible == visible) {
        return;
    }
    m_layers[*it].visible = visible;
    emit layersChanged();
}

//=============================================================================================================

void MultimodalScene::setLayerOpacity(const QString& id, float opacity)
{
    const auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) {
        return;
    }
    opacity = qBound(0.0f, opacity, 1.0f);
    if (qFuzzyCompare(m_layers[*it].opacity + 1.0f, opacity + 1.0f)) {
        return;
    }
    m_layers[*it].opacity = opacity;
    emit layersChanged();
}

//=============================================================================================================

int MultimodalScene::currentTimeSample() const
{
    return m_currentTimeSample;
}

//=============================================================================================================

void MultimodalScene::setCurrentTimeSample(int sample)
{
    if (sample < 0) {
        sample = -1;
    }
    if (sample == m_currentTimeSample) {
        return;
    }
    m_currentTimeSample = sample;
    emit timeSampleChanged(sample);
}

//=============================================================================================================

double MultimodalScene::timeCursor() const
{
    return m_timeCursor;
}

//=============================================================================================================

void MultimodalScene::setTimeCursor(double seconds)
{
    if (qFuzzyCompare(seconds + 1.0, m_timeCursor + 1.0)) {
        return;
    }
    m_timeCursor = seconds;
    emit timeCursorChanged(seconds);
}

//=============================================================================================================

float MultimodalScene::overlayFmin() const
{
    return m_overlayFmin;
}

//=============================================================================================================

float MultimodalScene::overlayFmid() const
{
    return m_overlayFmid;
}

//=============================================================================================================

float MultimodalScene::overlayFmax() const
{
    return m_overlayFmax;
}

//=============================================================================================================

void MultimodalScene::setOverlayThresholds(float fmin, float fmid, float fmax)
{
    // Clamp to monotone fmin <= fmid <= fmax.
    if (fmid < fmin) {
        fmid = fmin;
    }
    if (fmax < fmid) {
        fmax = fmid;
    }
    const bool same = qFuzzyCompare(fmin + 1.0f, m_overlayFmin + 1.0f)
                   && qFuzzyCompare(fmid + 1.0f, m_overlayFmid + 1.0f)
                   && qFuzzyCompare(fmax + 1.0f, m_overlayFmax + 1.0f);
    if (same) {
        return;
    }
    m_overlayFmin = fmin;
    m_overlayFmid = fmid;
    m_overlayFmax = fmax;
    emit overlayThresholdsChanged(fmin, fmid, fmax);
}

//=============================================================================================================

const PickResult& MultimodalScene::lastPick() const
{
    return m_lastPick;
}

//=============================================================================================================

void MultimodalScene::reportPick(const PickResult& pick)
{
    m_lastPick = pick;
    emit picked(pick);
}

//=============================================================================================================

void MultimodalScene::worldBounds(QVector3D& bbMin, QVector3D& bbMax) const
{
    constexpr float fmax = std::numeric_limits<float>::max();
    constexpr float flow = std::numeric_limits<float>::lowest();
    bbMin = QVector3D(fmax, fmax, fmax);
    bbMax = QVector3D(flow, flow, flow);

    bool anyHit = false;
    for (const auto& layer : m_layers) {
        if (!layer.visible) {
            continue;
        }
        const auto it = m_boundsFns.constFind(static_cast<int>(layer.kind));
        if (it == m_boundsFns.constEnd() || !*it) {
            continue;
        }
        QVector3D mn;
        QVector3D mx;
        if (!(*it)(layer, mn, mx)) {
            continue;
        }
        bbMin.setX(std::min(bbMin.x(), mn.x()));
        bbMin.setY(std::min(bbMin.y(), mn.y()));
        bbMin.setZ(std::min(bbMin.z(), mn.z()));
        bbMax.setX(std::max(bbMax.x(), mx.x()));
        bbMax.setY(std::max(bbMax.y(), mx.y()));
        bbMax.setZ(std::max(bbMax.z(), mx.z()));
        anyHit = true;
    }

    if (!anyHit) {
        bbMin = QVector3D(-1.0f, -1.0f, -1.0f);
        bbMax = QVector3D(1.0f, 1.0f, 1.0f);
    }
}

//=============================================================================================================

void MultimodalScene::registerBoundsFn(SceneLayerKind kind, BoundsFn fn)
{
    m_boundsFns.insert(static_cast<int>(kind), std::move(fn));
}

//=============================================================================================================

void MultimodalScene::rebuildOrder()
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
    for (int i = 0; i < m_layers.size(); ++i) {
        m_indexById.insert(m_layers.at(i).id, i);
    }
}
