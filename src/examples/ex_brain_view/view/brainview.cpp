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
#include "../renderable/brainsurface.h"
#include "../renderable/dipoleobject.h"
#include "../renderable/networkobject.h"
#include "../renderable/sourceestimateoverlay.h"
#include "../core/surfacekeys.h"
#include "../core/dataloader.h"
#include "../input/raypicker.h"
#include "../geometry/meshfactory.h"
#include "../model/braintreemodel.h"
#include "surfacetreeitem.h"

#include <rhi/qrhi.h>
#include "bemtreeitem.h"
#include "sensortreeitem.h"
#include "dipoletreeitem.h"
#include "sourcespacetreeitem.h"
#include "digitizertreeitem.h"
#include "../workers/stcloadingworker.h"
#include "../workers/rtsourcedatacontroller.h"
#include "../workers/rtsensordatacontroller.h"
#include "../helpers/field_map.h"

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
#include <QThread>
#include <algorithm>
#include <cmath>

#include <mne/mne_bem.h>
#include <mne/mne_sourcespace.h>
#include <fiff/fiff_evoked_set.h>
#include <connectivity/network/network.h>
#include <fs/surface.h>
#include <fiff/fiff_constants.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include <fwd/fwd_coil_set.h>
#include <disp/plots/helpers/colormap.h>

using namespace FIFFLIB;

// Anonymous-namespace helpers used only by buildSensorFieldMapping().

namespace
{

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

} // anonymous namespace

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
}

//=============================================================================================================

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

    const bool remapMegSurface = (m_megFieldMapOnHead != visibility.megFieldMapOnHead);
    m_megFieldMapOnHead = visibility.megFieldMapOnHead;
    m_dipolesVisible = visibility.dipoles;
    m_networkVisible = visibility.network;

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
    // The overlay mode is now passed as a per-draw uniform to the shader.
    // No need to mutate vertex data on the shared BrainSurface objects.
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

    // BEM / sensor surfaces use tissue-type / shell colours, not brain overlays
    BrainRenderer::SceneData bemSceneData = sceneData;
    bemSceneData.overlayMode = 0.0f; // Surface mode → anatomical uses tissue type, holographic uses shell

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
    for (int i = 0; i < m_subViews.size(); ++i) {
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

    // Also update thresholds on the real-time controller if active
    if (m_rtController) {
        m_rtController->setThresholds(min, mid, max);
    }
}

//=============================================================================================================

void BrainView::startRealtimeStreaming()
{
    if (m_isRtStreaming) {
        qDebug() << "BrainView: Real-time streaming already active";
        return;
    }

    // Require loaded STC data and interpolation matrices
    if (!m_sourceOverlay || !m_sourceOverlay->isLoaded()) {
        qWarning() << "BrainView: Cannot start streaming — no source estimate loaded";
        return;
    }

    // Create controller on first use
    if (!m_rtController) {
        m_rtController = std::make_unique<RtSourceDataController>(this);
        connect(m_rtController.get(), &RtSourceDataController::newSmoothedDataAvailable,
                this, &BrainView::onRealtimeColorsAvailable);
    }

    // Propagate interpolation matrices from the overlay
    m_rtController->setInterpolationMatrixLeft(m_sourceOverlay->interpolationMatLh());
    m_rtController->setInterpolationMatrixRight(m_sourceOverlay->interpolationMatRh());

    // Propagate current visualization parameters
    m_rtController->setColormapType(m_sourceOverlay->colormap());
    m_rtController->setThresholds(m_sourceOverlay->thresholdMin(),
                                   m_sourceOverlay->thresholdMid(),
                                   m_sourceOverlay->thresholdMax());
    m_rtController->setSFreq(1.0 / m_sourceOverlay->tstep());

    // Feed all STC time-points as data into the queue
    int nTimePoints = m_sourceOverlay->numTimePoints();
    qDebug() << "BrainView: Feeding" << nTimePoints << "time points into real-time queue";
    m_rtController->clearData();

    // Get source data column by column from the overlay and push to controller
    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXd col = m_sourceOverlay->sourceDataColumn(t);
        if (col.size() > 0) {
            m_rtController->addData(col);
        }
    }

    // Ensure source estimate overlay is in SourceEstimate mode
    setVisualizationMode("Source Estimate");

    // Start streaming
    m_rtController->setStreamingState(true);
    m_isRtStreaming = true;

    qDebug() << "BrainView: Real-time streaming started";
}

