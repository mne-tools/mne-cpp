//=============================================================================================================
/**
 * @file     brainview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
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
#include <QFrame>
#include <memory>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFile>
#include <QSettings>
#include <QCoreApplication>
#include <QMenu>
#include <mne/mne_surface.h>
#include <mne/mne_bem.h>
#include <algorithm>
#include <fs/surfaceset.h>
#include <fs/surface.h>
#include <fiff/fiff.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include <fwd/fwd_coil_set.h>
#include <inverse/dipoleFit/ecd_set.h>
#include <disp/plots/helpers/colormap.h>

#include <inverse/dipoleFit/ecd_set.h>
#include "sensortreeitem.h"
#include "dipoletreeitem.h"
#include "sourcespacetreeitem.h"
#include "digitizertreeitem.h"
#include "digitizersettreeitem.h"
#include "../workers/stcloadingworker.h"
#include "../helpers/geometryinfo.h"
#include "../helpers/interpolation.h"
#include "../helpers/field_map.h"

using namespace FSLIB;
using namespace FIFFLIB;
using namespace MNELIB;

namespace
{
QQuaternion perspectivePresetRotation()
{
    return QQuaternion::fromEulerAngles(-45.0f, -40.0f, -130.0f);
}


QString multiViewPresetName(int preset)
{
    switch (preset) {
    case 0: return QStringLiteral("Top");
    case 1: return QStringLiteral("Perspective");
    case 2: return QStringLiteral("Front");
    case 3: return QStringLiteral("Left");
    case 4: return QStringLiteral("Bottom");
    case 5: return QStringLiteral("Back");
    case 6: return QStringLiteral("Right");
    default: return QStringLiteral("Top");
    }
}

QQuaternion multiViewPresetOffset(int preset)
{
    switch (preset) {
    case 0:
        return QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 1:
        return perspectivePresetRotation();
    case 2:
        return QQuaternion::fromAxisAndAngle(0, 1, 0, 180)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180)
            * QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 3:
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 1, 0, -90);
    case 4: // Bottom – opposite of Top
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 180)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 5: // Back – camera behind the head at -Y, up = +Z
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90);
    case 6: // Right – opposite of Left
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 1, 0, 90);
    default:
        return QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    }
}

bool multiViewPresetIsPerspective(int preset)
{
    return preset == 1;
}
int normalizedVisualizationTarget(int target)
{
    return std::clamp(target, -1, 3);
}

BrainRenderer::ShaderMode shaderModeFromName(const QString& modeName)
{
    if (modeName == "Holographic") {
        return BrainRenderer::Holographic;
    }
    if (modeName == "Anatomical") {
        return BrainRenderer::Anatomical;
    }
    return BrainRenderer::Standard;
}

QString shaderModeName(BrainRenderer::ShaderMode mode)
{
    if (mode == BrainRenderer::Holographic) {
        return "Holographic";
    }
    if (mode == BrainRenderer::Anatomical) {
        return "Anatomical";
    }
    return "Standard";
}

BrainSurface::VisualizationMode visualizationModeFromName(const QString& modeName)
{
    if (modeName == "Annotation") {
        return BrainSurface::ModeAnnotation;
    }
    if (modeName == "Scientific") {
        return BrainSurface::ModeScientific;
    }
    if (modeName == "Source Estimate") {
        return BrainSurface::ModeSourceEstimate;
    }
    return BrainSurface::ModeSurface;
}

QString visualizationModeName(BrainSurface::VisualizationMode mode)
{
    if (mode == BrainSurface::ModeAnnotation) {
        return "Annotation";
    }
    if (mode == BrainSurface::ModeScientific) {
        return "Scientific";
    }
    if (mode == BrainSurface::ModeSourceEstimate) {
        return "Source Estimate";
    }
    return "Surface";
}

bool isTrue(const QVariant &value, bool fallback)
{
    return value.isValid() ? value.toBool() : fallback;
}

std::unique_ptr<FiffCoordTransOld> toOldTransform(const FiffCoordTrans& trans)
{
    if (trans.isEmpty()) {
        return nullptr;
    }

    auto old = std::make_unique<FiffCoordTransOld>();
    old->from = trans.from;
    old->to = trans.to;
    old->rot = trans.trans.block<3, 3>(0, 0);
    old->move = trans.trans.block<3, 1>(0, 3);
    FiffCoordTransOld::add_inverse(old.get());
    return old;
}

Eigen::Vector3f applyOldTransform(const Eigen::Vector3f& point, const FiffCoordTransOld* trans)
{
    if (!trans) {
        return point;
    }

    float r[3] = {point.x(), point.y(), point.z()};
    FiffCoordTransOld::fiff_coord_trans(r, trans, FIFFV_MOVE);
    return Eigen::Vector3f(r[0], r[1], r[2]);
}

/**
 * MNE analyze colormap: teal → blue → gray → red → yellow.
 *
 * Port of mne_analyze_colormap(format='vtk') from MNE-Python mne/viz/utils.py.
 * Input v is normalised to [0,1] where 0.5 corresponds to zero field.
 *
 * Control points (x mapped to [-1,1]):
 *   x = -1.00  →  (0, 1, 1)  teal          (full negative)
 *   x = -0.90  →  (0, 0, 1)  blue          (hot end)
 *   x = -0.30  →  (0.5, 0.5, 0.5)  gray    (gradual blue→gray)
 *   x =  0.00  →  (0.5, 0.5, 0.5)  gray    (zero — wide gray band)
 *   x = +0.30  →  (0.5, 0.5, 0.5)  gray    (gradual gray→red)
 *   x = +0.90  →  (1, 0, 0)  red            (hot end)
 *   x = +1.00  →  (1, 1, 0)  yellow         (full positive)
 */
QRgb mneAnalyzeColor(double v)
{
    // v in [0,1], map to x in [-1,1]
    double x = 2.0 * v - 1.0;
    x = std::clamp(x, -1.0, 1.0);

    // 7 control points: wide gray center, gradual blue→gray and gray→red
    static constexpr int N = 7;
    static const double pos[N] = { -1.0, -0.90, -0.30, 0.0, 0.30, 0.90, 1.0 };
    static const double rr[N]  = {  0.0,  0.0,   0.5,  0.5, 0.5,  1.0,  1.0 };
    static const double gg[N]  = {  1.0,  0.0,   0.5,  0.5, 0.5,  0.0,  1.0 };
    static const double bb[N]  = {  1.0,  1.0,   0.5,  0.5, 0.5,  0.0,  0.0 };

    // Find the segment
    int seg = 0;
    for (int i = 0; i < N - 1; ++i) {
        if (x >= pos[i] && x <= pos[i + 1]) {
            seg = i;
            break;
        }
    }
    if (x > pos[N - 1]) seg = N - 2;

    double t = (pos[seg + 1] != pos[seg])
               ? (x - pos[seg]) / (pos[seg + 1] - pos[seg])
               : 0.0;
    t = std::clamp(t, 0.0, 1.0);

    int r = static_cast<int>(std::round((rr[seg] + t * (rr[seg + 1] - rr[seg])) * 255.0));
    int g = static_cast<int>(std::round((gg[seg] + t * (gg[seg + 1] - gg[seg])) * 255.0));
    int b = static_cast<int>(std::round((bb[seg] + t * (bb[seg + 1] - bb[seg])) * 255.0));

    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);

    return qRgb(r, g, b);
}
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

BrainView::BrainView(QWidget *parent)
    : QRhiWidget(parent)
{
    setMinimumSize(800, 600);
    setSampleCount(1);

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    setApi(Api::Metal);
#elif defined(Q_OS_WIN)
    setApi(Api::Direct3D11);
#else
    setApi(Api::OpenGL);
#endif

    setMouseTracking(true); // Enable hover events
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&BrainView::update));
    m_updateTimer->start(16); // ~60 FPS update

    m_fpsLabel = new QLabel(this);
    m_fpsLabel->setStyleSheet("color: white; font-weight: bold; font-family: monospace; font-size: 13px; background: transparent; padding: 5px;");
    m_fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    m_fpsLabel->resize(220, 80);
    m_fpsLabel->move(width() - m_fpsLabel->width() - 10, 10);
    m_fpsLabel->raise();

    m_fpsTimer.start();

    m_regionLabel = new QLabel(this);
    m_regionLabel->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 16px; background: transparent; padding: 5px;");
    m_regionLabel->setText("");
    m_regionLabel->move(10, 10); 
    m_regionLabel->resize(300, 30);
    m_regionLabel->hide();

    const QStringList viewportNames = {"Top", "Perspective", "Front", "Left"};
    for (int i = 0; i < 4; ++i) {
        m_viewportNameLabels[i] = new QLabel(this);
        m_viewportNameLabels[i]->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 12px; background: transparent; padding: 2px 4px;");
        m_viewportNameLabels[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_viewportNameLabels[i]->setText(viewportNames[i]);
        m_viewportNameLabels[i]->adjustSize();
        m_viewportNameLabels[i]->hide();

        m_viewportInfoLabels[i] = new QLabel(this);
        m_viewportInfoLabels[i]->setStyleSheet("color: white; font-family: monospace; font-size: 10px; background: rgba(0,0,0,110); border-radius: 3px; padding: 2px 4px;");
        m_viewportInfoLabels[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_viewportInfoLabels[i]->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_viewportInfoLabels[i]->setText("");
        m_viewportInfoLabels[i]->adjustSize();
        m_viewportInfoLabels[i]->hide();
    }

    m_verticalSeparator = new QFrame(this);
    m_verticalSeparator->setFrameShape(QFrame::NoFrame);
    m_verticalSeparator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_verticalSeparator->hide();

    m_horizontalSeparator = new QFrame(this);
    m_horizontalSeparator->setFrameShape(QFrame::NoFrame);
    m_horizontalSeparator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_horizontalSeparator->hide();

    QColor sepColor = palette().color(QPalette::Midlight);
    if (sepColor.alpha() == 255) {
        sepColor.setAlpha(180);
    }
    const QString sepStyle = QString("background-color: rgba(%1,%2,%3,%4);")
                                 .arg(sepColor.red())
                                 .arg(sepColor.green())
                                 .arg(sepColor.blue())
                                 .arg(sepColor.alpha());
    m_verticalSeparator->setStyleSheet(sepStyle);
    m_horizontalSeparator->setStyleSheet(sepStyle);

    // ── Establish multi-view subview defaults (before loadMultiViewSettings) ──
    // Single view uses struct defaults (Standard shader, perspective preset 1, etc.)
    m_subViews[0].preset = 0;  // Top
    m_subViews[0].brainShader = BrainRenderer::Anatomical;
    m_subViews[1].preset = 1;  // Perspective
    m_subViews[1].brainShader = BrainRenderer::Standard;
    m_subViews[2].preset = 2;  // Front
    m_subViews[2].brainShader = BrainRenderer::Holographic;
    m_subViews[3].preset = 3;  // Left
    m_subViews[3].brainShader = BrainRenderer::Anatomical;

    loadMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();

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
    // Top view: rotate 180° around roll axis (Z)
    m_multiViewCameras[0] = QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    // Perspective view: rounded preset from user-selected orientation
    m_multiViewCameras[1] = perspectivePresetRotation();
    // Front view: current orientation plus relative +180° roll and +180° yaw
    m_multiViewCameras[2] = QQuaternion::fromAxisAndAngle(0, 1, 0, 180)
                            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180)
                            * QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
                            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    // Left view: current left orientation plus +90° pitch
    m_multiViewCameras[3] = QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
                            * QQuaternion::fromAxisAndAngle(0, 1, 0, -90);
}

