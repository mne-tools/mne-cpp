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
#include <QWheelEvent>
#include <QFile>
#include <mne/mne_surface.h>
#include <fs/surfaceset.h>
#include <fs/surface.h>
#include <fiff/fiff.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>

using namespace FSLIB;
using namespace FIFFLIB;
using namespace MNELIB;

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



//=============================================================================================================

bool BrainView::loadBem(const QString &fifPath)
{
    QFile file(fifPath);
    if (!file.exists()) {
        qWarning() << "BrainView: BEM file not found:" << fifPath;
        return false;
    }

    MNELIB::MNEBem bem(file);
    if (bem.isEmpty()) {
        qWarning() << "BrainView: Failed to load BEM from" << fifPath;
        return false;
    }
    
    qDebug() << "BrainView: Loaded BEM. Surfaces:" << bem.size();

    // Compute Scene Center (Centroid) including existing surfaces if any??
    // For now, let's let BEM define the center if loaded, as it's the reference frame usually.
    QVector3D sum(0,0,0);
    int totalVerts = 0;

    for(int i=0; i<bem.size(); ++i) {
        const auto& surf = bem[i];
        
        auto brainSurf = std::make_shared<BrainSurface>();
        
        // Color based on ID (same as ex_head_surface)
        // 4=Head (Reddish), 3=OuterSkull (Greenish), 1=InnerSkull (Blueish)
        QColor color(200, 200, 200);
        QString key;
        
        if (surf.id == 4) { 
            color = QColor(128, 77, 77); // Reddish
            key = "bem_head";
        }
        else if (surf.id == 3) {
            color = QColor(77, 128, 77); // Greenish
            key = "bem_outer_skull";
        }
        else if (surf.id == 1) {
            color = QColor(77, 77, 128); // Blueish
            key = "bem_inner_skull";
        } else {
            key = QString("bem_%1").arg(surf.id);
        }
        
        brainSurf->fromBemSurface(surf, color);
        
        // Accumulate for centroid
        const auto& rr = surf.rr;
        for(int r=0; r<rr.rows(); ++r) {
            sum += QVector3D(rr(r,0), rr(r,1), rr(r,2));
        }
        totalVerts += rr.rows();
        
        m_surfaces[key] = brainSurf;
        qDebug() << "  Added BEM surface:" << key << "verts:" << rr.rows();
    }
    
    if (totalVerts > 0) {
        m_sceneCenter = sum / totalVerts;
        qDebug() << "BrainView: Scene Center computed at:" << m_sceneCenter;
    }
    
    return true;
}