//=============================================================================================================

void BrainView::stopRealtimeStreaming()
{
    if (!m_isRtStreaming) return;

    if (m_rtController) {
        m_rtController->setStreamingState(false);
    }
    m_isRtStreaming = false;

    qDebug() << "BrainView: Real-time streaming stopped";
}

//=============================================================================================================

bool BrainView::isRealtimeStreaming() const
{
    return m_isRtStreaming;
}

//=============================================================================================================

void BrainView::pushRealtimeSourceData(const Eigen::VectorXd &data)
{
    if (m_rtController) {
        m_rtController->addData(data);
    }
}

//=============================================================================================================

void BrainView::setRealtimeInterval(int msec)
{
    if (m_rtController) {
        m_rtController->setTimeInterval(msec);
    }
}

//=============================================================================================================

void BrainView::setRealtimeLooping(bool enabled)
{
    if (m_rtController) {
        m_rtController->setLoopState(enabled);
    }
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
    return DataLoader::probeEvokedSets(evokedPath);
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
// ── Real-time sensor data streaming ────────────────────────────────────
//=============================================================================================================

void BrainView::startRealtimeSensorStreaming(const QString &modality)
{
    if (m_isRtSensorStreaming) {
        qDebug() << "BrainView: Real-time sensor streaming already active";
        return;
    }

    // Require loaded evoked data and a built mapping matrix
    if (!m_sensorFieldLoaded || m_sensorEvoked.isEmpty()) {
        qWarning() << "BrainView: Cannot start sensor streaming — no evoked data loaded";
        return;
    }

    // Select the mapping matrix and pick vector based on modality
    QSharedPointer<Eigen::MatrixXf> mappingMat;
    QVector<int> pick;
    QString surfaceKey;

    if (modality == QStringLiteral("MEG")) {
        mappingMat = m_megFieldMapping;
        pick = m_megFieldPick;
        surfaceKey = m_megFieldSurfaceKey;
    } else if (modality == QStringLiteral("EEG")) {
        mappingMat = m_eegFieldMapping;
        pick = m_eegFieldPick;
        surfaceKey = m_eegFieldSurfaceKey;
    } else {
        qWarning() << "BrainView: Unknown modality for sensor streaming:" << modality;
        return;
    }

    if (!mappingMat || mappingMat->rows() == 0 || pick.isEmpty()) {
        qWarning() << "BrainView: Mapping matrix not built for" << modality
                   << "— call loadSensorField() first";
        return;
    }

    if (surfaceKey.isEmpty() || !m_surfaces.contains(surfaceKey)) {
        qWarning() << "BrainView: Target surface not found for" << modality
                   << "streaming (key:" << surfaceKey << ")";
        return;
    }

    m_rtSensorModality = modality;

    // Create controller on first use
    if (!m_rtSensorController) {
        m_rtSensorController = std::make_unique<RtSensorDataController>(this);
        connect(m_rtSensorController.get(), &RtSensorDataController::newSensorColorsAvailable,
                this, &BrainView::onRealtimeSensorColorsAvailable);
    }

    // Propagate mapping matrix
    m_rtSensorController->setMappingMatrix(mappingMat);

    // Propagate current visualization parameters
    m_rtSensorController->setColormapType(m_sensorFieldColormap);

    // Compute sampling frequency from evoked data
    double sFreq = 1000.0;
    if (m_sensorEvoked.times.size() > 1) {
        double dt = m_sensorEvoked.times(1) - m_sensorEvoked.times(0);
        if (dt > 0) sFreq = 1.0 / dt;
    }
    m_rtSensorController->setSFreq(sFreq);

    // Feed evoked time points as data into the queue
    int nTimePoints = static_cast<int>(m_sensorEvoked.times.size());
    qDebug() << "BrainView: Feeding" << nTimePoints << modality
             << "sensor time points into real-time queue";
    m_rtSensorController->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i) {
            meas(i) = static_cast<float>(m_sensorEvoked.data(pick[i], t));
        }
        m_rtSensorController->addData(meas);
    }

    // Start streaming
    m_rtSensorController->setStreamingState(true);
    m_isRtSensorStreaming = true;

    qDebug() << "BrainView: Real-time sensor streaming started for" << modality;
}

//=============================================================================================================