BrainView::~BrainView()
{
    // Stop worker thread before destroying anything it may reference
    if (m_loadingThread) {
        m_loadingThread->quit();
        m_loadingThread->wait();
    }

    saveMultiViewSettings();
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
    saveMultiViewSettings();
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

            // Oriented barbell for MEG gradiometers (two spheres + thin rod)
            auto createOrientedBarbell = [](const QVector3D &pos, const QMatrix4x4 &orient, const QColor &color, float size) {
                auto surf = std::make_shared<BrainSurface>();

                Eigen::Matrix3f rot;
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        rot(r, c) = orient(r, c);

                const float halfSpan = size * 0.6f;
                const float sphereR = size * 0.2f;
                const float rodR = size * 0.08f;

                QVector<Eigen::Vector3f> verts;
                QVector<Eigen::Vector3f> norms;
                QVector<Eigen::Vector3i> tris;

                auto appendSphere = [&](const QVector3D &center, float radius) {
                    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
                    QVector<Eigen::Vector3f> sVerts;
                    sVerts << Eigen::Vector3f(-1, phi, 0) << Eigen::Vector3f( 1, phi, 0)
                           << Eigen::Vector3f(-1,-phi, 0) << Eigen::Vector3f( 1,-phi, 0)
                           << Eigen::Vector3f( 0,-1, phi) << Eigen::Vector3f( 0, 1, phi)
                           << Eigen::Vector3f( 0,-1,-phi) << Eigen::Vector3f( 0, 1,-phi)
                           << Eigen::Vector3f( phi, 0,-1) << Eigen::Vector3f( phi, 0, 1)
                           << Eigen::Vector3f(-phi, 0,-1) << Eigen::Vector3f(-phi, 0, 1);
                    for (auto &v : sVerts) v.normalize();

                    QVector<Eigen::Vector3i> sFaces;
                    sFaces << Eigen::Vector3i(0,11,5)  << Eigen::Vector3i(0,5,1)
                           << Eigen::Vector3i(0,1,7)   << Eigen::Vector3i(0,7,10)
                           << Eigen::Vector3i(0,10,11) << Eigen::Vector3i(1,5,9)
                           << Eigen::Vector3i(5,11,4)  << Eigen::Vector3i(11,10,2)
                           << Eigen::Vector3i(10,7,6)  << Eigen::Vector3i(7,1,8)
                           << Eigen::Vector3i(3,9,4)   << Eigen::Vector3i(3,4,2)
                           << Eigen::Vector3i(3,2,6)   << Eigen::Vector3i(3,6,8)
                           << Eigen::Vector3i(3,8,9)   << Eigen::Vector3i(4,9,5)
                           << Eigen::Vector3i(2,4,11)  << Eigen::Vector3i(6,2,10)
                           << Eigen::Vector3i(8,6,7)   << Eigen::Vector3i(9,8,1);

                    QMap<QPair<int,int>, int> cache;
                    auto mid = [&](int a, int b) -> int {
                        auto key = qMakePair(qMin(a,b), qMax(a,b));
                        if (cache.contains(key)) return cache[key];
                        int idx = sVerts.size();
                        sVerts.append((sVerts[a] + sVerts[b]).normalized());
                        cache[key] = idx;
                        return idx;
                    };
                    QVector<Eigen::Vector3i> newFaces;
                    newFaces.reserve(80);
                    for (const auto &f : sFaces) {
                        int ab = mid(f(0), f(1)), bc = mid(f(1), f(2)), ca = mid(f(2), f(0));
                        newFaces << Eigen::Vector3i(f(0), ab, ca) << Eigen::Vector3i(f(1), bc, ab)
                                 << Eigen::Vector3i(f(2), ca, bc) << Eigen::Vector3i(ab, bc, ca);
                    }

                    const int base = verts.size();
                    verts.reserve(base + sVerts.size());
                    norms.reserve(base + sVerts.size());
                    tris.reserve(tris.size() + newFaces.size());

                    QVector3D c = center;
                    for (const auto &v : sVerts) {
                        Eigen::Vector3f vn = rot * v;
                        Eigen::Vector3f vp = rot * (v * radius);
                        verts.append(Eigen::Vector3f(vp.x() + c.x(), vp.y() + c.y(), vp.z() + c.z()));
                        norms.append(vn.normalized());
                    }
                    for (const auto &f : newFaces) {
                        tris.append(Eigen::Vector3i(base + f(0), base + f(1), base + f(2)));
                    }
                };

                auto appendRod = [&]() {
                    float hx = rodR;
                    float hy = halfSpan;
                    float hz = rodR;
                    Eigen::Vector3f corners[8] = {
                        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
                        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz}
                    };
                    for (auto &c : corners) c = rot * c;

                    int faces[6][4] = {
                        {4,5,6,7}, {1,0,3,2},
                        {3,7,6,2}, {0,1,5,4},
                        {1,2,6,5}, {0,4,7,3}
                    };

                    const int base = verts.size();
                    verts.reserve(base + 24);
                    norms.reserve(base + 24);
                    tris.reserve(tris.size() + 12);

                    for (int f = 0; f < 6; ++f) {
                        Eigen::Vector3f v0 = corners[faces[f][0]];
                        Eigen::Vector3f v1 = corners[faces[f][1]];
                        Eigen::Vector3f v2 = corners[faces[f][2]];
                        Eigen::Vector3f fn = (v1 - v0).cross(v2 - v0).normalized();
                        int faceBase = base + f * 4;
                        for (int k = 0; k < 4; ++k) {
                            Eigen::Vector3f v = corners[faces[f][k]];
                            verts.append(Eigen::Vector3f(v.x() + pos.x(), v.y() + pos.y(), v.z() + pos.z()));
                            norms.append(fn);
                        }
                        tris.append(Eigen::Vector3i(faceBase, faceBase + 1, faceBase + 2));
                        tris.append(Eigen::Vector3i(faceBase, faceBase + 2, faceBase + 3));
                    }
                };

                QVector3D axis = QVector3D(rot(0,1), rot(1,1), rot(2,1));
                appendSphere(pos + axis * halfSpan, sphereR);
                appendSphere(pos - axis * halfSpan, sphereR);
                appendRod();

                Eigen::MatrixX3f rr(verts.size(), 3), nn(norms.size(), 3);
                Eigen::MatrixX3i tt(tris.size(), 3);
                for (int i = 0; i < verts.size(); ++i) {
                    rr(i, 0) = verts[i].x();
                    rr(i, 1) = verts[i].y();
                    rr(i, 2) = verts[i].z();
                    nn(i, 0) = norms[i].x();
                    nn(i, 1) = norms[i].y();
                    nn(i, 2) = norms[i].z();
                }
                for (int i = 0; i < tris.size(); ++i) {
                    tt.row(i) = tris[i];
                }

                surf->createFromData(rr, nn, tt, color);
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

              if (parentText.contains("MEG/Grad") && sensItem->hasOrientation()) {
                  brainSurf = createOrientedBarbell(sensItem->position(), sensItem->orientation(),
                                             sensItem->color(), sensItem->scale());
              } else if (parentText.contains("MEG/Mag") && sensItem->hasOrientation()) {
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
            if (parentText.contains("MEG/Grad")) keyPrefix = "sens_meg_grad_";
            else if (parentText.contains("MEG/Mag")) keyPrefix = "sens_meg_mag_";
            else if (parentText.contains("MEG")) keyPrefix = "sens_meg_";
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

        // Handle Digitizer Items (batched sphere mesh per category, same technique as Source Space)
        if (absItem && absItem->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType) {
            DigitizerTreeItem* digItem = static_cast<DigitizerTreeItem*>(absItem);
            const QVector<QVector3D>& positions = digItem->positions();
            if (positions.isEmpty()) continue;

            const float r = digItem->scale();
            const QColor color = digItem->color();
            const int nPts = positions.size();

            // Build subdivided icosahedron template (42 verts, 80 tris)
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

            QMap<QPair<int,int>, int> midCache;
            auto getMid = [&](int a, int b) -> int {
                auto key = qMakePair(qMin(a,b), qMax(a,b));
                if (midCache.contains(key)) return midCache[key];
                Eigen::Vector3f mid = (icoV[a] + icoV[b]).normalized();
                int idx = icoV.size();
                icoV.append(mid);
                midCache[key] = idx;
                return idx;
            };

            QVector<Eigen::Vector3i> newTris;
            newTris.reserve(80);
            for (const auto& tri : icoT) {
                int a = getMid(tri(0), tri(1));
                int b = getMid(tri(1), tri(2));
                int c = getMid(tri(2), tri(0));
                newTris << Eigen::Vector3i(tri(0), a, c)
                        << Eigen::Vector3i(tri(1), b, a)
                        << Eigen::Vector3i(tri(2), c, b)
                        << Eigen::Vector3i(a, b, c);
            }

            const int nVPerPt = icoV.size();
            const int nTPerPt = newTris.size();

            Eigen::MatrixX3f templateV(nVPerPt, 3), templateN(nVPerPt, 3);
            for (int iv = 0; iv < nVPerPt; ++iv) {
                templateN(iv, 0) = icoV[iv].x();
                templateN(iv, 1) = icoV[iv].y();
                templateN(iv, 2) = icoV[iv].z();
                templateV(iv, 0) = icoV[iv].x() * r;
                templateV(iv, 1) = icoV[iv].y() * r;
                templateV(iv, 2) = icoV[iv].z() * r;
            }
            Eigen::MatrixX3i templateT(nTPerPt, 3);
            for (int it = 0; it < nTPerPt; ++it)
                templateT.row(it) << newTris[it](0), newTris[it](1), newTris[it](2);

            // Merge all spheres into a single batched mesh
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
            brainSurf->setVisible(digItem->isVisible());

            // Apply Head-to-MRI transformation if available (same pattern as sensors:
            // always call applyTransform so m_originalVertexData is established;
            // identity matrix when transform is off)
            if (!m_headToMriTrans.isEmpty()) {
                QMatrix4x4 m;
                if (m_applySensorTrans) {
                    for(int rr=0; rr<4; ++rr) for(int cc=0; cc<4; ++cc) m(rr,cc) = m_headToMriTrans.trans(rr,cc);
                }
                brainSurf->applyTransform(m);
            }

            m_itemSurfaceMap[item] = brainSurf;

            // Category name for legacy map key
            QString catName;
            switch (digItem->pointKind()) {
                case DigitizerTreeItem::Cardinal: catName = "cardinal"; break;
                case DigitizerTreeItem::HPI:      catName = "hpi"; break;
                case DigitizerTreeItem::EEG:      catName = "eeg"; break;
                case DigitizerTreeItem::Extra:    catName = "extra"; break;
            }
            QString key = "dig_" + catName;
            m_surfaces[key] = brainSurf;
            qDebug() << "BrainView: Created batched digitizer mesh" << key
                     << "with" << nPts << "points," << allVerts.rows() << "vertices";
        }

        
        // Check children recursively
        if (m_model->hasChildren(index)) {
            onRowsInserted(index, 0, m_model->rowCount(index) - 1);
        }
    }
    updateInflatedSurfaceTransforms();
    updateSceneBounds();
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
    updateSceneBounds();
    update();
}

//=============================================================================================================




//=============================================================================================================



void BrainView::setActiveSurface(const QString &type)
{
    subViewForTarget(m_visualizationEditTarget).surfaceType = type;

    m_activeSurfaceType = type;
    
    // Update m_activeSurface pointer to one of the matching surfaces for stats/helpers
    QString key = "lh_" + type;
    if (m_surfaces.contains(key)) m_activeSurface = m_surfaces[key];
    else {
        key = "rh_" + type;
        if (m_surfaces.contains(key)) m_activeSurface = m_surfaces[key];
    }
    
    updateInflatedSurfaceTransforms();
    saveMultiViewSettings();
    
    updateSceneBounds();
    update();
}

void BrainView::updateSceneBounds()
{
    QVector3D min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    QVector3D max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
    bool hasContent = false;
    
    // Iterate over all surfaces
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.value()->isVisible()) {
            QVector3D sMin, sMax;
            it.value()->boundingBox(sMin, sMax);
            
            min.setX(std::min(min.x(), sMin.x()));
            min.setY(std::min(min.y(), sMin.y()));
            min.setZ(std::min(min.z(), sMin.z()));
            
            max.setX(std::max(max.x(), sMax.x()));
            max.setY(std::max(max.y(), sMax.y()));
            max.setZ(std::max(max.z(), sMax.z()));
            hasContent = true;
        }
    }
    
    // Iterate over all dipoles
    for (auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
        if (it.value()->isVisible()) {
            // Dipoles don't have a bounding box method in DipoleObject yet, 
            // but we can approximate or skip for now. 
            // Ideally DipoleObject should expose bounds. 
            // For now, let's assume surfaces dictate the scene size usually.
        }
    }
    
    if (hasContent) {
        m_sceneCenter = (min + max) * 0.5f;
        
        QVector3D diag = max - min;
        m_sceneSize = std::max(diag.x(), std::max(diag.y(), diag.z()));
        
        // Ensure non-zero size
        if (m_sceneSize < 0.01f) m_sceneSize = 0.3f;
        
    } else {
        // Default
        m_sceneCenter = QVector3D(0,0,0);
        m_sceneSize = 0.3f;
    }
}

//=============================================================================================================

