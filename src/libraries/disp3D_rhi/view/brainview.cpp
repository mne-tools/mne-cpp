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
#include "brainrenderer.h"
#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "renderable/networkobject.h"
#include "core/surfacekeys.h"
#include "core/dataloader.h"
#include "input/raypicker.h"
#include "geometry/meshfactory.h"
#include "model/braintreemodel.h"
#include "model/items/surfacetreeitem.h"

#include <rhi/qrhi.h>
#include "model/items/bemtreeitem.h"
#include "model/items/sensortreeitem.h"
#include "model/items/dipoletreeitem.h"
#include "model/items/sourcespacetreeitem.h"
#include "model/items/digitizertreeitem.h"\n#include "helpers/field_map.h"

#include <Eigen/Dense>
#include <QMatrix4x4>
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QFrame>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QCoreApplication>
#include <QMenu>
#include <QStandardItem>
#include <algorithm>
#include <cmath>

#include <mne/mne_bem.h>
#include <mne/mne_sourcespace.h>
#include <fiff/fiff_evoked_set.h>
#include <connectivity/network/network.h>

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

BrainView::BrainView(QWidget *parent)
    : QRhiWidget(parent)
{
    setMinimumSize(800, 600);
    setSampleCount(1);

#if defined(WASMBUILD) || defined(__EMSCRIPTEN__)
    setApi(Api::OpenGL);  // WebGL 2 (OpenGL ES 3.0) on WASM
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
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
    m_fpsLabel->setText("FPS: --.-\nVertices: 0");
    m_fpsLabel->adjustSize();
    m_fpsLabel->move(width() - m_fpsLabel->width() - 10, 10);
    m_fpsLabel->raise();

    m_singleViewInfoLabel = new QLabel(this);
    m_singleViewInfoLabel->setStyleSheet("color: white; font-family: monospace; font-size: 10px; background: rgba(0,0,0,110); border-radius: 3px; padding: 2px 4px;");
    m_singleViewInfoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_singleViewInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_singleViewInfoLabel->setText("");
    m_singleViewInfoLabel->adjustSize();
    m_singleViewInfoLabel->hide();

    m_fpsTimer.start();

    m_regionLabel = new QLabel(this);
    m_regionLabel->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 16px; background: transparent; padding: 5px;");
    m_regionLabel->setText("");
    m_regionLabel->move(10, 10);
    m_regionLabel->resize(300, 30);
    m_regionLabel->hide();

    // ── Initialise viewport labels (sized to kDefaultViewportCount) ────────
    m_subViews.resize(kDefaultViewportCount);
    m_viewportNameLabels.resize(kDefaultViewportCount, nullptr);
    m_viewportInfoLabels.resize(kDefaultViewportCount, nullptr);
    for (int i = 0; i < kDefaultViewportCount; ++i) {
        m_subViews[i] = SubView::defaultForIndex(i);

        m_viewportNameLabels[i] = new QLabel(this);
        m_viewportNameLabels[i]->setStyleSheet("color: white; font-weight: bold; font-family: sans-serif; font-size: 12px; background: transparent; padding: 2px 4px;");
        m_viewportNameLabels[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_viewportNameLabels[i]->setText(multiViewPresetName(m_subViews[i].preset));
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

    loadMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();

    // Setup Debug Pointer: Semi-transparent sphere for subtle intersection indicator
    m_debugPointerSurface = MeshFactory::createSphere(QVector3D(0, 0, 0), 0.002f,
                                                       QColor(200, 255, 255, 160));

    // ── Connect SourceEstimateManager signals ─────────────────────────
    connect(&m_sourceManager, &SourceEstimateManager::loaded,
            this, &BrainView::onSourceEstimateLoaded);
    connect(&m_sourceManager, &SourceEstimateManager::thresholdsUpdated,
            this, &BrainView::sourceThresholdsUpdated);
    connect(&m_sourceManager, &SourceEstimateManager::timePointChanged,
            this, &BrainView::timePointChanged);
    connect(&m_sourceManager, &SourceEstimateManager::loadingProgress,
            this, &BrainView::stcLoadingProgress);
    connect(&m_sourceManager, &SourceEstimateManager::realtimeColorsAvailable,
            this, &BrainView::onRealtimeColorsAvailable);

    // RtSensorStreamManager → BrainView
    connect(&m_sensorStreamManager, &RtSensorStreamManager::colorsAvailable,
            this, &BrainView::onSensorStreamColorsAvailable);
}

//=============================================================================================================

BrainView::~BrainView()
{
    saveMultiViewSettings();
}

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

             m_itemSurfaceMap[item] = brainSurf;

             // Legacy map support (Use item text e.g. "bem_head")
             m_surfaces["bem_" + bemItem->text()] = brainSurf;
        }

        // Handle Sensor Items
        if (absItem && absItem->type() == AbstractTreeItem::SensorItem + QStandardItem::UserType) {
            SensorTreeItem* sensItem = static_cast<SensorTreeItem*>(absItem);

            std::shared_ptr<BrainSurface> brainSurf;

            QString parentText = "";
            if (sensItem->parent()) parentText = sensItem->parent()->text();

            if (parentText.contains("MEG/Grad") && sensItem->hasOrientation()) {
                brainSurf = MeshFactory::createBarbell(sensItem->position(), sensItem->orientation(),
                                                       sensItem->color(), sensItem->scale());
            } else if (parentText.contains("MEG/Mag") && sensItem->hasOrientation()) {
                brainSurf = MeshFactory::createPlate(sensItem->position(), sensItem->orientation(),
                                                     sensItem->color(), sensItem->scale());
            } else {
                // EEG and other sensors: smooth icosphere
                brainSurf = MeshFactory::createSphere(sensItem->position(), sensItem->scale(),
                                                      sensItem->color());
            }

            brainSurf->setVisible(sensItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;

            // Apply Head-to-MRI transformation if available
            // Note: meg positions in info might already be head-space, but check if we need this global trans
            if (!m_headToMriTrans.isEmpty()) {
                QMatrix4x4 m;
                if (m_applySensorTrans) {
                    m = SurfaceKeys::toQMatrix4x4(m_headToMriTrans.trans);
                }
                brainSurf->applyTransform(m);
            }

            // Legacy map support
            const QString keyPrefix = SurfaceKeys::sensorParentToKeyPrefix(parentText);

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

            auto brainSurf = MeshFactory::createBatchedSpheres(positions, srcItem->scale(),
                                                                srcItem->color());
            brainSurf->setVisible(srcItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;

            QString key = "srcsp_" + srcItem->text();
            m_surfaces[key] = brainSurf;
            qDebug() << "BrainView: Created batched source space mesh" << key
                     << "with" << positions.size() << "points";
        }

        // Handle Digitizer Items (batched sphere mesh per category)
        if (absItem && absItem->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType) {
            DigitizerTreeItem* digItem = static_cast<DigitizerTreeItem*>(absItem);
            const QVector<QVector3D>& positions = digItem->positions();
            if (positions.isEmpty()) continue;

            auto brainSurf = MeshFactory::createBatchedSpheres(positions, digItem->scale(),
                                                                digItem->color());
            brainSurf->setVisible(digItem->isVisible());

            // Apply Head-to-MRI transformation if available
            if (!m_headToMriTrans.isEmpty()) {
                QMatrix4x4 m;
                if (m_applySensorTrans) {
                    m = SurfaceKeys::toQMatrix4x4(m_headToMriTrans.trans);
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
                     << "with" << positions.size() << "points";
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

void BrainView::setActiveSurface(const QString &type)
{
    qDebug() << "[setActiveSurface] type=" << type
             << "editTarget=" << m_visualizationEditTarget;

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
    m_visualizationEditTarget = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);

    const SubView &sv = subViewForTarget(m_visualizationEditTarget);
    m_activeSurfaceType = sv.surfaceType;
    m_brainShaderMode   = sv.brainShader;
    m_bemShaderMode     = sv.bemShader;
    m_currentVisMode    = sv.overlayMode;
    const ViewVisibilityProfile &visibility = sv.visibility;

    const bool remapMegSurface = (m_fieldMapper.megFieldMapOnHead() != visibility.megFieldMapOnHead);
    m_fieldMapper.setMegFieldMapOnHead(visibility.megFieldMapOnHead);
    m_dipolesVisible = visibility.dipoles;
    m_networkVisible = visibility.network;

    for (auto surf : m_surfaces) {
        surf->setVisualizationMode(m_currentVisMode);
    }

    if (m_fieldMapper.isLoaded()) {
        if (remapMegSurface) {
            m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        }
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
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

ViewVisibilityProfile& BrainView::visibilityProfileForTarget(int target)
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

const ViewVisibilityProfile& BrainView::visibilityProfileForTarget(int target) const
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

SubView& BrainView::subViewForTarget(int target)
{
    const int normalized = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

const SubView& BrainView::subViewForTarget(int target) const
{
    const int normalized = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

// Note: SubView::isBrainSurfaceKey, matchesSurfaceType, shouldRenderSurface,
// and applyOverlayToSurfaces are defined in core/viewstate.cpp.

//=============================================================================================================

bool BrainView::objectVisibleForTarget(const QString &object, int target) const
{
    return visibilityProfileForTarget(target).isObjectVisible(object);
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
                               || std::any_of(m_subViews.cbegin(), m_subViews.cend(),
                                    [](const SubView &sv) { return sv.surfaceType == "inflated"; });

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
    const BrainRenderer::ShaderMode mode = shaderModeFromName(modeName);

    subViewForTarget(m_visualizationEditTarget).bemShader = mode;

    m_bemShaderMode = mode;
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::syncBemShadersToBrainShaders()
{
    m_singleView.bemShader = m_singleView.brainShader;
    for (int i = 0; i < m_subViews.size(); ++i) {
        m_subViews[i].bemShader = m_subViews[i].brainShader;
    }

    m_bemShaderMode = subViewForTarget(m_visualizationEditTarget).bemShader;

    saveMultiViewSettings();
    update();
}

void BrainView::setSensorVisible(const QString &type, bool visible)
{
    const QString object = SurfaceKeys::sensorTypeToObjectKey(type);
    if (object.isEmpty()) return;

    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.setObjectVisible(object, visible);
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
    SubView &sv = subViewForTarget(m_visualizationEditTarget);
    sv.overlayMode = mode;

    m_currentVisMode = mode;

    // Propagate the mode to brain hemisphere surfaces only (lh_*, rh_*)
    // so that the primary colour channel holds the right data: curvature
    // grays for Scientific or STC colours for SourceEstimate.
    // BEM, sensor, and source-space surfaces are left untouched.
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        const QString &key = it.key();
        if (key.startsWith("lh_") || key.startsWith("rh_")) {
            it.value()->setVisualizationMode(mode);
        }
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
    profile.setObjectVisible("bem_" + name, visible);
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

void BrainView::setViewCount(int count)
{
    count = std::clamp(count, 1, static_cast<int>(m_subViews.size()));
    m_viewCount = count;

    if (count == 1) {
        m_viewMode = SingleView;
        m_isDraggingSplitter = false;
        m_activeSplitter = SplitterHit::None;
        unsetCursor();
        setVisualizationEditTarget(-1);
    } else {
        m_viewMode = MultiView;
        // Default edit target to first pane when entering multi-view
        if (m_visualizationEditTarget < 0)
            setVisualizationEditTarget(0);
    }

    // Enable first N sub-views, disable the rest
    for (int i = 0; i < m_subViews.size(); ++i)
        m_subViews[i].enabled = (i < count);

    saveMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();
    update();
}

//=============================================================================================================

void BrainView::resetMultiViewLayout()
{
    m_layout.resetSplits();
    m_multiSplitX = m_layout.splitX();
    m_multiSplitY = m_layout.splitY();
    saveMultiViewSettings();
    updateViewportSeparators();
    updateOverlayLayout();
    update();
}

bool BrainView::isViewportEnabled(int index) const
{
    if (index < 0 || index >= m_subViews.size()) {
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
    for (int i = 0; i < m_subViews.size(); ++i) {
        if (m_subViews[i].enabled) {
            ++numEnabled;
        }
    }

    return numEnabled > 0 ? numEnabled : 1;
}

//=============================================================================================================

QVector<int> BrainView::enabledViewportIndices() const
{
    QVector<int> vps;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < m_subViews.size(); ++i) {
            if (m_subViews[i].enabled)
                vps.append(i);
        }
        if (vps.isEmpty())
            vps.append(0);
    } else {
        vps.append(0);
    }
    return vps;
}

//=============================================================================================================

int BrainView::viewportIndexAt(const QPoint& pos) const
{
    if (m_viewMode != MultiView) {
        return 0;
    }

    const auto enabledViewports = enabledViewportIndices();
    return m_layout.viewportIndexAt(pos, enabledViewports, size());
}

//=============================================================================================================

QRect BrainView::multiViewSlotRect(int slot, int numEnabled, const QSize& outputSize) const
{
    return m_layout.slotRect(slot, numEnabled, outputSize);
}

//=============================================================================================================

SplitterHit BrainView::hitTestSplitter(const QPoint& pos, int numEnabled, const QSize& outputSize) const
{
    if (m_viewMode != MultiView || numEnabled <= 1) {
        return SplitterHit::None;
    }
    return m_layout.hitTestSplitter(pos, numEnabled, outputSize);
}

//=============================================================================================================

void BrainView::updateSplitterCursor(const QPoint& pos)
{
    const SplitterHit hit = hitTestSplitter(pos, enabledViewportCount(), size());
    const Qt::CursorShape shape = MultiViewLayout::cursorForHit(hit);
    if (shape == Qt::ArrowCursor) {
        unsetCursor();
    } else {
        setCursor(shape);
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

    QRect vRect, hRect;
    m_layout.separatorGeometries(numEnabled, size(), vRect, hRect);

    if (!vRect.isEmpty()) {
        m_verticalSeparator->setGeometry(vRect);
        m_verticalSeparator->show();
        m_verticalSeparator->raise();
    }
    if (!hRect.isEmpty()) {
        m_horizontalSeparator->setGeometry(hRect);
        m_horizontalSeparator->show();
        m_horizontalSeparator->raise();
    }

    updateOverlayLayout();
}

//=============================================================================================================

void BrainView::updateOverlayLayout()
{
    const auto enabledViewports = enabledViewportIndices();

    if (m_fpsLabel) {
        m_fpsLabel->setVisible(m_infoPanelVisible);
        m_fpsLabel->adjustSize();
        const int perfBottomMargin = 2;

        if (m_viewMode == MultiView) {
            m_fpsLabel->move(width() - m_fpsLabel->width() - 10,
                             height() - m_fpsLabel->height() - perfBottomMargin);
        } else {
            m_fpsLabel->move(width() - m_fpsLabel->width() - 10,
                             height() - m_fpsLabel->height() - perfBottomMargin);
        }

        m_fpsLabel->raise();
    }

    if (m_singleViewInfoLabel) {
        const bool showSingleInfo = (m_viewMode == SingleView) && m_infoPanelVisible;
        m_singleViewInfoLabel->setVisible(showSingleInfo);
        if (showSingleInfo) {
            m_singleViewInfoLabel->adjustSize();
            m_singleViewInfoLabel->move(width() - m_singleViewInfoLabel->width() - 8, 8);
            m_singleViewInfoLabel->raise();
        }
    }

    if (m_regionLabel) {
        const int regionY = (m_viewMode == MultiView) ? 38 : 10;
        m_regionLabel->move(10, regionY);
        if (!m_regionLabel->text().isEmpty()) {
            m_regionLabel->raise();
        }
    }

    for (int i = 0; i < m_viewportNameLabels.size(); ++i) {
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
        label->setVisible(true);
        label->raise();

        if (infoLabel) {
            infoLabel->adjustSize();
            infoLabel->move(pane.x() + pane.width() - infoLabel->width() - 8,
                            pane.y() + 8);
            infoLabel->setVisible(m_infoPanelVisible);
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

    for (int i = 0; i < m_viewportNameLabels.size(); ++i) {
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
    m_viewCount = std::clamp(settings.value("viewCount", 1).toInt(), 1, static_cast<int>(m_subViews.size()));
    // Reconcile: viewCount > 1 implies MultiView
    if (m_viewCount > 1) m_viewMode = MultiView;
    else m_viewMode = SingleView;

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

    // Reset per-index defaults, then load saved state on top
    for (int i = 0; i < m_subViews.size(); ++i) {
        m_subViews[i] = SubView::defaultForIndex(i);
        m_subViews[i].enabled = (i < m_viewCount);
    }

    // Delegate per-SubView serialization
    m_singleView.load(settings, "single_", m_cameraRotation);
    for (int i = 0; i < m_subViews.size(); ++i)
        m_subViews[i].load(settings, QStringLiteral("multi%1_").arg(i), m_cameraRotation);

    const int maxIdx = static_cast<int>(m_subViews.size()) - 1;
    m_visualizationEditTarget = normalizedVisualizationTarget(
        settings.value("visualizationEditTarget", -1).toInt(), maxIdx);

    settings.endGroup();

    m_multiSplitX = std::clamp(m_multiSplitX, 0.15f, 0.85f);
    m_multiSplitY = std::clamp(m_multiSplitY, 0.15f, 0.85f);
    m_layout.setSplitX(m_multiSplitX);
    m_layout.setSplitY(m_multiSplitY);

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
    settings.setValue("viewCount", m_viewCount);
    settings.setValue("cameraRotW", m_cameraRotation.scalar());
    settings.setValue("cameraRotX", m_cameraRotation.x());
    settings.setValue("cameraRotY", m_cameraRotation.y());
    settings.setValue("cameraRotZ", m_cameraRotation.z());
    for (int i = 0; i < m_subViews.size(); ++i)
        settings.setValue(QStringLiteral("viewportEnabled%1").arg(i), m_subViews[i].enabled);
    settings.setValue("visualizationEditTarget", m_visualizationEditTarget);

    // Delegate per-SubView serialization
    m_singleView.save(settings, "single_");
    for (int i = 0; i < m_subViews.size(); ++i)
        m_subViews[i].save(settings, QStringLiteral("multi%1_").arg(i));

    settings.endGroup();
}

//=============================================================================================================

void BrainView::setViewportEnabled(int index, bool enabled)
{
    if (index >= 0 && index < m_subViews.size()) {
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
        auto countVerticesForSubView = [this](const SubView &sv) -> qint64 {
            qint64 total = 0;

            for (auto it = m_surfaces.cbegin(); it != m_surfaces.cend(); ++it) {
                const QString &key = it.key();
                auto surface = it.value();
                if (!surface) {
                    continue;
                }

                if (!sv.shouldRenderSurface(key)) {
                    continue;
                }

                if (SubView::isBrainSurfaceKey(key)) {
                    if (!sv.matchesSurfaceType(key)) {
                        continue;
                    }
                } else {
                    if (!surface->isVisible()) {
                        continue;
                    }
                }

                total += surface->vertexCount();
            }

            return total;
        };

        qint64 vCount = 0;
        if (m_viewMode == MultiView) {
            for (int vp : enabledViewportIndices()) {
                vCount += countVerticesForSubView(m_subViews[vp]);
            }
        } else {
            vCount = countVerticesForSubView(m_singleView);
        }

        m_fpsLabel->setText(QString("FPS: %1\nVertices: %2").arg(fps, 0, 'f', 1).arg(vCount));
        updateOverlayLayout();
        m_fpsLabel->raise();
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    // Initialize renderer
    m_renderer->initialize(rhi(), renderTarget()->renderPassDescriptor(), sampleCount());

    // Determine viewport configuration
    QSize outputSize = renderTarget()->pixelSize();

    // Build list of enabled viewports
    const auto enabledViewports = enabledViewportIndices();
    int numEnabled = enabledViewports.size();

    // ── Pre-render phase ────────────────────────────────────────────────
    // Apply per-pane overlay modes and pre-upload ALL Immutable GPU buffers
    // BEFORE the render pass starts.  On Metal, uploading an Immutable
    // buffer during an active render pass forces a pass restart which
    // resets the viewport state, causing subsequent draws to cover the
    // full framebuffer instead of the intended pane.
    //
    // By doing all static uploads here (outside any render pass), we
    // guarantee that the draw loop below only records Dynamic uniform
    // updates — those never interrupt the pass.

    // Pre-upload every surface and dipole buffer that is dirty or new.
    // NOTE: Overlay modes are applied per-pane inside the render loop below
    // (not here), because different panes can have different overlays on the
    // same shared BrainSurface objects.  Applying all pane overlays
    // sequentially here would leave only the last pane's vertex colours.
    {
        QRhiResourceUpdateBatch *preUpload = rhi()->nextResourceUpdateBatch();
        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            it.value()->updateBuffers(rhi(), preUpload);
        }
        if (m_debugPointerSurface) {
            m_debugPointerSurface->updateBuffers(rhi(), preUpload);
        }
        for (auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
            it.value()->updateBuffers(rhi(), preUpload);
        }
        if (m_dipoles) {
            m_dipoles->updateBuffers(rhi(), preUpload);
        }
        // Network buffers are updated inside renderNetwork() via updateNodeBuffers/updateEdgeBuffers
        cb->resourceUpdate(preUpload);
    }

    // ── Render pass ─────────────────────────────────────────────────────
    m_renderer->beginFrame(cb, renderTarget());

    for (int slot = 0; slot < numEnabled; ++slot) {
        int vp = (m_viewMode == MultiView) ? enabledViewports[slot] : 0;
        const SubView &sv = (m_viewMode == MultiView) ? m_subViews[vp] : m_singleView;
        const int preset = (m_viewMode == MultiView) ? std::clamp(sv.preset, 0, 6) : 1;

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
            } else if (numEnabled == 3) {
                // 3-view: slot 0 = full top row, slots 1&2 = bottom row
                if (slot == 0) {
                    // Top pane: no right neighbor, has bottom neighbor
                    renderRect.setHeight(std::max(1, renderRect.height() - separatorPx));
                } else if (slot == 1) {
                    // Bottom-left: has right neighbor, no bottom neighbor
                    renderRect.setWidth(std::max(1, renderRect.width() - separatorPx));
                }
                // slot 2 (bottom-right): no insets needed
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
        QRhiScissor scissor(viewX, viewY, viewW, viewH);
        const float aspectRatio = float(viewW) / float(viewH);

        // Set viewport and scissor
        cb->setViewport(viewport);
        cb->setScissor(scissor);

        // Calculate camera for this viewport
        m_camera.setSceneCenter(m_sceneCenter);
        m_camera.setSceneSize(m_sceneSize);
        m_camera.setRotation(m_cameraRotation);
        m_camera.setZoom(m_zoom);
        const CameraResult cam = (m_viewMode == MultiView)
            ? m_camera.computeMultiView(sv, aspectRatio)
            : m_camera.computeSingleView(aspectRatio);

        BrainRenderer::SceneData sceneData;
        sceneData.mvp = rhi()->clipSpaceCorrMatrix();
        sceneData.mvp *= cam.projection;
        sceneData.mvp *= cam.view;
        sceneData.mvp *= cam.model;

        sceneData.cameraPos = cam.cameraPos;
        sceneData.lightDir = cam.cameraPos.normalized();
        sceneData.lightingEnabled = m_lightingEnabled;
        sceneData.viewportX = viewX;
        sceneData.viewportY = viewY;
        sceneData.viewportW = viewW;
        sceneData.viewportH = viewH;
        sceneData.scissorX = viewX;
        sceneData.scissorY = viewY;
        sceneData.scissorW = viewW;
        sceneData.scissorH = viewH;

    // Per-draw overlayMode uniform — the shader selects the vertex colour
    // channel (curvature / annotation) so no per-pane vertex buffer
    // re-uploads are needed.
    sceneData.overlayMode = static_cast<float>(sv.overlayMode);

    // Pass 1: Opaque Surfaces (Brain surfaces)
    // Use viewport-specific shader from subview
    BrainRenderer::ShaderMode currentShader = sv.brainShader;
    BrainRenderer::ShaderMode currentBemShader = sv.bemShader;
    const QString overlayName = visualizationModeName(sv.overlayMode);

    // Collect matched brain surface keys for this pane's info panel
    QStringList drawnKeys;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!sv.matchesSurfaceType(it.key())) continue;
        if (!sv.shouldRenderSurface(it.key())) continue;
        drawnKeys << it.key();
    }
    const QString drawnInfo = drawnKeys.isEmpty() ? "none" : drawnKeys.join(", ");

    if (m_viewMode == MultiView && m_viewportInfoLabels[vp]) {
        m_viewportInfoLabels[vp]->setText(
            QString("Shader: %1\nSurface: %2\nOverlay: %3\nDrawn: %4")
                .arg(shaderModeName(currentShader), sv.surfaceType, overlayName, drawnInfo));
    } else if (m_viewMode == SingleView && m_singleViewInfoLabel) {
        m_singleViewInfoLabel->setText(
            QString("Shader: %1\nSurface: %2\nOverlay: %3\nDrawn: %4")
                .arg(shaderModeName(currentShader), sv.surfaceType, overlayName, drawnInfo));
    }

    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!sv.matchesSurfaceType(it.key())) continue;
        if (!sv.shouldRenderSurface(it.key())) continue;

        m_renderer->renderSurface(cb, rhi(), sceneData, it.value().get(), currentShader);
    }

    // Pass 1b: Source Space Points (use same shader as brain for consistent depth/blend)
    // These use their own vertex colour, so force overlayMode to pass-through (Scientific)
    BrainRenderer::SceneData nonBrainSceneData = sceneData;
    nonBrainSceneData.overlayMode = static_cast<float>(BrainSurface::ModeScientific);

    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("srcsp_")) continue;
        if (!sv.shouldRenderSurface(it.key())) continue;
        if (!it.value()->isVisible()) continue;
        m_renderer->renderSurface(cb, rhi(), nonBrainSceneData, it.value().get(), currentShader);
    }

    // Pass 1c: Digitizer Points (opaque small spheres, render like source space)
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.key().startsWith("dig_")) continue;
        if (!sv.shouldRenderSurface(it.key())) continue;
        if (!it.value()->isVisible()) continue;
        m_renderer->renderSurface(cb, rhi(), nonBrainSceneData, it.value().get(), currentShader);
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
        if (!sv.shouldRenderSurface(it.key())) continue;
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

    // BEM / sensor surfaces pass vertex colours through via Scientific mode
    // so that BEM Red/Green/Blue colours (set via setUseDefaultColor) are
    // visible.  The holographic shader uses saturation detection: coloured
    // BEM → data mode, white BEM → shell mode.  The anatomical shader
    // ignores overlayMode for known tissue types and uses its own palette.
    BrainRenderer::SceneData bemSceneData = sceneData;
    bemSceneData.overlayMode = static_cast<float>(BrainSurface::ModeScientific);

    for (const auto &item : transparentItems) {
        m_renderer->renderSurface(cb, rhi(), bemSceneData, item.surf, item.mode);
    }

    // Render Dipoles
    for(auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
        if (it.value()->isVisible() && sv.visibility.dipoles) {
             m_renderer->renderDipoles(cb, rhi(), sceneData, it.value().get());
        }
    }

    if (sv.visibility.dipoles && m_dipoles) {
        m_renderer->renderDipoles(cb, rhi(), sceneData, m_dipoles.get());
    }

    // Render Connectivity Network
    if (sv.visibility.network && m_network) {
        m_renderer->renderNetwork(cb, rhi(), sceneData, m_network.get());
    }

    // Intersection Pointer
    if (m_hasIntersection && m_debugPointerSurface) {
        BrainRenderer::SceneData debugSceneData = sceneData;
        debugSceneData.overlayMode = 0.0f; // pass-through for holographic shell

        QMatrix4x4 translation;
        translation.translate(m_lastIntersectionPoint);

        debugSceneData.mvp = rhi()->clipSpaceCorrMatrix() * cam.projection * cam.view * cam.model * translation;

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
                if (clickedVp != m_visualizationEditTarget) {
                    setVisualizationEditTarget(clickedVp);
                }
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
        m_layout.dragSplitter(event->pos(), m_activeSplitter, size());
        m_multiSplitX = m_layout.splitX();
        m_multiSplitY = m_layout.splitY();

        m_lastMousePos = event->pos();
        updateViewportSeparators();
        update();
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        if (m_viewMode == MultiView) {
            const int activeVp = viewportIndexAt(event->pos());
            const int activePreset = (activeVp >= 0 && activeVp < m_subViews.size())
                ? std::clamp(m_subViews[activeVp].preset, 0, 6)
                : 1;

            if (activeVp >= 0 && !multiViewPresetIsPerspective(activePreset)) {
                // Planar views (Top/Front/Left): pan along the view plane
                const QPoint diff = event->pos() - m_lastMousePos;
                CameraController::applyMousePan(diff, m_subViews[activeVp].pan, m_sceneSize);
                m_lastMousePos = event->pos();
                update();
                return;
            }

            if (activeVp >= 0 && multiViewPresetIsPerspective(activePreset)) {
                // Perspective view: rotate
                QPoint diff = event->pos() - m_lastMousePos;
                CameraController::applyMouseRotation(diff, m_subViews[activeVp].perspectiveRotation);

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
        CameraController::applyMouseRotation(diff, m_cameraRotation);

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
        if (vp >= 0 && vp < m_subViews.size()) {
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
    return m_sourceManager.load(lhPath, rhPath, m_surfaces, m_activeSurfaceType);
}

//=============================================================================================================

void BrainView::onSourceEstimateLoaded(int numTimePoints)
{
    setVisualizationMode("Source Estimate");
    emit sourceEstimateLoaded(numTimePoints);
    setTimePoint(0);
}

//=============================================================================================================

void BrainView::setTimePoint(int index)
{
    m_sourceManager.setTimePoint(index, m_surfaces, m_singleView, m_subViews);
    update();
}

//=============================================================================================================

void BrainView::setSourceColormap(const QString &name)
{
    m_sourceManager.setColormap(name);
    setTimePoint(m_sourceManager.currentTimePoint());
}

//=============================================================================================================

void BrainView::setSourceThresholds(float min, float mid, float max)
{
    m_sourceManager.setThresholds(min, mid, max);
    setTimePoint(m_sourceManager.currentTimePoint());
}

//=============================================================================================================

void BrainView::startRealtimeStreaming()
{
    setVisualizationMode("Source Estimate");
    m_sourceManager.startStreaming(m_surfaces, m_singleView, m_subViews);
}

//=============================================================================================================

void BrainView::stopRealtimeStreaming()
{
    m_sourceManager.stopStreaming();
}

//=============================================================================================================

bool BrainView::isRealtimeStreaming() const
{
    return m_sourceManager.isStreaming();
}

//=============================================================================================================

void BrainView::pushRealtimeSourceData(const Eigen::VectorXd &data)
{
    m_sourceManager.pushData(data);
}

//=============================================================================================================

void BrainView::setRealtimeInterval(int msec)
{
    m_sourceManager.setInterval(msec);
}

//=============================================================================================================

void BrainView::setRealtimeLooping(bool enabled)
{
    m_sourceManager.setLooping(enabled);
}

//=============================================================================================================

void BrainView::onRealtimeColorsAvailable(const QVector<uint32_t> &colorsLh,
                                           const QVector<uint32_t> &colorsRh)
{
    // Apply colors to all brain surfaces matching active surface types
    QSet<QString> activeTypes;
    activeTypes.insert(m_singleView.surfaceType);
    for (int i = 0; i < m_subViews.size(); ++i) {
        activeTypes.insert(m_subViews[i].surfaceType);
    }

    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it.value() || it.value()->tissueType() != BrainSurface::TissueBrain)
            continue;

        for (const QString &type : activeTypes) {
            if (it.key().endsWith(type)) {
                int hemi = it.value()->hemi();
                const QVector<uint32_t> &colors = (hemi == 0) ? colorsLh : colorsRh;
                if (!colors.isEmpty()) {
                    it.value()->applySourceEstimateColors(colors);
                }
                break;
            }
        }
    }

    update();
}

//=============================================================================================================

bool BrainView::loadSensorField(const QString &evokedPath, int aveIndex)
{
    auto evoked = DataLoader::loadEvoked(evokedPath, aveIndex);
    if (evoked.isEmpty()) return false;

    m_fieldMapper.setEvoked(evoked);
    m_fieldMapper.setTimePoint(0);

    if (!m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans)) {
        m_fieldMapper.setEvoked(FIFFLIB::FiffEvoked());  // Clear state on failure
        return false;
    }

    emit sensorFieldLoaded(m_fieldMapper.evoked().times.size());
    setSensorFieldTimePoint(0);
    return true;
}

//=============================================================================================================

QStringList BrainView::probeEvokedSets(const QString &evokedPath)
{
    return DataLoader::probeEvokedSets(evokedPath);
}

//=============================================================================================================

void BrainView::setSensorFieldTimePoint(int index)
{
    if (!m_fieldMapper.isLoaded() || m_fieldMapper.evoked().isEmpty()) {
        return;
    }

    int maxIdx = static_cast<int>(m_fieldMapper.evoked().times.size()) - 1;
    if (maxIdx < 0) {
        return;
    }

    m_fieldMapper.setTimePoint(qBound(0, index, maxIdx));
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    emit sensorFieldTimePointChanged(m_fieldMapper.timePoint(), m_fieldMapper.evoked().times(m_fieldMapper.timePoint()));
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
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
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
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    update();
}

//=============================================================================================================

void BrainView::setMegFieldMapOnHead(bool useHead)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (profile.megFieldMapOnHead == useHead && m_fieldMapper.megFieldMapOnHead() == useHead) {
        return;
    }

    profile.megFieldMapOnHead = useHead;
    m_fieldMapper.setMegFieldMapOnHead(useHead);
    saveMultiViewSettings();
    if (m_fieldMapper.isLoaded()) {
        m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
        update();
    }
}

//=============================================================================================================

void BrainView::setSensorFieldColormap(const QString &name)
{
    if (m_fieldMapper.colormap() == name) {
        return;
    }
    m_fieldMapper.setColormap(name);
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    update();
}

//=============================================================================================================

float BrainView::stcStep() const
{
    return m_sourceManager.tstep();
}

//=============================================================================================================

float BrainView::stcTmin() const
{
    return m_sourceManager.tmin();
}

//=============================================================================================================

int BrainView::stcNumTimePoints() const
{
    return m_sourceManager.numTimePoints();
}

//=============================================================================================================

int BrainView::closestSensorFieldIndex(float timeSec) const
{
    if (!m_fieldMapper.isLoaded() || m_fieldMapper.evoked().nave == -1 || m_fieldMapper.evoked().times.size() == 0) {
        return -1;
    }

    int bestIdx = 0;
    float bestDist = std::abs(m_fieldMapper.evoked().times(0) - timeSec);
    for (int i = 1; i < m_fieldMapper.evoked().times.size(); ++i) {
        float dist = std::abs(m_fieldMapper.evoked().times(i) - timeSec);
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
    return m_sourceManager.closestIndex(timeSec);
}

//=============================================================================================================

bool BrainView::sensorFieldTimeRange(float &tmin, float &tmax) const
{
    if (!m_fieldMapper.isLoaded() || m_fieldMapper.evoked().nave == -1 || m_fieldMapper.evoked().times.size() == 0) {
        return false;
    }
    tmin = m_fieldMapper.evoked().times(0);
    tmax = m_fieldMapper.evoked().times(m_fieldMapper.evoked().times.size() - 1);
    return true;
}

//=============================================================================================================
// ── Real-time sensor data streaming ────────────────────────────────────
//=============================================================================================================

void BrainView::startRealtimeSensorStreaming(const QString &modality)
{
    m_sensorStreamManager.startStreaming(modality, m_fieldMapper, m_surfaces);
}

//=============================================================================================================

void BrainView::stopRealtimeSensorStreaming()
{
    m_sensorStreamManager.stopStreaming();
}

//=============================================================================================================

bool BrainView::isRealtimeSensorStreaming() const
{
    return m_sensorStreamManager.isStreaming();
}

//=============================================================================================================

void BrainView::pushRealtimeSensorData(const Eigen::VectorXf &data)
{
    m_sensorStreamManager.pushData(data);
}

//=============================================================================================================

void BrainView::setRealtimeSensorInterval(int msec)
{
    m_sensorStreamManager.setInterval(msec);
}

//=============================================================================================================

void BrainView::setRealtimeSensorLooping(bool enabled)
{
    m_sensorStreamManager.setLooping(enabled);
}

//=============================================================================================================

void BrainView::setRealtimeSensorAverages(int numAvr)
{
    m_sensorStreamManager.setAverages(numAvr);
}

//=============================================================================================================

void BrainView::setRealtimeSensorColormap(const QString &name)
{
    m_sensorStreamManager.setColormap(name);
}

//=============================================================================================================

void BrainView::onSensorStreamColorsAvailable(const QString &surfaceKey,
                                               const QVector<uint32_t> &colors)
{
    if (surfaceKey.isEmpty() || !m_surfaces.contains(surfaceKey)) {
        return;
    }

    auto surface = m_surfaces[surfaceKey];
    if (surface && !colors.isEmpty()) {
        surface->applySourceEstimateColors(colors);
    }

    update();
}

//=============================================================================================================

bool BrainView::loadSensors(const QString &fifPath) {
    auto r = DataLoader::loadSensors(fifPath, m_megHelmetOverridePath);
    if (!r.hasInfo && !r.hasDigitizer) return false;

    if (!r.megGradItems.isEmpty()) m_model->addSensors("MEG/Grad", r.megGradItems);
    if (!r.megMagItems.isEmpty())  m_model->addSensors("MEG/Mag",  r.megMagItems);
    if (!r.eegItems.isEmpty())     m_model->addSensors("EEG",      r.eegItems);

    if (r.helmetSurface)
        m_surfaces["sens_surface_meg"] = r.helmetSurface;

    if (!r.digitizerPoints.isEmpty())
        m_model->addDigitizerData(r.digitizerPoints);

    return true;
}

//=============================================================================================================

bool BrainView::loadDipoles(const QString &dipPath)
{
    auto ecdSet = DataLoader::loadDipoles(dipPath);
    if (ecdSet.size() == 0) return false;
    m_model->addDipoles(ecdSet);
    return true;
}

//=============================================================================================================

bool BrainView::loadNetwork(const CONNECTIVITYLIB::Network &network, const QString &name)
{
    if (network.getNodes().isEmpty()) return false;

    m_network = std::make_unique<NetworkObject>();
    m_network->load(network);
    m_network->setVisible(true);

    // Also register in the tree model
    m_model->addNetwork(network, name);

    qDebug() << "BrainView: Loaded network" << name
             << "with" << network.getNodes().size() << "nodes";

    update();
    return true;
}

//=============================================================================================================

void BrainView::setNetworkVisible(bool visible)
{
    auto &profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.network = visible;
    m_networkVisible = visible;
    if (m_network) m_network->setVisible(visible);
    saveMultiViewSettings();
    update();
}

//=============================================================================================================

void BrainView::setNetworkThreshold(double threshold)
{
    if (m_network) {
        m_network->setThreshold(threshold);
        update();
    }
}

//=============================================================================================================

void BrainView::setNetworkColormap(const QString &name)
{
    if (m_network) {
        m_network->setColormap(name);
        update();
    }
}

//=============================================================================================================

bool BrainView::loadSourceSpace(const QString &fwdPath)
{
    auto srcSpace = DataLoader::loadSourceSpace(fwdPath);
    if (srcSpace.isEmpty()) return false;
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
    FiffCoordTrans trans;
    if (!DataLoader::loadHeadToMriTransform(transPath, trans))
        return false;

    m_headToMriTrans = trans;
    refreshSensorTransforms();
    return true;
}

void BrainView::refreshSensorTransforms()
{
    QMatrix4x4 qmat;
    if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
        qmat = SurfaceKeys::toQMatrix4x4(m_headToMriTrans.trans);
    }

    int surfCount = 0;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if ((it.key().startsWith("sens_") || it.key().startsWith("dig_")) && it.value()) {
            it.value()->applyTransform(qmat);
            surfCount++;
        }
    }

    if (m_fieldMapper.isLoaded()) {
        m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    }
}

//=============================================================================================================

void BrainView::castRay(const QPoint &pos)
{
    // 1. Setup Matrix Stack (Must match render exactly, including multiview pane layout)
    const QSize outputSize = size();

    const auto enabledViewports = enabledViewportIndices();

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

    m_camera.setSceneCenter(m_sceneCenter);
    m_camera.setSceneSize(m_sceneSize);
    m_camera.setRotation(m_cameraRotation);
    m_camera.setZoom(m_zoom);
    const float aspect = float(std::max(1, activePane.width())) / float(std::max(1, activePane.height()));
    const CameraResult cam = (m_viewMode == MultiView)
        ? m_camera.computeMultiView(sv, aspect)
        : m_camera.computeSingleView(aspect);
    QMatrix4x4 pvm = cam.projection * cam.view * cam.model;

    // ── Unproject screen position to world-space ray ───────────────────
    QVector3D rayOrigin, rayDir;
    if (!RayPicker::unproject(pos, activePane, pvm, rayOrigin, rayDir))
        return;

    // ── Pick against all scene geometry ────────────────────────────────
    PickResult pickResult;
    if (hasValidPane) {
        pickResult = RayPicker::pick(rayOrigin, rayDir, sv, m_surfaces, m_itemSurfaceMap, m_itemDipoleMap);
    }
    m_hasIntersection = pickResult.hit;
    if (pickResult.hit) {
        m_lastIntersectionPoint = pickResult.hitPoint;
    }

    QStandardItem *hitItem  = pickResult.item;
    int            hitIndex = pickResult.vertexIndex;

    // ── Build hover label ──────────────────────────────────────────────
    const QString displayLabel = RayPicker::buildLabel(pickResult, m_itemSurfaceMap, m_surfaces);
    const QString &hitKey      = pickResult.surfaceKey;
    int currentRegionId        = pickResult.regionId;

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
                     const int vertsPerSphere = MeshFactory::sphereVertexCount();
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
            const int vertsPerSphere = MeshFactory::sphereVertexCount();
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
    if (viewport < 0 || viewport >= m_subViews.size()) {
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
