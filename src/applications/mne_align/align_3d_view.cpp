//=============================================================================================================
/**
 * @file     align_3d_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Implementation of the MNE Align 3-D viewer.
 */

#include "align_3d_view.h"

#include "acquired_points.h"

#include <disp3D/model/braintreemodel.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/view/brainview.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_file.h>
#include <mne/mne_bem.h>
#include <mne/mne_icp.h>

#include <QVBoxLayout>

using namespace MNEALIGN;
using DISP3DLIB::MultimodalScene;
using DISP3DLIB::SceneLayer;
using DISP3DLIB::SceneLayerKind;

namespace {
constexpr const char* kBemLayerId       = "head_bem";
constexpr const char* kAcquiredLayerId  = "acquired_points";
constexpr FiducialId  kFiducialOrder[]  = {FiducialId::NAS, FiducialId::LPA, FiducialId::RPA};
} // namespace

//=============================================================================================================

Align3DView::Align3DView(AcquiredPoints* acquired, QWidget* parent)
    : QWidget(parent)
    , m_pPoints(acquired)
    , m_pScene(std::make_unique<MultimodalScene>())
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_pBrainView = new BrainView(this);
    lay->addWidget(m_pBrainView);

    m_pBrainModel = new BrainTreeModel(m_pBrainView);
    m_pBrainView->setModel(m_pBrainModel);

    connect(m_pBrainView, &BrainView::viewCountChanged,
            this, &Align3DView::viewCountChanged);
    connect(m_pBrainView, &BrainView::shaderModeChanged,
            this, &Align3DView::renderModeChanged);
    connect(m_pBrainView, &BrainView::surfacePointClicked,
            this, &Align3DView::surfacePointClicked);
    connect(m_pBrainView, &BrainView::surfacePointDoubleClicked,
            this, &Align3DView::surfacePointDoubleClicked);

    // Throttle live tracker redraws to ~15 fps so the scene stays stable.
    m_liveUpdateTimer.setInterval(66);
    connect(&m_liveUpdateTimer, &QTimer::timeout,
            this, &Align3DView::onLiveUpdateTick);

    rebuildBemSurfaces();
    rebuildDigitizerLayer();

    if (m_pPoints) {
        connect(m_pPoints, &AcquiredPoints::pointsChanged,
                this, &Align3DView::onPointsChanged);
    }
}

Align3DView::~Align3DView() = default;

MultimodalScene* Align3DView::scene() const
{
    return m_pScene.get();
}

void Align3DView::setViewCount(int count)
{
    if (!m_pBrainView) return;
    m_pBrainView->setViewCount(qBound(1, count, 4));
    applyViewConfiguration();
}

void Align3DView::setRenderMode(const QString& modeName)
{
    if (modeName != QLatin1String("Anatomical") && modeName != QLatin1String("Holographic")) {
        return;
    }
    if (!m_pBrainView) return;
    m_pBrainView->setShaderMode(modeName);
    applyViewConfiguration();
}

void Align3DView::setCameraPreset(int preset)
{
    m_cameraPreset = qBound(0, preset, 6);
    applyViewConfiguration();
}

void Align3DView::setCameraFocus(CameraFocus focus)
{
    m_cameraFocus = focus;
    if (!m_pBrainView) return;

    if (focus == CameraFocus::Pointer && m_stationPoses.contains(m_penStation)) {
        m_pBrainView->setCameraFocusOverride(m_stationPoses[m_penStation].position, 0.08f);
    } else {
        m_pBrainView->clearCameraFocusOverride();
    }
}

void Align3DView::setPenStation(int station)
{
    m_penStation = qBound(1, station, 4);
}

//=============================================================================================================

