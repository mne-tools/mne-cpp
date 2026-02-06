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
#include <Eigen/Dense>
#include "bemtreeitem.h"
#include <QMatrix4x4>
#include <QVector4D>
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
#include <inverse/dipoleFit/ecd_set.h>

#include <inverse/dipoleFit/ecd_set.h>
#include "sensortreeitem.h"
#include "dipoletreeitem.h"

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
    setMouseTracking(true); // Enable hover events
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&BrainView::update));
    m_updateTimer->start(16); // ~60 FPS update

    m_fpsLabel = new QLabel(this);
    m_fpsLabel->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 16px; background: rgba(0,0,0,100); padding: 5px;");
    m_fpsLabel->move(10, 10);
    m_fpsLabel->resize(300, 100);

    m_fpsTimer.start();

    m_regionLabel = new QLabel(this);
    m_regionLabel->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 16px; background: rgba(0,0,0,100); padding: 5px;");
    m_regionLabel->setText("");
    m_regionLabel->move(10, 120); // Below FPS label
    m_regionLabel->resize(300, 40);
    m_regionLabel->hide();
}

BrainView::~BrainView()
{
}

//=============================================================================================================

//=============================================================================================================

void BrainView::setModel(BrainTreeModel *model)
{
    m_model = model;
    connect(m_model, &BrainTreeModel::rowsInserted, this, &BrainView::onRowsInserted);
    connect(m_model, &BrainTreeModel::dataChanged, this, &BrainView::onDataChanged);
    
    // Initial population if not empty?
    // For now assuming we set model before adding data or iterate.
}