void BrainView::setShaderMode(const QString &modeName)
{
    const BrainRenderer::ShaderMode mode = shaderModeFromName(modeName);
    subViewForTarget(m_visualizationEditTarget).brainShader = mode;

    m_brainShaderMode = mode;
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::setVisualizationEditTarget(int target)
{
    const int prev = m_visualizationEditTarget;
    m_visualizationEditTarget = normalizedVisualizationTarget(target);

    const SubView &sv = subViewForTarget(m_visualizationEditTarget);
    m_activeSurfaceType = sv.surfaceType;
    m_brainShaderMode   = sv.brainShader;
    m_bemShaderMode     = sv.bemShader;
    m_currentVisMode    = sv.overlayMode;
    const ViewVisibilityProfile &visibility = sv.visibility;

    const bool remapMegSurface = (m_megFieldMapOnHead != visibility.megFieldMapOnHead);
    m_megFieldMapOnHead = visibility.megFieldMapOnHead;
    m_dipolesVisible = visibility.dipoles;

    for (auto surf : m_surfaces) {
        surf->setVisualizationMode(m_currentVisMode);
    }

    if (m_sensorFieldLoaded) {
        if (remapMegSurface) {
            buildSensorFieldMapping();
        }
        applySensorFieldMap();
    }

    // Update viewport label highlighting
    updateViewportLabelHighlight();

    saveMultiViewSettings();

    if (prev != m_visualizationEditTarget) {
        emit visualizationEditTargetChanged(m_visualizationEditTarget);
    }
}

//=============================================================================================================

int BrainView::visualizationEditTarget() const
{
    return m_visualizationEditTarget;
}

//=============================================================================================================

QString BrainView::activeSurfaceForTarget(int target) const
{
    return subViewForTarget(target).surfaceType;
}

//=============================================================================================================

QString BrainView::shaderModeForTarget(int target) const
{
    return shaderModeName(subViewForTarget(target).brainShader);
}

//=============================================================================================================

QString BrainView::bemShaderModeForTarget(int target) const
{
    return shaderModeName(subViewForTarget(target).bemShader);
}

//=============================================================================================================

QString BrainView::overlayModeForTarget(int target) const
{
    return visualizationModeName(subViewForTarget(target).overlayMode);
}

//=============================================================================================================

bool BrainView::objectVisibleFromProfile(const ViewVisibilityProfile &profile, const QString &object)
{
    if (object == "lh") return profile.lh;
    if (object == "rh") return profile.rh;
    if (object == "bem_head") return profile.bemHead;
    if (object == "bem_outer_skull") return profile.bemOuterSkull;
    if (object == "bem_inner_skull") return profile.bemInnerSkull;
    if (object == "sens_meg") return profile.sensMeg;
    if (object == "sens_meg_grad") return profile.sensMegGrad;
    if (object == "sens_meg_mag") return profile.sensMegMag;
    if (object == "sens_meg_helmet") return profile.sensMegHelmet;
    if (object == "sens_eeg") return profile.sensEeg;
    if (object == "dig") return profile.dig;
    if (object == "dig_cardinal") return profile.digCardinal;
    if (object == "dig_hpi") return profile.digHpi;
    if (object == "dig_eeg") return profile.digEeg;
    if (object == "dig_extra") return profile.digExtra;
    if (object == "field_meg") return profile.megFieldMap;
    if (object == "field_eeg") return profile.eegFieldMap;
    if (object == "contour_meg") return profile.megFieldContours;
    if (object == "contour_eeg") return profile.eegFieldContours;
    if (object == "dipoles") return profile.dipoles;
    if (object == "source_space") return profile.sourceSpace;
    return true;
}

//=============================================================================================================

void BrainView::setObjectVisibleInProfile(ViewVisibilityProfile &profile, const QString &object, bool visible)
{
    if (object == "lh") profile.lh = visible;
    else if (object == "rh") profile.rh = visible;
    else if (object == "bem_head") profile.bemHead = visible;
    else if (object == "bem_outer_skull") profile.bemOuterSkull = visible;
    else if (object == "bem_inner_skull") profile.bemInnerSkull = visible;
    else if (object == "sens_meg") profile.sensMeg = visible;
    else if (object == "sens_meg_grad") profile.sensMegGrad = visible;
    else if (object == "sens_meg_mag") profile.sensMegMag = visible;
    else if (object == "sens_meg_helmet") profile.sensMegHelmet = visible;
    else if (object == "sens_eeg") profile.sensEeg = visible;
    else if (object == "dig") profile.dig = visible;
    else if (object == "dig_cardinal") profile.digCardinal = visible;
    else if (object == "dig_hpi") profile.digHpi = visible;
    else if (object == "dig_eeg") profile.digEeg = visible;
    else if (object == "dig_extra") profile.digExtra = visible;
    else if (object == "field_meg") profile.megFieldMap = visible;
    else if (object == "field_eeg") profile.eegFieldMap = visible;
    else if (object == "contour_meg") profile.megFieldContours = visible;
    else if (object == "contour_eeg") profile.eegFieldContours = visible;
    else if (object == "dipoles") profile.dipoles = visible;
    else if (object == "source_space") profile.sourceSpace = visible;
}

//=============================================================================================================

BrainView::ViewVisibilityProfile& BrainView::visibilityProfileForTarget(int target)
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

const BrainView::ViewVisibilityProfile& BrainView::visibilityProfileForTarget(int target) const
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

BrainView::SubView& BrainView::subViewForTarget(int target)
{
    const int normalized = normalizedVisualizationTarget(target);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

const BrainView::SubView& BrainView::subViewForTarget(int target) const
{
    const int normalized = normalizedVisualizationTarget(target);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

bool BrainView::shouldRenderSurfaceForView(const QString &key, const ViewVisibilityProfile &profile) const
{
    if (key.startsWith("lh_")) return profile.lh;
    if (key.startsWith("rh_")) return profile.rh;

    if (key == "bem_head") return profile.bemHead;
    if (key == "bem_outer_skull") return profile.bemOuterSkull;
    if (key == "bem_inner_skull") return profile.bemInnerSkull;

    if (key.startsWith("sens_contour_meg")) return profile.megFieldMap && profile.megFieldContours;
    if (key.startsWith("sens_contour_eeg")) return profile.eegFieldMap && profile.eegFieldContours;
    if (key.startsWith("sens_surface_meg")) return profile.sensMeg && profile.sensMegHelmet;
    if (key.startsWith("sens_meg_grad_")) return profile.sensMeg && profile.sensMegGrad;
    if (key.startsWith("sens_meg_mag_")) return profile.sensMeg && profile.sensMegMag;
    if (key.startsWith("sens_meg_")) return profile.sensMeg;
    if (key.startsWith("sens_eeg_")) return profile.sensEeg;

    if (key.startsWith("dig_cardinal")) return profile.dig && profile.digCardinal;
    if (key.startsWith("dig_hpi")) return profile.dig && profile.digHpi;
    if (key.startsWith("dig_eeg")) return profile.dig && profile.digEeg;
    if (key.startsWith("dig_extra")) return profile.dig && profile.digExtra;
    if (key.startsWith("dig_")) return profile.dig;

    if (key.startsWith("srcsp_")) return profile.sourceSpace;

    return true;
}

//=============================================================================================================

bool BrainView::objectVisibleForTarget(const QString &object, int target) const
{
    return objectVisibleFromProfile(visibilityProfileForTarget(target), object);
}

//=============================================================================================================

bool BrainView::megFieldMapOnHeadForTarget(int target) const
{
    return visibilityProfileForTarget(target).megFieldMapOnHead;
}

//=============================================================================================================

void BrainView::updateInflatedSurfaceTransforms()
{
    const bool needsInflated = (m_singleView.surfaceType == "inflated")
                               || (m_subViews[0].surfaceType == "inflated")
                               || (m_subViews[1].surfaceType == "inflated")
                               || (m_subViews[2].surfaceType == "inflated")
                               || (m_subViews[3].surfaceType == "inflated");

    const QString lhKey = "lh_inflated";
    const QString rhKey = "rh_inflated";

    if (!m_surfaces.contains(lhKey) || !m_surfaces.contains(rhKey)) {
        return;
    }

    auto lhSurf = m_surfaces[lhKey];
    auto rhSurf = m_surfaces[rhKey];

    QMatrix4x4 identity;
    lhSurf->applyTransform(identity);
    rhSurf->applyTransform(identity);

    if (!needsInflated) {
        return;
    }

    const float lhMaxX = lhSurf->maxX();
    const float rhMinX = rhSurf->minX();

    const float gap = 0.005f;
    const float lhOffset = -gap / 2.0f - lhMaxX;
    const float rhOffset = gap / 2.0f - rhMinX;

    lhSurf->translateX(lhOffset);
    rhSurf->translateX(rhOffset);
}

void BrainView::setBemShaderMode(const QString &modeName)
{
    BrainRenderer::ShaderMode mode = BrainRenderer::Standard;
    if (modeName == "Holographic") mode = BrainRenderer::Holographic;
    else if (modeName == "Anatomical") mode = BrainRenderer::Anatomical;

    subViewForTarget(m_visualizationEditTarget).bemShader = mode;

    m_bemShaderMode = mode;
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::syncBemShadersToBrainShaders()
{
    m_singleView.bemShader = m_singleView.brainShader;
    for (int i = 0; i < 4; ++i) {
        m_subViews[i].bemShader = m_subViews[i].brainShader;
    }

    m_bemShaderMode = subViewForTarget(m_visualizationEditTarget).bemShader;

    saveMultiViewSettings();
    update();
}

void BrainView::setSensorVisible(const QString &type, bool visible)
{
    QString object;
    if (type == "MEG") object = "sens_meg";
    else if (type == "MEG/Grad") object = "sens_meg_grad";
    else if (type == "MEG/Mag") object = "sens_meg_mag";
    else if (type == "MEG Helmet") object = "sens_meg_helmet";
    else if (type == "EEG") object = "sens_eeg";
    else if (type == "Digitizer") object = "dig";
    else if (type == "Digitizer/Cardinal") object = "dig_cardinal";
    else if (type == "Digitizer/HPI") object = "dig_hpi";
    else if (type == "Digitizer/EEG") object = "dig_eeg";
    else if (type == "Digitizer/Extra") object = "dig_extra";
    else return;

    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    setObjectVisibleInProfile(profile, object, visible);
    saveMultiViewSettings();
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

//=============================================================================================================

void BrainView::setMegHelmetOverride(const QString &path)
{
    m_megHelmetOverridePath = path;
}

void BrainView::setDipoleVisible(bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.dipoles = visible;
    m_dipolesVisible = visible;
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::setVisualizationMode(const QString &modeName)
{
    const BrainSurface::VisualizationMode mode = visualizationModeFromName(modeName);
    subViewForTarget(m_visualizationEditTarget).overlayMode = mode;
    
    m_currentVisMode = mode;
    for (auto surf : m_surfaces) {
        surf->setVisualizationMode(mode);
    }
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::setHemiVisible(int hemiIdx, bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (hemiIdx == 0) {
        profile.lh = visible;
    } else if (hemiIdx == 1) {
        profile.rh = visible;
    }
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::setBemVisible(const QString &name, bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    setObjectVisibleInProfile(profile, "bem_" + name, visible);
    saveMultiViewSettings();
    update();
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
    m_isDraggingSplitter = false;
    m_activeSplitter = SplitterHit::None;
    unsetCursor();
    saveMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();
    update();
}

//=============================================================================================================

void BrainView::showMultiView()
{
    m_viewMode = MultiView;
    saveMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();
    update();
}

//=============================================================================================================

void BrainView::resetMultiViewLayout()
{
    m_multiSplitX = 0.5f;
    m_multiSplitY = 0.5f;
    saveMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();
    update();
}

bool BrainView::isViewportEnabled(int index) const
{
    if (index < 0 || index >= 4) {
        return false;
    }

    return m_subViews[index].enabled;
}

//=============================================================================================================

int BrainView::enabledViewportCount() const
{
    if (m_viewMode != MultiView) {
        return 1;
    }

    int numEnabled = 0;
    for (int i = 0; i < 4; ++i) {
        if (m_subViews[i].enabled) {
            ++numEnabled;
        }
    }

    return numEnabled > 0 ? numEnabled : 1;
}

//=============================================================================================================

int BrainView::viewportIndexAt(const QPoint& pos) const
{
    if (m_viewMode != MultiView) {
        return 0;
    }

    QVector<int> enabledViewports;
    for (int i = 0; i < 4; ++i) {
        if (m_subViews[i].enabled) {
            enabledViewports.append(i);
        }
    }
    if (enabledViewports.isEmpty()) {
        enabledViewports.append(0);
    }

    const int numEnabled = enabledViewports.size();
    for (int slot = 0; slot < numEnabled; ++slot) {
        const QRect pane = multiViewSlotRect(slot, numEnabled, size());
        if (pane.contains(pos)) {
            return enabledViewports[slot];
        }
    }

    return -1;
}

//=============================================================================================================

QRect BrainView::multiViewSlotRect(int slot, int numEnabled, const QSize& outputSize) const
{
    if (numEnabled <= 1) {
        return QRect(0, 0, outputSize.width(), outputSize.height());
    }

    const int width = outputSize.width();
    const int height = outputSize.height();

    if (numEnabled == 2) {
        const int leftW = std::clamp(static_cast<int>(std::lround(width * m_multiSplitX)),
                                     m_splitterMinPanePx,
                                     std::max(m_splitterMinPanePx, width - m_splitterMinPanePx));
        const int rightW = std::max(1, width - leftW);
        if (slot == 0) {
            return QRect(0, 0, leftW, height);
        }
        return QRect(leftW, 0, rightW, height);
    }

    const int leftW = std::clamp(static_cast<int>(std::lround(width * m_multiSplitX)),
                                 m_splitterMinPanePx,
                                 std::max(m_splitterMinPanePx, width - m_splitterMinPanePx));
    const int rightW = std::max(1, width - leftW);
    const int topH = std::clamp(static_cast<int>(std::lround(height * m_multiSplitY)),
                                m_splitterMinPanePx,
                                std::max(m_splitterMinPanePx, height - m_splitterMinPanePx));
    const int bottomH = std::max(1, height - topH);

    const int col = slot % 2;
    const int row = slot / 2;

    const int x = (col == 0) ? 0 : leftW;
    const int y = (row == 0) ? 0 : topH;
    const int w = (col == 0) ? leftW : rightW;
    const int h = (row == 0) ? topH : bottomH;

    return QRect(x, y, w, h);
}

//=============================================================================================================

BrainView::SplitterHit BrainView::hitTestSplitter(const QPoint& pos, int numEnabled, const QSize& outputSize) const
{
    if (m_viewMode != MultiView || numEnabled <= 1) {
        return SplitterHit::None;
    }

    const int width = outputSize.width();
    const int height = outputSize.height();

    if (numEnabled == 2) {
        const int splitX = std::clamp(static_cast<int>(std::lround(width * m_multiSplitX)),
                                      m_splitterMinPanePx,
                                      std::max(m_splitterMinPanePx, width - m_splitterMinPanePx));
        const bool nearVertical = std::abs(pos.x() - splitX) <= m_splitterHitTolerancePx;
        return nearVertical ? SplitterHit::Vertical : SplitterHit::None;
    }

    const int splitX = std::clamp(static_cast<int>(std::lround(width * m_multiSplitX)),
                                  m_splitterMinPanePx,
                                  std::max(m_splitterMinPanePx, width - m_splitterMinPanePx));
    const int splitY = std::clamp(static_cast<int>(std::lround(height * m_multiSplitY)),
                                  m_splitterMinPanePx,
                                  std::max(m_splitterMinPanePx, height - m_splitterMinPanePx));

    const bool nearVertical = std::abs(pos.x() - splitX) <= m_splitterHitTolerancePx;
    const bool nearHorizontal = std::abs(pos.y() - splitY) <= m_splitterHitTolerancePx;

    if (nearVertical && nearHorizontal) {
        return SplitterHit::Both;
    }
    if (nearVertical) {
        return SplitterHit::Vertical;
    }
    if (nearHorizontal) {
        return SplitterHit::Horizontal;
    }

    return SplitterHit::None;
}

//=============================================================================================================

void BrainView::updateSplitterCursor(const QPoint& pos)
{
    const SplitterHit hit = hitTestSplitter(pos, enabledViewportCount(), size());

    switch (hit) {
        case SplitterHit::Vertical:
            setCursor(Qt::SizeHorCursor);
            break;
        case SplitterHit::Horizontal:
            setCursor(Qt::SizeVerCursor);
            break;
        case SplitterHit::Both:
            setCursor(Qt::SizeAllCursor);
            break;
        case SplitterHit::None:
        default:
            unsetCursor();
            break;
    }
}

//=============================================================================================================

void BrainView::updateViewportSeparators()
{
    if (!m_verticalSeparator || !m_horizontalSeparator) {
        return;
    }

    m_verticalSeparator->hide();
    m_horizontalSeparator->hide();

    const int numEnabled = enabledViewportCount();
    if (m_viewMode != MultiView || numEnabled <= 1) {
        return;
    }

    const int widthPx = std::max(1, width());
    const int heightPx = std::max(1, height());

    const int splitX = std::clamp(static_cast<int>(std::lround(widthPx * m_multiSplitX)),
                                  m_splitterMinPanePx,
                                  std::max(m_splitterMinPanePx, widthPx - m_splitterMinPanePx));

    m_verticalSeparator->setGeometry(splitX - m_separatorLinePx / 2,
                                     0,
                                     m_separatorLinePx,
                                     heightPx);
    m_verticalSeparator->show();
    m_verticalSeparator->raise();

    if (numEnabled >= 3) {
        const int splitY = std::clamp(static_cast<int>(std::lround(heightPx * m_multiSplitY)),
                                      m_splitterMinPanePx,
                                      std::max(m_splitterMinPanePx, heightPx - m_splitterMinPanePx));

        m_horizontalSeparator->setGeometry(0,
                                           splitY - m_separatorLinePx / 2,
                                           widthPx,
                                           m_separatorLinePx);
        m_horizontalSeparator->show();
        m_horizontalSeparator->raise();
    }

    updateOverlayLayout();
}

//=============================================================================================================

void BrainView::updateOverlayLayout()
{
    QVector<int> enabledViewports;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < 4; ++i) {
            if (m_subViews[i].enabled) {
                enabledViewports.append(i);
            }
        }
        if (enabledViewports.isEmpty()) {
            enabledViewports.append(0);
        }
    }

    if (m_fpsLabel) {
        m_fpsLabel->setVisible(m_infoPanelVisible);

        if (m_viewMode == MultiView && m_infoPanelVisible) {
            const int numEnabled = enabledViewports.size();
            const QSize overlaySize = size();
            int perspectiveSlot = -1;
            for (int slot = 0; slot < numEnabled; ++slot) {
                const int vp = enabledViewports[slot];
                const int preset = std::clamp(m_subViews[vp].preset, 0, 6);
                if (multiViewPresetIsPerspective(preset)) {
                    perspectiveSlot = slot;
                    break;
                }
            }

            if (perspectiveSlot >= 0) {
                const QRect pane = multiViewSlotRect(perspectiveSlot, numEnabled, overlaySize);
                m_fpsLabel->move(pane.x() + pane.width() - m_fpsLabel->width() - 8,
                                 pane.y() + 8);
            } else {
                m_fpsLabel->move(width() - m_fpsLabel->width() - 10, 10);
            }
        } else {
            m_fpsLabel->move(width() - m_fpsLabel->width() - 10, 10);
        }

        m_fpsLabel->raise();
    }

    if (m_regionLabel) {
        const int regionY = (m_viewMode == MultiView) ? 38 : 10;
        m_regionLabel->move(10, regionY);
        if (!m_regionLabel->text().isEmpty()) {
            m_regionLabel->raise();
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (m_viewportNameLabels[i]) {
            m_viewportNameLabels[i]->hide();
        }
        if (m_viewportInfoLabels[i]) {
            m_viewportInfoLabels[i]->hide();
        }
    }

    if (m_viewMode != MultiView) {
        return;
    }

    const int numEnabled = enabledViewports.size();
    const QSize overlaySize = size();
    for (int slot = 0; slot < numEnabled; ++slot) {
        const int vp = enabledViewports[slot];
        QLabel* label = m_viewportNameLabels[vp];
        QLabel* infoLabel = m_viewportInfoLabels[vp];
        if (!label) {
            continue;
        }

        const int preset = std::clamp(m_subViews[vp].preset, 0, 6);
        label->setText(multiViewPresetName(preset));

        const QRect pane = multiViewSlotRect(slot, numEnabled, overlaySize);
        label->adjustSize();
        label->move(pane.x() + 8, pane.y() + 8);
        label->setVisible(m_infoPanelVisible);
        label->raise();

        if (infoLabel) {
            infoLabel->adjustSize();
            infoLabel->move(pane.x() + pane.width() - infoLabel->width() - 8,
                            pane.y() + 8);
            const bool usePerspectiveFpsPanel = multiViewPresetIsPerspective(preset);
            infoLabel->setVisible(m_infoPanelVisible && !usePerspectiveFpsPanel);
            infoLabel->raise();
        }
    }

    updateViewportLabelHighlight();
}

//=============================================================================================================

void BrainView::updateViewportLabelHighlight()
{
    static const QString normalStyle =
        QStringLiteral("color: white; font-weight: bold; font-family: sans-serif; "
                       "font-size: 12px; background: transparent; padding: 2px 4px;");
    static const QString selectedStyle =
        QStringLiteral("color: #FFD54F; font-weight: bold; font-family: sans-serif; "
                       "font-size: 13px; background: rgba(255,213,79,40); "
                       "border: 1px solid #FFD54F; border-radius: 3px; padding: 2px 6px;");

    for (int i = 0; i < 4; ++i) {
        if (!m_viewportNameLabels[i]) continue;
        const bool selected = (m_viewMode == MultiView && m_visualizationEditTarget == i);
        m_viewportNameLabels[i]->setStyleSheet(selected ? selectedStyle : normalStyle);
        m_viewportNameLabels[i]->adjustSize();
    }
}

//=============================================================================================================

void BrainView::logPerspectiveRotation(const QString& context) const
{
    const QQuaternion perspectivePreset = perspectivePresetRotation();
    const QQuaternion effective = m_cameraRotation * perspectivePreset;

    qDebug() << "BrainView Perspective Rotation [" << context << "]"
             << "userQuat=" << m_cameraRotation
             << "userEuler=" << m_cameraRotation.toEulerAngles()
             << "effectiveQuat=" << effective
             << "effectiveEuler=" << effective.toEulerAngles();
}

//=============================================================================================================

void BrainView::loadMultiViewSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup("ex_brain_view/BrainView");

    m_multiSplitX = settings.value("multiSplitX", 0.5f).toFloat();
    m_multiSplitY = settings.value("multiSplitY", 0.5f).toFloat();

    const int savedViewMode = settings.value("viewMode", static_cast<int>(SingleView)).toInt();
    m_viewMode = (savedViewMode == static_cast<int>(MultiView)) ? MultiView : SingleView;

    const bool hasCameraQuat = settings.contains("cameraRotW")
                               && settings.contains("cameraRotX")
                               && settings.contains("cameraRotY")
                               && settings.contains("cameraRotZ");
    if (hasCameraQuat) {
        const float w = settings.value("cameraRotW", 1.0f).toFloat();
        const float x = settings.value("cameraRotX", 0.0f).toFloat();
        const float y = settings.value("cameraRotY", 0.0f).toFloat();
        const float z = settings.value("cameraRotZ", 0.0f).toFloat();
        m_cameraRotation = QQuaternion(w, x, y, z);
        if (m_cameraRotation.lengthSquared() <= std::numeric_limits<float>::epsilon()) {
            m_cameraRotation = QQuaternion();
        } else {
            m_cameraRotation.normalize();
        }
    }

    m_subViews[0].enabled = settings.value("viewportEnabled0", true).toBool();
    m_subViews[1].enabled = settings.value("viewportEnabled1", true).toBool();
    m_subViews[2].enabled = settings.value("viewportEnabled2", true).toBool();
    m_subViews[3].enabled = settings.value("viewportEnabled3", true).toBool();

    m_singleView.surfaceType = settings.value("singleViewSurfaceType", "pial").toString();
    for (int i = 0; i < 4; ++i)
        m_subViews[i].surfaceType = settings.value(QStringLiteral("multiSurfaceType%1").arg(i), "pial").toString();

    m_singleView.brainShader = shaderModeFromName(settings.value("singleViewShader", "Standard").toString());
    m_subViews[0].brainShader = shaderModeFromName(settings.value("multiShader0", "Anatomical").toString());
    m_subViews[1].brainShader = shaderModeFromName(settings.value("multiShader1", "Standard").toString());
    m_subViews[2].brainShader = shaderModeFromName(settings.value("multiShader2", "Holographic").toString());
    m_subViews[3].brainShader = shaderModeFromName(settings.value("multiShader3", "Anatomical").toString());

    m_singleView.bemShader = shaderModeFromName(settings.value("singleViewBemShader", "Standard").toString());
    for (int i = 0; i < 4; ++i)
        m_subViews[i].bemShader = shaderModeFromName(settings.value(QStringLiteral("multiBemShader%1").arg(i), "Standard").toString());

    m_singleView.overlayMode = visualizationModeFromName(settings.value("singleViewOverlay", "Surface").toString());
    for (int i = 0; i < 4; ++i)
        m_subViews[i].overlayMode = visualizationModeFromName(settings.value(QStringLiteral("multiOverlay%1").arg(i), "Surface").toString());

    m_visualizationEditTarget = normalizedVisualizationTarget(settings.value("visualizationEditTarget", -1).toInt());

    auto loadVisibilityProfile = [&settings](const QString &prefix, ViewVisibilityProfile &profile) {
        profile.lh = isTrue(settings.value(prefix + "lh"), profile.lh);
        profile.rh = isTrue(settings.value(prefix + "rh"), profile.rh);
        profile.bemHead = isTrue(settings.value(prefix + "bemHead"), profile.bemHead);
        profile.bemOuterSkull = isTrue(settings.value(prefix + "bemOuterSkull"), profile.bemOuterSkull);
        profile.bemInnerSkull = isTrue(settings.value(prefix + "bemInnerSkull"), profile.bemInnerSkull);
        profile.sensMeg = isTrue(settings.value(prefix + "sensMeg"), profile.sensMeg);
        profile.sensMegGrad = isTrue(settings.value(prefix + "sensMegGrad"), profile.sensMegGrad);
        profile.sensMegMag = isTrue(settings.value(prefix + "sensMegMag"), profile.sensMegMag);
        profile.sensMegHelmet = isTrue(settings.value(prefix + "sensMegHelmet"), profile.sensMegHelmet);
        profile.sensEeg = isTrue(settings.value(prefix + "sensEeg"), profile.sensEeg);
        profile.dig = isTrue(settings.value(prefix + "dig"), profile.dig);
        profile.digCardinal = isTrue(settings.value(prefix + "digCardinal"), profile.digCardinal);
        profile.digHpi = isTrue(settings.value(prefix + "digHpi"), profile.digHpi);
        profile.digEeg = isTrue(settings.value(prefix + "digEeg"), profile.digEeg);
        profile.digExtra = isTrue(settings.value(prefix + "digExtra"), profile.digExtra);
        profile.megFieldMap = isTrue(settings.value(prefix + "megFieldMap"), profile.megFieldMap);
        profile.eegFieldMap = isTrue(settings.value(prefix + "eegFieldMap"), profile.eegFieldMap);
        profile.megFieldContours = isTrue(settings.value(prefix + "megFieldContours"), profile.megFieldContours);
        profile.eegFieldContours = isTrue(settings.value(prefix + "eegFieldContours"), profile.eegFieldContours);
        profile.dipoles = isTrue(settings.value(prefix + "dipoles"), profile.dipoles);
        profile.sourceSpace = isTrue(settings.value(prefix + "sourceSpace"), profile.sourceSpace);
        profile.megFieldMapOnHead = isTrue(settings.value(prefix + "megFieldMapOnHead"), profile.megFieldMapOnHead);
    };

    loadVisibilityProfile("singleVis_", m_singleView.visibility);
    for (int i = 0; i < 4; ++i) {
        loadVisibilityProfile(QStringLiteral("multiVis%1_").arg(i), m_subViews[i].visibility);
    }

    for (int i = 0; i < 4; ++i) {
        m_subViews[i].zoom = settings.value(QStringLiteral("multiViewZoom%1").arg(i), 0.0f).toFloat();
        m_subViews[i].pan = QVector2D(
            settings.value(QStringLiteral("multiViewPanX%1").arg(i), 0.0f).toFloat(),
            settings.value(QStringLiteral("multiViewPanY%1").arg(i), 0.0f).toFloat());
        m_subViews[i].preset = std::clamp(
            settings.value(QStringLiteral("multiViewPreset%1").arg(i), i).toInt(),
            0,
            6);

        const bool hasPaneQuat = settings.contains(QStringLiteral("multiViewPerspectiveRotW%1").arg(i))
                                 && settings.contains(QStringLiteral("multiViewPerspectiveRotX%1").arg(i))
                                 && settings.contains(QStringLiteral("multiViewPerspectiveRotY%1").arg(i))
                                 && settings.contains(QStringLiteral("multiViewPerspectiveRotZ%1").arg(i));
        if (hasPaneQuat) {
            const float w = settings.value(QStringLiteral("multiViewPerspectiveRotW%1").arg(i), 1.0f).toFloat();
            const float x = settings.value(QStringLiteral("multiViewPerspectiveRotX%1").arg(i), 0.0f).toFloat();
            const float y = settings.value(QStringLiteral("multiViewPerspectiveRotY%1").arg(i), 0.0f).toFloat();
            const float z = settings.value(QStringLiteral("multiViewPerspectiveRotZ%1").arg(i), 0.0f).toFloat();
            m_subViews[i].perspectiveRotation = QQuaternion(w, x, y, z);
            if (m_subViews[i].perspectiveRotation.lengthSquared() <= std::numeric_limits<float>::epsilon()) {
                m_subViews[i].perspectiveRotation = QQuaternion();
            } else {
                m_subViews[i].perspectiveRotation.normalize();
            }
        } else {
            m_subViews[i].perspectiveRotation = m_cameraRotation;
        }
    }

    settings.endGroup();

    m_multiSplitX = std::clamp(m_multiSplitX, 0.15f, 0.85f);
    m_multiSplitY = std::clamp(m_multiSplitY, 0.15f, 0.85f);

    setVisualizationEditTarget(m_visualizationEditTarget);
}

//=============================================================================================================

void BrainView::saveMultiViewSettings() const
{
    QSettings settings("MNECPP");
    settings.beginGroup("ex_brain_view/BrainView");
    settings.setValue("multiSplitX", m_multiSplitX);
    settings.setValue("multiSplitY", m_multiSplitY);
    settings.setValue("viewMode", static_cast<int>(m_viewMode));
    settings.setValue("cameraRotW", m_cameraRotation.scalar());
    settings.setValue("cameraRotX", m_cameraRotation.x());
    settings.setValue("cameraRotY", m_cameraRotation.y());
    settings.setValue("cameraRotZ", m_cameraRotation.z());
    settings.setValue("viewportEnabled0", m_subViews[0].enabled);
    settings.setValue("viewportEnabled1", m_subViews[1].enabled);
    settings.setValue("viewportEnabled2", m_subViews[2].enabled);
    settings.setValue("viewportEnabled3", m_subViews[3].enabled);
    settings.setValue("singleViewSurfaceType", m_singleView.surfaceType);
    for (int i = 0; i < 4; ++i)
        settings.setValue(QStringLiteral("multiSurfaceType%1").arg(i), m_subViews[i].surfaceType);
    settings.setValue("singleViewShader", shaderModeName(m_singleView.brainShader));
    for (int i = 0; i < 4; ++i)
        settings.setValue(QStringLiteral("multiShader%1").arg(i), shaderModeName(m_subViews[i].brainShader));
    settings.setValue("singleViewBemShader", shaderModeName(m_singleView.bemShader));
    for (int i = 0; i < 4; ++i)
        settings.setValue(QStringLiteral("multiBemShader%1").arg(i), shaderModeName(m_subViews[i].bemShader));
    settings.setValue("singleViewOverlay", visualizationModeName(m_singleView.overlayMode));
    for (int i = 0; i < 4; ++i)
        settings.setValue(QStringLiteral("multiOverlay%1").arg(i), visualizationModeName(m_subViews[i].overlayMode));
    settings.setValue("visualizationEditTarget", m_visualizationEditTarget);

    auto saveVisibilityProfile = [&settings](const QString &prefix, const ViewVisibilityProfile &profile) {
        settings.setValue(prefix + "lh", profile.lh);
        settings.setValue(prefix + "rh", profile.rh);
        settings.setValue(prefix + "bemHead", profile.bemHead);
        settings.setValue(prefix + "bemOuterSkull", profile.bemOuterSkull);
        settings.setValue(prefix + "bemInnerSkull", profile.bemInnerSkull);
        settings.setValue(prefix + "sensMeg", profile.sensMeg);
        settings.setValue(prefix + "sensMegGrad", profile.sensMegGrad);
        settings.setValue(prefix + "sensMegMag", profile.sensMegMag);
        settings.setValue(prefix + "sensMegHelmet", profile.sensMegHelmet);
        settings.setValue(prefix + "sensEeg", profile.sensEeg);
        settings.setValue(prefix + "dig", profile.dig);
        settings.setValue(prefix + "digCardinal", profile.digCardinal);
        settings.setValue(prefix + "digHpi", profile.digHpi);
        settings.setValue(prefix + "digEeg", profile.digEeg);
        settings.setValue(prefix + "digExtra", profile.digExtra);
        settings.setValue(prefix + "megFieldMap", profile.megFieldMap);
        settings.setValue(prefix + "eegFieldMap", profile.eegFieldMap);
        settings.setValue(prefix + "megFieldContours", profile.megFieldContours);
        settings.setValue(prefix + "eegFieldContours", profile.eegFieldContours);
        settings.setValue(prefix + "dipoles", profile.dipoles);
        settings.setValue(prefix + "sourceSpace", profile.sourceSpace);
        settings.setValue(prefix + "megFieldMapOnHead", profile.megFieldMapOnHead);
    };

    saveVisibilityProfile("singleVis_", m_singleView.visibility);
    for (int i = 0; i < 4; ++i) {
        saveVisibilityProfile(QStringLiteral("multiVis%1_").arg(i), m_subViews[i].visibility);
    }

    for (int i = 0; i < 4; ++i) {
        settings.setValue(QStringLiteral("multiViewZoom%1").arg(i), m_subViews[i].zoom);
        settings.setValue(QStringLiteral("multiViewPanX%1").arg(i), m_subViews[i].pan.x());
        settings.setValue(QStringLiteral("multiViewPanY%1").arg(i), m_subViews[i].pan.y());
        settings.setValue(QStringLiteral("multiViewPreset%1").arg(i), m_subViews[i].preset);
        settings.setValue(QStringLiteral("multiViewPerspectiveRotW%1").arg(i), m_subViews[i].perspectiveRotation.scalar());
        settings.setValue(QStringLiteral("multiViewPerspectiveRotX%1").arg(i), m_subViews[i].perspectiveRotation.x());
        settings.setValue(QStringLiteral("multiViewPerspectiveRotY%1").arg(i), m_subViews[i].perspectiveRotation.y());
        settings.setValue(QStringLiteral("multiViewPerspectiveRotZ%1").arg(i), m_subViews[i].perspectiveRotation.z());
    }
    settings.endGroup();
}

//=============================================================================================================

void BrainView::setViewportEnabled(int index, bool enabled)
{
    if (index >= 0 && index < 4) {
        m_subViews[index].enabled = enabled;
        saveMultiViewSettings();
        updateViewportSeparators();
        updateOverlayLayout();
        update();
    }
}

//=============================================================================================================

void BrainView::setInfoPanelVisible(bool visible)
{
    m_infoPanelVisible = visible;
    updateOverlayLayout();
}

//=============================================================================================================

void BrainView::resizeEvent(QResizeEvent *event)
{
    QRhiWidget::resizeEvent(event);
    updateViewportSeparators();
    updateOverlayLayout();
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
    // Check if there is anything to render
    bool hasSurfaces = !m_surfaces.isEmpty();
    bool hasDipoles = !m_itemDipoleMap.isEmpty() || m_dipoles; // Check managed dipoles too
    
    // If absolutely nothing is loaded, render black background
    if (!hasSurfaces && !hasDipoles) {
        // No surface loaded: render a black background instead of leaving the widget uninitialized
        if (!m_renderer) {
            m_renderer = std::make_unique<BrainRenderer>();
        }
        m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount());
        m_renderer->beginFrame(cb, renderTarget());
        m_renderer->endFrame(cb);
        return;
    }
    
    // Ensure active surface pointer is valid if possible, otherwise just use first available for stats
    if (!m_activeSurface && !m_surfaces.isEmpty()) {
        m_activeSurface = m_surfaces.begin().value();
    }


    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 500) {
        float fps = m_frameCount / (m_fpsTimer.elapsed() / 1000.0f);
        const BrainRenderer::ShaderMode fpsShaderMode = (m_viewMode == MultiView)
            ? m_subViews[1].brainShader
            : m_singleView.brainShader;
        QString modeStr = (fpsShaderMode == BrainRenderer::Holographic) ? "Holographic" : (fpsShaderMode == BrainRenderer::Anatomical) ? "Anatomical" : "Standard";
        int vCount = m_activeSurface ? m_activeSurface->vertexCount() : 0;
        m_fpsLabel->setText(QString("FPS: %1\nVertices: %2\nShader: %3").arg(fps, 0, 'f', 1).arg(vCount).arg(modeStr));
        updateOverlayLayout();
        m_fpsLabel->raise();
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    // Initialize renderer
    m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount());
    m_renderer->beginFrame(cb, renderTarget());
    
    // Determine viewport configuration
    QSize outputSize = renderTarget()->pixelSize();
    
    // Build list of enabled viewports
    QVector<int> enabledViewports;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < 4; ++i) {
            if (m_subViews[i].enabled) {
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
        const SubView &sv = (m_viewMode == MultiView) ? m_subViews[vp] : m_singleView;
        const int preset = (m_viewMode == MultiView) ? std::clamp(sv.preset, 0, 6) : 1;
        const ViewVisibilityProfile &visibility = sv.visibility;

        const BrainSurface::VisualizationMode desiredVisMode = sv.overlayMode;
        if (m_currentVisMode != desiredVisMode) {
            m_currentVisMode = desiredVisMode;
            for (auto surf : m_surfaces) {
                surf->setVisualizationMode(m_currentVisMode);
            }
        }
        
        const QRect paneRect = (m_viewMode == MultiView)
            ? multiViewSlotRect(slot, numEnabled, outputSize)
            : QRect(0, 0, outputSize.width(), outputSize.height());

        QRect renderRect = paneRect;
        if (m_viewMode == MultiView && numEnabled > 1) {
            constexpr int separatorPx = 2;

            if (numEnabled == 2) {
                if (slot == 0) {
                    renderRect.setWidth(std::max(1, renderRect.width() - separatorPx));
                }
            } else {
                const int col = slot % 2;
                const int row = slot / 2;

                const bool hasRightNeighbor = (col == 0)
                                              && (slot + 1 < numEnabled)
                                              && ((slot / 2) == ((slot + 1) / 2));
                const bool hasBottomNeighbor = (row == 0)
                                               && (slot + 2 < numEnabled);

                if (hasRightNeighbor) {
                    renderRect.setWidth(std::max(1, renderRect.width() - separatorPx));
                }
                if (hasBottomNeighbor) {
                    renderRect.setHeight(std::max(1, renderRect.height() - separatorPx));
                }
            }
        }

        const int viewX = renderRect.x();
        const int viewY = outputSize.height() - (renderRect.y() + renderRect.height());
        const int viewW = std::max(1, renderRect.width());
        const int viewH = std::max(1, renderRect.height());

        QRhiViewport viewport(viewX, viewY, viewW, viewH);
        const float aspectRatio = float(viewW) / float(viewH);
        
        // Set viewport
        cb->setViewport(viewport);
        
        // Calculate camera for this viewport
        const QQuaternion perspectivePreset = perspectivePresetRotation();
        QQuaternion effectiveRotation = m_cameraRotation * perspectivePreset;
        if (m_viewMode == MultiView) {
            const QQuaternion presetOffset = multiViewPresetOffset(preset);
            const QQuaternion panePerspectiveRotation = sv.perspectiveRotation;
            effectiveRotation = multiViewPresetIsPerspective(preset) ? (panePerspectiveRotation * presetOffset)
                                                                     : presetOffset;
        }
        
        QMatrix4x4 projection;
        float farPlane = m_sceneSize * 20.0f;
        if (farPlane < 100.0f) farPlane = 100.0f;
        projection.perspective(45.0f, aspectRatio, m_sceneSize * 0.01f, farPlane);

        // Per-viewport zoom: use per-view zoom in multiview, global zoom in single view
        const float vpZoom = (m_viewMode == MultiView) ? sv.zoom : m_zoom;
        float baseDistance = m_sceneSize * 1.5f;
        float distance = baseDistance - vpZoom * (m_sceneSize * 0.05f);
        
        QVector3D cameraPos = effectiveRotation.rotatedVector(QVector3D(0, 0, distance));
        QVector3D upVector = effectiveRotation.rotatedVector(QVector3D(0, 1, 0));

        // Per-viewport pan: shift look-at target in the view plane
        QVector3D lookAt(0, 0, 0);
        if (m_viewMode == MultiView && !multiViewPresetIsPerspective(preset)) {
            const QVector2D &pan = sv.pan;
            const QVector3D right = effectiveRotation.rotatedVector(QVector3D(1, 0, 0)).normalized();
            const QVector3D up = upVector.normalized();
            lookAt += right * pan.x() + up * pan.y();
            cameraPos += right * pan.x() + up * pan.y();
        }
        
        QMatrix4x4 view;
        view.lookAt(cameraPos, lookAt, upVector);

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
    // Use viewport-specific shader from subview
    BrainRenderer::ShaderMode currentShader = sv.brainShader;
    BrainRenderer::ShaderMode currentBemShader = sv.bemShader;
    const QString activeSurfaceType = sv.surfaceType;
    const QString overlayName = visualizationModeName(sv.overlayMode);

    if (m_viewMode == MultiView && m_viewportInfoLabels[vp]) {
        m_viewportInfoLabels[vp]->setText(
            QString("Shader: %1\nSurface: %2\nOverlay: %3")
                .arg(shaderModeName(currentShader), activeSurfaceType, overlayName));
    }
    
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it.key().startsWith("bem_")) continue;
        if (it.key().startsWith("sens_")) continue;
        if (it.key().startsWith("srcsp_")) continue;
        if (it.key().startsWith("dig_")) continue;
        if (!shouldRenderSurfaceForView(it.key(), visibility)) continue;
        
        if (it.key().endsWith(activeSurfaceType)) {
            m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), currentShader);
        }
    }
    
    // Pass 1b: Source Space Points (use same shader as brain for consistent depth/blend)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("srcsp_")) continue;
        if (!shouldRenderSurfaceForView(it.key(), visibility)) continue;
        if (!it.value()->isVisible()) continue;
        m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), currentShader);
    }

    // Pass 1c: Digitizer Points (opaque small spheres, render like source space)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("dig_")) continue;
        if (!shouldRenderSurfaceForView(it.key(), visibility)) continue;
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
        if (!shouldRenderSurfaceForView(it.key(), visibility)) continue;
        if (!it.value()->isVisible()) continue;

        QVector3D min, max;
        it.value()->boundingBox(min, max);
        QVector3D center = (min + max) * 0.5f;
        float d = (sceneData.cameraPos - center).lengthSquared();
        
        BrainRenderer::ShaderMode mode = BrainRenderer::Holographic;
        if (isBem) mode = currentBemShader;
        
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
        if (it.value()->isVisible() && visibility.dipoles) {
             m_renderer->renderDipoles(cb, rhi(), sceneData, it.value().get());
        }
    }
    
    if (visibility.dipoles && m_dipoles) {
        m_renderer->renderDipoles(cb, rhi(), sceneData, m_dipoles.get());
    }

    // Intersection Pointer
    if (m_hasIntersection && m_debugPointerSurface) {
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
    if (e->button() == Qt::LeftButton) {
        m_perspectiveRotatedSincePress = false;
    }

    if (e->button() == Qt::LeftButton && m_viewMode == MultiView) {
        const int clickedVp = viewportIndexAt(e->pos());
        if (clickedVp >= 0 && m_viewportNameLabels[clickedVp] && m_viewportNameLabels[clickedVp]->isVisible()) {
            if (m_viewportNameLabels[clickedVp]->geometry().contains(e->pos())) {
                showViewportPresetMenu(clickedVp, mapToGlobal(e->pos()));
                m_lastMousePos = e->pos();
                return;
            }
        }

        const int numEnabled = enabledViewportCount();
        const SplitterHit hit = hitTestSplitter(e->pos(), numEnabled, size());
        if (hit != SplitterHit::None) {
            m_isDraggingSplitter = true;
            m_activeSplitter = hit;
            m_lastMousePos = e->pos();
            updateSplitterCursor(e->pos());
            return;
        }

        // Select the clicked viewport as the active edit target
        const int clickedVpForSelection = viewportIndexAt(e->pos());
        if (clickedVpForSelection >= 0 && clickedVpForSelection != m_visualizationEditTarget) {
            setVisualizationEditTarget(clickedVpForSelection);
        }
    }

    m_lastMousePos = e->pos();
}