bool BrainView::loadSensors(const QString &fifPath)
{
    QFile file(fifPath);
    if (!file.exists()) {
        qWarning() << "BrainView: File does not exist:" << fifPath;
        return false;
    }

    // Read Fiff Info using FiffStream
    FiffInfo info;
    FiffDigPointSet digSet;
    FiffStream::SPtr stream;
    FiffDirNode::SPtr tree;
    
    // 1. Try to read as a raw/evoked file to get Info
    stream = FiffStream::SPtr(new FiffStream(&file));
    if (!stream->open()) {
        qWarning() << "BrainView: Failed to open stream:" << fifPath;
        return false;
    }

    tree = stream->dirtree();
    // Read info
    FiffDirNode::SPtr nodeInfo;
    if(!stream->read_meas_info(tree, info, nodeInfo)) {
        qWarning() << "BrainView: Failed to read measurement info";
        return false;
    }
    
    // Use the loaded info
    bool hasInfo = !info.isEmpty();

    if (hasInfo) {
        // Extract MEG/EEG
        qDebug() << "BrainView: Loaded Info. Channels:" << info.chs.size();
        
        m_megSensors.clear();
        m_eegSensors.clear();
        
        // 1. Cube Generator (Flat Shading, 24 vertices)
        auto createCube = [](const QVector3D &pos, const QColor &color, float size) -> std::shared_ptr<BrainSurface> {
            auto surf = std::make_shared<BrainSurface>();
            float s = size / 2.0f;
            
            // 6 faces * 4 verts = 24 vertices
            Eigen::MatrixX3f rr(24, 3);
            Eigen::MatrixX3f nn(24, 3);
            
            // Define faces: Front, Back, Top, Bottom, Right, Left
            // Front (z=s)
            rr.row(0) << -s, -s, s; nn.row(0) << 0, 0, 1;
            rr.row(1) <<  s, -s, s; nn.row(1) << 0, 0, 1;
            rr.row(2) <<  s,  s, s; nn.row(2) << 0, 0, 1;
            rr.row(3) << -s,  s, s; nn.row(3) << 0, 0, 1;
            // Back (z=-s)
            rr.row(4) <<  s, -s, -s; nn.row(4) << 0, 0, -1;
            rr.row(5) << -s, -s, -s; nn.row(5) << 0, 0, -1;
            rr.row(6) << -s,  s, -s; nn.row(6) << 0, 0, -1;
            rr.row(7) <<  s,  s, -s; nn.row(7) << 0, 0, -1;
            // Top (y=s)
            rr.row(8) << -s, s,  s; nn.row(8) << 0, 1, 0;
            rr.row(9) <<  s, s,  s; nn.row(9) << 0, 1, 0;
            rr.row(10)<<  s, s, -s; nn.row(10)<< 0, 1, 0;
            rr.row(11)<< -s, s, -s; nn.row(11)<< 0, 1, 0;
            // Bottom (y=-s)
            rr.row(12)<< -s, -s, -s; nn.row(12)<< 0, -1, 0;
            rr.row(13)<<  s, -s, -s; nn.row(13)<< 0, -1, 0;
            rr.row(14)<<  s, -s,  s; nn.row(14)<< 0, -1, 0;
            rr.row(15)<< -s, -s,  s; nn.row(15)<< 0, -1, 0;
            // Right (x=s)
            rr.row(16)<< s, -s,  s; nn.row(16)<< 1, 0, 0;
            rr.row(17)<< s, -s, -s; nn.row(17)<< 1, 0, 0;
            rr.row(18)<< s,  s, -s; nn.row(18)<< 1, 0, 0;
            rr.row(19)<< s,  s,  s; nn.row(19)<< 1, 0, 0;
            // Left (x=-s)
            rr.row(20)<< -s, -s, -s; nn.row(20)<< -1, 0, 0;
            rr.row(21)<< -s, -s,  s; nn.row(21)<< -1, 0, 0;
            rr.row(22)<< -s,  s,  s; nn.row(22)<< -1, 0, 0;
            rr.row(23)<< -s,  s, -s; nn.row(23)<< -1, 0, 0;

            // Shift to position
            for(int i=0; i<24; ++i) {
                rr(i,0) += pos.x();
                rr(i,1) += pos.y();
                rr(i,2) += pos.z();
            }

            // Indices
            Eigen::MatrixX3i tris(12, 3);
            // 0,1,2, 0,2,3 for each face
            for(int f=0; f<6; ++f) {
                int base = f*4;
                tris.row(f*2)   << base, base+1, base+2;
                tris.row(f*2+1) << base, base+2, base+3;
            }
            
            surf->createFromData(rr, nn, tris, color);
            return surf;
        };
        
        // 2. Icosahedron Generator (Smooth Shading, 12 vertices)
        auto createIcosahedron = [](const QVector3D &pos, const QColor &color, float radius) -> std::shared_ptr<BrainSurface> {
            auto surf = std::make_shared<BrainSurface>();
            float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
            
            Eigen::MatrixX3f rr(12, 3);
            rr << -1, t, 0,  1, t, 0,  -1, -t, 0,  1, -t, 0,
                  0, -1, t,  0, 1, t,  0, -1, -t,  0, 1, -t,
                  t, 0, -1,  t, 0, 1,  -t, 0, -1,  -t, 0, 1;
            
            Eigen::MatrixX3f nn(12, 3);
            
            for(int i=0; i<12; ++i) {
                QVector3D v(rr(i,0), rr(i,1), rr(i,2));
                v.normalize();
                nn(i,0)=v.x(); nn(i,1)=v.y(); nn(i,2)=v.z(); // Normal is radial
                // Scale position
                rr(i,0) = v.x() * radius + pos.x();
                rr(i,1) = v.y() * radius + pos.y();
                rr(i,2) = v.z() * radius + pos.z();
            }
            
            Eigen::MatrixX3i tris(20, 3);
            tris << 0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
                    1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                    3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
                    4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1;

            surf->createFromData(rr, nn, tris, color);
            return surf;
        };

        for (const auto &ch : info.chs) {
            if (ch.kind == FIFFV_MEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
                // Gold for MEG: #FFD700
                auto surf = createCube(pos, QColor(255, 215, 0), 0.012f); 
                m_megSensors.append(surf);
            }
            else if (ch.kind == FIFFV_EEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
                // Cyan for EEG: #00FFFF
                auto surf = createIcosahedron(pos, QColor(0, 255, 255), 0.008f); 
                m_eegSensors.append(surf);
            }
        }
    }
    
    // 3. Digitizers
    // Try to read dig points if not in info (or just use info.dig)
    QList<FiffDigPoint> points;
    if (hasInfo) {
        // If we read info successfully, use its digitizers (even if empty - likely means none exist)
        points = info.dig;
    } else {
        // Try reading directly as dig set (only if stream was valid but no info read)
        // Since stream is valid, the file is a FIF file.
        file.reset();
        digSet = FiffDigPointSet(file);
        // Copy to list
        for(int i=0; i<digSet.size(); ++i) points.append(digSet[i]);
    }

    m_digitizers.clear();
    
    // Check if we already defined createIcosahedron or need to redefine?
    // It's in the scope of `if(hasInfo)` above. We need these helpers available here too.
    // Let's redefine createIcosahedron for digitizers here (or move out of if(hasInfo))
    // Moving out is better but changing indentation is annoying with replace tool.
    // I'll just copy the lambda definition.
    auto createSphereDig = [](const QVector3D &pos, const QColor &color, float radius) -> std::shared_ptr<BrainSurface> {
        auto surf = std::make_shared<BrainSurface>();
        float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        
        Eigen::MatrixX3f rr(12, 3);
        rr << -1, t, 0,  1, t, 0,  -1, -t, 0,  1, -t, 0,
              0, -1, t,  0, 1, t,  0, -1, -t,  0, 1, -t,
              t, 0, -1,  t, 0, 1,  -t, 0, -1,  -t, 0, 1;
        
        Eigen::MatrixX3f nn(12, 3);
        for(int i=0; i<12; ++i) {
            QVector3D v(rr(i,0), rr(i,1), rr(i,2));
            v.normalize();
            nn(i,0)=v.x(); nn(i,1)=v.y(); nn(i,2)=v.z();
            rr(i,0) = v.x() * radius + pos.x();
            rr(i,1) = v.y() * radius + pos.y();
            rr(i,2) = v.z() * radius + pos.z();
        }
        Eigen::MatrixX3i tris(20, 3);
        tris << 0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
                1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
                4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1;
        surf->createFromData(rr, nn, tris, color);
        return surf;
    };

    for (const auto &p : points) {
        QVector3D pos(p.r[0], p.r[1], p.r[2]);
        QColor col(192, 192, 192); // Silver
        float size = 0.005f; // 5mm
        
        if (p.kind == FIFFV_POINT_CARDINAL) {
            col = QColor(255, 0, 0); // Red
            size = 0.01f;
        } else if (p.kind == FIFFV_POINT_HPI) {
            col = QColor(255, 165, 0); // Orange
            size = 0.008f;
        } else if (p.kind == FIFFV_POINT_EEG) {
            col = QColor(0, 255, 255); // Cyan
             size = 0.008f;
        }
        
        auto surf = createSphereDig(pos, col, size);
        m_digitizers.append(surf);
    }
    
    qDebug() << "BrainView: Sensors Loaded. MEG:" << m_megSensors.size() 
             << "EEG:" << m_eegSensors.size() 
             << "Dig:" << m_digitizers.size();
             
    // Store in general surfaces map for rendering updates? 
    // Ideally we keep them separate to toggle easier, but render loop needs to see them.
    // For now, let's add them to m_surfaces with special keys.
    
    // Helper to add list
    auto addList = [&](const QList<std::shared_ptr<BrainSurface>> &list, QString prefix) {
        for(int i=0; i<list.size(); ++i) {
           m_surfaces[prefix + QString::number(i)] = list[i];
           // Default to hidden
           list[i]->setVisible(false); 
        }
    };
    
    // Clear old sensor surfaces from map first (if any)
    for(auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if(it.key().startsWith("sens_")) it = m_surfaces.erase(it);
        else ++it;
    }

    addList(m_megSensors, "sens_meg_");
    addList(m_eegSensors, "sens_eeg_");
    addList(m_digitizers, "sens_dig_");

    return hasInfo || !digSet.isEmpty();
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
    if (modeName == "Standard") m_brainShaderMode = BrainRenderer::Standard;
    else if (modeName == "Holographic") m_brainShaderMode = BrainRenderer::Holographic;
    else if (modeName == "Glossy Realistic") m_brainShaderMode = BrainRenderer::Atlas;
    update();
}

