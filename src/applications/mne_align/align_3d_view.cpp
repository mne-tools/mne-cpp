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

#include <disp3D/core/viewstate.h>
#include <disp3D/model/braintreemodel.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/view/brainview.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <mne/mne_bem.h>

#include <QVBoxLayout>

using namespace MNEALIGN;
using DISP3DLIB::MultimodalScene;
using DISP3DLIB::SceneLayer;
using DISP3DLIB::SceneLayerKind;

namespace {
constexpr const char* kBemLayerId       = "head_bem";
constexpr const char* kAcquiredLayerId  = "acquired_points";
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

    // Wipe any per-pane SubView state persisted by previous runs (or by
    // other applications that share the BrainView QSettings group). Without
    // this, a stale `visibility.bemHead = false` for one of the panes
    // leaves that pane blank even though the camera/preset are correct.
    m_pBrainView->resetAllSubViewState();

    m_pBrainModel = new BrainTreeModel(m_pBrainView);
    m_pBrainView->setModel(m_pBrainModel);

    applyViewConfiguration();
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
    m_viewCount = qBound(1, count, 4);
    applyViewConfiguration();
}

void Align3DView::setRenderMode(const QString& modeName)
{
    if (modeName != QLatin1String("Anatomical") && modeName != QLatin1String("Holographic")) {
        return;
    }

    m_renderMode = modeName;
    applyViewConfiguration();
}

void Align3DView::setCameraPreset(int preset)
{
    m_cameraPreset = qBound(0, preset, 6);
    applyViewConfiguration();
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

//=============================================================================================================

void Align3DView::onPointsChanged()
{
    rebuildAcquiredLayer();
    rebuildDigitizerLayer();
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
            case 4: name = QStringLiteral("head");        break;
            case 3: name = QStringLiteral("outer_skull"); break;
            case 1: name = QStringLiteral("inner_skull"); break;
            default: name = QString::number(i);           break;
        }
        m_pBrainModel->addBemSurface(QStringLiteral("align"), name, (*m_pBem)[i]);
    }
}

void Align3DView::rebuildDigitizerLayer()
{
    if (!m_pBrainView || !m_pBrainModel) {
        return;
    }

    if (!m_pPoints || m_pPoints->points().isEmpty()) {
        m_pBrainView->clearSensors();
        return;
    }

    QList<FIFFLIB::FiffDigPoint> digitizerPoints;
    digitizerPoints.reserve(m_pPoints->points().size());

    for (const auto& point : m_pPoints->points()) {
        FIFFLIB::FiffDigPoint dig;
        dig.coord_frame = FIFFV_COORD_HEAD;
        dig.r[0] = point.position.x();
        dig.r[1] = point.position.y();
        dig.r[2] = point.position.z();

        switch (point.kind) {
            case PointKind::Fiducial:
                dig.kind = FIFFV_POINT_CARDINAL;
                dig.ident = point.identNumber;
                break;
            case PointKind::Eeg:
                dig.kind = FIFFV_POINT_EEG;
                dig.ident = point.identNumber;
                break;
            case PointKind::HeadShape:
                dig.kind = FIFFV_POINT_EXTRA;
                dig.ident = point.identNumber;
                break;
        }

        digitizerPoints.append(dig);
    }

    m_pBrainView->clearSensors();
    m_pBrainModel->addDigitizerData(digitizerPoints);
}

void Align3DView::applyViewConfiguration()
{
    if (!m_pBrainView) {
        return;
    }

    // Default per-pane preset assignment for the 2x2 multi-view grid.
    // Slot 0 mirrors the user-selected camera; the remaining slots show
    // complementary orthographic angles useful for fiducial alignment.
    static constexpr int kFallbackMultiViewPresets[4] = {1, 2, 3, 0};

    m_pBrainView->setViewCount(m_viewCount);
    m_pBrainView->resetSingleViewCameraState();
    m_pBrainView->setInitialCameraRotation(multiViewPresetOffset(m_cameraPreset));

    if (m_viewCount > 1) {
        for (int i = 0; i < m_viewCount; ++i) {
            const int preset = (i == 0) ? m_cameraPreset : kFallbackMultiViewPresets[i];
            m_pBrainView->resetViewportCameraState(i);
            m_pBrainView->setViewportCameraPreset(i, preset);
        }
        m_pBrainView->resetMultiViewLayout();
    }

    // Apply alignment-specific shader/overlay to single-view + every pane.
    auto applyShaders = [this](int target) {
        m_pBrainView->setVisualizationEditTarget(target);
        m_pBrainView->setShaderMode(m_renderMode);
        m_pBrainView->setBemShaderMode(m_renderMode);
        m_pBrainView->setVisualizationMode(QStringLiteral("Scientific"));
    };

    applyShaders(-1);
    for (int i = 0; i < m_viewCount && i < 4; ++i) {
        applyShaders(i);
    }

    m_pBrainView->setVisualizationEditTarget(m_viewCount > 1 ? 0 : -1);
}