void Align3DView::setBem(std::shared_ptr<MNELIB::MNEBem> bem)
{
    m_pBem = std::move(bem);
    m_pScene->removeLayer(QString::fromLatin1(kBemLayerId));

    if (m_pBem) {
        SceneLayer layer;
        layer.id          = QString::fromLatin1(kBemLayerId);
        layer.displayName = QStringLiteral("Head BEM");
        layer.kind        = SceneLayerKind::BrainSurface;
        layer.payload     = std::shared_ptr<void>(m_pBem, m_pBem.get());
        m_pScene->addLayer(layer);
    }
    rebuildBemSurfaces();
}

void Align3DView::setDigitizerConnected(bool connected)
{
    m_digitizerConnected = connected;
    if (!connected) {
        m_liveTrackerDirty = false;
        m_stationPoses.clear();
        m_liveUpdateTimer.stop();
        if (m_pBrainView) {
            m_pBrainView->clearLiveMarkers();
            m_pBrainView->clearCameraFocusOverride();
        }
    } else if (!m_liveUpdateTimer.isActive()) {
        m_liveUpdateTimer.start();
    }
}

void Align3DView::setLiveDigitizerPose(int station,
                                       const QVector3D& position,
                                       const QQuaternion& orientation)
{
    m_digitizerConnected = true;
    m_stationPoses[station] = {position, orientation};
    m_liveTrackerDirty = true;

    if (!m_liveUpdateTimer.isActive()) {
        m_liveUpdateTimer.start();
    }
}

void Align3DView::onLiveUpdateTick()
{
    if (!m_liveTrackerDirty || !m_pBrainView) {
        return;
    }
    m_liveTrackerDirty = false;

    constexpr float kBaseRadius    = 0.004f;   // 4 mm — transmitter base
    constexpr float kTrackerRadius = 0.005f;   // 5 mm — stylus tip
    constexpr float kAxisRadius    = 0.002f;   // 2 mm — axis-tip spheres
    constexpr float kAxisLength    = 0.015f;   // 1.5 cm axis arm

    // Per-station colours: distinct hues for up to 4 stations.
    static const QColor kStationColors[] = {
        QColor(255, 220,  40),  // station 1 — yellow
        QColor(255, 120,   0),  // station 2 — orange
        QColor(180,  60, 255),  // station 3 — purple
        QColor(  0, 220, 220),  // station 4 — cyan
    };
    constexpr int kMaxStationColors = 4;

    QVector<LiveMarker> markers;

    const QMatrix4x4 devToMri = trackerToMri();

    // Base station (transmitter origin, transformed)
    if (m_digitizerConnected) {
        markers.append({devToMri.map(QVector3D(0.0f, 0.0f, 0.0f)),
                        QColor(60, 120, 255), kBaseRadius});
    }

    QVector3D focusPos;
    bool haveFocus = false;

    for (auto it = m_stationPoses.constBegin(); it != m_stationPoses.constEnd(); ++it) {
        const int station = it.key();
        const StationPose& pose = it.value();
        const QColor tipColor = kStationColors[qBound(0, station - 1, kMaxStationColors - 1)];

        // Transform tracker positions into MRI space
        const QVector3D pos = devToMri.map(pose.position);

        // Tracker tip — pen station gets a transparent aura sphere
        if (station == m_penStation) {
            markers.append({pos, tipColor, 0.008f, true});  // 8 mm transparent aura
            markers.append({pos, tipColor, kTrackerRadius}); // solid core
        } else {
            markers.append({pos, tipColor, kTrackerRadius});
        }

        // Orientation axes: R/G/B tip spheres (transformed)
        const QVector3D xTip = devToMri.map(
            pose.position + pose.orientation.rotatedVector(QVector3D(kAxisLength, 0.0f, 0.0f)));
        const QVector3D yTip = devToMri.map(
            pose.position + pose.orientation.rotatedVector(QVector3D(0.0f, kAxisLength, 0.0f)));
        const QVector3D zTip = devToMri.map(
            pose.position + pose.orientation.rotatedVector(QVector3D(0.0f, 0.0f, kAxisLength)));

        markers.append({xTip, QColor(255, 50, 50),  kAxisRadius});
        markers.append({yTip, QColor(50, 220, 50),  kAxisRadius});
        markers.append({zTip, QColor(80, 140, 255), kAxisRadius});

        // Use pen station for camera focus
        if (station == m_penStation) {
            focusPos = pos;
            haveFocus = true;
        }
    }

    m_pBrainView->setLiveMarkers(markers);

    if (m_cameraFocus == CameraFocus::Pointer && haveFocus) {
        m_pBrainView->setCameraFocusOverride(focusPos, 0.08f);
    }
}