void BrainView::onRowsInserted(const QModelIndex &parent, int first, int last)
{

    if (!m_model) return;

    for (int i = first; i <= last; ++i) {
        QModelIndex index = m_model->index(i, 0, parent);
        QStandardItem* item = m_model->itemFromIndex(index);
        
        AbstractTreeItem* absItem = dynamic_cast<AbstractTreeItem*>(item);
        
        // Handle Surface Items
        if (absItem && absItem->type() == AbstractTreeItem::SurfaceItem + QStandardItem::UserType) {
            SurfaceTreeItem* surfItem = static_cast<SurfaceTreeItem*>(absItem);
            auto brainSurf = std::make_shared<BrainSurface>();
            
            // Load geometry from item
            brainSurf->fromSurface(surfItem->surfaceData());
            
            // Determine Hemisphere from Parent
            if (absItem->parent()) {
                QString parentText = absItem->parent()->text();
                if (parentText == "lh") brainSurf->setHemi(0);
                else if (parentText == "rh") brainSurf->setHemi(1);
            }
            
            // Set properties
            brainSurf->setVisible(surfItem->isVisible());
            
            // Map it
            // Map it
            m_itemSurfaceMap[item] = brainSurf;
            
            // Key generation: "hemi_type" e.g. "lh_pial"
            QString key;
            if (absItem->parent()) {
                key = absItem->parent()->text() + "_" + surfItem->text();
            } else {
                key = surfItem->text(); 
            }
            m_surfaces[key] = brainSurf;
            

            
            // Check for annotations
            if (!surfItem->annotationData().isEmpty()) {
                brainSurf->addAnnotation(surfItem->annotationData());
            }


            
            // Set active if first
            // Set active if first
            if (!m_activeSurface) {
                m_activeSurface = brainSurf;
                m_activeSurfaceType = surfItem->text();
                

            }
        }
        // Check for BEM Item (using dynamic_cast for safety)
        BemTreeItem* bemItem = dynamic_cast<BemTreeItem*>(absItem);
        if (bemItem) {
             const MNELIB::MNEBemSurface &bemSurfData = bemItem->bemSurfaceData();
             
             auto brainSurf = std::make_shared<BrainSurface>();

             // Load BEM geometry with color from item
             brainSurf->fromBemSurface(bemSurfData, bemItem->color());
             
             brainSurf->setVisible(bemItem->isVisible());
             
             // Map it
             m_itemSurfaceMap[item] = brainSurf;
             
 //            // Apply Head-to-MRI transformation if available (BEM is in Head space)
 //            if (!m_headToMriTrans.isEmpty()) {
 //                QMatrix4x4 m;
 //                for(int r=0; r<4; ++r) for(int c=0; c<4; ++c) m(r,c) = m_headToMriTrans.trans(r,c);
 //                brainSurf->transform(m);
 //            }

             // Legacy map support (Use item text e.g. "bem_head")
             m_surfaces["bem_" + bemItem->text()] = brainSurf;
             

        }
        
        // Handle Sensor Items
        if (absItem && absItem->type() == AbstractTreeItem::SensorItem + QStandardItem::UserType) {
            SensorTreeItem* sensItem = static_cast<SensorTreeItem*>(absItem);
            
            std::shared_ptr<BrainSurface> brainSurf;
            
            // ... (keep generators) ...
            auto createCube = [](const QVector3D &pos, const QColor &color, float size) {
                auto surf = std::make_shared<BrainSurface>();
                float s = size / 2.0f;
                // 6 faces * 4 verts = 24 vertices
                Eigen::MatrixX3f rr(24, 3); Eigen::MatrixX3f nn(24, 3);
                // Front
                rr.row(0) << -s, -s, s; nn.row(0) << 0, 0, 1; rr.row(1) << s, -s, s;  nn.row(1) << 0, 0, 1;
                rr.row(2) << s, s, s;   nn.row(2) << 0, 0, 1; rr.row(3) << -s, s, s;  nn.row(3) << 0, 0, 1;
                // Back
                rr.row(4) << s, -s, -s;   nn.row(4) << 0, 0, -1; rr.row(5) << -s, -s, -s;  nn.row(5) << 0, 0, -1;
                rr.row(6) << -s, s, -s;   nn.row(6) << 0, 0, -1; rr.row(7) << s, s, -s;    nn.row(7) << 0, 0, -1;
                // Top
                rr.row(8) << -s, s, s;    nn.row(8) << 0, 1, 0;  rr.row(9) << s, s, s;     nn.row(9) << 0, 1, 0;
                rr.row(10) << s, s, -s;   nn.row(10) << 0, 1, 0; rr.row(11) << -s, s, -s;  nn.row(11) << 0, 1, 0;
                // Bottom
                rr.row(12) << -s, -s, -s; nn.row(12) << 0, -1, 0; rr.row(13) << s, -s, -s;  nn.row(13) << 0, -1, 0;
                rr.row(14) << s, -s, s;   nn.row(14) << 0, -1, 0; rr.row(15) << -s, -s, s;  nn.row(15) << 0, -1, 0;
                // Right
                rr.row(16) << s, -s, s;   nn.row(16) << 1, 0, 0;  rr.row(17) << s, -s, -s;  nn.row(17) << 1, 0, 0;
                rr.row(18) << s, s, -s;   nn.row(18) << 1, 0, 0;  rr.row(19) << s, s, s;    nn.row(19) << 1, 0, 0;
                // Left
                rr.row(20) << -s, -s, -s; nn.row(20) << -1, 0, 0; rr.row(21) << -s, -s, s;  nn.row(21) << -1, 0, 0;
                rr.row(22) << -s, s, s;   nn.row(22) << -1, 0, 0; rr.row(23) << -s, s, -s;  nn.row(23) << -1, 0, 0;

                for (int i = 0; i < 24; ++i) { rr(i, 0) += pos.x(); rr(i, 1) += pos.y(); rr(i, 2) += pos.z(); }
                Eigen::MatrixX3i tris(12, 3);
                for (int f = 0; f < 6; ++f) {
                    int base = f * 4; tris.row(f * 2) << base, base + 1, base + 2; tris.row(f * 2 + 1) << base, base + 2, base + 3;
                }
                surf->createFromData(rr, nn, tris, color);
                return surf;
            };

            auto createIcosahedron = [](const QVector3D &pos, const QColor &color, float radius) {
                auto surf = std::make_shared<BrainSurface>();
                float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
                Eigen::MatrixX3f rr(12, 3);
                rr << -1, t, 0, 1, t, 0, -1, -t, 0, 1, -t, 0, 0, -1, t, 0, 1, t, 0, -1, -t, 0, 1, -t, t, 0, -1, t, 0, 1, -t, 0, -1, -t, 0, 1;
                Eigen::MatrixX3f nn(12, 3);
                for (int i = 0; i < 12; ++i) {
                    QVector3D v(rr(i, 0), rr(i, 1), rr(i, 2)); v.normalize(); nn(i, 0) = v.x(); nn(i, 1) = v.y(); nn(i, 2) = v.z();
                    rr(i, 0) = v.x() * radius + pos.x(); rr(i, 1) = v.y() * radius + pos.y(); rr(i, 2) = v.z() * radius + pos.z();
                }
                Eigen::MatrixX3i tris(20, 3);
                tris << 0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1;
                surf->createFromData(rr, nn, tris, color);
                return surf;
            };
            
            QString parentText = "";
            if (sensItem->parent()) parentText = sensItem->parent()->text();
            

            
            if (parentText.contains("MEG")) {
                 brainSurf = createCube(sensItem->position(), sensItem->color(), sensItem->scale());
            } else if (parentText.contains("EEG")) {
                 brainSurf = createIcosahedron(sensItem->position(), sensItem->color(), sensItem->scale());
            } else {
                 brainSurf = createIcosahedron(sensItem->position(), sensItem->color(), sensItem->scale());
            }
            
            brainSurf->setVisible(sensItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;
            
            // Apply Head-to-MRI transformation if available
            // Note: meg positions in info might already be head-space, but check if we need this global trans
            if (!m_headToMriTrans.isEmpty()) {
                QMatrix4x4 m;
                if (m_applySensorTrans) {
                    for(int r=0; r<4; ++r) for(int c=0; c<4; ++c) m(r,c) = m_headToMriTrans.trans(r,c);
                }
                brainSurf->applyTransform(m);
            }

            // Legacy map support
            QString keyPrefix = "sens_";
            if (parentText.contains("MEG")) keyPrefix = "sens_meg_";
            else if (parentText.contains("EEG")) keyPrefix = "sens_eeg_";
            else if (parentText.contains("Digitizer")) keyPrefix = "sens_dig_";
            
            QString key = keyPrefix + sensItem->text() + "_" + QString::number((quintptr)sensItem);
            m_surfaces[key] = brainSurf;
            

        }
        
        // Handle Dipole Items
        if (absItem && absItem->type() == AbstractTreeItem::DipoleItem + QStandardItem::UserType) {
            DipoleTreeItem* dipItem = static_cast<DipoleTreeItem*>(absItem);
            auto dipObject = std::make_shared<DipoleObject>();
            dipObject->load(dipItem->ecdSet());
            dipObject->setVisible(dipItem->isVisible());
            
            m_itemDipoleMap[item] = dipObject;
        }
        

        
        // Check children recursively
        if (m_model->hasChildren(index)) {
            onRowsInserted(index, 0, m_model->rowCount(index) - 1);
        }
    }
    update();
}

void BrainView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    // Update visuals based on roles
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
         QModelIndex index = m_model->index(i, 0, topLeft.parent());
         QStandardItem* item = m_model->itemFromIndex(index);
         
         if (m_itemSurfaceMap.contains(item)) {
             auto surf = m_itemSurfaceMap[item];
             
             AbstractTreeItem* absItem = dynamic_cast<AbstractTreeItem*>(item);
             if (absItem) {
                 if (roles.contains(AbstractTreeItem::VisibleRole)) {
                     surf->setVisible(absItem->isVisible());

                 }
                 if (roles.contains(AbstractTreeItem::ColorRole)) {
                     // Update color (not fully impl in BrainSurface yet for uniform override, but prepared)
                 }
                 if (roles.contains(SurfaceTreeItem::AnnotationDataRole)) {
                      SurfaceTreeItem* sItem = static_cast<SurfaceTreeItem*>(absItem);
                      if (!sItem->annotationData().isEmpty()) {
                          surf->addAnnotation(sItem->annotationData());
                      }
                 }
             }
         }
    }
    update();
}

