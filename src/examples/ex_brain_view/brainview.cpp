//=============================================================================================================
/**
 * @file     brainview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    BrainView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainview.h"
#include <QMatrix4x4>
#include <cmath>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QLabel>
#include <QMouseEvent>
#include <QKeyEvent>
#include <mne/mne_surface.h>
#include <fs/surfaceset.h>
#include <fs/surface.h>

using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

BrainView::BrainView(QWidget *parent)
    : QRhiWidget(parent)
{
    setMinimumSize(800, 600);
    setSampleCount(1);
    setApi(Api::Metal);
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&BrainView::update));
    m_updateTimer->start(16); // ~60 FPS update

    m_fpsLabel = new QLabel(this);
    m_fpsLabel->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 16px; background: rgba(0,0,0,100); padding: 5px;");
    m_fpsLabel->move(10, 10);
    m_fpsLabel->resize(300, 100);

    m_fpsTimer.start();
}

BrainView::~BrainView()
{
}

//=============================================================================================================

bool BrainView::loadSurface(const QString &subjectPath, const QString &subject, const QString &hemi, const QString &type)
{
    QString hemiStr = hemi;
    Surface surf(subjectPath + "/" + subject + "/surf/" + hemiStr + "." + type);
    if (surf.isEmpty()) return false;

    auto brainSurf = std::make_shared<BrainSurface>();
    brainSurf->fromSurface(surf);
    brainSurf->fromSurface(surf);
    // Set the hemisphere ID for the BrainSurface
    int hemiId = (hemi == "lh") ? 0 : 1;
    brainSurf->setHemi(hemiId);
    
    qDebug() << "Loaded Surface:" << type << "Hemi:" << hemi << "ID:" << hemiId;
    
    // Store with unique key: hemi + "_" + type (e.g., "lh_pial")
    m_surfaces[hemi + "_" + type] = brainSurf;
    
    // If this is the first surface, set it as active type
    if (m_activeSurfaceType.isEmpty()) {
        m_activeSurfaceType = type;
        m_activeSurface = brainSurf; // Keep a reference for stats
    }
    
    // Fix inflated surface coordinates (they share center 0,0,0 causing overlap)
    if (type == "inflated") {
        if (hemi == "lh") {
             float offset = brainSurf->maxX();
             brainSurf->translateX(-offset);
             qDebug() << "Shifted LH Inflated by " << -offset;
        } else if (hemi == "rh") {
             float offset = brainSurf->minX();
             brainSurf->translateX(-offset);
             qDebug() << "Shifted RH Inflated by " << -offset;
        }
    }

    return true;
}

//=============================================================================================================

bool BrainView::loadAtlas(const QString &subjectPath, const QString &subject, const QString &hemi)
{
    QString hemiStr = hemi;
    QString annotPath = subjectPath + "/" + subject + "/label/" + hemiStr + ".aparc.annot";
    qDebug() << "Loading Atlas from" << annotPath;
    
    int targetHemi = (hemi == "lh") ? 0 : 1;
    
    bool allSuccess = true;
    for (auto surf : m_surfaces.values()) {
        if (surf->hemi() != targetHemi) continue;

        if (!surf->loadAnnotation(annotPath)) {
            allSuccess = false;
        }
    }
    update();
    return allSuccess;
}

void BrainView::setActiveSurface(const QString &type)
{
    // Just update the active type. Render loop will pick matching surfaces.
    m_activeSurfaceType = type;
    
    // Update m_activeSurface pointer to one of the matching surfaces for stats/helpers
    QString key = "lh_" + type;
    if (m_surfaces.contains(key)) m_activeSurface = m_surfaces[key];
    else {
        key = "rh_" + type;
        if (m_surfaces.contains(key)) m_activeSurface = m_surfaces[key];
    }
    update();
}

//=============================================================================================================

void BrainView::setShaderMode(const QString &modeName)
{
    if (modeName == "Standard") m_shaderMode = BrainRenderer::Standard;
    else if (modeName == "Holographic") m_shaderMode = BrainRenderer::Holographic;
    else if (modeName == "Glossy Realistic") m_shaderMode = BrainRenderer::Atlas;
    update();
}

//=============================================================================================================

void BrainView::setVisualizationMode(const QString &modeName)
{
    BrainSurface::VisualizationMode mode = BrainSurface::ModeSurface;
    if (modeName == "Annotation") mode = BrainSurface::ModeAnnotation;
    if (modeName == "Scientific") mode = BrainSurface::ModeScientific;
    
    for (auto surf : m_surfaces) {
        surf->setVisualizationMode(mode);
    }
    update();
}

//=============================================================================================================

void BrainView::setHemiVisible(int hemiIdx, bool visible)
{
    qDebug() << "setHemiVisible called for hemi" << hemiIdx << "Visible:" << visible;
    int count = 0;
    for (auto surf : m_surfaces) {
        if (surf->hemi() == hemiIdx) {
            surf->setVisible(visible);
            qDebug() << "  Surf:" << surf.get() << "Hemi:" << surf->hemi() << "Set Visible:" << visible;
            count++;
        }
    }
    qDebug() << "Updated visibility for" << count << "surfaces.";
    update();
}

//=============================================================================================================

void BrainView::setLightingEnabled(bool enabled)
{
    m_lightingEnabled = enabled;
    update();
}

//=============================================================================================================

void BrainView::saveSnapshot()
{
    QImage img = grabFramebuffer();
    QString fileName = QString("snapshot_refactor_%1.png").arg(m_snapshotCounter++, 4, 10, QChar('0'));
    img.save(fileName);
    qDebug() << "Saved snapshot to" << fileName;
}

//=============================================================================================================

void BrainView::initialize(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    qDebug() << "RHI Initialized. Backend:" << rhi()->backendName();
    m_renderer = std::make_unique<BrainRenderer>();
}

//=============================================================================================================

void BrainView::render(QRhiCommandBuffer *cb)
{
    if (!m_activeSurface) return;

    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 500) {
        float fps = m_frameCount / (m_fpsTimer.elapsed() / 1000.0f);
        QString modeStr = (m_shaderMode == BrainRenderer::Holographic) ? "Holographic" : (m_shaderMode == BrainRenderer::Atlas) ? "Atlas" : "Standard";
        m_fpsLabel->setText(QString("FPS: %1\nVertices: %2\nShader: %3").arg(fps, 0, 'f', 1).arg(m_activeSurface->vertexCount()).arg(modeStr));
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    QSize outputSize = renderTarget()->pixelSize();
    QMatrix4x4 projection;
    projection.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), 0.01f, 100.0f);

    float azimuth = m_rotation * M_PI / 180.0f;
    float elevation = m_pitch * M_PI / 180.0f;
    float distance = 0.5f - m_zoom * 0.03f;

    QVector3D cameraPos(distance * cos(elevation) * sin(azimuth), distance * sin(elevation), distance * cos(elevation) * cos(azimuth));
    QMatrix4x4 view;
    view.lookAt(cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    BrainRenderer::SceneData sceneData;
    sceneData.mvp = rhi()->clipSpaceCorrMatrix();
    sceneData.mvp *= projection;
    sceneData.mvp *= view;
    sceneData.cameraPos = cameraPos;
    sceneData.lightDir = cameraPos.normalized(); // Headlight
    sceneData.lightingEnabled = m_lightingEnabled;

    m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount(), m_shaderMode);
    
    m_renderer->beginFrame(cb, renderTarget());
    
    // Render all visible surfaces matching the active type
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        // Check if the key ends with the active type (e.g. "lh_pial" ends with "pial")
        if (it.key().endsWith(m_activeSurfaceType)) {
            m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get());
        }
    }
    
    m_renderer->endFrame(cb);
}

//=============================================================================================================

void BrainView::mousePressEvent(QMouseEvent *e)
{
    m_lastMousePos = e->pos();
}

//=============================================================================================================

void BrainView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint diff = event->pos() - m_lastMousePos;
        m_rotation -= diff.x() * 0.5f;
        m_pitch += diff.y() * 0.5f;

        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        m_lastMousePos = event->pos();
        update();
    }
}

//=============================================================================================================

void BrainView::wheelEvent(QWheelEvent *event)
{
    m_zoom += event->angleDelta().y() / 120.0f;
    update();
}

//=============================================================================================================

void BrainView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_S) {
        saveSnapshot();
    }
}

//=============================================================================================================

bool BrainView::loadSourceEstimate(const QString &lhPath, const QString &rhPath)
{
    m_sourceOverlay = std::make_unique<SourceEstimateOverlay>();
    
    bool success = true;
    
    if (!lhPath.isEmpty()) {
        if (!m_sourceOverlay->loadStc(lhPath, 0)) {
            qWarning() << "BrainView: Failed to load LH source estimate:" << lhPath;
            success = false;
        }
    }
    
    if (!rhPath.isEmpty()) {
        if (!m_sourceOverlay->loadStc(rhPath, 1)) {
            qWarning() << "BrainView: Failed to load RH source estimate:" << rhPath;
            success = false;
        }
    }
    
    if (m_sourceOverlay->isLoaded()) {
        // Compute interpolation matrices for all surfaces
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (it.key().endsWith(m_activeSurfaceType)) {
                int hemi = it.value()->hemi();
                qDebug() << "BrainView: Computing interpolation for" << it.key() << "hemi" << hemi;
                m_sourceOverlay->computeInterpolationMatrix(it.value().get(), hemi, 0.15);
            }
        }
        
        emit sourceEstimateLoaded(m_sourceOverlay->numTimePoints());
        setTimePoint(0);
        return true;
    }
    
    m_sourceOverlay.reset();
    return false;
}

//=============================================================================================================

void BrainView::setTimePoint(int index)
{
    if (!m_sourceOverlay || !m_sourceOverlay->isLoaded()) return;
    
    m_currentTimePoint = qBound(0, index, m_sourceOverlay->numTimePoints() - 1);
    
    // Apply source estimate to all surfaces matching active type
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().endsWith(m_activeSurfaceType)) {
            m_sourceOverlay->applyToSurface(it.value().get(), m_currentTimePoint);
        }
    }
    
    emit timePointChanged(m_currentTimePoint, m_sourceOverlay->timeAtIndex(m_currentTimePoint));
    update();
}

//=============================================================================================================

void BrainView::setSourceColormap(const QString &name)
{
    if (m_sourceOverlay) {
        m_sourceOverlay->setColormap(name);
        // Re-apply current time point with new colormap
        setTimePoint(m_currentTimePoint);
    }
}

//=============================================================================================================

void BrainView::setSourceThresholds(float min, float mid, float max)
{
    if (m_sourceOverlay) {
        m_sourceOverlay->setThresholds(min, mid, max);
        // Re-apply current time point with new thresholds
        setTimePoint(m_currentTimePoint);
    }
}