//=============================================================================================================

void BrainView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDraggingSplitter && (event->buttons() & Qt::LeftButton)) {
        const int widthPx = std::max(1, width());
        const int heightPx = std::max(1, height());

        if (m_activeSplitter == SplitterHit::Vertical || m_activeSplitter == SplitterHit::Both) {
            const int minX = m_splitterMinPanePx;
            const int maxX = std::max(minX, widthPx - m_splitterMinPanePx);
            const int clampedX = std::clamp(event->pos().x(), minX, maxX);
            m_multiSplitX = float(clampedX) / float(widthPx);
        }

        if (m_activeSplitter == SplitterHit::Horizontal || m_activeSplitter == SplitterHit::Both) {
            const int minY = m_splitterMinPanePx;
            const int maxY = std::max(minY, heightPx - m_splitterMinPanePx);
            const int clampedY = std::clamp(event->pos().y(), minY, maxY);
            m_multiSplitY = float(clampedY) / float(heightPx);
        }

        m_lastMousePos = event->pos();
        updateViewportSeparators();
        update();
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        if (m_viewMode == MultiView) {
            const int activeVp = viewportIndexAt(event->pos());
            const int activePreset = (activeVp >= 0 && activeVp < 4)
                ? std::clamp(m_subViews[activeVp].preset, 0, 6)
                : 1;

            if (activeVp >= 0 && !multiViewPresetIsPerspective(activePreset)) {
                // Planar views (Top/Front/Left): pan along the view plane
                const QPoint diff = event->pos() - m_lastMousePos;
                const float panSpeed = m_sceneSize * 0.002f;
                m_subViews[activeVp].pan += QVector2D(-diff.x() * panSpeed,
                                                       diff.y() * panSpeed);
                m_lastMousePos = event->pos();
                update();
                return;
            }

            if (activeVp >= 0 && multiViewPresetIsPerspective(activePreset)) {
                // Perspective view: rotate
                QPoint diff = event->pos() - m_lastMousePos;
                float speed = 0.5f;

                const QQuaternion perspectivePreset = perspectivePresetRotation();
                QQuaternion effectiveRotation = m_subViews[activeVp].perspectiveRotation * perspectivePreset;

                const QVector3D upAxis = effectiveRotation.rotatedVector(QVector3D(0, 1, 0)).normalized();
                const QVector3D rightAxis = effectiveRotation.rotatedVector(QVector3D(1, 0, 0)).normalized();

                QQuaternion yaw = QQuaternion::fromAxisAndAngle(upAxis, -diff.x() * speed);
                QQuaternion pitch = QQuaternion::fromAxisAndAngle(rightAxis, -diff.y() * speed);

                effectiveRotation = yaw * pitch * effectiveRotation;
                m_subViews[activeVp].perspectiveRotation = effectiveRotation * perspectivePreset.conjugated();
                m_subViews[activeVp].perspectiveRotation.normalize();

                m_perspectiveRotatedSincePress = true;
                m_lastMousePos = event->pos();
                update();
                return;
            }

            m_lastMousePos = event->pos();
            return;
        }

        // Single-view rotation
        QPoint diff = event->pos() - m_lastMousePos;
        float speed = 0.5f;

        const QQuaternion perspectivePreset = perspectivePresetRotation();
        QQuaternion effectiveRotation = m_cameraRotation * perspectivePreset;

        const QVector3D upAxis = effectiveRotation.rotatedVector(QVector3D(0, 1, 0)).normalized();
        const QVector3D rightAxis = effectiveRotation.rotatedVector(QVector3D(1, 0, 0)).normalized();

        QQuaternion yaw = QQuaternion::fromAxisAndAngle(upAxis, -diff.x() * speed);
        QQuaternion pitch = QQuaternion::fromAxisAndAngle(rightAxis, -diff.y() * speed);

        effectiveRotation = yaw * pitch * effectiveRotation;
        m_cameraRotation = effectiveRotation * perspectivePreset.conjugated();
        m_cameraRotation.normalize();

        m_lastMousePos = event->pos();
        update();
    } else {
        if (m_viewMode == MultiView) {
            updateSplitterCursor(event->pos());
        } else {
            unsetCursor();
        }
        castRay(event->pos());
    }
}