//=============================================================================================================

void Align3DView::onPointsChanged()
{
    recomputeAlignment();
    rebuildAcquiredLayer();
    rebuildDigitizerLayer();
    rebuildStaticMarkers();

    // Force live markers to refresh with the new transform even if no
    // new Polhemus data has arrived (e.g. stream paused by pen button).
    m_liveTrackerDirty = true;
    onLiveUpdateTick();
}

void Align3DView::rebuildAcquiredLayer()
{
    m_pScene->removeLayer(QString::fromLatin1(kAcquiredLayerId));
    if (!m_pPoints || m_pPoints->points().isEmpty()) return;

    auto snapshot = std::make_shared<QVector<DigitizedPoint>>(m_pPoints->points());

    SceneLayer layer;
    layer.id          = QString::fromLatin1(kAcquiredLayerId);
    layer.displayName = QStringLiteral("Digitised points");
    layer.kind        = SceneLayerKind::Custom;
    layer.payload     = std::shared_ptr<void>(snapshot, snapshot.get());
    m_pScene->addLayer(layer);
}

void Align3DView::rebuildBemSurfaces()
{
    if (!m_pBrainView || !m_pBrainModel) {
        return;
    }

    m_pBrainView->clearBem();
    if (!m_pBem) {
        return;
    }

    for (int i = 0; i < m_pBem->size(); ++i) {
        QString name;
        switch ((*m_pBem)[i].id) {
            case FIFFV_BEM_SURF_ID_HEAD:  name = QStringLiteral("head");        break;
            case FIFFV_BEM_SURF_ID_SKULL: name = QStringLiteral("outer_skull"); break;
            case FIFFV_BEM_SURF_ID_BRAIN: name = QStringLiteral("inner_skull"); break;
            default:                      name = QString::number(i);           break;
        }
        m_pBrainModel->addBemSurface(QStringLiteral("align"), name, (*m_pBem)[i]);
    }
}

void Align3DView::rebuildDigitizerLayer()
{
    if (!m_pBrainView || !m_pBrainModel) {
        return;
    }

    m_pBrainView->clearSensors();

    if (!m_pPoints || m_pPoints->points().isEmpty()) {
        return;
    }

    QList<FIFFLIB::FiffDigPoint> fiffPts;
    fiffPts.reserve(m_pPoints->points().size());

    const QMatrix4x4 devToMri = trackerToMri();

    for (const auto& pt : m_pPoints->points()) {
        FIFFLIB::FiffDigPoint dig;
        dig.coord_frame = FIFFV_COORD_HEAD;
        const QVector3D pos = devToMri.map(pt.position);
        dig.r[0] = pos.x();
        dig.r[1] = pos.y();
        dig.r[2] = pos.z();

        switch (pt.kind) {
            case PointKind::Fiducial:
                dig.kind  = FIFFV_POINT_CARDINAL;
                dig.ident = pt.identNumber;
                break;
            case PointKind::Eeg:
                dig.kind  = FIFFV_POINT_EEG;
                dig.ident = pt.identNumber;
                break;
            case PointKind::HeadShape:
                dig.kind  = FIFFV_POINT_EXTRA;
                dig.ident = pt.identNumber;
                break;
        }
        fiffPts.append(dig);
    }

    m_pBrainModel->addDigitizerData(fiffPts);
}

