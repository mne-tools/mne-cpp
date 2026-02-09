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
#include "sourcespacetreeitem.h"
#include "../workers/stcloadingworker.h"

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
    m_regionLabel->move(10, 120); 
    m_regionLabel->resize(300, 40);
    m_regionLabel->hide();

    // Setup Debug Pointer: Semi-transparent sphere for subtle intersection indicator
    m_debugPointerSurface = std::make_shared<BrainSurface>();
    
    // Create UV sphere (more visually appealing than a cube)
    const float radius = 0.002f; // 2mm radius
    const int latSegments = 8;   // Latitude bands
    const int lonSegments = 12;  // Longitude segments
    
    int numVerts = (latSegments - 1) * lonSegments + 2; // +2 for poles
    int numTris = (latSegments - 2) * lonSegments * 2 + lonSegments * 2; // Quads + pole fans
    
    Eigen::MatrixX3f rr(numVerts, 3);
    Eigen::MatrixX3f nn(numVerts, 3);
    Eigen::MatrixX3i tris(numTris, 3);
    
    int vIdx = 0;
    // Top pole
    rr.row(vIdx) << 0, radius, 0;
    nn.row(vIdx) << 0, 1, 0;
    vIdx++;
    
    // Middle rings
    for (int lat = 1; lat < latSegments; ++lat) {
        float theta = M_PI * lat / latSegments;
        float sinT = std::sin(theta);
        float cosT = std::cos(theta);
        for (int lon = 0; lon < lonSegments; ++lon) {
            float phi = 2.0f * M_PI * lon / lonSegments;
            float x = sinT * std::cos(phi);
            float y = cosT;
            float z = sinT * std::sin(phi);
            rr.row(vIdx) << radius * x, radius * y, radius * z;
            nn.row(vIdx) << x, y, z;
            vIdx++;
        }
    }
    
    // Bottom pole
    rr.row(vIdx) << 0, -radius, 0;
    nn.row(vIdx) << 0, -1, 0;
    int bottomPole = vIdx;
    
    // Triangles
    int tIdx = 0;
    // Top cap (fan)
    for (int lon = 0; lon < lonSegments; ++lon) {
        int next = (lon + 1) % lonSegments;
        tris.row(tIdx++) << 0, 1 + lon, 1 + next;
    }
    
    // Middle quads
    for (int lat = 0; lat < latSegments - 2; ++lat) {
        int ringStart = 1 + lat * lonSegments;
        int nextRingStart = ringStart + lonSegments;
        for (int lon = 0; lon < lonSegments; ++lon) {
            int curr = ringStart + lon;
            int next = ringStart + (lon + 1) % lonSegments;
            int currBelow = nextRingStart + lon;
            int nextBelow = nextRingStart + (lon + 1) % lonSegments;
            tris.row(tIdx++) << curr, currBelow, next;
            tris.row(tIdx++) << next, currBelow, nextBelow;
        }
    }
    
    // Bottom cap (fan)
    int lastRingStart = 1 + (latSegments - 2) * lonSegments;
    for (int lon = 0; lon < lonSegments; ++lon) {
        int next = (lon + 1) % lonSegments;
        tris.row(tIdx++) << lastRingStart + lon, bottomPole, lastRingStart + next;
    }
    
    // Semi-transparent white/cyan for subtle appearance on any surface
    m_debugPointerSurface->createFromData(rr, nn, tris, QColor(200, 255, 255, 160));
    
    // Initialize multi-view fixed cameras
    // Top view: looking down Y-axis (from above)
    m_multiViewCameras[0] = QQuaternion::fromAxisAndAngle(1, 0, 0, 90);
    // Left view: looking from -X towards center (side view)
    m_multiViewCameras[1] = QQuaternion::fromAxisAndAngle(0, 1, 0, -90);
    // Front view: looking from +Z towards center (default front)
    m_multiViewCameras[2] = QQuaternion::fromAxisAndAngle(0, 0, 1, 0); // Identity for front
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

//=============================================================================================================