//=============================================================================================================

void BrainView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isDraggingSplitter) {
        m_isDraggingSplitter = false;
        m_activeSplitter = SplitterHit::None;
        saveMultiViewSettings();
        updateSplitterCursor(event->pos());
        return;
    }

    if (event->button() == Qt::LeftButton && m_viewMode == MultiView && m_perspectiveRotatedSincePress) {
        m_perspectiveRotatedSincePress = false;
        saveMultiViewSettings();
    }

    // Save pan offset after dragging in a planar viewport
    if (event->button() == Qt::LeftButton && m_viewMode == MultiView && !m_perspectiveRotatedSincePress) {
        saveMultiViewSettings();
    }

    if (m_viewMode == MultiView) {
        updateSplitterCursor(event->pos());
    } else {
        unsetCursor();
    }
}

//=============================================================================================================

void BrainView::wheelEvent(QWheelEvent *event)
{
    const float delta = event->angleDelta().y() / 120.0f;

    if (m_viewMode == MultiView) {
        const int vp = viewportIndexAt(event->position().toPoint());
        if (vp >= 0 && vp < 4) {
            m_subViews[vp].zoom += delta;
            saveMultiViewSettings();
        }
    } else {
        m_zoom += delta;
    }
    update();
}

//=============================================================================================================