void Align3DView::rebuildStaticMarkers()
{
    if (!m_pBrainView || !m_pPoints) return;

    QVector<LiveMarker> markers;

    constexpr float kFidRadius = 0.004f;   // 4 mm
    constexpr float kEegRadius = 0.003f;   // 3 mm
    constexpr float kHspRadius = 0.002f;   // 2 mm

    // Twin fiducials (BEM-space, clicked on surface)
    static const QColor kTwinColors[] = {
        QColor(0, 220, 0),    // NAS — green
        QColor(220, 0, 0),    // LPA — red
        QColor(0, 0, 220),    // RPA — blue
    };
    for (int i = 0; i < 3; ++i) {
        if (m_pPoints->hasTwinFiducial(kFiducialOrder[i])) {
            markers.append({m_pPoints->twinFiducial(kFiducialOrder[i]),
                            kTwinColors[i], kFidRadius});
        }
    }

    // Captured fiducial colors — match twin colors but lighter/brighter
    static const QColor kCapFidColors[] = {
        QColor(120, 255, 120),  // NAS — bright green
        QColor(255, 120, 120),  // LPA — bright red
        QColor(120, 120, 255),  // RPA — bright blue
    };
    constexpr float kCapFidRadius = 0.005f;  // 5 mm — slightly larger than twins

    // Acquired points (sensor-frame → transformed to MRI space for display)
    const QMatrix4x4 devToMri = trackerToMri();
    for (const auto& pt : m_pPoints->points()) {
        const QVector3D pos = devToMri.map(pt.position);
        switch (pt.kind) {
            case PointKind::Fiducial: {
                // Color per identity so captured NAS/LPA/RPA are visually distinct
                const int idx = qBound(0, pt.identNumber - 1, 2); // FiducialId 1..3 → 0..2
                markers.append({pos, kCapFidColors[idx], kCapFidRadius});
                break;
            }
            case PointKind::Eeg:
                markers.append({pos, QColor(220, 120, 255), kEegRadius}); // purple
                break;
            case PointKind::HeadShape:
                markers.append({pos, QColor(200, 200, 200), kHspRadius}); // light grey
                break;
        }
    }

    if (markers.isEmpty()) {
        m_pBrainView->clearStaticMarkers();
    } else {
        m_pBrainView->setStaticMarkers(markers);
    }
}

