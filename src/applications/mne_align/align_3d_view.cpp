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

#include <disp3D/scene/multimodalscene.h>
#include <mne/mne_bem.h>

#include <QLabel>
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

    m_pStatusLabel = new QLabel(this);
    m_pStatusLabel->setAlignment(Qt::AlignCenter);
    m_pStatusLabel->setTextFormat(Qt::RichText);
    m_pStatusLabel->setMinimumSize(320, 240);
    m_pStatusLabel->setStyleSheet(
        QStringLiteral("background-color: #1a1a1a; color: #cfcfcf; padding: 8px;"));
    lay->addWidget(m_pStatusLabel);

    if (m_pPoints) {
        connect(m_pPoints, &AcquiredPoints::pointsChanged,
                this, &Align3DView::onPointsChanged);
    }
    refreshStatus();
}

Align3DView::~Align3DView() = default;

MultimodalScene* Align3DView::scene() const
{
    return m_pScene.get();
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
    refreshStatus();
}

//=============================================================================================================

void Align3DView::onPointsChanged()
{
    rebuildAcquiredLayer();
    refreshStatus();
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

void Align3DView::refreshStatus()
{
    if (!m_pStatusLabel) return;

    const int nLayers = static_cast<int>(m_pScene->layers().size());
    const int nPoints = m_pPoints ? m_pPoints->points().size() : 0;
    const QString bemTxt = m_pBem ? QStringLiteral("loaded") : QStringLiteral("(none)");

    m_pStatusLabel->setText(QStringLiteral(
        "<h3>MNE Align 3-D viewer</h3>"
        "<p>BEM: <b>%1</b><br>"
        "Acquired points: <b>%2</b><br>"
        "Scene layers: <b>%3</b></p>"
        "<p><i>Renderer not yet wired — placeholder view.</i></p>")
        .arg(bemTxt)
        .arg(nPoints)
        .arg(nLayers));
}