void BrainView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_S) {
        saveSnapshot();
    } else if (event->key() == Qt::Key_R) {
        m_cameraRotation = QQuaternion();
        logPerspectiveRotation("reset-initial");
        saveMultiViewSettings();
        update();
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
    
    // Fallback: if active surface type didn't match, search for any lh_*/rh_* brain surface
    if (!lhSurface || !rhSurface) {
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (it.value() && it.value()->tissueType() == BrainSurface::TissueBrain) {
                if (!lhSurface && it.key().startsWith("lh_")) {
                    lhSurface = it.value().get();
                    qDebug() << "BrainView: Using fallback LH surface:" << it.key();
                } else if (!rhSurface && it.key().startsWith("rh_")) {
                    rhSurface = it.value().get();
                    qDebug() << "BrainView: Using fallback RH surface:" << it.key();
                }
            }
        }
    }
    
    if (!lhSurface && !rhSurface) {
        qWarning() << "BrainView: No surfaces available for STC loading."
                   << "Active surface type:" << m_activeSurfaceType
                   << "Available keys:" << m_surfaces.keys();
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
    emit sourceThresholdsUpdated(m_sourceOverlay->thresholdMin(),
                                 m_sourceOverlay->thresholdMid(),
                                 m_sourceOverlay->thresholdMax());
    
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
    
    // Collect all distinct surface types used across single + multi views
    QSet<QString> activeTypes;
    activeTypes.insert(m_singleView.surfaceType);
    for (int i = 0; i < 4; ++i) {
        activeTypes.insert(m_subViews[i].surfaceType);
    }
    
    // Apply source estimate to surfaces matching ANY active type
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        for (const QString &type : activeTypes) {
            if (it.key().endsWith(type)) {
                m_sourceOverlay->applyToSurface(it.value().get(), m_currentTimePoint);
                break;
            }
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

bool BrainView::loadSensorField(const QString &evokedPath, int aveIndex)
{
    QFile file(evokedPath);
    if (!file.exists()) {
        qWarning() << "BrainView: Sensor evoked file not found:" << evokedPath;
        return false;
    }

    FIFFLIB::FiffEvoked evoked(file, aveIndex);
    if (evoked.isEmpty()) {
        qWarning() << "BrainView: Failed to read evoked data from" << evokedPath;
        return false;
    }

    m_sensorEvoked = evoked;
    m_sensorFieldLoaded = true;
    m_sensorFieldTimePoint = 0;

    if (!buildSensorFieldMapping()) {
        m_sensorFieldLoaded = false;
        return false;
    }

    emit sensorFieldLoaded(m_sensorEvoked.times.size());
    setSensorFieldTimePoint(0);
    return true;
}

//=============================================================================================================

QStringList BrainView::probeEvokedSets(const QString &evokedPath)
{
    QStringList result;
    QFile file(evokedPath);
    if (!file.exists()) {
        return result;
    }

    FIFFLIB::FiffEvokedSet evokedSet(file);
    for (int i = 0; i < evokedSet.evoked.size(); ++i) {
        const auto &ev = evokedSet.evoked.at(i);
        QString label = QString("%1: %2 (%3, nave=%4)")
            .arg(i)
            .arg(ev.comment.isEmpty() ? QStringLiteral("Set %1").arg(i) : ev.comment)
            .arg(ev.aspectKindToString())
            .arg(ev.nave);
        result.append(label);
    }
    return result;
}

//=============================================================================================================

void BrainView::setSensorFieldTimePoint(int index)
{
    if (!m_sensorFieldLoaded || m_sensorEvoked.isEmpty()) {
        return;
    }

    int maxIdx = static_cast<int>(m_sensorEvoked.times.size()) - 1;
    if (maxIdx < 0) {
        return;
    }

    m_sensorFieldTimePoint = qBound(0, index, maxIdx);
    applySensorFieldMap();
    emit sensorFieldTimePointChanged(m_sensorFieldTimePoint, m_sensorEvoked.times(m_sensorFieldTimePoint));
    update();
}

//=============================================================================================================

void BrainView::setSensorFieldVisible(const QString &type, bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (type == "MEG") {
        profile.megFieldMap = visible;
    } else if (type == "EEG") {
        profile.eegFieldMap = visible;
    } else {
        return;
    }

    saveMultiViewSettings();
    applySensorFieldMap();
    update();
}

//=============================================================================================================

void BrainView::setSensorFieldContourVisible(const QString &type, bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (type == "MEG") {
        profile.megFieldContours = visible;
    } else if (type == "EEG") {
        profile.eegFieldContours = visible;
    } else {
        return;
    }

    saveMultiViewSettings();
    applySensorFieldMap();
    update();
}

//=============================================================================================================

void BrainView::setMegFieldMapOnHead(bool useHead)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (profile.megFieldMapOnHead == useHead && m_megFieldMapOnHead == useHead) {
        return;
    }

    profile.megFieldMapOnHead = useHead;
    m_megFieldMapOnHead = useHead;
    saveMultiViewSettings();
    if (m_sensorFieldLoaded) {
        buildSensorFieldMapping();
        applySensorFieldMap();
        update();
    }
}

//=============================================================================================================

void BrainView::setSensorFieldColormap(const QString &name)
{
    if (m_sensorFieldColormap == name) {
        return;
    }
    m_sensorFieldColormap = name;
    applySensorFieldMap();
    update();
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

float BrainView::stcTmin() const
{
    if (m_sourceOverlay && m_sourceOverlay->isLoaded()) {
        return m_sourceOverlay->tmin();
    }
    return 0.0f;
}

//=============================================================================================================

int BrainView::stcNumTimePoints() const
{
    if (m_sourceOverlay && m_sourceOverlay->isLoaded()) {
        return m_sourceOverlay->numTimePoints();
    }
    return 0;
}

//=============================================================================================================

int BrainView::closestSensorFieldIndex(float timeSec) const
{
    if (!m_sensorFieldLoaded || m_sensorEvoked.nave == -1 || m_sensorEvoked.times.size() == 0) {
        return -1;
    }

    int bestIdx = 0;
    float bestDist = std::abs(m_sensorEvoked.times(0) - timeSec);
    for (int i = 1; i < m_sensorEvoked.times.size(); ++i) {
        float dist = std::abs(m_sensorEvoked.times(i) - timeSec);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

//=============================================================================================================

int BrainView::closestStcIndex(float timeSec) const
{
    if (!m_sourceOverlay || !m_sourceOverlay->isLoaded()) {
        return -1;
    }

    float tmin = m_sourceOverlay->tmin();
    float tstep = m_sourceOverlay->tstep();
    int numPts = m_sourceOverlay->numTimePoints();
    if (numPts <= 0 || tstep <= 0.0f) {
        return -1;
    }

    int idx = qRound((timeSec - tmin) / tstep);
    return qBound(0, idx, numPts - 1);
}

//=============================================================================================================

bool BrainView::sensorFieldTimeRange(float &tmin, float &tmax) const
{
    if (!m_sensorFieldLoaded || m_sensorEvoked.nave == -1 || m_sensorEvoked.times.size() == 0) {
        return false;
    }
    tmin = m_sensorEvoked.times(0);
    tmax = m_sensorEvoked.times(m_sensorEvoked.times.size() - 1);
    return true;
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
      QList<QStandardItem*> megGradItems;
      QList<QStandardItem*> megMagItems;
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

              if (ch.unit == FIFF_UNIT_T_M) {
                  megGradItems.append(item);
              } else {
                  megMagItems.append(item);
              }
          } else if (ch.kind == FIFFV_EEG_CH) {
              QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
              // EEG: cyan spheres, 2mm radius
              eegItems.append(new SensorTreeItem(ch.ch_name, pos, QColor(0, 200, 220), 0.002f));
          }
      }

      if (!megGradItems.isEmpty()) m_model->addSensors("MEG/Grad", megGradItems);
      if (!megMagItems.isEmpty()) m_model->addSensors("MEG/Mag", megMagItems);
      if (!eegItems.isEmpty()) m_model->addSensors("EEG", eegItems);

      // Load and add MEG helmet surface if MEG channels are present
      if (!megGradItems.isEmpty() || !megMagItems.isEmpty()) {
          auto pickHelmetFile = [&info]() -> QString {
              int coilType = -1;
              int nMeg = 0;
              for (const auto &ch : info.chs) {
                  if (ch.kind == FIFFV_MEG_CH) {
                      coilType = ch.chpos.coil_type & 0xFFFF;
                      ++nMeg;
                  }
              }

              QString fileName = "306m.fif";
              if (coilType == FIFFV_COIL_BABY_GRAD) {
                  fileName = "BabySQUID.fif";
              } else if (coilType == FIFFV_COIL_NM_122) {
                  fileName = "122m.fif";
              } else if (coilType == FIFFV_COIL_CTF_GRAD) {
                  fileName = "CTF_275.fif";
              } else if (coilType == FIFFV_COIL_KIT_GRAD) {
                  fileName = "KIT.fif";
              } else if (coilType == FIFFV_COIL_MAGNES_MAG || coilType == FIFFV_COIL_MAGNES_GRAD) {
                  fileName = (nMeg > 150) ? "Magnes_3600wh.fif" : "Magnes_2500wh.fif";
              } else if (coilType / 1000 == 3) {
                  fileName = "306m.fif";
              }

              return QCoreApplication::applicationDirPath()
                  + "/../resources/general/sensorSurfaces/" + fileName;
          };

          QString helmetPath;
          if (!m_megHelmetOverridePath.isEmpty()) {
              helmetPath = m_megHelmetOverridePath;
              if (!QFile::exists(helmetPath)) {
                  qWarning() << "MEG helmet override file not found:" << helmetPath
                             << "- falling back to auto selection.";
                  helmetPath.clear();
              }
          }

          if (helmetPath.isEmpty()) {
              helmetPath = pickHelmetFile();
          }

          if (!QFile::exists(helmetPath)) {
              QString fallback = QCoreApplication::applicationDirPath()
                  + "/../resources/general/sensorSurfaces/306m.fif";
              if (QFile::exists(fallback)) {
                  helmetPath = fallback;
              }
          }

          if (!QFile::exists(helmetPath)) {
              qWarning() << "MEG helmet surface file not found. Checked:" << helmetPath;
          }

          if (QFile::exists(helmetPath)) {
              QFile helmetFile(helmetPath);
              MNELIB::MNEBem helmetBem(helmetFile);
              if (helmetBem.size() > 0) {
                  MNELIB::MNEBemSurface helmetSurf = helmetBem[0];

                  if (helmetSurf.nn.rows() != helmetSurf.rr.rows()) {
                      helmetSurf.nn = FSLIB::Surface::compute_normals(helmetSurf.rr, helmetSurf.tris);
                  }

                  if (hasDevHead) {
                      QMatrix3x3 normalMat = devHeadQTrans.normalMatrix();
                      for (int i = 0; i < helmetSurf.rr.rows(); ++i) {
                          QVector3D pos(helmetSurf.rr(i, 0), helmetSurf.rr(i, 1), helmetSurf.rr(i, 2));
                          pos = devHeadQTrans.map(pos);
                          helmetSurf.rr(i, 0) = pos.x();
                          helmetSurf.rr(i, 1) = pos.y();
                          helmetSurf.rr(i, 2) = pos.z();

                          QVector3D nn(helmetSurf.nn(i, 0), helmetSurf.nn(i, 1), helmetSurf.nn(i, 2));
                          const float *d = normalMat.constData();
                          float nx = d[0] * nn.x() + d[3] * nn.y() + d[6] * nn.z();
                          float ny = d[1] * nn.x() + d[4] * nn.y() + d[7] * nn.z();
                          float nz = d[2] * nn.x() + d[5] * nn.y() + d[8] * nn.z();
                          QVector3D n = QVector3D(nx, ny, nz).normalized();
                          helmetSurf.nn(i, 0) = n.x();
                          helmetSurf.nn(i, 1) = n.y();
                          helmetSurf.nn(i, 2) = n.z();
                      }
                  }

                  auto helmetSurface = std::make_shared<BrainSurface>();
                  helmetSurface->fromBemSurface(helmetSurf, QColor(0, 0, 77, 200));
                  helmetSurface->setVisible(false);
                  m_surfaces["sens_surface_meg"] = helmetSurface;
              }
          }
      }
  }
  
  // Load digitizer points with proper categorization (Cardinal, HPI, EEG, Extra)
  bool hasDigitizer = false;
  if (hasInfo && info.dig.size() > 0) {
      m_model->addDigitizerData(info.dig);
      hasDigitizer = true;
  } else if (!hasInfo) {
      // If no info block, try to load as a standalone dig point set
      file.reset();
      digSet = FiffDigPointSet(file);
      if (digSet.size() > 0) {
          QList<FiffDigPoint> digPoints;
          for (int i = 0; i < digSet.size(); ++i) {
              digPoints.append(digSet[i]);
          }
          m_model->addDigitizerData(digPoints);
          hasDigitizer = true;
      }
  }
  
  return hasInfo || hasDigitizer;
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
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.sourceSpace = visible;
    saveMultiViewSettings();
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
        if ((it.key().startsWith("sens_") || it.key().startsWith("dig_")) && it.value()) {
            it.value()->applyTransform(qmat);
            surfCount++;
        }
    }
    // Also check item map just in case (redundant but safe)
    for (auto surf : m_itemSurfaceMap) {
         // We don't have a reliable way to check prefix here without extra logic, 
         // but m_surfaces should cover it.
    }

    if (m_sensorFieldLoaded) {
        buildSensorFieldMapping();
        applySensorFieldMap();
    }
}

//=============================================================================================================

QString BrainView::findHeadSurfaceKey() const
{
    if (m_surfaces.contains("bem_head")) {
        return "bem_head";
    }

    QString fallback;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("bem_")) {
            continue;
        }
        if (it.value() && it.value()->tissueType() == BrainSurface::TissueSkin) {
            return it.key();
        }
        if (fallback.isEmpty()) {
            fallback = it.key();
        }
    }
    return fallback;
}

//=============================================================================================================

QString BrainView::findHelmetSurfaceKey() const
{
    if (m_surfaces.contains("sens_surface_meg")) {
        return "sens_surface_meg";
    }
    return QString();
}

//=============================================================================================================

float BrainView::contourStep(float minVal, float maxVal, int targetTicks) const
{
    if (targetTicks <= 0) {
        return 0.0f;
    }
    double range = static_cast<double>(maxVal - minVal);
    if (range <= 0.0) {
        return 0.0f;
    }

    double raw = range / static_cast<double>(targetTicks);
    double exponent = std::floor(std::log10(raw));
    double base = std::pow(10.0, exponent);
    double frac = raw / base;

    double niceFrac = 1.0;
    if (frac <= 1.0) niceFrac = 1.0;
    else if (frac <= 2.0) niceFrac = 2.0;
    else if (frac <= 5.0) niceFrac = 5.0;
    else niceFrac = 10.0;

    return static_cast<float>(niceFrac * base);
}

//=============================================================================================================