void BrainView::setBemShaderMode(const QString &modeName)
{
    if (modeName == "Standard") m_bemShaderMode = BrainRenderer::Standard;
    else if (modeName == "Holographic") m_bemShaderMode = BrainRenderer::Holographic;
    else if (modeName == "Glossy Realistic") m_bemShaderMode = BrainRenderer::Atlas;
    update();
}

void BrainView::setSensorVisible(const QString &type, bool visible)
{
    QString prefix;
    if (type == "MEG") prefix = "sens_meg_";
    else if (type == "EEG") prefix = "sens_eeg_";
    else if (type == "Digitizer") prefix = "sens_dig_";
    else return;
    
    int count = 0;
    for(auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith(prefix)) {
            it.value()->setVisible(visible);
            count++;
        }
    }
    qDebug() << "Toggled" << type << "sensors:" << visible << "(" << count << "items)";
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

void BrainView::setBemVisible(const QString &name, bool visible)
{
    QString key = "bem_" + name;
    if (m_surfaces.contains(key)) {
        m_surfaces[key]->setVisible(visible);
        update();
    } else {
        qDebug() << "BrainView: setBemVisible could not find key:" << key;
    }
}

void BrainView::setBemHighContrast(bool enabled)
{
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("bem_")) {
            it.value()->setUseDefaultColor(enabled);
        }
    }
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
        QString modeStr = (m_brainShaderMode == BrainRenderer::Holographic) ? "Holographic" : (m_brainShaderMode == BrainRenderer::Atlas) ? "Atlas" : "Standard";
        m_fpsLabel->setText(QString("FPS: %1\nVertices: %2\nShader: %3").arg(fps, 0, 'f', 1).arg(m_activeSurface->vertexCount()).arg(modeStr));
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    QSize outputSize = renderTarget()->pixelSize();
    QMatrix4x4 projection;
    projection.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), 0.01f, 100.0f);

    float distance = 0.5f - m_zoom * 0.03f;
    QVector3D cameraPos = m_cameraRotation.rotatedVector(QVector3D(0, 0, distance));
    QVector3D upVector = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));

    QMatrix4x4 view;
    view.lookAt(cameraPos, QVector3D(0, 0, 0), upVector);

    BrainRenderer::SceneData sceneData;
    sceneData.mvp = rhi()->clipSpaceCorrMatrix();
    sceneData.mvp *= projection;
    
    // View Matrix
    sceneData.mvp *= view;
    
    // Model Matrix: Translate to center
    QMatrix4x4 model;
    model.translate(-m_sceneCenter);
    sceneData.mvp *= model; // Apply model transform
    
    sceneData.cameraPos = cameraPos; // Camera pos is in world space, but we moved object. 
    // Actually, view matrix looks at (0,0,0). Object is moved to (0,0,0). So cameraPos is correct relative to object center.
    
    sceneData.lightDir = cameraPos.normalized(); // Headlight
    sceneData.lightingEnabled = m_lightingEnabled;

    // Initialize renderer without specific mode
    m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount());
    
    m_renderer->beginFrame(cb, renderTarget());
    
    // Render all visible surfaces matching the active type OR BEM surfaces
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        bool isBem = it.key().startsWith("bem_");
        bool isMatchingType = it.key().endsWith(m_activeSurfaceType);
        
        // Render if it's a BEM surface (always render if loaded) or matches active type (pial/inflated)
        // OR if it's a sensor (starts with sens_)
        bool isSensor = it.key().startsWith("sens_");
        
        if (isBem || isMatchingType || isSensor) {
            BrainRenderer::ShaderMode mode = isBem ? m_bemShaderMode : m_brainShaderMode;
            // Force holographic shader for sensors
            if (isSensor) mode = BrainRenderer::Holographic;
            
            m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), mode);
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
        float speed = 0.5f;
        // Rotate around local Y (up) and X (right) axes
        QQuaternion paramY = QQuaternion::fromAxisAndAngle(0, 1, 0, -diff.x() * speed);
        QQuaternion paramX = QQuaternion::fromAxisAndAngle(1, 0, 0, -diff.y() * speed);

        // Apply rotations relative to current camera orientation
        m_cameraRotation = m_cameraRotation * paramY * paramX;
        m_cameraRotation.normalize();

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
                m_sourceOverlay->computeInterpolationMatrix(it.value().get(), hemi);
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

//=============================================================================================================

float BrainView::stcStep() const
{
    if (m_sourceOverlay && m_sourceOverlay->isLoaded()) {
        return m_sourceOverlay->tstep();
    }
    return 0.0f;
}