void Align3DView::recomputeAlignment()
{
    m_deviceToHead.setToIdentity();
    m_headToMri.setToIdentity();
    if (!m_pPoints) return;

    // ── 1. Device → Head (runtime offset, NOT stored in trans.fif) ─────
    //
    // Once we have all 3 captured fiducials in device/tracker space, we
    // can define the Neuromag HEAD coordinate system using
    // FiffCoordTrans::fromCardinalPoints.  With fewer points we fall
    // back to a progressive translation/rotation.

    const bool hasCapNas = m_pPoints->hasFiducial(FiducialId::NAS);
    const bool hasCapLpa = m_pPoints->hasFiducial(FiducialId::LPA);
    const bool hasCapRpa = m_pPoints->hasFiducial(FiducialId::RPA);

    if (hasCapNas && hasCapLpa && hasCapRpa) {
        // All 3 captured → proper Neuromag HEAD frame definition
        const QVector3D cN = m_pPoints->fiducial(FiducialId::NAS);
        const QVector3D cL = m_pPoints->fiducial(FiducialId::LPA);
        const QVector3D cR = m_pPoints->fiducial(FiducialId::RPA);
        const float rL[3] = {cL.x(), cL.y(), cL.z()};
        const float rN[3] = {cN.x(), cN.y(), cN.z()};
        const float rR[3] = {cR.x(), cR.y(), cR.z()};
        auto fct = FIFFLIB::FiffCoordTrans::fromCardinalPoints(
            FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, rL, rN, rR);
        // Copy the 4×4 inverse (device→head = invtrans of the "from device, to head" result)
        // fromCardinalPoints stores the axes in `trans` columns with origin in move(),
        // but its `invtrans` maps device→head.
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                m_deviceToHead(r, c) = fct.invtrans(r, c);
    }
    // With < 3 captured fiducials, deviceToHead stays identity — points
    // are shown in raw device coords until we can define the HEAD frame.

    // ── 2. Head → MRI (coregistration, stored in trans.fif) ────────────
    //
    // Match captured fiducials (in HEAD space) with twin fiducials (MRI
    // space) using Procrustes.  With fewer than 3 matched pairs we use
    // progressive approximation; with all 3 we get a proper rigid body.

    QVector<QVector3D> headPts;  // fiducials in HEAD space
    QVector<QVector3D> mriPts;   // twin fiducials in MRI space

    for (FiducialId id : kFiducialOrder) {
        const bool hasCap  = m_pPoints->hasFiducial(id);
        const bool hasTwin = m_pPoints->hasTwinFiducial(id);
        if (hasCap && hasTwin) {
            // Transform captured fiducial into HEAD space
            const QVector3D capDev = m_pPoints->fiducial(id);
            const QVector3D capHead = m_deviceToHead.map(capDev);
            headPts.append(capHead);
            mriPts.append(m_pPoints->twinFiducial(id));
        }
    }

    if (headPts.size() == 1) {
        // Pure translation in head→MRI space
        const QVector3D delta = mriPts[0] - headPts[0];
        m_headToMri.translate(delta);
    } else if (headPts.size() >= 2) {
        // SVD-based rigid body alignment (Procrustes)
        Eigen::MatrixXf src(headPts.size(), 3);
        Eigen::MatrixXf dst(mriPts.size(), 3);
        for (int i = 0; i < headPts.size(); ++i) {
            src(i, 0) = headPts[i].x(); src(i, 1) = headPts[i].y(); src(i, 2) = headPts[i].z();
            dst(i, 0) = mriPts[i].x();  dst(i, 1) = mriPts[i].y();  dst(i, 2) = mriPts[i].z();
        }
        Eigen::Matrix4f matTrans;
        if (MNELIB::fitMatchedPoints(src, dst, matTrans)) {
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    m_headToMri(r, c) = matTrans(r, c);
        }
    }
}

void Align3DView::applyViewConfiguration()
{
    if (!m_pBrainView) {
        return;
    }

    // Read the authoritative state from BrainView itself — setters that
    // call us have already pushed their values down.
    const int viewCount       = qBound(1, m_pBrainView->viewCount(), 4);
    const QString renderMode  = m_pBrainView->shaderModeForTarget(-1);

    // Default per-pane preset assignment for the 2x2 multi-view grid.
    // Slot 0 mirrors the user-selected camera; the remaining slots show
    // complementary orthographic angles useful for fiducial alignment.
    static constexpr int kFallbackMultiViewPresets[4] = {1, 2, 3, 0};

    m_pBrainView->resetSingleViewCameraState();
    m_pBrainView->setInitialCameraRotation(multiViewPresetOffset(m_cameraPreset));

    if (viewCount > 1) {
        for (int i = 0; i < viewCount; ++i) {
            const int preset = (i == 0) ? m_cameraPreset : kFallbackMultiViewPresets[i];
            m_pBrainView->resetViewportCameraState(i);
            m_pBrainView->setViewportCameraPreset(i, preset);
        }
        m_pBrainView->resetMultiViewLayout();
    }

    // Apply alignment-specific shader/overlay to single-view + every pane.
    auto applyShaders = [this, &renderMode](int target) {
        m_pBrainView->setVisualizationEditTarget(target);
        m_pBrainView->setShaderMode(renderMode);
        m_pBrainView->setBemShaderMode(renderMode);
        m_pBrainView->setVisualizationMode(QStringLiteral("Scientific"));
    };

    applyShaders(-1);
    for (int i = 0; i < viewCount && i < 4; ++i) {
        applyShaders(i);
    }

    m_pBrainView->setVisualizationEditTarget(viewCount > 1 ? 0 : -1);
}