//=============================================================================================================




//=============================================================================================================



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
    update();
}

void BrainView::setSensorTransEnabled(bool enabled)
{
    if (m_applySensorTrans != enabled) {
        m_applySensorTrans = enabled;
        refreshSensorTransforms();
        update();
    }
}

void BrainView::setDipoleVisible(bool visible)
{
    m_dipolesVisible = visible;
    update();
}

//=============================================================================================================

void BrainView::setVisualizationMode(const QString &modeName)
{
    BrainSurface::VisualizationMode mode = BrainSurface::ModeSurface;
    if (modeName == "Annotation") mode = BrainSurface::ModeAnnotation;
    if (modeName == "Scientific") mode = BrainSurface::ModeScientific;
    
    m_currentVisMode = mode;
    for (auto surf : m_surfaces) {
        surf->setVisualizationMode(mode);
    }
    update();
}

//=============================================================================================================

void BrainView::setHemiVisible(int hemiIdx, bool visible)
{
    int count = 0;
    for (auto surf : m_surfaces) {
        if (surf->hemi() == hemiIdx) {
            surf->setVisible(visible);
        }
    }
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
        // Fallback: Search in Model
        if (m_model) {
            QList<QStandardItem*> items = m_model->findItems(key, Qt::MatchRecursive);
            if (!items.isEmpty()) {
                 QStandardItem* item = items.first();
                 AbstractTreeItem* absItem = dynamic_cast<AbstractTreeItem*>(item);
                 if (absItem) {
                     absItem->setVisible(visible); // Triggers onDataChanged -> Updates Surface
                     // Also ensure BrainSurface matches if onDataChanged handling is partial
                     if (m_itemSurfaceMap.contains(item)) {
                         m_itemSurfaceMap[item]->setVisible(visible);
                     }
                     return;
                 }
            }
        }
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

}