void BrainView::setInitialCameraRotation(const QQuaternion &rotation)
{
    m_cameraRotation = rotation;
    update();
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
            
            // Brain surfaces (pial, white, inflated, etc.) are brain tissue
            brainSurf->setTissueType(BrainSurface::TissueBrain);
            
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
             
             // Set tissue type based on surface name
             QString surfName = bemItem->text().toLower();
             if (surfName.contains("head") || surfName.contains("skin") || surfName.contains("scalp")) {
                 brainSurf->setTissueType(BrainSurface::TissueSkin);
             } else if (surfName.contains("outer") && surfName.contains("skull")) {
                 brainSurf->setTissueType(BrainSurface::TissueOuterSkull);
             } else if (surfName.contains("inner") && surfName.contains("skull")) {
                 brainSurf->setTissueType(BrainSurface::TissueInnerSkull);
             } else if (surfName.contains("skull")) {
                 brainSurf->setTissueType(BrainSurface::TissueOuterSkull); // Default skull to outer
             } else if (surfName.contains("brain")) {
                 brainSurf->setTissueType(BrainSurface::TissueBrain);
             }
             
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
            
            // Oriented flat plate for MEG sensors (10x10x1mm cuboid with coil_trans rotation)
            auto createOrientedPlate = [](const QVector3D &pos, const QMatrix4x4 &orient, const QColor &color, float size) {
                auto surf = std::make_shared<BrainSurface>();
                float hw = size / 2.0f;      // half-width  (5mm)
                float hh = size / 2.0f;      // half-height (5mm)
                float hd = size * 0.05f;     // half-depth  (0.5mm) — thin plate

                // 8 corners in local space, then rotated by coil orientation
                Eigen::Vector3f corners[8] = {
                    {-hw, -hh, -hd}, { hw, -hh, -hd}, { hw,  hh, -hd}, {-hw,  hh, -hd},  // back
                    {-hw, -hh,  hd}, { hw, -hh,  hd}, { hw,  hh,  hd}, {-hw,  hh,  hd}   // front
                };

                // Extract 3x3 rotation from orientation
                Eigen::Matrix3f rot;
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        rot(r, c) = orient(r, c);

                for (auto &c : corners) c = rot * c;

                // 6 faces, 4 verts each = 24 verts
                Eigen::MatrixX3f rr(24, 3), nn(24, 3);
                Eigen::MatrixX3i tris(12, 3);

                // Face definitions: {v0,v1,v2,v3}, normal direction
                int faces[6][4] = {
                    {4,5,6,7}, {1,0,3,2},  // front(+Z), back(-Z)
                    {3,7,6,2}, {0,1,5,4},  // top(+Y), bottom(-Y)
                    {1,2,6,5}, {0,4,7,3}   // right(+X), left(-X)
                };

                for (int f = 0; f < 6; ++f) {
                    Eigen::Vector3f v0 = corners[faces[f][0]];
                    Eigen::Vector3f v1 = corners[faces[f][1]];
                    Eigen::Vector3f v2 = corners[faces[f][2]];
                    Eigen::Vector3f fn = (v1 - v0).cross(v2 - v0).normalized();
                    int base = f * 4;
                    for (int k = 0; k < 4; ++k) {
                        Eigen::Vector3f v = corners[faces[f][k]];
                        rr.row(base + k) << v.x() + pos.x(), v.y() + pos.y(), v.z() + pos.z();
                        nn.row(base + k) = fn;
                    }
                    tris.row(f * 2)     << base, base + 1, base + 2;
                    tris.row(f * 2 + 1) << base, base + 2, base + 3;
                }

                surf->createFromData(rr, nn, tris, color);
                return surf;
            };

            // Smooth sphere (subdivided icosahedron) for EEG and digitizer points
            auto createSphere = [](const QVector3D &pos, const QColor &color, float radius) {
                auto surf = std::make_shared<BrainSurface>();
                const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
                QVector<Eigen::Vector3f> verts;
                verts.reserve(42);
                verts << Eigen::Vector3f(-1, phi, 0) << Eigen::Vector3f( 1, phi, 0)
                      << Eigen::Vector3f(-1,-phi, 0) << Eigen::Vector3f( 1,-phi, 0)
                      << Eigen::Vector3f( 0,-1, phi) << Eigen::Vector3f( 0, 1, phi)
                      << Eigen::Vector3f( 0,-1,-phi) << Eigen::Vector3f( 0, 1,-phi)
                      << Eigen::Vector3f( phi, 0,-1) << Eigen::Vector3f( phi, 0, 1)
                      << Eigen::Vector3f(-phi, 0,-1) << Eigen::Vector3f(-phi, 0, 1);
                for (auto &v : verts) v.normalize();

                QVector<Eigen::Vector3i> faces;
                faces << Eigen::Vector3i(0,11,5)  << Eigen::Vector3i(0,5,1)
                      << Eigen::Vector3i(0,1,7)   << Eigen::Vector3i(0,7,10)
                      << Eigen::Vector3i(0,10,11) << Eigen::Vector3i(1,5,9)
                      << Eigen::Vector3i(5,11,4)  << Eigen::Vector3i(11,10,2)
                      << Eigen::Vector3i(10,7,6)  << Eigen::Vector3i(7,1,8)
                      << Eigen::Vector3i(3,9,4)   << Eigen::Vector3i(3,4,2)
                      << Eigen::Vector3i(3,2,6)   << Eigen::Vector3i(3,6,8)
                      << Eigen::Vector3i(3,8,9)   << Eigen::Vector3i(4,9,5)
                      << Eigen::Vector3i(2,4,11)  << Eigen::Vector3i(6,2,10)
                      << Eigen::Vector3i(8,6,7)   << Eigen::Vector3i(9,8,1);

                // Subdivide once for smoothness (42 verts, 80 tris)
                QMap<QPair<int,int>, int> cache;
                auto mid = [&](int a, int b) -> int {
                    auto key = qMakePair(qMin(a,b), qMax(a,b));
                    if (cache.contains(key)) return cache[key];
                    int idx = verts.size();
                    verts.append((verts[a] + verts[b]).normalized());
                    cache[key] = idx;
                    return idx;
                };
                QVector<Eigen::Vector3i> newFaces;
                newFaces.reserve(80);
                for (const auto &f : faces) {
                    int ab = mid(f(0), f(1)), bc = mid(f(1), f(2)), ca = mid(f(2), f(0));
                    newFaces << Eigen::Vector3i(f(0), ab, ca) << Eigen::Vector3i(f(1), bc, ab)
                             << Eigen::Vector3i(f(2), ca, bc) << Eigen::Vector3i(ab, bc, ca);
                }

                int nV = verts.size(), nT = newFaces.size();
                Eigen::MatrixX3f rr(nV, 3), nn(nV, 3);
                for (int i = 0; i < nV; ++i) {
                    nn(i, 0) = verts[i].x(); nn(i, 1) = verts[i].y(); nn(i, 2) = verts[i].z();
                    rr(i, 0) = verts[i].x() * radius + pos.x();
                    rr(i, 1) = verts[i].y() * radius + pos.y();
                    rr(i, 2) = verts[i].z() * radius + pos.z();
                }
                Eigen::MatrixX3i tris(nT, 3);
                for (int i = 0; i < nT; ++i) tris.row(i) = newFaces[i];

                surf->createFromData(rr, nn, tris, color);
                return surf;
            };
            
            QString parentText = "";
            if (sensItem->parent()) parentText = sensItem->parent()->text();

            if (parentText.contains("MEG") && sensItem->hasOrientation()) {
                 brainSurf = createOrientedPlate(sensItem->position(), sensItem->orientation(),
                                                  sensItem->color(), sensItem->scale());
            } else {
                 // EEG and Digitizer: smooth spheres
                 brainSurf = createSphere(sensItem->position(), sensItem->color(), sensItem->scale());
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
        
        // Handle Source Space Items (one item per hemisphere, batched mesh)
        if (absItem && absItem->type() == AbstractTreeItem::SourceSpaceItem + QStandardItem::UserType) {
            SourceSpaceTreeItem* srcItem = static_cast<SourceSpaceTreeItem*>(absItem);
            const QVector<QVector3D>& positions = srcItem->positions();
            if (positions.isEmpty()) continue;

            const float r = srcItem->scale();
            const QColor color = srcItem->color();
            const int nPts = positions.size();

            // Build template: subdivided icosahedron for smooth sphere look (matches disp3D)
            // Start with base icosahedron (12 verts, 20 tris)
            const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
            QVector<Eigen::Vector3f> icoV;
            icoV.reserve(42);
            icoV << Eigen::Vector3f(-1, phi, 0) << Eigen::Vector3f( 1, phi, 0)
                 << Eigen::Vector3f(-1,-phi, 0) << Eigen::Vector3f( 1,-phi, 0)
                 << Eigen::Vector3f( 0,-1, phi) << Eigen::Vector3f( 0, 1, phi)
                 << Eigen::Vector3f( 0,-1,-phi) << Eigen::Vector3f( 0, 1,-phi)
                 << Eigen::Vector3f( phi, 0,-1) << Eigen::Vector3f( phi, 0, 1)
                 << Eigen::Vector3f(-phi, 0,-1) << Eigen::Vector3f(-phi, 0, 1);
            for (auto& v : icoV) v.normalize();

            QVector<Eigen::Vector3i> icoT;
            icoT.reserve(80);
            icoT << Eigen::Vector3i(0,11,5)  << Eigen::Vector3i(0,5,1)
                 << Eigen::Vector3i(0,1,7)   << Eigen::Vector3i(0,7,10)
                 << Eigen::Vector3i(0,10,11) << Eigen::Vector3i(1,5,9)
                 << Eigen::Vector3i(5,11,4)  << Eigen::Vector3i(11,10,2)
                 << Eigen::Vector3i(10,7,6)  << Eigen::Vector3i(7,1,8)
                 << Eigen::Vector3i(3,9,4)   << Eigen::Vector3i(3,4,2)
                 << Eigen::Vector3i(3,2,6)   << Eigen::Vector3i(3,6,8)
                 << Eigen::Vector3i(3,8,9)   << Eigen::Vector3i(4,9,5)
                 << Eigen::Vector3i(2,4,11)  << Eigen::Vector3i(6,2,10)
                 << Eigen::Vector3i(8,6,7)   << Eigen::Vector3i(9,8,1);

            // Subdivide once: split each triangle edge at midpoint → 42 verts, 80 tris
            QMap<QPair<int,int>, int> midpointCache;
            auto getMidpoint = [&](int a, int b) -> int {
                auto key = qMakePair(qMin(a,b), qMax(a,b));
                if (midpointCache.contains(key)) return midpointCache[key];
                Eigen::Vector3f mid = (icoV[a] + icoV[b]).normalized();
                int idx = icoV.size();
                icoV.append(mid);
                midpointCache[key] = idx;
                return idx;
            };

            QVector<Eigen::Vector3i> newTris;
            newTris.reserve(80);
            for (const auto& tri : icoT) {
                int a = getMidpoint(tri(0), tri(1));
                int b = getMidpoint(tri(1), tri(2));
                int c = getMidpoint(tri(2), tri(0));
                newTris << Eigen::Vector3i(tri(0), a, c)
                        << Eigen::Vector3i(tri(1), b, a)
                        << Eigen::Vector3i(tri(2), c, b)
                        << Eigen::Vector3i(a, b, c);
            }

            const int nVPerPt = icoV.size();   // 42
            const int nTPerPt = newTris.size(); // 80

            // Build template arrays scaled to radius
            Eigen::MatrixX3f templateV(nVPerPt, 3);
            Eigen::MatrixX3f templateN(nVPerPt, 3);
            for (int iv = 0; iv < nVPerPt; ++iv) {
                templateN(iv, 0) = icoV[iv].x();
                templateN(iv, 1) = icoV[iv].y();
                templateN(iv, 2) = icoV[iv].z();
                templateV(iv, 0) = icoV[iv].x() * r;
                templateV(iv, 1) = icoV[iv].y() * r;
                templateV(iv, 2) = icoV[iv].z() * r;
            }
            Eigen::MatrixX3i templateT(nTPerPt, 3);
            for (int it = 0; it < nTPerPt; ++it) {
                templateT(it, 0) = newTris[it](0);
                templateT(it, 1) = newTris[it](1);
                templateT(it, 2) = newTris[it](2);
            }

            // Merge all spheres into a single mesh
            Eigen::MatrixX3f allVerts(nPts * nVPerPt, 3);
            Eigen::MatrixX3f allNorms(nPts * nVPerPt, 3);
            Eigen::MatrixX3i allTris(nPts * nTPerPt, 3);

            for (int p = 0; p < nPts; ++p) {
                const QVector3D& pos = positions[p];
                int vOff = p * nVPerPt;
                int tOff = p * nTPerPt;
                for (int iv = 0; iv < nVPerPt; ++iv) {
                    allVerts(vOff + iv, 0) = templateV(iv, 0) + pos.x();
                    allVerts(vOff + iv, 1) = templateV(iv, 1) + pos.y();
                    allVerts(vOff + iv, 2) = templateV(iv, 2) + pos.z();
                    allNorms.row(vOff + iv) = templateN.row(iv);
                }
                for (int it = 0; it < nTPerPt; ++it) {
                    allTris(tOff + it, 0) = templateT(it, 0) + vOff;
                    allTris(tOff + it, 1) = templateT(it, 1) + vOff;
                    allTris(tOff + it, 2) = templateT(it, 2) + vOff;
                }
            }

            auto brainSurf = std::make_shared<BrainSurface>();
            brainSurf->createFromData(allVerts, allNorms, allTris, color);
            brainSurf->setVisible(srcItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;

            QString key = "srcsp_" + srcItem->text();
            m_surfaces[key] = brainSurf;
            qDebug() << "BrainView: Created batched source space mesh" << key
                     << "with" << nPts << "points," << allVerts.rows() << "vertices,"
                     << allTris.rows() << "triangles";
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
    
    // Reposition hemispheres for inflated surfaces
    // Inflated surfaces in FreeSurfer are centered at origin for each hemisphere,
    // so they overlap when rendered together. We need to separate them side-by-side.
    bool isInflated = (type == "inflated");
    
    QString lhKey = "lh_" + type;
    QString rhKey = "rh_" + type;
    
    if (isInflated && m_surfaces.contains(lhKey) && m_surfaces.contains(rhKey)) {
        auto lhSurf = m_surfaces[lhKey];
        auto rhSurf = m_surfaces[rhKey];
        
        // First reset to original positions
        QMatrix4x4 identity;
        lhSurf->applyTransform(identity);
        rhSurf->applyTransform(identity);
        
        // Get bounding boxes after reset
        float lhMaxX = lhSurf->maxX();
        float rhMinX = rhSurf->minX();
        
        // Small gap between hemispheres (5mm)
        const float gap = 0.005f;
        
        // Translate left hemisphere so its right max is at -gap/2
        // Translate right hemisphere so its left min is at +gap/2
        // This centers the zero point between both hemispheres
        float lhOffset = -gap/2.0f - lhMaxX;
        float rhOffset = gap/2.0f - rhMinX;
        
        lhSurf->translateX(lhOffset);
        rhSurf->translateX(rhOffset);
    } else {
        // For non-inflated surfaces, reset to original positions
        // This removes any translations applied for inflated view
        QMatrix4x4 identity;
        if (m_surfaces.contains(lhKey)) {
            m_surfaces[lhKey]->applyTransform(identity);
        }
        if (m_surfaces.contains(rhKey)) {
            m_surfaces[rhKey]->applyTransform(identity);
        }
    }
    
    update();
}

//=============================================================================================================

void BrainView::setShaderMode(const QString &modeName)
{
    if (modeName == "Standard") m_brainShaderMode = BrainRenderer::Standard;
    else if (modeName == "Holographic") m_brainShaderMode = BrainRenderer::Holographic;
    else if (modeName == "Anatomical") m_brainShaderMode = BrainRenderer::Anatomical;
    update();
}

void BrainView::setBemShaderMode(const QString &modeName)
{
    if (modeName == "Standard") m_bemShaderMode = BrainRenderer::Standard;
    else if (modeName == "Holographic") m_bemShaderMode = BrainRenderer::Holographic;
    else if (modeName == "Anatomical") m_bemShaderMode = BrainRenderer::Anatomical;
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
    if (modeName == "Source Estimate") mode = BrainSurface::ModeSourceEstimate;
    
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

void BrainView::showSingleView()
{
    m_viewMode = SingleView;
    update();
}

//=============================================================================================================

void BrainView::showMultiView()
{
    m_viewMode = MultiView;
    update();
}

//=============================================================================================================

void BrainView::setViewportEnabled(int index, bool enabled)
{
    if (index >= 0 && index < 4) {
        m_viewportEnabled[index] = enabled;
        update();
    }
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
        QString modeStr = (m_brainShaderMode == BrainRenderer::Holographic) ? "Holographic" : (m_brainShaderMode == BrainRenderer::Anatomical) ? "Anatomical" : "Standard";
        m_fpsLabel->setText(QString("FPS: %1\nVertices: %2\nShader: %3").arg(fps, 0, 'f', 1).arg(m_activeSurface->vertexCount()).arg(modeStr));
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    // Initialize renderer
    m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount());
    m_renderer->beginFrame(cb, renderTarget());
    
    // Determine viewport configuration
    QSize outputSize = renderTarget()->pixelSize();
    
    // Fixed camera offsets for multi-view (relative to user's interactive rotation)
    // Top, Bottom, Front, Left in 2x2 grid
    QVector<QQuaternion> viewOffsets = {
        QQuaternion::fromAxisAndAngle(1, 0, 0, 90),   // Top - look from +Y down
        QQuaternion::fromAxisAndAngle(1, 0, 0, -90),  // Bottom - look from -Y up
        QQuaternion(),                                  // Front - default
        QQuaternion::fromAxisAndAngle(0, 1, 0, -90)   // Left - look from -X
    };
    
    // Per-viewport shader modes for multi-view (different visualization per viewport)
    QVector<BrainRenderer::ShaderMode> viewportShaders = {
        BrainRenderer::Anatomical,   // Top - detailed anatomy view
        BrainRenderer::Standard,     // Bottom - standard solid view
        BrainRenderer::Holographic,  // Front - holographic/activity view
        BrainRenderer::Anatomical    // Left - anatomical side view
    };
    
    // Build list of enabled viewports
    QVector<int> enabledViewports;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < 4; ++i) {
            if (m_viewportEnabled[i]) {
                enabledViewports.append(i);
            }
        }
        // Fallback: if all disabled, show first one
        if (enabledViewports.isEmpty()) {
            enabledViewports.append(0);
        }
    } else {
        enabledViewports.append(0); // Single view mode
    }
    
    int numEnabled = enabledViewports.size();
    
    for (int slot = 0; slot < numEnabled; ++slot) {
        int vp = (m_viewMode == MultiView) ? enabledViewports[slot] : 0;
        
        // Calculate viewport position based on number of enabled viewports
        QRhiViewport viewport;
        float aspectRatio;
        
        if (m_viewMode == MultiView && numEnabled > 1) {
            // Dynamic layout based on number of enabled viewports
            int cols = (numEnabled <= 2) ? numEnabled : 2;  // 1-2: columns, 3-4: 2 columns
            int rows = (numEnabled <= 2) ? 1 : 2;            // 1-2: 1 row, 3-4: 2 rows
            
            int slotCol = slot % cols;
            int slotRow = slot / cols;
            
            int cellW = outputSize.width() / cols;
            int cellH = outputSize.height() / rows;
            
            viewport = QRhiViewport(slotCol * cellW, (rows - 1 - slotRow) * cellH, cellW, cellH);
            aspectRatio = float(cellW) / float(cellH);
        } else {
            viewport = QRhiViewport(0, 0, outputSize.width(), outputSize.height());
            aspectRatio = float(outputSize.width()) / float(outputSize.height());
        }
        
        // Set viewport
        cb->setViewport(viewport);
        
        // Calculate camera for this viewport
        QQuaternion effectiveRotation = m_cameraRotation;
        if (m_viewMode == MultiView) {
            effectiveRotation = viewOffsets[vp] * m_cameraRotation;
        }
        
        QMatrix4x4 projection;
        float farPlane = m_sceneSize * 20.0f;
        if (farPlane < 100.0f) farPlane = 100.0f;
        projection.perspective(45.0f, aspectRatio, m_sceneSize * 0.01f, farPlane);

        float baseDistance = m_sceneSize * 1.5f;
        float distance = baseDistance - m_zoom * (m_sceneSize * 0.05f);
        
        QVector3D cameraPos = effectiveRotation.rotatedVector(QVector3D(0, 0, distance));
        QVector3D upVector = effectiveRotation.rotatedVector(QVector3D(0, 1, 0));
        
        QMatrix4x4 view;
        view.lookAt(cameraPos, QVector3D(0, 0, 0), upVector);

        BrainRenderer::SceneData sceneData;
        sceneData.mvp = rhi()->clipSpaceCorrMatrix();
        sceneData.mvp *= projection;
        sceneData.mvp *= view;
        
        QMatrix4x4 model;
        model.translate(-m_sceneCenter);
        sceneData.mvp *= model;
        
        sceneData.cameraPos = cameraPos;
        sceneData.lightDir = cameraPos.normalized();
        sceneData.lightingEnabled = m_lightingEnabled;

    // Pass 1: Opaque Surfaces (Brain surfaces)
    // Use viewport-specific shader in multi-view mode, otherwise use global setting
    BrainRenderer::ShaderMode currentShader = (m_viewMode == MultiView) ? viewportShaders[vp] : m_brainShaderMode;
    
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("bem_")) continue;
        if (it.key().startsWith("sens_")) continue;
        if (it.key().startsWith("srcsp_")) continue;
        
        if (it.key().endsWith(m_activeSurfaceType)) {
            m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), currentShader);
        }
    }
    
    // Pass 1b: Source Space Points (use same shader as brain for consistent depth/blend)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("srcsp_")) continue;
        if (!it.value()->isVisible()) continue;
        m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), currentShader);
    }
    
    // Pass 2: Transparent Surfaces sorted Back-to-Front
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
        float d = (sceneData.cameraPos - center).lengthSquared();
        
        BrainRenderer::ShaderMode mode = BrainRenderer::Holographic;
        if (isBem) mode = m_bemShaderMode;
        
        transparentItems.append({it.value().get(), d, mode});
    }
    
    std::sort(transparentItems.begin(), transparentItems.end(), [](const RenderItem &a, const RenderItem &b) {
        return a.dist > b.dist;
    });
    
    for (const auto &item : transparentItems) {
        m_renderer->renderSurface(cb, rhi(), sceneData, item.surf, item.mode);
    }
    
    // Render Dipoles
    for(auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
        if (it.value()->isVisible() && m_dipolesVisible) {
             m_renderer->renderDipoles(cb, rhi(), sceneData, it.value().get());
        }
    }
    
    if (m_dipolesVisible && m_dipoles) {
        m_renderer->renderDipoles(cb, rhi(), sceneData, m_dipoles.get());
    }

    // Intersection Pointer
    if (m_viewMode == SingleView && m_hasIntersection && m_debugPointerSurface) {
        BrainRenderer::SceneData debugSceneData = sceneData;
        
        QMatrix4x4 translation;
        translation.translate(m_lastIntersectionPoint);
        
        QMatrix4x4 modelMat;
        modelMat.translate(-m_sceneCenter);
        debugSceneData.mvp = rhi()->clipSpaceCorrMatrix() * projection * view * modelMat * translation;
        
        m_renderer->renderSurface(cb, rhi(), debugSceneData, m_debugPointerSurface.get(), BrainRenderer::Holographic);
    }
    
    } // End of viewport loop
    
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
    // Prevent multiple simultaneous loads
    if (m_isLoadingStc) {
        qWarning() << "BrainView: STC loading already in progress";
        return false;
    }
    
    // Find surfaces for the current active type
    BrainSurface* lhSurface = nullptr;
    BrainSurface* rhSurface = nullptr;
    
    QString lhKey = "lh_" + m_activeSurfaceType;
    QString rhKey = "rh_" + m_activeSurfaceType;
    
    if (m_surfaces.contains(lhKey)) {
        lhSurface = m_surfaces[lhKey].get();
    }
    if (m_surfaces.contains(rhKey)) {
        rhSurface = m_surfaces[rhKey].get();
    }
    
    if (!lhSurface && !rhSurface) {
        qWarning() << "BrainView: No surfaces available for STC loading";
        return false;
    }
    
    // Clean up any previous loading thread
    if (m_loadingThread) {
        m_loadingThread->quit();
        m_loadingThread->wait();
        delete m_loadingThread;
        m_loadingThread = nullptr;
    }
    
    // Create overlay for results
    m_sourceOverlay = std::make_unique<SourceEstimateOverlay>();
    
    // Create worker and thread
    m_loadingThread = new QThread(this);
    m_stcWorker = new StcLoadingWorker(lhPath, rhPath, lhSurface, rhSurface);
    m_stcWorker->moveToThread(m_loadingThread);
    
    // Connect signals
    connect(m_loadingThread, &QThread::started, m_stcWorker, &StcLoadingWorker::process);
    connect(m_stcWorker, &StcLoadingWorker::progress, this, &BrainView::stcLoadingProgress);
    connect(m_stcWorker, &StcLoadingWorker::finished, this, &BrainView::onStcLoadingFinished);
    connect(m_stcWorker, &StcLoadingWorker::finished, m_loadingThread, &QThread::quit);
    connect(m_loadingThread, &QThread::finished, m_stcWorker, &QObject::deleteLater);
    
    m_isLoadingStc = true;
    
    // Start loading
    m_loadingThread->start();
    
    return true;
}