void BrainView::updateContourSurfaces(const QString &prefix,
                                      const BrainSurface &surface,
                                      const QVector<float> &values,
                                      float step,
                                      bool visible)
{
    auto hideContours = [this, &prefix]() {
        QStringList suffixes = {"_neg", "_zero", "_pos"};
        for (const auto &suffix : suffixes) {
            QString key = prefix + suffix;
            if (m_surfaces.contains(key)) {
                m_surfaces[key]->setVisible(false);
            }
        }
    };

    if (!visible || values.isEmpty() || step <= 0.0f) {
        hideContours();
        return;
    }

    float minVal = values[0];
    float maxVal = values[0];
    for (int i = 1; i < values.size(); ++i) {
        minVal = std::min(minVal, values[i]);
        maxVal = std::max(maxVal, values[i]);
    }

    QVector<float> negLevels;
    QVector<float> posLevels;
    bool hasZero = (minVal < 0.0f && maxVal > 0.0f);

    for (float level = -step; level >= minVal; level -= step) {
        negLevels.append(level);
    }
    for (float level = step; level <= maxVal; level += step) {
        posLevels.append(level);
    }

    struct ContourBuffers {
        QVector<Eigen::Vector3f> verts;
        QVector<Eigen::Vector3f> norms;
        QVector<Eigen::Vector3i> tris;
    };

    auto addSegment = [](ContourBuffers &buf,
                         const QVector3D &p0,
                         const QVector3D &p1,
                         const QVector3D &normal,
                         float halfWidth,
                         float shift) {
        QVector3D dir = (p1 - p0);
        float len = dir.length();
        if (len < 1e-6f) {
            return;
        }
        dir /= len;

        QVector3D binormal = QVector3D::crossProduct(normal, dir);
        if (binormal.length() < 1e-6f) {
            binormal = QVector3D::crossProduct(QVector3D(0, 1, 0), dir);
        }
        if (binormal.length() < 1e-6f) {
            binormal = QVector3D::crossProduct(QVector3D(1, 0, 0), dir);
        }
        binormal.normalize();

        QVector3D offset = normal * shift;
        QVector3D w = binormal * halfWidth;

        int base = buf.verts.size();
        buf.verts.append(Eigen::Vector3f(p0.x() - w.x() + offset.x(), p0.y() - w.y() + offset.y(), p0.z() - w.z() + offset.z()));
        buf.verts.append(Eigen::Vector3f(p0.x() + w.x() + offset.x(), p0.y() + w.y() + offset.y(), p0.z() + w.z() + offset.z()));
        buf.verts.append(Eigen::Vector3f(p1.x() - w.x() + offset.x(), p1.y() - w.y() + offset.y(), p1.z() - w.z() + offset.z()));
        buf.verts.append(Eigen::Vector3f(p1.x() + w.x() + offset.x(), p1.y() + w.y() + offset.y(), p1.z() + w.z() + offset.z()));

        Eigen::Vector3f n(normal.x(), normal.y(), normal.z());
        buf.norms.append(n);
        buf.norms.append(n);
        buf.norms.append(n);
        buf.norms.append(n);

        buf.tris.append(Eigen::Vector3i(base + 0, base + 1, base + 2));
        buf.tris.append(Eigen::Vector3i(base + 1, base + 3, base + 2));
    };

    auto buildContours = [&](const QVector<float> &levels, ContourBuffers &buf) {
        const Eigen::MatrixX3f rr = surface.vertexPositions();
        const Eigen::MatrixX3f nn = surface.vertexNormals();
        const QVector<uint32_t> idx = surface.triangleIndices();
        if (rr.rows() == 0 || nn.rows() == 0 || idx.isEmpty()) {
            return;
        }

        const float shift = 0.001f;
        const float halfWidth = 0.0005f;

        for (float level : levels) {
            for (int t = 0; t + 2 < idx.size(); t += 3) {
                int i0 = idx[t];
                int i1 = idx[t + 1];
                int i2 = idx[t + 2];

                float v0 = values[i0];
                float v1 = values[i1];
                float v2 = values[i2];

                QVector3D p0(rr(i0, 0), rr(i0, 1), rr(i0, 2));
                QVector3D p1(rr(i1, 0), rr(i1, 1), rr(i1, 2));
                QVector3D p2(rr(i2, 0), rr(i2, 1), rr(i2, 2));

                QVector3D n0(nn(i0, 0), nn(i0, 1), nn(i0, 2));
                QVector3D n1(nn(i1, 0), nn(i1, 1), nn(i1, 2));
                QVector3D n2(nn(i2, 0), nn(i2, 1), nn(i2, 2));
                QVector3D normal = (n0 + n1 + n2).normalized();
                if (normal.length() < 1e-6f) {
                    normal = QVector3D::crossProduct(p1 - p0, p2 - p0).normalized();
                }

                QVector<QVector3D> hits;
                auto checkEdge = [&](const QVector3D &a, const QVector3D &b, float va, float vb) {
                    if (va == vb) {
                        return;
                    }
                    float tval = (level - va) / (vb - va);
                    if (tval >= 0.0f && tval < 1.0f) {
                        hits.append(a + (b - a) * tval);
                    }
                };

                checkEdge(p0, p1, v0, v1);
                checkEdge(p1, p2, v1, v2);
                checkEdge(p2, p0, v2, v0);

                if (hits.size() == 2) {
                    addSegment(buf, hits[0], hits[1], normal, halfWidth, shift);
                }
            }
        }
    };

    ContourBuffers negBuf;
    ContourBuffers posBuf;
    ContourBuffers zeroBuf;

    buildContours(negLevels, negBuf);
    buildContours(posLevels, posBuf);
    if (hasZero) {
        QVector<float> zeroLevels = {0.0f};
        buildContours(zeroLevels, zeroBuf);
    }

    auto updateSurface = [this, &prefix](const QString &suffix,
                                         const ContourBuffers &buf,
                                         const QColor &color,
                                         bool show) {
        QString key = prefix + suffix;
        if (!show || buf.verts.isEmpty()) {
            if (m_surfaces.contains(key)) {
                m_surfaces[key]->setVisible(false);
            }
            return;
        }

        Eigen::MatrixX3f rr(buf.verts.size(), 3);
        Eigen::MatrixX3f nn(buf.norms.size(), 3);
        Eigen::MatrixX3i tris(buf.tris.size(), 3);
        for (int i = 0; i < buf.verts.size(); ++i) {
            rr(i, 0) = buf.verts[i].x();
            rr(i, 1) = buf.verts[i].y();
            rr(i, 2) = buf.verts[i].z();
            nn(i, 0) = buf.norms[i].x();
            nn(i, 1) = buf.norms[i].y();
            nn(i, 2) = buf.norms[i].z();
        }
        for (int i = 0; i < buf.tris.size(); ++i) {
            tris.row(i) = buf.tris[i];
        }

        std::shared_ptr<BrainSurface> contourSurface;
        if (m_surfaces.contains(key)) {
            contourSurface = m_surfaces[key];
        } else {
            contourSurface = std::make_shared<BrainSurface>();
            m_surfaces[key] = contourSurface;
        }
        contourSurface->createFromData(rr, nn, tris, color);
        contourSurface->setVisible(true);
    };

    updateSurface("_neg", negBuf, QColor(0, 0, 255, 200), visible && !negBuf.verts.isEmpty());
    updateSurface("_zero", zeroBuf, QColor(0, 0, 0, 220), visible && !zeroBuf.verts.isEmpty());
    updateSurface("_pos", posBuf, QColor(255, 0, 0, 200), visible && !posBuf.verts.isEmpty());
}

//=============================================================================================================

bool BrainView::buildSensorFieldMapping()
{
    if (!m_sensorFieldLoaded || m_sensorEvoked.isEmpty()) {
        return false;
    }

    m_megFieldPick.clear();
    m_eegFieldPick.clear();
    m_megFieldPositions.clear();
    m_eegFieldPositions.clear();
    m_megFieldMapping.reset();
    m_eegFieldMapping.reset();

    QList<FiffChInfo> megChs;
    QList<FiffChInfo> eegChs;

    m_megFieldSurfaceKey = m_megFieldMapOnHead ? findHeadSurfaceKey() : findHelmetSurfaceKey();
    if (m_megFieldMapOnHead && m_megFieldSurfaceKey.isEmpty()) {
        m_megFieldSurfaceKey = findHelmetSurfaceKey();
        if (!m_megFieldSurfaceKey.isEmpty()) {
            qWarning() << "BrainView: Head surface missing for MEG map, falling back to helmet.";
        }
    }
    m_eegFieldSurfaceKey = findHeadSurfaceKey();

    if (m_megFieldSurfaceKey.isEmpty() && m_eegFieldSurfaceKey.isEmpty()) {
        qWarning() << "BrainView: No helmet/head surface available for sensor field mapping.";
        return false;
    }

    bool hasDevHead = false;
    QMatrix4x4 devHeadQTrans;
    if (!m_sensorEvoked.info.dev_head_t.isEmpty() &&
        m_sensorEvoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
        m_sensorEvoked.info.dev_head_t.to == FIFFV_COORD_HEAD &&
        !m_sensorEvoked.info.dev_head_t.trans.isIdentity()) {
        hasDevHead = true;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                devHeadQTrans(r, c) = m_sensorEvoked.info.dev_head_t.trans(r, c);
            }
        }
    }

    QMatrix4x4 headToMri;
    if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                headToMri(r, c) = m_headToMriTrans.trans(r, c);
            }
        }
    }

    auto isBad = [this](const QString &name) -> bool {
        return m_sensorEvoked.info.bads.contains(name);
    };

    for (int k = 0; k < m_sensorEvoked.info.chs.size(); ++k) {
        const auto &ch = m_sensorEvoked.info.chs[k];
        if (isBad(ch.ch_name)) {
            continue;
        }

        QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

        if (ch.kind == FIFFV_MEG_CH) {
            if (hasDevHead) {
                pos = devHeadQTrans.map(pos);
            }
            if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
                pos = headToMri.map(pos);
            }
            m_megFieldPick.append(k);
            m_megFieldPositions.append(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            megChs.append(ch);
        } else if (ch.kind == FIFFV_EEG_CH) {
            if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
                pos = headToMri.map(pos);
            }
            m_eegFieldPick.append(k);
            m_eegFieldPositions.append(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            eegChs.append(ch);
        }
    }

    // Constants matching MNE-Python _setup_dots / _make_surface_mapping
    constexpr float kIntrad  = 0.06f;   // int_rad in _setup_dots
    constexpr float kMegMiss = 1e-4f;   // miss for MEG in _make_surface_mapping
    constexpr float kEegMiss = 1e-3f;   // miss for EEG in _make_surface_mapping
    const Eigen::Vector3f defaultOrigin(0.0f, 0.0f, 0.04f);

    auto headMriOld = (m_applySensorTrans && !m_headToMriTrans.isEmpty()) ? toOldTransform(m_headToMriTrans) : nullptr;
    auto devHeadOld = (!m_sensorEvoked.info.dev_head_t.isEmpty() &&
                       m_sensorEvoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
                       m_sensorEvoked.info.dev_head_t.to == FIFFV_COORD_HEAD)
                          ? toOldTransform(m_sensorEvoked.info.dev_head_t)
                          : nullptr;

    if (!m_megFieldSurfaceKey.isEmpty() && m_surfaces.contains(m_megFieldSurfaceKey) && !megChs.isEmpty()) {
        const BrainSurface& surface = *m_surfaces[m_megFieldSurfaceKey];
        Eigen::MatrixX3f verts = surface.vertexPositions();
        Eigen::MatrixX3f norms = surface.vertexNormals();
        if (norms.rows() != verts.rows()) {
            const QVector<uint32_t> idx = surface.triangleIndices();
            const int nTris = idx.size() / 3;
            if (nTris > 0) {
                Eigen::MatrixX3i tris(nTris, 3);
                for (int t = 0; t < nTris; ++t) {
                    tris(t, 0) = static_cast<int>(idx[t * 3]);
                    tris(t, 1) = static_cast<int>(idx[t * 3 + 1]);
                    tris(t, 2) = static_cast<int>(idx[t * 3 + 2]);
                }
                norms = FSLIB::Surface::compute_normals(verts, tris);
            }
        }

        if (verts.rows() > 0 && norms.rows() == verts.rows()) {
            const QString coilPath = QCoreApplication::applicationDirPath()
                + "/../resources/general/coilDefinitions/coil_def.dat";
            std::unique_ptr<FWDLIB::FwdCoilSet> templates(FWDLIB::FwdCoilSet::read_coil_defs(coilPath));
            if (templates) {
                std::unique_ptr<FiffCoordTransOld> devToTarget;
                if (m_megFieldMapOnHead && headMriOld) {
                    if (devHeadOld) {
                        devToTarget.reset(FiffCoordTransOld::fiff_combine_transforms(
                            FIFFV_COORD_DEVICE, FIFFV_COORD_MRI, devHeadOld.get(), headMriOld.get()));
                    }
                } else if (devHeadOld) {
                    devToTarget = std::make_unique<FiffCoordTransOld>(*devHeadOld);
                }

                Eigen::Vector3f origin = defaultOrigin;
                if (m_megFieldMapOnHead && headMriOld) {
                    origin = applyOldTransform(origin, headMriOld.get());
                }

                std::unique_ptr<FWDLIB::FwdCoilSet> coils(templates->create_meg_coils(
                    megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, devToTarget.get()));
                if (coils && coils->ncoil > 0) {
                    m_megFieldMapping = BRAINVIEWLIB::FieldMap::computeMegMapping(
                        *coils, verts, norms, origin, kIntrad, kMegMiss);
                }
            }
        }
    }

    if (!m_eegFieldSurfaceKey.isEmpty() && m_surfaces.contains(m_eegFieldSurfaceKey) && !eegChs.isEmpty()) {
        const BrainSurface& surface = *m_surfaces[m_eegFieldSurfaceKey];
        Eigen::MatrixX3f verts = surface.vertexPositions();
        if (verts.rows() > 0) {
            Eigen::Vector3f origin = defaultOrigin;
            if (headMriOld) {
                origin = applyOldTransform(origin, headMriOld.get());
            }

            std::unique_ptr<FWDLIB::FwdCoilSet> eegCoils(FWDLIB::FwdCoilSet::create_eeg_els(
                eegChs, eegChs.size(), headMriOld.get()));
            if (eegCoils && eegCoils->ncoil > 0) {
                m_eegFieldMapping = BRAINVIEWLIB::FieldMap::computeEegMapping(
                    *eegCoils, verts, origin, kIntrad, kEegMiss);
            }
        }
    }

    return true;
}

//=============================================================================================================