//=============================================================================================================

void BrainView::initialize(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);

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
    // Adjust planes based on scene size (e.g. 15cm -> Far 3m, Near 1mm)
    float farPlane = m_sceneSize * 20.0f;
    if (farPlane < 100.0f) farPlane = 100.0f; // Minimum 100 units
    projection.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), m_sceneSize * 0.01f, farPlane);

    // Default distance relative to size. 1.5x diagonal is usually good for framing.
    float baseDistance = m_sceneSize * 1.5f;
    // Adapt zoom speed: 100 zoom steps should cover the range
    float distance = baseDistance - m_zoom * (m_sceneSize * 0.05f); 
    
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
    // Render all visible surfaces matching the active type OR BEM surfaces
    // Pass 1: Opaque Surfaces (Head, Brain, etc.)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("bem_")) continue;
        if (it.key().startsWith("sens_")) continue;
        
        if (it.key().endsWith(m_activeSurfaceType)) {
            m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), m_brainShaderMode);
        }
    }
    
    // Pass 2: Transparent Surfaces (Sensors & BEM) sorted Back-to-Front
    struct RenderItem {
        BrainSurface* surf;
        float dist;
        BrainRenderer::ShaderMode mode;
    };
    QVector<RenderItem> transparentItems;
    
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        bool isSensor = it.key().startsWith("sens_");
        bool isBem = it.key().startsWith("bem_");
        
        if (!isSensor && !isBem) continue;
        if (!it.value()->isVisible()) continue;

        QVector3D min, max;
        it.value()->boundingBox(min, max);
        QVector3D center = (min + max) * 0.5f;
        // Distance to camera position
        float d = (sceneData.cameraPos - center).lengthSquared();
        
        BrainRenderer::ShaderMode mode = BrainRenderer::Holographic;
        if (isBem) mode = m_bemShaderMode;
        
        transparentItems.append({it.value().get(), d, mode});
    }
    
    // Sort Back-to-Front (Descenting distance)
    std::sort(transparentItems.begin(), transparentItems.end(), [](const RenderItem &a, const RenderItem &b) {
        return a.dist > b.dist; 
    });
    
    for (const auto &item : transparentItems) {
        m_renderer->renderSurface(cb, rhi(), sceneData, item.surf, item.mode);
    }
    
    // Render Dipoles from Map
    for(auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
        if (it.value()->isVisible() && m_dipolesVisible) {
             m_renderer->renderDipoles(cb, rhi(), sceneData, it.value().get());
        }
    }
    
    
    // Render Dipoles
    if (m_dipolesVisible && m_dipoles) {
        m_renderer->renderDipoles(cb, rhi(), sceneData, m_dipoles.get());
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
    } else {
        castRay(event->pos());
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
        m_currentVisMode = BrainSurface::ModeSourceEstimate;
        // Compute interpolation matrices for all surfaces
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (it.key().endsWith(m_activeSurfaceType)) {
                int hemi = it.value()->hemi();

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

//=============================================================================================================

bool BrainView::loadSensors(const QString &fifPath) {
  QFile file(fifPath);
  if (!file.exists()) return false;

  FiffInfo info;
  FiffDigPointSet digSet;
  FiffStream::SPtr stream(new FiffStream(&file));
  
  bool hasInfo = false;
  if (stream->open()) {
      FiffDirNode::SPtr tree = stream->dirtree();
      FiffDirNode::SPtr nodeInfo;
      if (stream->read_meas_info(tree, info, nodeInfo)) {
          hasInfo = !info.isEmpty();
      }
  }

  if (hasInfo) {
      QList<QStandardItem*> megItems;
      QList<QStandardItem*> eegItems;
      
      // Prepare Device->Head transformation
      QMatrix4x4 devHeadQTrans;
      bool hasDevHead = false;
      if (!info.dev_head_t.isEmpty() && 
           info.dev_head_t.from == FIFFV_COORD_DEVICE && 
           info.dev_head_t.to == FIFFV_COORD_HEAD &&
           !info.dev_head_t.trans.isIdentity()) {
          hasDevHead = true;
          for(int r=0; r<4; ++r)
              for(int c=0; c<4; ++c)
                  devHeadQTrans(r,c) = info.dev_head_t.trans(r,c);
      } else if (!info.dev_head_t.isEmpty()) {
      }

      for (const auto &ch : info.chs) {
          if (ch.kind == FIFFV_MEG_CH) {
              QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
              
              // Transform MEG from Device to Head space
              if (hasDevHead) {
                  pos = devHeadQTrans.map(pos);
              }

              megItems.append(new SensorTreeItem(ch.ch_name, pos, QColor(255, 215, 0), 0.012f));
          } else if (ch.kind == FIFFV_EEG_CH) {
              QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
              eegItems.append(new SensorTreeItem(ch.ch_name, pos, QColor(0, 255, 255), 0.008f));
          }
      }
      
      if (!megItems.isEmpty()) m_model->addSensors("MEG", megItems);
      if (!eegItems.isEmpty()) m_model->addSensors("EEG", eegItems);
  }
  
  // Try to load digitizers if available in info
  QList<QStandardItem*> digItems;
  if (hasInfo && info.dig.size() > 0) {
      for (const auto &p : info.dig) {
        QVector3D pos(p.r[0], p.r[1], p.r[2]);
        QColor col(192, 192, 192);
        float size = 0.005f;
        
        if (p.kind == FIFFV_POINT_CARDINAL) { col = QColor(255, 0, 0); size = 0.01f; }
        else if (p.kind == FIFFV_POINT_HPI) { col = QColor(255, 165, 0); size = 0.008f; }
        else if (p.kind == FIFFV_POINT_EEG) { col = QColor(0, 255, 255); size = 0.008f; }
        
        digItems.append(new SensorTreeItem("Dig", pos, col, size));
      }
      m_model->addSensors("Digitizer", digItems);
  } else if (!hasInfo) { // If no info, try to load as dig set
      file.reset(); // Reset to read as dig set
      digSet = FiffDigPointSet(file);
      if (digSet.size() > 0) {
          for(int i=0; i<digSet.size(); ++i) {
              const auto &p = digSet[i];
              QVector3D pos(p.r[0], p.r[1], p.r[2]);
              QColor col(192, 192, 192); // Silver
              float size = 0.005f;

              if (p.kind == FIFFV_POINT_CARDINAL) { col = QColor(255, 0, 0); size = 0.01f; }
              else if (p.kind == FIFFV_POINT_HPI) { col = QColor(255, 165, 0); size = 0.008f; }
              else if (p.kind == FIFFV_POINT_EEG) { col = QColor(0, 255, 255); size = 0.008f; }
              
              digItems.append(new SensorTreeItem("Dig", pos, col, size));
          }
          m_model->addSensors("Digitizer", digItems);
      }
  }
  
  return hasInfo || !digItems.isEmpty();
}

//=============================================================================================================

bool BrainView::loadDipoles(const QString &dipPath)
{
    INVERSELIB::ECDSet ecdSet = INVERSELIB::ECDSet::read_dipoles_dip(dipPath);
    if (ecdSet.size() == 0) {
        qWarning() << "BrainView: Failed to load dipoles from" << dipPath;
        return false;
    }
    
    // Check if dipoles are already added or if we want to overwrite?
    // The model's addDipoles appends a new row.
    
    m_model->addDipoles(ecdSet);
    
    return true;
}

//=============================================================================================================

bool BrainView::loadTransformation(const QString &transPath)
{
    QFile file(transPath);
    // Don't open explicitly, FiffCoordTrans::read opens the device via FiffStream.
    
    FiffCoordTrans trans;
    if (!FiffCoordTrans::read(file, trans)) {
        qWarning() << "BrainView: Failed to load transformation from" << transPath;
        return false;
    }
    file.close();
    
    // Check if it is Head->MRI or MRI->Head
    // FIFFV_COORD_HEAD=4, FIFFV_COORD_MRI=5
    // We want Head->MRI to transform sensors (Head) to MRI (Surface).
    
    if (trans.from == FIFFV_COORD_HEAD && trans.to == FIFFV_COORD_MRI) {
        m_headToMriTrans = trans;
    } else if (trans.from == FIFFV_COORD_MRI && trans.to == FIFFV_COORD_HEAD) {
        // Invert: MRI->Head becomes Head->MRI
        m_headToMriTrans.from = trans.to;
        m_headToMriTrans.to = trans.from;
        m_headToMriTrans.trans = trans.trans.inverse();
        m_headToMriTrans.invtrans = trans.trans;
    } else {
        qWarning() << "BrainView: Loaded transformation is not Head<->MRI (from" << trans.from << "to" << trans.to << "). Using as is.";
        m_headToMriTrans = trans;
    }
    

    refreshSensorTransforms();

    return true;
}

void BrainView::refreshSensorTransforms()
{
    QMatrix4x4 qmat;
    if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
        for(int r=0; r<4; ++r) {
            for(int c=0; c<4; ++c) {
                qmat(r,c) = m_headToMriTrans.trans(r,c);
            }
        }
    }

    int surfCount = 0;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("sens_") && it.value()) {
            it.value()->applyTransform(qmat);
            surfCount++;
        }
    }
    // Also check item map just in case (redundant but safe)
    for (auto surf : m_itemSurfaceMap) {
         // We don't have a reliable way to check prefix here without extra logic, 
         // but m_surfaces should cover it.
    }
}