void BrainView::stopRealtimeSensorStreaming()
{
    if (!m_isRtSensorStreaming) return;

    if (m_rtSensorController) {
        m_rtSensorController->setStreamingState(false);
    }
    m_isRtSensorStreaming = false;

    qDebug() << "BrainView: Real-time sensor streaming stopped";
}

//=============================================================================================================

bool BrainView::isRealtimeSensorStreaming() const
{
    return m_isRtSensorStreaming;
}

//=============================================================================================================

void BrainView::pushRealtimeSensorData(const Eigen::VectorXf &data)
{
    if (m_rtSensorController) {
        m_rtSensorController->addData(data);
    }
}

//=============================================================================================================

void BrainView::setRealtimeSensorInterval(int msec)
{
    if (m_rtSensorController) {
        m_rtSensorController->setTimeInterval(msec);
    }
}

//=============================================================================================================

void BrainView::setRealtimeSensorLooping(bool enabled)
{
    if (m_rtSensorController) {
        m_rtSensorController->setLoopState(enabled);
    }
}

//=============================================================================================================

void BrainView::setRealtimeSensorAverages(int numAvr)
{
    if (m_rtSensorController) {
        m_rtSensorController->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void BrainView::setRealtimeSensorColormap(const QString &name)
{
    if (m_rtSensorController) {
        m_rtSensorController->setColormapType(name);
    }
}

//=============================================================================================================

void BrainView::onRealtimeSensorColorsAvailable(const QString &surfaceKey,
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
    if (targetTicks <= 0) return 0.0f;
    double range = static_cast<double>(maxVal - minVal);
    if (range <= 0.0) return 0.0f;

    double raw = range / static_cast<double>(targetTicks);
    double exponent = std::floor(std::log10(raw));
    double base = std::pow(10.0, exponent);
    double frac = raw / base;

    double niceFrac = 1.0;
    if (frac <= 1.0)      niceFrac = 1.0;
    else if (frac <= 2.0) niceFrac = 2.0;
    else if (frac <= 5.0) niceFrac = 5.0;
    else                  niceFrac = 10.0;

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

    float minVal = values[0], maxVal = values[0];
    for (int i = 1; i < values.size(); ++i) {
        minVal = std::min(minVal, values[i]);
        maxVal = std::max(maxVal, values[i]);
    }

    QVector<float> negLevels, posLevels;
    bool hasZero = (minVal < 0.0f && maxVal > 0.0f);

    for (float level = -step; level >= minVal; level -= step) negLevels.append(level);
    for (float level = step;  level <= maxVal; level += step) posLevels.append(level);

    struct ContourBuffers {
        QVector<Eigen::Vector3f> verts;
        QVector<Eigen::Vector3f> norms;
        QVector<Eigen::Vector3i> tris;
    };

    auto addSegment = [](ContourBuffers &buf, const QVector3D &p0, const QVector3D &p1,
                         const QVector3D &normal, float halfWidth, float shift) {
        QVector3D dir = (p1 - p0);
        float len = dir.length();
        if (len < 1e-6f) return;
        dir /= len;

        QVector3D binormal = QVector3D::crossProduct(normal, dir);
        if (binormal.length() < 1e-6f) binormal = QVector3D::crossProduct(QVector3D(0,1,0), dir);
        if (binormal.length() < 1e-6f) binormal = QVector3D::crossProduct(QVector3D(1,0,0), dir);
        binormal.normalize();

        QVector3D offset = normal * shift;
        QVector3D w = binormal * halfWidth;

        int base = buf.verts.size();
        buf.verts.append(Eigen::Vector3f(p0.x()-w.x()+offset.x(), p0.y()-w.y()+offset.y(), p0.z()-w.z()+offset.z()));
        buf.verts.append(Eigen::Vector3f(p0.x()+w.x()+offset.x(), p0.y()+w.y()+offset.y(), p0.z()+w.z()+offset.z()));
        buf.verts.append(Eigen::Vector3f(p1.x()-w.x()+offset.x(), p1.y()-w.y()+offset.y(), p1.z()-w.z()+offset.z()));
        buf.verts.append(Eigen::Vector3f(p1.x()+w.x()+offset.x(), p1.y()+w.y()+offset.y(), p1.z()+w.z()+offset.z()));

        Eigen::Vector3f n(normal.x(), normal.y(), normal.z());
        buf.norms.append(n); buf.norms.append(n); buf.norms.append(n); buf.norms.append(n);

        buf.tris.append(Eigen::Vector3i(base+0, base+1, base+2));
        buf.tris.append(Eigen::Vector3i(base+1, base+3, base+2));
    };

    auto buildContours = [&](const QVector<float> &levels, ContourBuffers &buf) {
        const Eigen::MatrixX3f rr = surface.vertexPositions();
        const Eigen::MatrixX3f nn = surface.vertexNormals();
        const QVector<uint32_t> idx = surface.triangleIndices();
        if (rr.rows() == 0 || nn.rows() == 0 || idx.isEmpty()) return;

        const float shift = 0.001f, halfWidth = 0.0005f;
        for (float level : levels) {
            for (int t = 0; t + 2 < idx.size(); t += 3) {
                int i0 = idx[t], i1 = idx[t+1], i2 = idx[t+2];
                float v0 = values[i0], v1 = values[i1], v2 = values[i2];

                QVector3D p0(rr(i0,0), rr(i0,1), rr(i0,2));
                QVector3D p1(rr(i1,0), rr(i1,1), rr(i1,2));
                QVector3D p2(rr(i2,0), rr(i2,1), rr(i2,2));

                QVector3D n0(nn(i0,0), nn(i0,1), nn(i0,2));
                QVector3D n1(nn(i1,0), nn(i1,1), nn(i1,2));
                QVector3D n2(nn(i2,0), nn(i2,1), nn(i2,2));
                QVector3D normal = (n0 + n1 + n2).normalized();
                if (normal.length() < 1e-6f)
                    normal = QVector3D::crossProduct(p1 - p0, p2 - p0).normalized();

                QVector<QVector3D> hits;
                auto checkEdge = [&](const QVector3D &a, const QVector3D &b, float va, float vb) {
                    if (va == vb) return;
                    float tval = (level - va) / (vb - va);
                    if (tval >= 0.0f && tval < 1.0f) hits.append(a + (b - a) * tval);
                };

                checkEdge(p0, p1, v0, v1);
                checkEdge(p1, p2, v1, v2);
                checkEdge(p2, p0, v2, v0);

                if (hits.size() == 2) addSegment(buf, hits[0], hits[1], normal, halfWidth, shift);
            }
        }
    };

    ContourBuffers negBuf, posBuf, zeroBuf;
    buildContours(negLevels, negBuf);
    buildContours(posLevels, posBuf);
    if (hasZero) {
        QVector<float> zeroLevels = {0.0f};
        buildContours(zeroLevels, zeroBuf);
    }

    auto updateSurface = [this, &prefix](const QString &suffix, const ContourBuffers &buf,
                                         const QColor &color, bool show) {
        QString key = prefix + suffix;
        if (!show || buf.verts.isEmpty()) {
            if (m_surfaces.contains(key)) m_surfaces[key]->setVisible(false);
            return;
        }

        Eigen::MatrixX3f rr(buf.verts.size(), 3);
        Eigen::MatrixX3f nn(buf.norms.size(), 3);
        Eigen::MatrixX3i tris(buf.tris.size(), 3);
        for (int i = 0; i < buf.verts.size(); ++i) {
            rr(i,0) = buf.verts[i].x(); rr(i,1) = buf.verts[i].y(); rr(i,2) = buf.verts[i].z();
            nn(i,0) = buf.norms[i].x(); nn(i,1) = buf.norms[i].y(); nn(i,2) = buf.norms[i].z();
        }
        for (int i = 0; i < buf.tris.size(); ++i) tris.row(i) = buf.tris[i];

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

    updateSurface("_neg",  negBuf,  QColor(0,0,255,200),   visible && !negBuf.verts.isEmpty());
    updateSurface("_zero", zeroBuf, QColor(0,0,0,220),     visible && !zeroBuf.verts.isEmpty());
    updateSurface("_pos",  posBuf,  QColor(255,0,0,200),   visible && !posBuf.verts.isEmpty());
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
        devHeadQTrans = SurfaceKeys::toQMatrix4x4(m_sensorEvoked.info.dev_head_t.trans);
    }

    QMatrix4x4 headToMri;
    if (m_applySensorTrans && !m_headToMriTrans.isEmpty()) {
        headToMri = SurfaceKeys::toQMatrix4x4(m_headToMriTrans.trans);
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
    for (int i = 0; i < m_subViews.size(); ++i) {
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