//=============================================================================================================

void BrainView::onStcLoadingFinished(bool success)
{
    m_isLoadingStc = false;
    
    if (!success || !m_stcWorker) {
        qWarning() << "BrainView: Async STC loading failed";
        m_sourceOverlay.reset();
        return;
    }
    
    // Transfer data from worker to overlay
    if (m_stcWorker->hasLh()) {
        m_sourceOverlay->setStcData(m_stcWorker->stcLh(), 0);
        if (m_stcWorker->interpolationMatLh()) {
            m_sourceOverlay->setInterpolationMatrix(m_stcWorker->interpolationMatLh(), 0);
        }
    }
    
    if (m_stcWorker->hasRh()) {
        m_sourceOverlay->setStcData(m_stcWorker->stcRh(), 1);
        if (m_stcWorker->interpolationMatRh()) {
            m_sourceOverlay->setInterpolationMatrix(m_stcWorker->interpolationMatRh(), 1);
        }
    }
    
    // Update thresholds based on data
    m_sourceOverlay->updateThresholdsFromData();
    
    if (m_sourceOverlay->isLoaded()) {
        setVisualizationMode("Source Estimate");
        emit sourceEstimateLoaded(m_sourceOverlay->numTimePoints());
        setTimePoint(0);
    } else {
        m_sourceOverlay.reset();
    }
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

              // disp3D style: gray, 10mm flat plate
              auto *item = new SensorTreeItem(ch.ch_name, pos, QColor(100, 100, 100), 0.01f);

              // Store coil orientation (ex, ey, ez columns from coil_trans)
              QMatrix4x4 orient;
              for (int r = 0; r < 3; ++r)
                  for (int c = 0; c < 3; ++c)
                      orient(r, c) = ch.coil_trans(r, c);
              // If transforming MEG to head space, also rotate the orientation
              if (hasDevHead) {
                  QMatrix4x4 devHeadRot;
                  for (int r = 0; r < 3; ++r)
                      for (int c = 0; c < 3; ++c)
                          devHeadRot(r, c) = devHeadQTrans(r, c);
                  orient = devHeadRot * orient;
              }
              item->setOrientation(orient);

              megItems.append(item);
          } else if (ch.kind == FIFFV_EEG_CH) {
              QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
              // EEG: cyan spheres, 2mm radius
              eegItems.append(new SensorTreeItem(ch.ch_name, pos, QColor(0, 200, 220), 0.002f));
          }
      }
      
      if (!megItems.isEmpty()) m_model->addSensors("MEG", megItems);
      if (!eegItems.isEmpty()) m_model->addSensors("EEG", eegItems);
  }
  
  // Try to load digitizers if available in info
  QList<QStandardItem*> digItems;
  if (hasInfo && info.dig.size() > 0) {
      int hpiCount = 0, eegCount = 0, extraCount = 0;
      for (const auto &p : info.dig) {
        QVector3D pos(p.r[0], p.r[1], p.r[2]);
        QColor col(255, 0, 255);  // Default: magenta (extra/head shape points)
        float size = 0.001f;       // Default: 1mm
        QString name;
        
        if (p.kind == FIFFV_POINT_CARDINAL) {
            size = 0.002f;  // 2mm for cardinal points
            if (p.ident == FIFFV_POINT_NASION)       { col = QColor(0, 255, 0); name = "Nasion"; }
            else if (p.ident == FIFFV_POINT_LPA)     { col = QColor(255, 0, 0); name = "LPA"; }
            else if (p.ident == FIFFV_POINT_RPA)     { col = QColor(0, 0, 255); name = "RPA"; }
            else                                     { col = QColor(0, 255, 0); name = QString("Cardinal %1").arg(p.ident); }
        }
        else if (p.kind == FIFFV_POINT_HPI)    { col = QColor(128, 0, 0); size = 0.001f; name = QString("HPI %1").arg(++hpiCount); }
        else if (p.kind == FIFFV_POINT_EEG)    { col = QColor(0, 255, 255); size = 0.001f; name = QString("EEG %1").arg(++eegCount); }
        else if (p.kind == FIFFV_POINT_EXTRA)  { col = QColor(255, 0, 255); size = 0.001f; name = QString("Head Shape %1").arg(++extraCount); }
        else                                   { name = QString("Point %1").arg(digItems.size() + 1); }
        
        digItems.append(new SensorTreeItem(name, pos, col, size));
      }
      m_model->addSensors("Digitizer", digItems);
  } else if (!hasInfo) { // If no info, try to load as dig set
      file.reset(); // Reset to read as dig set
      digSet = FiffDigPointSet(file);
      if (digSet.size() > 0) {
          int hpiCount = 0, eegCount = 0, extraCount = 0;
          for(int i=0; i<digSet.size(); ++i) {
              const auto &p = digSet[i];
              QVector3D pos(p.r[0], p.r[1], p.r[2]);
              QColor col(255, 0, 255);  // Default: magenta
              float size = 0.001f;
              QString name;

              if (p.kind == FIFFV_POINT_CARDINAL) {
                  size = 0.002f;
                  if (p.ident == FIFFV_POINT_NASION)       { col = QColor(0, 255, 0); name = "Nasion"; }
                  else if (p.ident == FIFFV_POINT_LPA)     { col = QColor(255, 0, 0); name = "LPA"; }
                  else if (p.ident == FIFFV_POINT_RPA)     { col = QColor(0, 0, 255); name = "RPA"; }
                  else                                     { col = QColor(0, 255, 0); name = QString("Cardinal %1").arg(p.ident); }
              }
              else if (p.kind == FIFFV_POINT_HPI)    { col = QColor(128, 0, 0); size = 0.001f; name = QString("HPI %1").arg(++hpiCount); }
              else if (p.kind == FIFFV_POINT_EEG)    { col = QColor(0, 255, 255); size = 0.001f; name = QString("EEG %1").arg(++eegCount); }
              else if (p.kind == FIFFV_POINT_EXTRA)  { col = QColor(255, 0, 255); size = 0.001f; name = QString("Head Shape %1").arg(++extraCount); }
              else                                   { name = QString("Point %1").arg(i + 1); }
              
              digItems.append(new SensorTreeItem(name, pos, col, size));
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

bool BrainView::loadSourceSpace(const QString &fwdPath)
{
    QFile file(fwdPath);
    if (!file.exists()) {
        qWarning() << "BrainView: Source space file not found:" << fwdPath;
        return false;
    }

    MNELIB::MNESourceSpace srcSpace;
    FIFFLIB::FiffStream::SPtr stream(new FIFFLIB::FiffStream(&file));
    if (!stream->open()) {
        qWarning() << "BrainView: Failed to open FIF stream for source space";
        return false;
    }

    if (!MNELIB::MNESourceSpace::readFromStream(stream, true, srcSpace)) {
        qWarning() << "BrainView: Failed to read source space from" << fwdPath;
        return false;
    }

    if (srcSpace.isEmpty()) {
        qWarning() << "BrainView: Source space is empty";
        return false;
    }

    qDebug() << "BrainView: Loaded source space with" << srcSpace.size() << "hemispheres";
    for (int h = 0; h < srcSpace.size(); ++h) {
        qDebug() << "  Hemi" << h << ": nuse =" << srcSpace[h].nuse << "np =" << srcSpace[h].np;
    }

    m_model->addSourceSpace(srcSpace);
    return true;
}

//=============================================================================================================

void BrainView::setSourceSpaceVisible(bool visible)
{
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("srcsp_")) {
            it.value()->setVisible(visible);
        }
    }
    update();
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
    bool invertible;
    QMatrix4x4 invPVM = pvm.inverted(&invertible);
    if (!invertible) return;
    
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
    
    m_hasIntersection = false;
    
    float closestDist = std::numeric_limits<float>::max();
    QStandardItem* hitItem = nullptr;
    QString hitInfo;
    
    int hitIndex = -1;
    
    // Check Surfaces (Sensors, Hemisphere, BEM)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.value()->isVisible()) continue;
        if (it.key().startsWith("srcsp_")) continue;
        
        bool isSensor = it.key().startsWith("sens_");
        bool isBem = it.key().startsWith("bem_");
        
        // Brain surfaces: only pick if matching active surface type (same as render)
        if (!isSensor && !isBem) {
            if (!it.key().endsWith(m_activeSurfaceType)) continue;
        }
        
        float dist;
        int vertexIdx = -1;
        if (it.value()->intersects(rayOrigin, rayDir, dist, vertexIdx)) {
             if (dist < closestDist) {
                 closestDist = dist;
                 m_hasIntersection = true;
                 m_lastIntersectionPoint = rayOrigin + dist * rayDir;

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

    // Build display label: show contextual info depending on what was hit
    QString displayLabel;
    if (!currentRegion.isEmpty()) {
        // Brain surface with annotation
        displayLabel = QString("Region: %1").arg(currentRegion);
    } else if (!hitInfo.isEmpty()) {
        // Determine what type of object was hit from the surface key
        QString hitKey;
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (hitItem && m_itemSurfaceMap.contains(hitItem) && m_itemSurfaceMap[hitItem] == it.value()) {
                hitKey = it.key();
                break;
            }
        }
        // If no key found via item map, try hitInfo directly
        if (hitKey.isEmpty()) hitKey = hitInfo;

        if (hitKey.startsWith("sens_meg_")) {
            displayLabel = QString("MEG: %1").arg(hitInfo);
        } else if (hitKey.startsWith("sens_eeg_")) {
            displayLabel = QString("EEG: %1").arg(hitInfo);
        } else if (hitKey.startsWith("sens_dig_")) {
            displayLabel = QString("Digitizer: %1").arg(hitInfo);
        } else if (hitKey.startsWith("bem_")) {
            QString compartment = hitKey.mid(4); // strip "bem_"
            // Capitalize first letter
            if (!compartment.isEmpty()) compartment[0] = compartment[0].toUpper();
            compartment.replace("_", " ");
            displayLabel = QString("BEM: %1").arg(compartment);
        } else if (hitInfo.contains("Dipole")) {
            displayLabel = hitInfo;
        }
    }

    if (displayLabel != m_hoveredRegion) {
        m_hoveredRegion = displayLabel;
        emit hoveredRegionChanged(m_hoveredRegion);
        if (m_regionLabel) {
            if (m_hoveredRegion.isEmpty()) {
                m_regionLabel->hide();
            } else {
                m_regionLabel->setText(m_hoveredRegion);
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