//=============================================================================================================

void BrainView::castRay(const QPoint &pos)
{
    // 1. Setup Matrix Stack (Must match render exactly)
    QSize outputSize = renderTarget()->pixelSize();
    QMatrix4x4 projection;
    float farPlane = m_sceneSize * 20.0f;
    if (farPlane < 100.0f) farPlane = 100.0f;
    projection.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), m_sceneSize * 0.01f, farPlane);
    
    float baseDistance = m_sceneSize * 1.5f;
    float distance = baseDistance - m_zoom * (m_sceneSize * 0.05f); 
    QVector3D cameraPos = m_cameraRotation.rotatedVector(QVector3D(0, 0, distance));
    QVector3D upVector = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));
    QMatrix4x4 view;
    view.lookAt(cameraPos, QVector3D(0, 0, 0), upVector);
    
    QMatrix4x4 model;
    model.translate(-m_sceneCenter);
    QMatrix4x4 pvm = projection * view * model;
    QMatrix4x4 invPVM = pvm.inverted();
    
    float ndcX = (2.0f * pos.x()) / width() - 1.0f;
    float ndcY = 1.0f - (2.0f * pos.y()) / height(); 
    
    QVector4D vNear(ndcX, ndcY, -1.0f, 1.0f);
    QVector4D vFar(ndcX, ndcY, 1.0f, 1.0f);
    
    QVector4D pNear = invPVM * vNear;
    QVector4D pFar = invPVM * vFar;
    pNear /= pNear.w();
    pFar /= pFar.w();
    
    QVector3D rayOrigin = pNear.toVector3D();
    QVector3D rayDir = (pFar.toVector3D() - pNear.toVector3D()).normalized();
    
    
    
    float closestDist = std::numeric_limits<float>::max();
    QStandardItem* hitItem = nullptr;
    QString hitInfo;
    
    int hitIndex = -1;
    
    // Check Surfaces (Sensors, Hemisphere, BEM)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.value()->isVisible()) continue;
        
        bool isSensor = it.key().startsWith("sens_");
        
        float dist;
        int vertexIdx = -1;
        if (it.value()->intersects(rayOrigin, rayDir, dist, vertexIdx)) {
             if (dist < closestDist) {
                 closestDist = dist;
                 for(auto i = m_itemSurfaceMap.begin(); i != m_itemSurfaceMap.end(); ++i) {
                     if (i.value() == it.value()) {
                         hitItem = const_cast<QStandardItem*>(i.key());
                         hitInfo = hitItem->text();
                         break;
                     }
                 }
                 if (!hitItem && isSensor) hitInfo = it.key(); 
                 hitIndex = vertexIdx;
             }
        }
    }
    
    // Check Dipoles
    for(auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
         if (!it.value()->isVisible()) continue;
         
         float dist;
         int dipIdx = it.value()->intersect(rayOrigin, rayDir, dist);
         if (dipIdx != -1) {
             if (dist < closestDist) {
                 closestDist = dist;
                 hitItem = const_cast<QStandardItem*>(it.key());
                 hitInfo = QString("%1 (Dipole %2)").arg(hitItem->text()).arg(dipIdx);
                 hitIndex = dipIdx;
             }
         }
     }

    // Handle Region Name for Annotations
    QString currentRegion;
    int currentRegionId = -1;
    if (hitItem && m_itemSurfaceMap.contains(hitItem)) {
        currentRegion = m_itemSurfaceMap[hitItem]->getAnnotationLabel(hitIndex);
        currentRegionId = m_itemSurfaceMap[hitItem]->getAnnotationLabelId(hitIndex);
    }

    if (currentRegion != m_hoveredRegion) {
        m_hoveredRegion = currentRegion;
        emit hoveredRegionChanged(m_hoveredRegion);
        if (m_regionLabel) {
            if (m_hoveredRegion.isEmpty()) {
                m_regionLabel->hide();
            } else {
                m_regionLabel->setText(QString("Region: %1").arg(m_hoveredRegion));
                m_regionLabel->show();
            }
        }
    }
    
    if (hitItem != m_hoveredItem || hitIndex != m_hoveredIndex) {
        // Deselect previous
        if (m_hoveredItem) {
             if (m_itemSurfaceMap.contains(m_hoveredItem)) {
                 m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
                 m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
             } else if (m_itemDipoleMap.contains(m_hoveredItem)) {
                 m_itemDipoleMap[m_hoveredItem]->setSelected(m_hoveredIndex, false);
             }
        }
    
        m_hoveredItem = hitItem;
        m_hoveredIndex = hitIndex;
        
        if (m_hoveredItem) {
             // Select new
             if (m_itemSurfaceMap.contains(m_hoveredItem)) {
                 if (currentRegionId != -1) {
                     m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
                     m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
                 } else {
                     m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
                     m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
                 }
             } else if (m_itemDipoleMap.contains(m_hoveredItem)) {
                 m_itemDipoleMap[m_hoveredItem]->setSelected(m_hoveredIndex, true);
             }
        }
    } else if (m_hoveredItem && m_itemSurfaceMap.contains(m_hoveredItem)) {
        if (currentRegionId != -1) {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
        } else {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
        }
    }
    update();
}