void BrainView::applySensorFieldMap()
{
    if (!m_sensorFieldLoaded || m_sensorEvoked.isEmpty()) {
        return;
    }

    auto applyMap = [this](const QString &key,
                           const QString &contourPrefix,
                           const QVector<int> &pick,
                           const QSharedPointer<Eigen::MatrixXf> &mat,
                           bool visible,
                           bool showContours) {
        if (key.isEmpty() || !m_surfaces.contains(key)) {
            return;
        }

        auto surface = m_surfaces[key];
        if (!visible || !mat || pick.isEmpty()) {
            surface->setVisualizationMode(BrainSurface::ModeSurface);
            updateContourSurfaces(contourPrefix, *surface, QVector<float>(), 0.0f, false);
            return;
        }

        if (mat->cols() != pick.size()) {
            surface->setVisualizationMode(BrainSurface::ModeSurface);
            updateContourSurfaces(contourPrefix, *surface, QVector<float>(), 0.0f, false);
            return;
        }

        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i) {
            meas(i) = static_cast<float>(m_sensorEvoked.data(pick[i], m_sensorFieldTimePoint));
        }

        Eigen::VectorXf mapped = (*mat) * meas;

        float maxAbs = 0.0f;
        float minVal = 0.0f;
        float maxVal = 0.0f;
        for (int i = 0; i < mapped.size(); ++i) {
            float v = mapped(i);
            if (i == 0) {
                minVal = v;
                maxVal = v;
            } else {
                minVal = std::min(minVal, v);
                maxVal = std::max(maxVal, v);
            }
            maxAbs = std::max(maxAbs, std::abs(v));
        }
        if (maxAbs <= 0.0f) {
            maxAbs = 1.0f;
        }

        QVector<uint32_t> colors(mapped.size());
        for (int i = 0; i < mapped.size(); ++i) {
            double norm = (mapped(i) / maxAbs) * 0.5 + 0.5;
            norm = qBound(0.0, norm, 1.0);
            QRgb rgb;
            if (m_sensorFieldColormap == "MNE") {
                rgb = mneAnalyzeColor(norm);
            } else {
                rgb = DISPLIB::ColorMap::valueToColor(norm, m_sensorFieldColormap);
            }
            uint32_t r = qRed(rgb);
            uint32_t g = qGreen(rgb);
            uint32_t b = qBlue(rgb);
            colors[i] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }

        surface->applySourceEstimateColors(colors);

        QVector<float> values(mapped.size());
        for (int i = 0; i < mapped.size(); ++i) {
            values[i] = mapped(i);
        }
        float step = contourStep(-maxAbs, maxAbs, 20);
        updateContourSurfaces(contourPrefix, *surface, values, step, showContours);
    };

    // Aggregate visibility across all SubViews so that field
    // maps / contours are computed whenever ANY pane needs them.
    bool anyMegField = m_singleView.visibility.megFieldMap;
    bool anyEegField = m_singleView.visibility.eegFieldMap;
    bool anyMegContours = m_singleView.visibility.megFieldContours;
    bool anyEegContours = m_singleView.visibility.eegFieldContours;
    for (int i = 0; i < 4; ++i) {
        anyMegField    |= m_subViews[i].visibility.megFieldMap;
        anyEegField    |= m_subViews[i].visibility.eegFieldMap;
        anyMegContours |= m_subViews[i].visibility.megFieldContours;
        anyEegContours |= m_subViews[i].visibility.eegFieldContours;
    }

    applyMap(m_megFieldSurfaceKey,
             m_megFieldContourPrefix,
             m_megFieldPick,
             m_megFieldMapping,
             anyMegField,
             anyMegContours);
    applyMap(m_eegFieldSurfaceKey,
             m_eegFieldContourPrefix,
             m_eegFieldPick,
             m_eegFieldMapping,
             anyEegField,
             anyEegContours);
}


//=============================================================================================================

void BrainView::castRay(const QPoint &pos)
{
    // 1. Setup Matrix Stack (Must match render exactly, including multiview pane layout)
    const QSize outputSize = size();

    QVector<int> enabledViewports;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < 4; ++i) {
            if (m_subViews[i].enabled) {
                enabledViewports.append(i);
            }
        }
        if (enabledViewports.isEmpty()) {
            enabledViewports.append(0);
        }
    } else {
        enabledViewports.append(0);
    }

    const int numEnabled = enabledViewports.size();
    int activeSlot = 0;
    QRect activePane(0, 0, outputSize.width(), outputSize.height());

    bool hasValidPane = true;
    if (m_viewMode == MultiView && numEnabled > 1) {
        bool foundSlot = false;
        for (int slot = 0; slot < numEnabled; ++slot) {
            const QRect pane = multiViewSlotRect(slot, numEnabled, outputSize);
            if (pane.contains(pos)) {
                activeSlot = slot;
                activePane = pane;
                foundSlot = true;
                break;
            }
        }

        hasValidPane = foundSlot;
    }

    const int vp = (m_viewMode == MultiView) ? enabledViewports[activeSlot] : 0;
    const SubView &sv = (m_viewMode == MultiView) ? m_subViews[vp] : m_singleView;
    const int preset = (m_viewMode == MultiView) ? std::clamp(sv.preset, 0, 6) : 1;
    const QString activeSurfaceType = sv.surfaceType;
    const ViewVisibilityProfile &visibility = sv.visibility;

    const QQuaternion perspectivePreset = perspectivePresetRotation();
    QQuaternion effectiveRotation = m_cameraRotation * perspectivePreset;
    if (m_viewMode == MultiView) {
        const QQuaternion presetOffset = multiViewPresetOffset(preset);
        const QQuaternion panePerspectiveRotation = sv.perspectiveRotation;
        effectiveRotation = multiViewPresetIsPerspective(preset) ? (panePerspectiveRotation * presetOffset)
                                                                 : presetOffset;
    }

    QMatrix4x4 projection;
    float farPlane = m_sceneSize * 20.0f;
    if (farPlane < 100.0f) farPlane = 100.0f;
    const float aspect = float(std::max(1, activePane.width())) / float(std::max(1, activePane.height()));
    projection.perspective(45.0f, aspect, m_sceneSize * 0.01f, farPlane);

    // Use per-viewport zoom (must match render loop)
    const float vpZoom = (m_viewMode == MultiView) ? sv.zoom : m_zoom;
    float baseDistance = m_sceneSize * 1.5f;
    float distance = baseDistance - vpZoom * (m_sceneSize * 0.05f);
    QVector3D cameraPos = effectiveRotation.rotatedVector(QVector3D(0, 0, distance));
    QVector3D upVector = effectiveRotation.rotatedVector(QVector3D(0, 1, 0));

    // Apply per-viewport pan offset for planar views (must match render loop)
    QVector3D lookAt(0, 0, 0);
    if (m_viewMode == MultiView && !multiViewPresetIsPerspective(preset)) {
        const QVector2D &pan = sv.pan;
        const QVector3D right = effectiveRotation.rotatedVector(QVector3D(1, 0, 0)).normalized();
        const QVector3D up = upVector.normalized();
        lookAt += right * pan.x() + up * pan.y();
        cameraPos += right * pan.x() + up * pan.y();
    }

    QMatrix4x4 view;
    view.lookAt(cameraPos, lookAt, upVector);

    QMatrix4x4 model;
    model.translate(-m_sceneCenter);
    QMatrix4x4 pvm = projection * view * model;
    bool invertible;
    QMatrix4x4 invPVM = pvm.inverted(&invertible);
    if (!invertible) return;

    const float localX = float(pos.x() - activePane.x());
    const float localY = float(pos.y() - activePane.y());
    const float paneW = float(std::max(1, activePane.width()));
    const float paneH = float(std::max(1, activePane.height()));

    float ndcX = (2.0f * localX) / paneW - 1.0f;
    float ndcY = 1.0f - (2.0f * localY) / paneH;

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
    
    if (hasValidPane) {
    // Check Surfaces (Sensors, Hemisphere, BEM)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.value()->isVisible()) continue;
        if (!shouldRenderSurfaceForView(it.key(), visibility)) continue;
        if (it.key().startsWith("srcsp_")) continue;
        
        bool isSensor = it.key().startsWith("sens_");
        bool isBem = it.key().startsWith("bem_");
        bool isDig = it.key().startsWith("dig_");
        
        // Brain surfaces: only pick if matching active surface type (same as render)
        if (!isSensor && !isBem && !isDig) {
            if (!it.key().endsWith(activeSurfaceType)) continue;
        }
        
        float dist;
        int vertexIdx = -1;
        if (it.value()->intersects(rayOrigin, rayDir, dist, vertexIdx)) {
             if (dist < closestDist) {
                 closestDist = dist;
                 m_hasIntersection = true;
                 m_lastIntersectionPoint = rayOrigin + dist * rayDir;

                 hitItem = nullptr;
                 hitInfo.clear();

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
            if (!visibility.dipoles) continue;
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
    QString hitKey;
    if (!currentRegion.isEmpty()) {
        // Brain surface with annotation — determine hemisphere from surface key
        QString hemi;
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (hitItem && m_itemSurfaceMap.contains(hitItem) && m_itemSurfaceMap[hitItem] == it.value()) {
                if (it.key().startsWith("lh")) hemi = "lh";
                else if (it.key().startsWith("rh")) hemi = "rh";
                break;
            }
        }
        if (!hemi.isEmpty())
            displayLabel = QString("Region: %1 (%2)").arg(currentRegion, hemi);
        else
            displayLabel = QString("Region: %1").arg(currentRegion);
    } else if (!hitInfo.isEmpty()) {
        // Determine what type of object was hit from the surface key
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (hitItem && m_itemSurfaceMap.contains(hitItem) && m_itemSurfaceMap[hitItem] == it.value()) {
                hitKey = it.key();
                break;
            }
        }
        // If no key found via item map, try hitInfo directly
        if (hitKey.isEmpty()) hitKey = hitInfo;

        if (hitKey.startsWith("sens_surface_meg")) {
            displayLabel = "MEG Helmet";
        } else if (hitKey.startsWith("sens_meg_")) {
            displayLabel = QString("MEG: %1").arg(hitInfo);
        } else if (hitKey.startsWith("sens_eeg_")) {
            displayLabel = QString("EEG: %1").arg(hitInfo);
        } else if (hitKey.startsWith("dig_")) {
            // Resolve the specific point name from the batched mesh.
            // Each sphere in the batch has 42 vertices (icosahedron subdivided once).
            QString pointName;
            if (hitItem && hitIndex >= 0) {
                AbstractTreeItem* absHit = dynamic_cast<AbstractTreeItem*>(hitItem);
                if (absHit && absHit->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType) {
                    DigitizerTreeItem* digHit = static_cast<DigitizerTreeItem*>(absHit);
                    const int vertsPerSphere = 42;
                    int ptIdx = hitIndex / vertsPerSphere;
                    const QStringList& names = digHit->pointNames();
                    if (ptIdx >= 0 && ptIdx < names.size()) {
                        pointName = names[ptIdx];
                    }
                }
            }
            QString category = hitKey.mid(4); // strip "dig_"
            if (!category.isEmpty()) category[0] = category[0].toUpper();
            if (!pointName.isEmpty()) {
                displayLabel = QString("Digitizer: %1 (%2)").arg(pointName, category);
            } else {
                displayLabel = QString("Digitizer (%1)").arg(category);
            }
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
        } else {
            // Fallback: Check if it's a hemisphere based on the key
            if (hitKey.startsWith("lh_")) {
                displayLabel = "Left Hemisphere";
            } else if (hitKey.startsWith("rh_")) {
                displayLabel = "Right Hemisphere";
            }
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

    QString hoveredSurfaceKey;
    if (hitKey.startsWith("sens_surface_meg")) {
        hoveredSurfaceKey = hitKey;
    }
    
    if (hitItem != m_hoveredItem || hitIndex != m_hoveredIndex || hoveredSurfaceKey != m_hoveredSurfaceKey) {
        // Deselect previous
        if (m_hoveredItem) {
             if (m_itemSurfaceMap.contains(m_hoveredItem)) {
                 m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
                 m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
                 m_itemSurfaceMap[m_hoveredItem]->setSelectedVertexRange(-1, 0);
             } else if (m_itemDipoleMap.contains(m_hoveredItem)) {
                 m_itemDipoleMap[m_hoveredItem]->setSelected(m_hoveredIndex, false);
             }
        }
        if (!m_hoveredSurfaceKey.isEmpty() && m_surfaces.contains(m_hoveredSurfaceKey)) {
            m_surfaces[m_hoveredSurfaceKey]->setSelected(false);
            m_surfaces[m_hoveredSurfaceKey]->setSelectedRegion(-1);
            m_surfaces[m_hoveredSurfaceKey]->setSelectedVertexRange(-1, 0);
        }
    
        m_hoveredItem = hitItem;
        m_hoveredIndex = hitIndex;
        m_hoveredSurfaceKey = hoveredSurfaceKey;
        
        if (m_hoveredItem) {
             // Select new
             if (m_itemSurfaceMap.contains(m_hoveredItem)) {
                 // Check if this is a digitizer batched mesh — highlight single sphere
                 AbstractTreeItem* absHitSel = dynamic_cast<AbstractTreeItem*>(m_hoveredItem);
                 bool isDigitizer = absHitSel &&
                     (absHitSel->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType);

                 if (isDigitizer && m_hoveredIndex >= 0) {
                     const int vertsPerSphere = 42;
                     int sphereIdx = m_hoveredIndex / vertsPerSphere;
                     m_itemSurfaceMap[m_hoveredItem]->setSelectedVertexRange(
                         sphereIdx * vertsPerSphere, vertsPerSphere);
                 } else if (currentRegionId != -1) {
                     m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
                     m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
                 } else {
                     m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
                     m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
                 }
             } else if (m_itemDipoleMap.contains(m_hoveredItem)) {
                 m_itemDipoleMap[m_hoveredItem]->setSelected(m_hoveredIndex, true);
             }
        } else if (!m_hoveredSurfaceKey.isEmpty() && m_surfaces.contains(m_hoveredSurfaceKey)) {
            m_surfaces[m_hoveredSurfaceKey]->setSelected(true);
            m_surfaces[m_hoveredSurfaceKey]->setSelectedRegion(-1);
        }
    } else if (m_hoveredItem && m_itemSurfaceMap.contains(m_hoveredItem)) {
        AbstractTreeItem* absHitUpd = dynamic_cast<AbstractTreeItem*>(m_hoveredItem);
        bool isDigitizer = absHitUpd &&
            (absHitUpd->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType);

        if (isDigitizer && m_hoveredIndex >= 0) {
            const int vertsPerSphere = 42;
            int sphereIdx = m_hoveredIndex / vertsPerSphere;
            m_itemSurfaceMap[m_hoveredItem]->setSelectedVertexRange(
                sphereIdx * vertsPerSphere, vertsPerSphere);
        } else if (currentRegionId != -1) {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(false);
        } else {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
        }
    } else if (!m_hoveredSurfaceKey.isEmpty() && m_surfaces.contains(m_hoveredSurfaceKey)) {
        m_surfaces[m_hoveredSurfaceKey]->setSelected(true);
    }
    update();
}

//=============================================================================================================

void BrainView::showViewportPresetMenu(int viewport, const QPoint &globalPos)
{
    if (viewport < 0 || viewport >= 4) {
        return;
    }

    QMenu menu;
    QAction *topAction = menu.addAction("Top");
    QAction *perspectiveAction = menu.addAction("Perspective");
    QAction *frontAction = menu.addAction("Front");
    QAction *leftAction = menu.addAction("Left");
    menu.addSeparator();
    QAction *bottomAction = menu.addAction("Bottom");
    QAction *backAction = menu.addAction("Back");
    QAction *rightAction = menu.addAction("Right");

    const int currentPreset = std::clamp(m_subViews[viewport].preset, 0, 6);
    topAction->setCheckable(true);
    perspectiveAction->setCheckable(true);
    frontAction->setCheckable(true);
    leftAction->setCheckable(true);
    bottomAction->setCheckable(true);
    backAction->setCheckable(true);
    rightAction->setCheckable(true);

    topAction->setChecked(currentPreset == 0);
    perspectiveAction->setChecked(currentPreset == 1);
    frontAction->setChecked(currentPreset == 2);
    leftAction->setChecked(currentPreset == 3);
    bottomAction->setChecked(currentPreset == 4);
    backAction->setChecked(currentPreset == 5);
    rightAction->setChecked(currentPreset == 6);

    QAction *selected = menu.exec(globalPos);
    if (!selected) {
        return;
    }

    int newPreset = currentPreset;
    if (selected == topAction) {
        newPreset = 0;
    } else if (selected == perspectiveAction) {
        newPreset = 1;
    } else if (selected == frontAction) {
        newPreset = 2;
    } else if (selected == leftAction) {
        newPreset = 3;
    } else if (selected == bottomAction) {
        newPreset = 4;
    } else if (selected == backAction) {
        newPreset = 5;
    } else if (selected == rightAction) {
        newPreset = 6;
    }

    if (newPreset == currentPreset) {
        return;
    }

    m_subViews[viewport].preset = newPreset;
    saveMultiViewSettings();
    updateOverlayLayout();
    update();
}
