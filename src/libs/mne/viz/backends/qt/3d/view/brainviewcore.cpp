//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainviewcore.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainViewCore definition: scene-model wiring, camera interaction, picking, 2-D overlay and frame recording.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainviewcore.h"

#include <mne/base/log.h>
#include <mne/base/string_utils.h>

#include "brainrenderer.h"
#include "../../../../brain/brainsurface.h"
#include "../renderable/dipoleobject.h"
#include "../renderable/electrodeobject.h"
#include "../renderable/networkobject.h"
#include "../renderable/polylineobject.h"
#include "../renderable/videooverlay.h"
#include "../renderable/sliceobject.h"
#include "../core/surfacekeys.h"
#include "../core/dataloader.h"
#include "../input/raypicker.h"
#include "../../qtconversions.h"
#include "../../../../brain/meshfactory.h"
#include "../model/braintreemodel.h"
#include "../model/items/surfacetreeitem.h"
#include "../model/items/bemtreeitem.h"
#include "../model/items/sensortreeitem.h"
#include "../model/items/dipoletreeitem.h"
#include "../model/items/sourcespacetreeitem.h"
#include "../model/items/digitizertreeitem.h"
#include <rhi/qrhi.h>
#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_source_spaces.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/connectivity/network/network.h>
#include <mne/viz/backends/qt/plots/helpers/sceneinput.h>
#include <mne/viz/backends/qt/plots/helpers/scenepainter.h>
#include <mne/viz/views/ray_picking.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QKeyEvent>
#include <QMatrix4x4>
#include <QSettings>
#include <QTimer>
#include <QStandardItem>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QtMath>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace mne;

namespace mne::viz::qt
{

namespace
{

QColor itemColor(const AbstractTreeItem& item)
{
    QColor color = item.color();
    color.setAlphaF(color.alphaF() * item.alpha());
    return color;
}

// The overlay asked for the generic "monospace" / "sans-serif" families, i.e. the system fonts
mne::viz::BrainViewOverlay::Fonts overlayFonts()
{
    viz::BrainViewOverlay::Fonts fonts;
    fonts.monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont).family().toStdString();
    fonts.sans_serif = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family().toStdString();
    return fonts;
}

QFont overlayBaseFont()
{
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont);
}

Qt::CursorShape toQtCursor(viz::CursorShape shape)
{
    switch (shape) {
        case viz::CursorShape::SizeHorizontal:
            return Qt::SizeHorCursor;
        case viz::CursorShape::SizeVertical:
            return Qt::SizeVerCursor;
        case viz::CursorShape::SizeAll:
            return Qt::SizeAllCursor;
        case viz::CursorShape::Default:
            break;
    }
    return Qt::ArrowCursor;
}

mne::viz::PixelSize toPixelSize(const QSize& size)
{
    return {size.width(), size.height()};
}

QRect toQRect(const viz::PixelRect& rect)
{
    return {rect.x, rect.y, rect.width, rect.height};
}

QPoint toQPoint(const viz::Point& point)
{
    return {static_cast<int>(point.x), static_cast<int>(point.y)};
}

} // namespace

//=============================================================================================================
// BrainViewHost
//=============================================================================================================

BrainViewHost::~BrainViewHost() = default;

//=============================================================================================================

void BrainViewHost::requestViewportPresetMenu(int viewport, const QPoint& pos)
{
    Q_UNUSED(viewport);
    Q_UNUSED(pos);
}

//=============================================================================================================

bool BrainViewHost::beginOffscreenCapture(const QSize& size)
{
    Q_UNUSED(size);
    return false;
}

//=============================================================================================================

void BrainViewHost::endOffscreenCapture()
{
}

// QSettings is constructed with its default ctor below; it picks up the
// organisation and application names that each host (mne_align,
// mne_inspect, ex_disp_3D, ...) sets on QCoreApplication in its main(),
// so persisted view state is naturally scoped per-application.

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

BrainViewCore::BrainViewCore(BrainViewHost* host, QObject* parent)
: QObject(parent)
, m_host(host)
{
    // No periodic update timer — redraws are demand-driven via requestUpdate().
    // Every setter that changes scene state calls m_sceneDirty = true
    // followed by requestUpdate(), which the host coalesces into one frame.

    m_fpsStart = std::chrono::steady_clock::now();

    // ── Initialise viewports and their labels (sized to kDefaultViewportCount) ──
    m_subViews.resize(kDefaultViewportCount);
    for (int i = 0; i < kDefaultViewportCount; ++i) {
        m_subViews[i] = SubView::defaultForIndex(i);
    }
    m_overlay.set_viewport_count(kDefaultViewportCount);
    m_overlay.set_fonts(overlayFonts());
    QColor separatorColor = QGuiApplication::palette().color(QPalette::Midlight);
    if (separatorColor.alpha() == 255) {
        separatorColor.setAlpha(180);
    }
    m_overlay.set_separator_color(toSceneColor(separatorColor));
    setupInteraction();

    loadMultiViewSettings();
    updateOverlayLayout();

    // Setup Debug Pointer: Semi-transparent sphere for subtle intersection indicator
    m_debugPointerSurface = MeshFactory::createSphere(Eigen::Vector3f::Zero(), 0.002f, Color{200, 255, 255, 160});

    // ── Connect SourceEstimateManager signals ─────────────────────────
    connect(&m_sourceManager, &SourceEstimateManager::loaded,
            this, &BrainViewCore::onSourceEstimateLoaded);
    connect(&m_sourceManager, &SourceEstimateManager::thresholdsUpdated,
            this, &BrainViewCore::sourceThresholdsUpdated);
    connect(&m_sourceManager, &SourceEstimateManager::timePointChanged,
            this, &BrainViewCore::timePointChanged);
    connect(&m_sourceManager, &SourceEstimateManager::loadingProgress,
            this, [this](int percent, const std::string& message) { emit stcLoadingProgress(percent, QString::fromStdString(message)); });
    connect(&m_sourceManager, &SourceEstimateManager::realtimeColorsAvailable,
            this, &BrainViewCore::onRealtimeColorsAvailable);

    // RtSensorStreamManager → BrainView
    connect(&m_sensorStreamManager, &RtSensorStreamManager::colorsAvailable,
            this, &BrainViewCore::onSensorStreamColorsAvailable);

    // Video overlay starts disabled with a default focus point near the
    // top of a typical FreeSurfer head; the application toggles it on.
    m_videoOverlay = std::make_unique<mne::viz::qt::VideoOverlay>();
}

//=============================================================================================================

BrainViewCore::~BrainViewCore()
{
    saveMultiViewSettings();
}

//=============================================================================================================

void BrainViewCore::setHost(BrainViewHost* host)
{
    m_host = host;
    updateOverlayLayout();
    requestUpdate();
}

//=============================================================================================================

BrainViewHost* BrainViewCore::host() const
{
    return m_host;
}

//=============================================================================================================

void BrainViewCore::requestUpdate()
{
    if (m_host) {
        m_host->requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::markOverlayDirty()
{
    m_overlayDirty = true;
    // Labels change while a frame is recorded (FPS, pane info); that frame already paints them
    if (!m_inRender) {
        requestUpdate();
    }
}

//=============================================================================================================

QSize BrainViewCore::hostSize() const
{
    return m_host ? m_host->logicalSize() : QSize();
}

//=============================================================================================================

QImage BrainViewCore::grabSceneFrame()
{
    if (!m_host) {
        return {};
    }
    // Screenshots hold the scene only, as when the labels were child widgets outside the framebuffer
    m_overlaySuppressed = true;
    QImage frame = m_host->grabFrame();
    m_overlaySuppressed = false;
    // A host that grabbed its on-screen target shows the overlay again with the next frame
    requestUpdate();
    return frame;
}

//=============================================================================================================

void BrainViewCore::setHostCursor(viz::CursorShape shape)
{
    if (m_host) {
        m_host->setCursorShape(toQtCursor(shape));
    }
}

//=============================================================================================================

void BrainViewCore::setupInteraction()
{
    std::vector<viz::PaneCamera*> panes;
    panes.reserve(m_subViews.size());
    for (SubView& subView : m_subViews) {
        panes.push_back(&subView);
    }
    m_interaction.attach_cameras(&m_singleView, std::move(panes));

    viz::BrainViewInteraction::Effects effects;
    effects.redraw = [this] {
        cameraChanged();
    };
    effects.layout_changed = [this] {
        updateOverlayLayout();
        cameraChanged();
    };
    effects.settings_changed = [this] {
        saveMultiViewSettings();
    };
    effects.cursor_changed = [this](viz::CursorShape shape) {
        setHostCursor(shape);
    };
    effects.viewport_selected = [this](int viewport) {
        if (viewport != m_visualizationEditTarget) {
            setVisualizationEditTarget(viewport);
        }
    };
    // A hidden surface shows no labels, so its name labels take no clicks
    effects.hits_viewport_label = [this](int viewport, viz::Point pos) {
        const viz::BrainViewOverlay::Label* name = m_overlay.item(viz::BrainViewOverlay::Item::ViewportNameLabel, viewport);
        return name && name->visible && m_host && m_host->isVisible() && toQRect(name->geometry).contains(toQPoint(pos));
    };
    effects.preset_menu_requested = [this](int viewport, viz::Point pos) {
        if (m_host) {
            m_host->requestViewportPresetMenu(viewport, toQPoint(pos));
        }
    };
    effects.hovered = [this](viz::Point pos) {
        castRay(toQPoint(pos));
    };
    effects.clicked = [this](viz::Point pos) {
        castRay(toQPoint(pos));
        if (const mne::IeegContact* contact = m_electrodes ? m_electrodes->contactAt(m_electrodes->hoveredContact()) : nullptr) {
            emit electrodeContactClicked(QString::fromStdString(contact->name), QVector3D(contact->position.x(), contact->position.y(), contact->position.z()));
        } else if (m_hasIntersection) {
            emit surfacePointClicked(m_lastIntersectionPoint);
        }
    };
    effects.double_clicked = [this](viz::Point pos) {
        castRay(toQPoint(pos));
        if (m_hasIntersection) {
            emit surfacePointDoubleClicked(m_lastIntersectionPoint);
            return true;
        }
        return false;
    };
    effects.snapshot_requested = [this] {
        saveSnapshot();
    };
    m_interaction.set_effects(std::move(effects));
}

//=============================================================================================================

void BrainViewCore::syncInteraction()
{
    m_interaction.set_view_size(toPixelSize(hostSize()));
    m_interaction.set_panes(m_viewMode == MultiView, enabledViewportIndices());
    m_interaction.set_scene_size(m_sceneSize);
}

//=============================================================================================================

void BrainViewCore::cameraChanged()
{
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setModel(BrainTreeModel* model)
{
    if (model == m_model)
        return;

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
        for (auto it = m_itemSurfaceMap.cbegin(); it != m_itemSurfaceMap.cend(); ++it) {
            for (auto sit = m_surfaces.begin(); sit != m_surfaces.end();) {
                sit = (sit->second == it->second) ? m_surfaces.erase(sit) : std::next(sit);
            }
        }
        m_itemSurfaceMap.clear();
        m_itemDipoleMap.clear();
        m_hoveredItem = nullptr;
        m_activeSurface.reset();
    }

    m_model = model;
    if (!m_model) {
        updateSceneBounds();
        m_sceneDirty = true;
        requestUpdate();
        return;
    }
    connect(m_model, &BrainTreeModel::rowsInserted, this, &BrainViewCore::onRowsInserted);
    connect(m_model, &BrainTreeModel::dataChanged, this, &BrainViewCore::onDataChanged);

    if (m_model->rowCount() > 0) {
        onRowsInserted(QModelIndex(), 0, m_model->rowCount() - 1);
    } else {
        updateSceneBounds();
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::setInitialCameraRotation(const QQuaternion& rotation)
{
    m_singleView.rotation = toEigen(rotation);
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    if (!m_model)
        return;

    for (int i = first; i <= last; ++i) {
        QModelIndex index = m_model->index(i, 0, parent);
        QStandardItem* item = m_model->itemFromIndex(index);

        AbstractTreeItem* absItem = dynamic_cast<AbstractTreeItem*>(item);

        // Handle FsSurface Items
        if (absItem && absItem->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::SurfaceItem)) {
            SurfaceTreeItem* surfItem = static_cast<SurfaceTreeItem*>(absItem);
            auto brainSurf = std::make_shared<BrainSurface>();

            // Load geometry from item
            brainSurf->fromSurface(surfItem->surfaceData());

            // Determine Hemisphere from Parent
            if (absItem->parent()) {
                QString parentText = absItem->parent()->text();
                if (parentText == "lh")
                    brainSurf->setHemi(0);
                else if (parentText == "rh")
                    brainSurf->setHemi(1);
            }

            // Set properties
            brainSurf->setVisible(surfItem->isVisible());

            // Brain surfaces (pial, white, inflated, etc.) are brain tissue
            brainSurf->setTissueType(BrainSurface::TissueBrain);

            m_itemSurfaceMap[item] = brainSurf;

            // Key generation: "hemi_type" e.g. "lh_pial"
            std::string key;
            if (absItem->parent()) {
                key = (absItem->parent()->text() + "_" + surfItem->text()).toStdString();
            } else {
                key = surfItem->text().toStdString();
            }
            m_surfaces[key] = brainSurf;

            // Check for annotations
            if (!surfItem->annotationData().empty()) {
                brainSurf->addAnnotation(surfItem->annotationData());
            }

            // Set active if first
            if (!m_activeSurface) {
                m_activeSurface = brainSurf;
                m_activeSurfaceType = surfItem->text().toStdString();
            }
        }
        // Check for BEM Item (using dynamic_cast for safety)
        BemTreeItem* bemItem = dynamic_cast<BemTreeItem*>(absItem);
        if (bemItem) {
            const mne::MNEBemSurface& bemSurfData = bemItem->bemSurfaceData();

            auto brainSurf = std::make_shared<BrainSurface>();

            // Load BEM geometry with color from item
            brainSurf->fromBemSurface(bemSurfData, toColor(itemColor(*bemItem)));

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
            m_surfaces["bem_" + bemItem->text().toStdString()] = brainSurf;
        }

        // Handle Sensor Items
        if (absItem && absItem->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::SensorItem)) {
            SensorTreeItem* sensItem = static_cast<SensorTreeItem*>(absItem);

            std::shared_ptr<BrainSurface> brainSurf;

            QString parentText = "";
            if (sensItem->parent())
                parentText = sensItem->parent()->text();

            if (parentText.contains("MEG/Grad") && sensItem->hasOrientation()) {
                brainSurf = MeshFactory::createBarbell(toEigen(sensItem->position()), toEigen(sensItem->orientation()),
                                                       toColor(itemColor(*sensItem)), sensItem->scale());
            } else if (parentText.contains("MEG/Mag") && sensItem->hasOrientation()) {
                brainSurf = MeshFactory::createPlate(toEigen(sensItem->position()), toEigen(sensItem->orientation()),
                                                     toColor(itemColor(*sensItem)), sensItem->scale());
            } else {
                // EEG and other sensors: smooth icosphere
                brainSurf = MeshFactory::createSphere(toEigen(sensItem->position()), sensItem->scale(),
                                                      toColor(itemColor(*sensItem)));
            }

            brainSurf->setVisible(sensItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;


            // Legacy map support
            const std::string keyPrefix = mne::viz::qt::surface_keys::sensorParentToKeyPrefix(parentText.toStdString());

            const std::string key = std::format("{}{}_{}", keyPrefix, sensItem->text().toStdString(), reinterpret_cast<std::uintptr_t>(sensItem));
            m_surfaces[key] = brainSurf;
        }

        // Handle Dipole Items
        if (absItem && absItem->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::DipoleItem)) {
            DipoleTreeItem* dipItem = static_cast<DipoleTreeItem*>(absItem);
            auto dipObject = std::make_shared<DipoleObject>();
            dipObject->load(dipItem->ecdSet());
            dipObject->setVisible(dipItem->isVisible());
            dipObject->applyTransform(itemTransform(item));

            m_itemDipoleMap[item] = dipObject;
        }

        // Handle Source Space Items (one item per hemisphere, batched mesh)
        if (absItem && absItem->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::SourceSpaceItem)) {
            SourceSpaceTreeItem* srcItem = static_cast<SourceSpaceTreeItem*>(absItem);
            const std::vector<QVector3D>& positions = srcItem->positions();
            if (positions.empty())
                continue;

            auto brainSurf = MeshFactory::createBatchedSpheres(toEigen(positions), srcItem->scale(),
                                                               toColor(itemColor(*srcItem)));
            brainSurf->setVisible(srcItem->isVisible());
            m_itemSurfaceMap[item] = brainSurf;

            const std::string key = "srcsp_" + srcItem->text().toStdString();
            m_surfaces[key] = brainSurf;
        }

        // Handle Digitizer Items (batched sphere mesh per category)
        if (absItem && absItem->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::DigitizerItem)) {
            DigitizerTreeItem* digItem = static_cast<DigitizerTreeItem*>(absItem);
            const std::vector<QVector3D>& positions = digItem->positions();
            if (positions.empty())
                continue;

            auto brainSurf = MeshFactory::createBatchedSpheres(toEigen(positions), digItem->scale(),
                                                               toColor(itemColor(*digItem)));
            brainSurf->setVisible(digItem->isVisible());

            m_itemSurfaceMap[item] = brainSurf;

            // Category name for surface-map key.  A monotonic counter
            // suffix guarantees uniqueness so that multiple items of the
            // same PointKind each get their own entry in m_surfaces.
            // shouldRenderSurface() matches by prefix ("dig_cardinal",
            // "dig_hpi", …) so the suffix is transparent to visibility.
            static int s_digKeyCounter = 0;
            std::string catName;
            switch (digItem->pointKind()) {
                case DigitizerTreeItem::Cardinal:
                    catName = "cardinal";
                    break;
                case DigitizerTreeItem::HPI:
                    catName = "hpi";
                    break;
                case DigitizerTreeItem::EEG:
                    catName = "eeg";
                    break;
                case DigitizerTreeItem::Extra:
                    catName = "extra";
                    break;
                default:
                    catName = digItem->text().toLower().replace(' ', '_').toStdString();
                    break;
            }
            const std::string key = std::format("dig_{}_{}", catName, s_digKeyCounter++);
            m_surfaces[key] = brainSurf;
        }

        if (m_itemSurfaceMap.contains(item))
            m_itemSurfaceMap[item]->applyTransform(toEigen(itemTransform(item)));

        // Check children recursively
        if (m_model->hasChildren(index)) {
            onRowsInserted(index, 0, m_model->rowCount(index) - 1);
        }
    }
    updateInflatedSurfaceTransforms();
    updateSceneBounds();
    m_vertexCountDirty = true;
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
{
    bool transformChanged = false;
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        QModelIndex index = m_model->index(i, 0, topLeft.parent());
        QStandardItem* item = m_model->itemFromIndex(index);

        if (m_itemSurfaceMap.contains(item)) {
            auto surf = m_itemSurfaceMap[item];

            AbstractTreeItem* absItem = dynamic_cast<AbstractTreeItem*>(item);
            if (absItem) {
                if (roles.contains(AbstractTreeItem::VisibleRole)) {
                    surf->setVisible(absItem->isVisible());
                    m_vertexCountDirty = true;
                }
                if ((roles.contains(AbstractTreeItem::ColorRole) || roles.contains(AbstractTreeItem::AlphaRole)) && absItem->color().isValid()) {
                    surf->setColor(toColor(itemColor(*absItem)));
                }
                if (roles.contains(AbstractTreeItem::TransformRole)) {
                    surf->applyTransform(toEigen(itemTransform(item)));
                    transformChanged = true;
                }
                if (roles.contains(SurfaceTreeItem::AnnotationDataRole)) {
                    SurfaceTreeItem* sItem = static_cast<SurfaceTreeItem*>(absItem);
                    if (!sItem->annotationData().empty()) {
                        surf->addAnnotation(sItem->annotationData());
                    }
                }
            }
        }

        if (m_itemDipoleMap.contains(item)) {
            auto* absItem = dynamic_cast<AbstractTreeItem*>(item);
            if (absItem && roles.contains(AbstractTreeItem::VisibleRole))
                m_itemDipoleMap[item]->setVisible(absItem->isVisible());
            if (roles.contains(AbstractTreeItem::TransformRole))
                m_itemDipoleMap[item]->applyTransform(itemTransform(item));
        }
    }
    if (transformChanged) {
        updateInflatedSurfaceTransforms();
        m_vertexCountDirty = true;
    }
    updateSceneBounds();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setActiveSurface(const std::string& type)
{
    subViewForTarget(m_visualizationEditTarget).surfaceType = type;

    m_activeSurfaceType = type;

    // Update m_activeSurface pointer to one of the matching surfaces for stats/helpers
    if (const auto lh = m_surfaces.find("lh_" + type); lh != m_surfaces.end())
        m_activeSurface = lh->second;
    else if (const auto rh = m_surfaces.find("rh_" + type); rh != m_surfaces.end())
        m_activeSurface = rh->second;

    updateInflatedSurfaceTransforms();
    saveMultiViewSettings();

    updateSceneBounds();
    m_vertexCountDirty = true;
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::updateSceneBounds()
{
    QVector3D min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    QVector3D max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
    bool hasContent = false;

    // Iterate over all surfaces
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it->second->isVisible()) {
            Eigen::Vector3f sMinEigen, sMaxEigen;
            it->second->boundingBox(sMinEigen, sMaxEigen);
            QVector3D sMin = toQVector3D(sMinEigen), sMax = toQVector3D(sMaxEigen);

            min.setX(std::min(min.x(), sMin.x()));
            min.setY(std::min(min.y(), sMin.y()));
            min.setZ(std::min(min.z(), sMin.z()));

            max.setX(std::max(max.x(), sMax.x()));
            max.setY(std::max(max.y(), sMax.y()));
            max.setZ(std::max(max.z(), sMax.z()));
            hasContent = true;
        }
    }

    for (auto it = m_itemDipoleMap.cbegin(); it != m_itemDipoleMap.cend(); ++it) {
        QVector3D dMin, dMax;
        if (it->second->isVisible() && it->second->boundingBox(dMin, dMax)) {
            min = QVector3D(std::min(min.x(), dMin.x()), std::min(min.y(), dMin.y()), std::min(min.z(), dMin.z()));
            max = QVector3D(std::max(max.x(), dMax.x()), std::max(max.y(), dMax.y()), std::max(max.z(), dMax.z()));
            hasContent = true;
        }
    }

    if (hasContent) {
        m_sceneCenter = (min + max) * 0.5f;

        QVector3D diag = max - min;
        m_sceneSize = std::max(diag.x(), std::max(diag.y(), diag.z()));

        // Ensure non-zero size
        if (m_sceneSize < 0.01f)
            m_sceneSize = 0.3f;

    } else {
        // Default
        m_sceneCenter = QVector3D(0, 0, 0);
        m_sceneSize = 0.3f;
    }
}

//=============================================================================================================

void BrainViewCore::setShaderMode(const std::string& modeName)
{
    const mne::viz::ShaderMode mode = shaderModeFromName(modeName);
    subViewForTarget(m_visualizationEditTarget).brainShader = mode;

    m_brainShaderMode = mode;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
    emit shaderModeChanged(QString::fromStdString(shaderModeName(mode)));
}

//=============================================================================================================

void BrainViewCore::setVisualizationEditTarget(int target)
{
    const int prev = m_visualizationEditTarget;
    m_visualizationEditTarget = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);

    const SubView& sv = subViewForTarget(m_visualizationEditTarget);
    m_activeSurfaceType = sv.surfaceType;
    m_brainShaderMode = sv.brainShader;
    m_bemShaderMode = sv.bemShader;
    m_currentVisMode = sv.overlayMode;
    const ViewVisibilityProfile& visibility = sv.visibility;

    const bool remapMegSurface = (m_fieldMapper.megFieldMapOnHead() != visibility.megFieldMapOnHead);
    m_fieldMapper.setMegFieldMapOnHead(visibility.megFieldMapOnHead);
    m_dipolesVisible = visibility.dipoles;
    m_networkVisible = visibility.network;

    // Note: we intentionally do NOT call setVisualizationMode() on surfaces
    // here.  Each viewport's overlay mode is sent as a per-draw shader
    // uniform (sceneData.overlayMode), so the surface objects must keep
    // their vertex data intact — in particular the STC colour channel —
    // regardless of which viewport is currently selected for editing.

    if (m_fieldMapper.isLoaded()) {
        if (remapMegSurface) {
            m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        }
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    }

    // Update viewport label highlighting
    updateOverlayLayout();

    saveMultiViewSettings();

    if (prev != m_visualizationEditTarget) {
        emit visualizationEditTargetChanged(m_visualizationEditTarget);
    }
}

//=============================================================================================================

int BrainViewCore::visualizationEditTarget() const
{
    return m_visualizationEditTarget;
}

//=============================================================================================================

std::string BrainViewCore::activeSurfaceForTarget(int target) const
{
    return subViewForTarget(target).surfaceType;
}

//=============================================================================================================

std::string BrainViewCore::shaderModeForTarget(int target) const
{
    return shaderModeName(subViewForTarget(target).brainShader);
}

//=============================================================================================================

std::string BrainViewCore::bemShaderModeForTarget(int target) const
{
    return shaderModeName(subViewForTarget(target).bemShader);
}

//=============================================================================================================

std::string BrainViewCore::overlayModeForTarget(int target) const
{
    return visualizationModeName(subViewForTarget(target).overlayMode);
}

//=============================================================================================================

ViewVisibilityProfile& BrainViewCore::visibilityProfileForTarget(int target)
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

const ViewVisibilityProfile& BrainViewCore::visibilityProfileForTarget(int target) const
{
    return subViewForTarget(target).visibility;
}

//=============================================================================================================

SubView& BrainViewCore::subViewForTarget(int target)
{
    const int normalized = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

const SubView& BrainViewCore::subViewForTarget(int target) const
{
    const int normalized = normalizedVisualizationTarget(target, static_cast<int>(m_subViews.size()) - 1);
    return (normalized < 0) ? m_singleView : m_subViews[normalized];
}

//=============================================================================================================

// Note: SubView::isBrainSurfaceKey, matchesSurfaceType, shouldRenderSurface,
// and applyOverlayToSurfaces are defined in core/viewstate.cpp.

//=============================================================================================================

bool BrainViewCore::objectVisibleForTarget(const std::string& object, int target) const
{
    return visibilityProfileForTarget(target).isObjectVisible(object);
}

//=============================================================================================================

bool BrainViewCore::megFieldMapOnHeadForTarget(int target) const
{
    return visibilityProfileForTarget(target).megFieldMapOnHead;
}

//=============================================================================================================

void BrainViewCore::updateInflatedSurfaceTransforms()
{
    const bool needsInflated = (m_singleView.surfaceType == "inflated") || std::any_of(m_subViews.cbegin(), m_subViews.cend(), [](const SubView& sv) { return sv.surfaceType == "inflated"; });

    const auto lhIt = m_surfaces.find("lh_inflated");
    const auto rhIt = m_surfaces.find("rh_inflated");
    if (lhIt == m_surfaces.end() || rhIt == m_surfaces.end()) {
        return;
    }

    auto lhSurf = lhIt->second;
    auto rhSurf = rhIt->second;

    lhSurf->applyTransform(toEigen(itemTransform(itemForSurface(lhSurf))));
    rhSurf->applyTransform(toEigen(itemTransform(itemForSurface(rhSurf))));

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

void BrainViewCore::setBemShaderMode(const std::string& modeName)
{
    const mne::viz::ShaderMode mode = shaderModeFromName(modeName);

    subViewForTarget(m_visualizationEditTarget).bemShader = mode;

    m_bemShaderMode = mode;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::syncBemShadersToBrainShaders()
{
    m_singleView.bemShader = m_singleView.brainShader;
    for (int i = 0; i < std::ssize(m_subViews); ++i) {
        m_subViews[i].bemShader = m_subViews[i].brainShader;
    }

    m_bemShaderMode = subViewForTarget(m_visualizationEditTarget).bemShader;

    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::setSensorVisible(const std::string& type, bool visible)
{
    const std::string object = mne::viz::qt::surface_keys::sensorTypeToObjectKey(type);
    if (object.empty())
        return;

    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.setObjectVisible(object, visible);

    // Cascade parent toggle to child sub-types so that e.g. "MEG" also
    // enables/disables MEG/Grad and MEG/Mag sub-types.
    // Note: MEG Helmet has its own independent checkbox and is NOT cascaded.
    if (type == "MEG") {
        profile.sensMegGrad = visible;
        profile.sensMegMag = visible;
    } else if (type == "EEG") {
        // No sub-types for EEG currently, but keep symmetric.
    } else if (type == "Digitizer") {
        profile.digCardinal = visible;
        profile.digHpi = visible;
        profile.digEeg = visible;
        profile.digExtra = visible;
    }

    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::setSensorTransEnabled(bool enabled)
{
    if (m_applySensorTrans != enabled) {
        m_applySensorTrans = enabled;
        refreshSensorTransforms();
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::setMegHelmetOverride(const std::filesystem::path& path)
{
    m_megHelmetOverridePath = path;
}

void BrainViewCore::setDipoleVisible(bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.dipoles = visible;
    m_dipolesVisible = visible;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setVisualizationMode(const std::string& modeName)
{
    const mne::viz::VisualizationMode mode = visualizationModeFromName(modeName);
    SubView& sv = subViewForTarget(m_visualizationEditTarget);
    sv.overlayMode = mode;

    m_currentVisMode = mode;

    // Propagate the mode to brain hemisphere surfaces only (lh_*, rh_*)
    // so that the primary colour channel holds the right data: curvature
    // grays for Scientific or STC colours for SourceEstimate.
    // BEM, sensor, and source-space surfaces are left untouched.
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        const std::string& key = it->first;
        if (key.starts_with("lh_") || key.starts_with("rh_")) {
            it->second->setVisualizationMode(mode);
        }
    }

    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setHemiVisible(int hemiIdx, bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (hemiIdx == 0) {
        profile.lh = visible;
    } else if (hemiIdx == 1) {
        profile.rh = visible;
    }
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setBemVisible(const std::string& name, bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.setObjectVisible("bem_" + name, visible);
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::setBemHighContrast(bool enabled)
{
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it->first.starts_with("bem_")) {
            it->second->setUseDefaultColor(enabled);
        }
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setLightingEnabled(bool enabled)
{
    m_lightingEnabled = enabled;
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setAutomatedRotation(bool enabled)
{
    if (!enabled) {
        if (m_pAutoRotateTimer) {
            m_pAutoRotateTimer->stop();
        }
        return;
    }

    if (!m_pAutoRotateTimer) {
        m_pAutoRotateTimer = new QTimer(this);
        m_pAutoRotateTimer->setInterval(33);

        connect(m_pAutoRotateTimer, &QTimer::timeout, this, [this]() {
            // Rotate the camera rather than the objects, so that every surface
            // keeps its own coregistration transform and head and sensors stay
            // aligned with each other.
            syncInteraction();
            m_interaction.orbit_perspective_views(2.0, 0.0);
        });
    }

    m_pAutoRotateTimer->start();
}

//=============================================================================================================

bool BrainViewCore::automatedRotation() const
{
    return m_pAutoRotateTimer && m_pAutoRotateTimer->isActive();
}

//=============================================================================================================

void BrainViewCore::setHeadMovementPath(const std::vector<Eigen::Vector3f>& vecPositions)
{
    // A path needs at least one segment to be meaningful.
    if (vecPositions.size() < 2) {
        clearHeadMovementPath();
        return;
    }

    if (!m_headPath) {
        m_headPath = std::make_unique<PolylineObject>();
    }

    m_headPath->setPoints(vecPositions);
    m_headPath->setVisible(true);

    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearHeadMovementPath()
{
    if (!m_headPath) {
        return;
    }

    m_headPath.reset();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

bool BrainViewCore::saveSnapshot()
{
    const std::time_t seconds = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    std::array<char, 64> name{};
    std::strftime(name.data(), name.size(), "./Screenshots/%Y_%m_%d-%H_%M_%S-3DView.png", &local);
    return takeScreenshot(std::filesystem::path(name.data()));
}

//=============================================================================================================

bool BrainViewCore::takeScreenshot(const std::filesystem::path& fileName)
{
    const QImage frame = grabSceneFrame();
    if (frame.isNull()) {
        mne::Log::warning("[BrainView::takeScreenshot] No rendered frame - nothing written to {}", mne::utf8_from_path(fileName));
        return false;
    }
    std::error_code error;
    const std::filesystem::path directory = std::filesystem::absolute(fileName, error).parent_path();
    std::filesystem::create_directories(directory, error);
    if (!std::filesystem::is_directory(directory, error) || !frame.save(QString::fromStdU16String(fileName.u16string()))) {
        mne::Log::warning("[BrainView::takeScreenshot] Cannot write {}", mne::utf8_from_path(fileName));
        return false;
    }
    return true;
}

//=============================================================================================================

bool BrainViewCore::savePng(const std::filesystem::path& path, int width, int height,
                            const std::string& surfaceType)
{
    if (!m_host) {
        return false;
    }

    // Set the surface type filter so the render loop matches the
    // loaded surface names (e.g. "white" vs default "pial").
    m_singleView.surfaceType = surfaceType;

    // Let the host realise its surface off-screen (QRhiWidget with
    // Qt::WA_DontShowOnScreen, a hidden QWindow, ...) so that frames are
    // rendered without a window-manager surface.
    m_host->beginOffscreenCapture(QSize(width, height));

    // Spin the event loop until the renderer exists (at least one frame
    // has been recorded) and the surface map is populated.
    const auto waitStart = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - waitStart < std::chrono::seconds(5)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 16);
        if (m_renderer && !m_surfaces.empty())
            break;
    }

    if (!m_renderer) {
        m_host->endOffscreenCapture();
        return false;
    }

    // Force a dirty scene so the next grab renders everything.
    m_sceneDirty = true;
    updateSceneBounds();

    // Pump a few more frames so pipeline resources are fully created.
    for (int i = 0; i < 5; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 16);

    // The host renders a frame and reads the texture back.
    QImage img = grabSceneFrame();

    m_host->endOffscreenCapture();

    if (img.isNull())
        return false;

    return img.save(QString::fromStdU16String(path.u16string()), "PNG");
}

//=============================================================================================================

void BrainViewCore::showSingleView()
{
    m_viewMode = SingleView;
    m_interaction.cancel_splitter_drag();
    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::showMultiView()
{
    m_viewMode = MultiView;
    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setViewCount(int count)
{
    count = std::clamp(count, 1, static_cast<int>(m_subViews.size()));
    m_viewCount = count;

    if (count == 1) {
        m_viewMode = SingleView;
        m_interaction.cancel_splitter_drag();
        setVisualizationEditTarget(-1);
    } else {
        m_viewMode = MultiView;
        // Default edit target to first pane when entering multi-view
        if (m_visualizationEditTarget < 0)
            setVisualizationEditTarget(0);
    }

    // Enable first N sub-views, disable the rest
    for (int i = 0; i < std::ssize(m_subViews); ++i)
        m_subViews[i].enabled = (i < count);

    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
    emit viewCountChanged(m_viewCount);
}

//=============================================================================================================

void BrainViewCore::resetMultiViewLayout()
{
    m_interaction.layout().reset_splits();
    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
}

bool BrainViewCore::isViewportEnabled(int index) const
{
    if (index < 0 || index >= std::ssize(m_subViews)) {
        return false;
    }

    return m_subViews[index].enabled;
}

//=============================================================================================================

std::vector<int> BrainViewCore::enabledViewportIndices() const
{
    std::vector<int> vps;
    if (m_viewMode == MultiView) {
        for (int i = 0; i < std::ssize(m_subViews); ++i) {
            if (m_subViews[i].enabled)
                vps.push_back(i);
        }
        if (vps.empty())
            vps.push_back(0);
    } else {
        vps.push_back(0);
    }
    return vps;
}

//=============================================================================================================

void BrainViewCore::updateOverlayLayout()
{
    const std::vector<int> viewports = enabledViewportIndices();
    std::vector<int> presets;
    presets.reserve(m_subViews.size());
    for (const SubView& subView : m_subViews) {
        presets.push_back(subView.preset);
    }
    viz::BrainViewOverlay::LayoutInput input;
    input.size = toPixelSize(hostSize());
    input.multi_view = m_viewMode == MultiView;
    input.viewports = viewports;
    input.presets = presets;
    input.edit_target = m_visualizationEditTarget;
    input.info_visible = m_infoPanelVisible;
    input.layout = &m_interaction.layout();
    m_overlay.update_layout(input, QtTextMetrics(overlayBaseFont()));
    markOverlayDirty();
}

//=============================================================================================================

void BrainViewCore::loadMultiViewSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("BrainView"));

    m_interaction.layout().set_split_x(settings.value("multiSplitX", 0.5f).toFloat());
    m_interaction.layout().set_split_y(settings.value("multiSplitY", 0.5f).toFloat());

    const int savedViewMode = settings.value("viewMode", static_cast<int>(SingleView)).toInt();
    m_viewCount = std::clamp(settings.value("viewCount", 1).toInt(), 1, static_cast<int>(m_subViews.size()));
    // showSingleView keeps the pane count for the next showMultiView; one pane is always the single view
    m_viewMode = (savedViewMode == static_cast<int>(MultiView) && m_viewCount > 1) ? MultiView : SingleView;

    Eigen::Quaternionf cameraRotation = m_singleView.rotation;
    const bool hasCameraQuat = settings.contains("cameraRotW") && settings.contains("cameraRotX") && settings.contains("cameraRotY") && settings.contains("cameraRotZ");
    if (hasCameraQuat) {
        const float w = settings.value("cameraRotW", 1.0f).toFloat();
        const float x = settings.value("cameraRotX", 0.0f).toFloat();
        const float y = settings.value("cameraRotY", 0.0f).toFloat();
        const float z = settings.value("cameraRotZ", 0.0f).toFloat();
        cameraRotation = Eigen::Quaternionf(w, x, y, z);
        if (cameraRotation.squaredNorm() <= std::numeric_limits<float>::epsilon()) {
            cameraRotation = Eigen::Quaternionf::Identity();
        } else {
            cameraRotation.normalize();
        }
    }

    // Reset per-index defaults, then load saved state on top
    for (int i = 0; i < std::ssize(m_subViews); ++i) {
        m_subViews[i] = SubView::defaultForIndex(i);
        m_subViews[i].enabled = (i < m_viewCount);
    }

    // Delegate per-SubView serialization
    m_singleView.load(settings, "single_", cameraRotation);
    // The single view turns by the camera rotation, which is stored on its own
    m_singleView.rotation = cameraRotation;
    for (int i = 0; i < std::ssize(m_subViews); ++i)
        m_subViews[i].load(settings, QStringLiteral("multi%1_").arg(i), cameraRotation);

    const int maxIdx = static_cast<int>(m_subViews.size()) - 1;
    m_visualizationEditTarget = normalizedVisualizationTarget(
        settings.value("visualizationEditTarget", -1).toInt(), maxIdx);

    m_infoPanelVisible = settings.value("infoPanelVisible", true).toBool();

    settings.endGroup();

    setVisualizationEditTarget(m_visualizationEditTarget);

    // Notify observers that persisted state has been restored. Defer to
    // the next event-loop tick so that callers constructing BrainView
    // and connecting to these signals immediately afterwards still
    // receive the initial state — direct emits from inside the ctor
    // chain would fire before any external connect() call.
    QTimer::singleShot(0, this, [this]() {
        emit viewCountChanged(m_viewCount);
        emit shaderModeChanged(QString::fromStdString(shaderModeName(m_brainShaderMode)));
    });
}

//=============================================================================================================

void BrainViewCore::saveMultiViewSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("BrainView"));
    settings.setValue("multiSplitX", m_interaction.layout().split_x());
    settings.setValue("multiSplitY", m_interaction.layout().split_y());
    settings.setValue("viewMode", static_cast<int>(m_viewMode));
    settings.setValue("viewCount", m_viewCount);
    settings.setValue("cameraRotW", m_singleView.rotation.w());
    settings.setValue("cameraRotX", m_singleView.rotation.x());
    settings.setValue("cameraRotY", m_singleView.rotation.y());
    settings.setValue("cameraRotZ", m_singleView.rotation.z());
    for (int i = 0; i < std::ssize(m_subViews); ++i)
        settings.setValue(QStringLiteral("viewportEnabled%1").arg(i), m_subViews[i].enabled);
    settings.setValue("visualizationEditTarget", m_visualizationEditTarget);
    settings.setValue("infoPanelVisible", m_infoPanelVisible);

    // Delegate per-SubView serialization
    m_singleView.save(settings, "single_");
    for (int i = 0; i < std::ssize(m_subViews); ++i)
        m_subViews[i].save(settings, QStringLiteral("multi%1_").arg(i));

    settings.endGroup();
}

//=============================================================================================================

void BrainViewCore::setViewportEnabled(int index, bool enabled)
{
    if (index >= 0 && index < std::ssize(m_subViews)) {
        m_subViews[index].enabled = enabled;
        saveMultiViewSettings();
        updateOverlayLayout();
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::setViewportCameraPreset(int index, int preset)
{
    if (index < 0 || index >= static_cast<int>(m_subViews.size()))
        return;
    preset = viz::clamp_view_preset(preset);
    if (m_subViews[index].preset == preset)
        return;
    m_subViews[index].preset = preset;
    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::resetSingleViewCameraState()
{
    m_singleView.rotation = Eigen::Quaternionf::Identity();
    m_singleView.zoom = 0.0f;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::resetViewportCameraState(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subViews.size())) {
        return;
    }

    m_subViews[index].zoom = 0.0f;
    m_subViews[index].pan = Eigen::Vector2f::Zero();
    m_subViews[index].rotation = Eigen::Quaternionf::Identity();
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::resetAllSubViewState()
{
    m_singleView = SubView{};
    for (int i = 0; i < std::ssize(m_subViews); ++i) {
        const bool wasEnabled = m_subViews[i].enabled;
        m_subViews[i] = SubView::defaultForIndex(i);
        m_subViews[i].enabled = wasEnabled;
    }
    saveMultiViewSettings();
    updateOverlayLayout();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

int BrainViewCore::viewportCameraPreset(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_subViews.size()))
        return -1;
    return viz::clamp_view_preset(m_subViews[index].preset);
}

//=============================================================================================================

void BrainViewCore::setInfoPanelVisible(bool visible)
{
    m_infoPanelVisible = visible;
    saveMultiViewSettings();
    updateOverlayLayout();
}

//=============================================================================================================

void BrainViewCore::handleResize()
{
    updateOverlayLayout();
}

//=============================================================================================================

void BrainViewCore::initialize(QRhi* rhi, QRhiTexture* colorTexture)
{
    m_renderer = std::make_unique<BrainRenderer>();

    // Create dual render targets (clearing + preserving) sharing the
    // host's color texture.
    if (rhi && colorTexture) {
        m_renderer->ensureRenderTargets(rhi, colorTexture, colorTexture->pixelSize());
    }
    markOverlayDirty();
}

//=============================================================================================================

void BrainViewCore::releaseResources()
{
    // The renderer owns every GPU resource of the scene, so dropping it frees them all
    m_renderer.reset();
}

//=============================================================================================================

bool BrainViewCore::hasRenderer() const
{
    return m_renderer != nullptr;
}

//=============================================================================================================

void BrainViewCore::render(QRhi* rhi, QRhiTexture* colorTexture, int sampleCount, QRhiCommandBuffer* cb)
{
    if (!rhi || !colorTexture || !cb) {
        return;
    }
    if (!m_renderer) {
        m_renderer = std::make_unique<BrainRenderer>();
    }
    m_renderer->setExternalRenderTarget(nullptr);
    m_renderer->ensureRenderTargets(rhi, colorTexture, colorTexture->pixelSize());
    renderFrame(rhi, sampleCount, cb);
}

//=============================================================================================================

void BrainViewCore::render(QRhi* rhi, QRhiRenderTarget* renderTarget, QRhiCommandBuffer* cb)
{
    if (!rhi || !renderTarget || !cb) {
        return;
    }
    if (!m_renderer) {
        m_renderer = std::make_unique<BrainRenderer>();
    }
    m_renderer->setExternalRenderTarget(renderTarget);
    renderFrame(rhi, renderTarget->sampleCount(), cb);
}

//=============================================================================================================

void BrainViewCore::renderFrame(QRhi* rhi, int sampleCount, QRhiCommandBuffer* cb)
{
    m_inRender = true;
    const QImage noOverlay;
    const auto overlayFor = [this, &noOverlay](const QSize& pixelSize) -> const QImage& {
        if (m_overlaySuppressed) {
            return noOverlay;
        }
        const qreal dpr = m_host ? m_host->devicePixelRatio() : 1.0;
        if (m_overlayDirty || m_overlayCache.size() != pixelSize || m_overlayCache.devicePixelRatio() != dpr) {
            m_overlayCache = overlayImage(pixelSize, dpr);
            m_overlayDirty = false;
        }
        return m_overlayCache;
    };

    // Check if there is anything to render
    bool hasSurfaces = !m_surfaces.empty();
    bool hasDipoles = !m_itemDipoleMap.empty() || m_dipoles; // Check managed dipoles too

    // If absolutely nothing is loaded, render black background
    if (!hasSurfaces && !hasDipoles && !m_electrodes) {
        // No surface loaded: render a black background (and the overlay) instead of leaving the target uninitialized
        m_renderer->initialize(rhi, m_renderer->rtClear()->renderPassDescriptor(), sampleCount);
        QRhiResourceUpdateBatch* updates = rhi->nextResourceUpdateBatch();
        m_renderer->prepareScreenOverlay(rhi, updates, overlayFor(m_renderer->rtClear()->pixelSize()));
        m_renderer->beginFrame(cb, updates);
        m_renderer->issueScreenOverlayDraw(cb);
        m_renderer->endPass(cb);
        m_inRender = false;
        return;
    }

    // Ensure active surface pointer is valid if possible, otherwise just use first available for stats
    if (!m_activeSurface && !m_surfaces.empty()) {
        m_activeSurface = m_surfaces.begin()->second;
    }


    m_frameCount++;
    const std::chrono::duration<float> fpsElapsed = std::chrono::steady_clock::now() - m_fpsStart;
    if (fpsElapsed >= std::chrono::milliseconds(500)) {
        float fps = m_frameCount / fpsElapsed.count();

        // Recount vertices only when surface list/visibility changed
        if (m_vertexCountDirty) {
            auto countVerticesForSubView = [this](const SubView& sv) -> qint64 {
                qint64 total = 0;
                for (const auto& [key, surface] : m_surfaces) {
                    if (!surface)
                        continue;
                    if (!sv.shouldRenderSurface(key))
                        continue;
                    if (SubView::isBrainSurfaceKey(key)) {
                        if (!sv.matchesSurfaceType(key))
                            continue;
                    } else {
                        if (!surface->isVisible())
                            continue;
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
            m_cachedVertexCount = vCount;
            m_vertexCountDirty = false;
        }

        m_overlay.set_text(viz::BrainViewOverlay::Item::FpsLabel, std::format("FPS: {:.1f}\nVertices: {}", fps, m_cachedVertexCount));
        updateOverlayLayout();
        m_frameCount = 0;
        m_fpsStart = std::chrono::steady_clock::now();
    }

    // Initialize renderer
    m_renderer->initialize(rhi, m_renderer->rtClear()->renderPassDescriptor(), sampleCount);

    // Determine viewport configuration
    QSize outputSize = m_renderer->rtClear()->pixelSize();

    // Build list of enabled viewports
    const auto enabledViewports = enabledViewportIndices();
    int numEnabled = static_cast<int>(enabledViewports.size());

    // ── Prepare phase ──────────────────────────────────────────────────
    // Every upload and uniform of the frame goes into one batch that
    // beginFrame() commits: QRhi forbids resource updates inside a pass.
    // The prepare calls upload only the streams that changed, and only for
    // objects that are drawn. Overlay modes and highlights are per-draw
    // uniforms, so panes with different overlays share the same buffers.
    m_renderer->newFrame();
    QRhiResourceUpdateBatch* updates = rhi->nextResourceUpdateBatch();
    if (m_videoOverlay && m_videoOverlay->isEnabled()) {
        m_renderer->prepareVideoOverlay(rhi, updates, m_videoOverlay.get());
    }
    for (int i = 0; i < kMaxSliceSlots; ++i) {
        m_renderer->prepareSlice(rhi, updates, (m_slices[i] && m_sliceVisible[i]) ? m_slices[i] : nullptr, i);
    }

    BrainSurface* videoOverlayTargetSurface = nullptr;
    const bool hasVideoOverlay = m_videoOverlay && m_videoOverlay->isEnabled() && m_videoOverlay->hasFrame();
    if (hasVideoOverlay) {
        // Target is always the head surface (BEM head or TissueSkin).
        if (const auto head = m_surfaces.find("bem_head"); head != m_surfaces.end()) {
            videoOverlayTargetSurface = head->second.get();
        } else {
            for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
                if (it->second && it->second->tissueType() == BrainSurface::TissueSkin) {
                    videoOverlayTargetSurface = it->second.get();
                    break;
                }
            }
        }
    }

    struct DrawItem
    {
        BrainSurface* surface;
        mne::viz::ShaderMode mode;
        float overlayMode;
        float distSq;      // for transparent back-to-front sort
        int uniformOffset; // filled by prepareSurfaceDraw
    };

    // What each pane draws, with the uniform slots prepared for it
    struct PaneDraws
    {
        QRhiViewport viewport;
        QRhiScissor scissor;
        std::vector<DrawItem> opaqueDraws;
        std::vector<DrawItem> transparentDraws;
        int sliceOffsets[kMaxSliceSlots] = {-1, -1, -1};
        int videoOverlayOffset = -1;
        int electrodeOffset = -1;
        std::vector<std::pair<DipoleObject*, int>> dipoleDraws;
        int networkOffset = -1;
        int headPathOffset = -1;
        int pointerOffset = -1;
    };
    std::vector<PaneDraws> panes(static_cast<size_t>(numEnabled));

    for (int slot = 0; slot < numEnabled; ++slot) {
        PaneDraws& pane = panes[static_cast<size_t>(slot)];
        int vp = (m_viewMode == MultiView) ? enabledViewports[slot] : 0;
        const SubView& sv = (m_viewMode == MultiView) ? m_subViews[vp] : m_singleView;

        // The scene leaves the separator lines to the right and below a pane free
        const viz::PaneLayout& layout = m_interaction.layout();
        const viz::PixelRect renderPixels = (m_viewMode == MultiView)
            ? layout.inset_for_separator(layout.slot_rect(slot, numEnabled, toPixelSize(outputSize)), slot, numEnabled)
            : viz::PixelRect{0, 0, outputSize.width(), outputSize.height()};
        const QRect renderRect = toQRect(renderPixels);

        const int viewX = renderRect.x();
        const int viewY = outputSize.height() - (renderRect.y() + renderRect.height());
        const int viewW = std::max(1, renderRect.width());
        const int viewH = std::max(1, renderRect.height());

        pane.viewport = QRhiViewport(viewX, viewY, viewW, viewH);
        pane.scissor = QRhiScissor(viewX, viewY, viewW, viewH);
        const float aspectRatio = float(viewW) / float(viewH);

        // Calculate camera for this viewport
        m_camera.set_scene_center(toEigen(m_cameraFocusOverride ? m_cameraFocusCenter : m_sceneCenter));
        m_camera.set_scene_size(m_cameraFocusOverride ? m_cameraFocusSize : m_sceneSize);
        const viz::CameraMatrices cam = (m_viewMode == MultiView)
            ? m_camera.pane_view(sv, aspectRatio)
            : m_camera.single_view(m_singleView.rotation, m_singleView.zoom, aspectRatio);
        const Eigen::Matrix4f pvm = cam.pvm();

        BrainRenderer::SceneData sceneData;
        sceneData.mvp = rhi->clipSpaceCorrMatrix() * toQMatrix(pvm);

        sceneData.cameraPos = toQVector3D(cam.eye);
        sceneData.lightDir = sceneData.cameraPos.normalized();
        sceneData.lightingEnabled = m_lightingEnabled;

        // Per-draw overlayMode uniform — the shader selects the vertex colour
        // channel (curvature / annotation) so no per-pane vertex buffer
        // re-uploads are needed.
        sceneData.overlayMode = static_cast<float>(sv.overlayMode);

        // Pass 1: Opaque Surfaces (Brain surfaces)
        // Use viewport-specific shader from subview
        mne::viz::ShaderMode currentShader = sv.brainShader;
        const std::string overlayName = visualizationModeName(sv.overlayMode);

        // Collect matched brain surface keys for this pane's info panel
        mne::StringList drawnKeys;

        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            if (!sv.matchesSurfaceType(it->first))
                continue;
            if (!sv.shouldRenderSurface(it->first))
                continue;
            drawnKeys.push_back(it->first);
            BrainSurface* surf = it->second.get();
            pane.opaqueDraws.push_back({surf, currentShader, sceneData.overlayMode, 0.0f,
                                        m_renderer->prepareSurfaceDraw(updates, sceneData, surf)});
        }

        // Info panel; relayout only on change
        std::string info = std::format("Shader: {}\nSurface: {}\nOverlay: {}\nDrawn: {}", shaderModeName(currentShader),
                                       sv.surfaceType, overlayName, drawnKeys.empty() ? std::string("none") : mne::join(drawnKeys, ", "));
        const bool infoChanged = (m_viewMode == MultiView)
            ? m_overlay.set_text(viz::BrainViewOverlay::Item::ViewportInfoLabel, std::move(info), vp)
            : m_overlay.set_text(viz::BrainViewOverlay::Item::SingleViewInfoLabel, std::move(info));
        if (infoChanged) {
            updateOverlayLayout();
        }

        // Single pass over m_surfaces categorises non-brain items into opaque
        // and transparent draw lists.

        // Determine per-viewport field-map visibility
        const bool megFieldVisible = sv.visibility.megFieldMap;
        const bool eegFieldVisible = sv.visibility.eegFieldMap;
        const mne::viz::ShaderMode currentBemShader = sv.bemShader;
        const std::string& megFieldKey = m_fieldMapper.megSurfaceKey();
        const std::string& eegFieldKey = m_fieldMapper.eegSurfaceKey();

        // Brain surfaces were added above; the other opaque items follow them
        std::vector<DrawItem> opaqueDraws;
        std::vector<DrawItem> transparentDraws;

        for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
            const std::string& key = it->first;
            BrainSurface* surf = it->second.get();

            if (SubView::isBrainSurfaceKey(key)) {
                // Brain surfaces already rendered above
                continue;
            } else if (key.starts_with("srcsp_") || key.starts_with("dig_")) {
                if (!sv.shouldRenderSurface(key))
                    continue;
                if (!surf->isVisible())
                    continue;
                if (key.starts_with("dig_live_t_") || key.starts_with("dig_ray_") || key.starts_with("dig_probe_")) {
                    Eigen::Vector3f bminEigen, bmaxEigen;
                    surf->boundingBox(bminEigen, bmaxEigen);
                    QVector3D bmin = toQVector3D(bminEigen), bmax = toQVector3D(bmaxEigen);
                    QVector3D ctr = (bmin + bmax) * 0.5f;
                    float dist = (sceneData.cameraPos - ctr).lengthSquared();
                    transparentDraws.push_back({surf, mne::viz::Holographic,
                                                static_cast<float>(mne::viz::ModeScientific),
                                                dist, -1});
                } else {
                    opaqueDraws.push_back({surf, currentShader,
                                           static_cast<float>(mne::viz::ModeScientific),
                                           0.0f, -1});
                }
            } else {
                bool isSensor = key.starts_with("sens_");
                bool isBem = key.starts_with("bem_");
                if (!isSensor && !isBem)
                    continue;
                if (!sv.shouldRenderSurface(key))
                    continue;
                if (!surf->isVisible())
                    continue;

                Eigen::Vector3f bminEigen, bmaxEigen;
                surf->boundingBox(bminEigen, bmaxEigen);
                QVector3D bmin = toQVector3D(bminEigen), bmax = toQVector3D(bmaxEigen);
                QVector3D center = (bmin + bmax) * 0.5f;
                float dist = (sceneData.cameraPos - center).lengthSquared();

                auto mode = isBem ? currentBemShader : mne::viz::Holographic;
                float itemOverlay = static_cast<float>(mne::viz::ModeScientific);
                if (key == megFieldKey && !megFieldVisible)
                    itemOverlay = static_cast<float>(mne::viz::ModeSurface);
                else if (key == eegFieldKey && !eegFieldVisible)
                    itemOverlay = static_cast<float>(mne::viz::ModeSurface);

                transparentDraws.push_back({surf, mode, itemOverlay, dist, -1});
            }
        }

        // Sort transparent items back-to-front for correct alpha blending
        std::sort(transparentDraws.begin(), transparentDraws.end(),
                  [](const DrawItem& a, const DrawItem& b) { return a.distSq > b.distSq; });

        BrainRenderer::SceneData itemData = sceneData;
        for (auto& item : opaqueDraws) {
            itemData.overlayMode = item.overlayMode;
            item.uniformOffset = m_renderer->prepareSurfaceDraw(updates, itemData, item.surface);
        }
        pane.opaqueDraws.insert(pane.opaqueDraws.end(), opaqueDraws.begin(), opaqueDraws.end());
        for (auto& item : transparentDraws) {
            itemData.overlayMode = item.overlayMode;
            item.uniformOffset = m_renderer->prepareSurfaceDraw(updates, itemData, item.surface);
        }
        pane.transparentDraws = transparentDraws;
        pane.electrodeOffset = m_renderer->prepareElectrodeDraw(updates, sceneData, m_electrodes);

        if (sv.visibility.mriSlices) {
            for (int i = 0; i < kMaxSliceSlots; ++i) {
                if (m_slices[i] && m_sliceVisible[i]) {
                    pane.sliceOffsets[i] = m_renderer->prepareSliceDraw(updates, sceneData, i);
                }
            }
        }

        if (hasVideoOverlay) {
            pane.videoOverlayOffset = m_renderer->prepareVideoOverlayDraw(updates, sceneData, m_videoOverlay.get(), videoOverlayTargetSurface);
        }

        // The head movement path is not connectivity data, so it is not tied to the network
        // visibility flag.
        if (sv.visibility.dipoles) {
            for (auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end(); ++it) {
                const int offset = m_renderer->prepareDipoleDraw(updates, sceneData, it->second.get());
                if (offset >= 0)
                    pane.dipoleDraws.push_back({it->second.get(), offset});
            }
            const int offset = m_renderer->prepareDipoleDraw(updates, sceneData, m_dipoles.get());
            if (offset >= 0)
                pane.dipoleDraws.push_back({m_dipoles.get(), offset});
        }
        if (sv.visibility.network) {
            pane.networkOffset = m_renderer->prepareNetworkDraw(updates, sceneData, m_network.get());
        }
        pane.headPathOffset = m_renderer->preparePolylineDraw(updates, sceneData, m_headPath.get());

        // Intersection Pointer
        if (m_hasIntersection && m_debugPointerSurface) {
            BrainRenderer::SceneData debugSceneData = sceneData;
            debugSceneData.overlayMode = 0.0f; // pass-through for holographic shell

            debugSceneData.mvp = rhi->clipSpaceCorrMatrix() * toQMatrix(pvm * viz::translation_matrix(toEigen(m_lastIntersectionPoint)));

            pane.pointerOffset = m_renderer->prepareSurfaceDraw(updates, debugSceneData, m_debugPointerSurface.get());
        }
    } // End of viewport loop

    // The 2-D overlay is painted after the panes updated their info labels
    m_renderer->prepareScreenOverlay(rhi, updates, overlayFor(outputSize));

    // ── Draw phase: bind and draw only ──────────────────────────────────
    m_renderer->beginFrame(cb, updates);

    for (const PaneDraws& pane : panes) {
        cb->setViewport(pane.viewport);
        cb->setScissor(pane.scissor);

        for (const auto& item : pane.opaqueDraws)
            m_renderer->issueSurfaceDraw(cb, item.surface, item.mode, item.uniformOffset);
        m_renderer->issueElectrodeDraw(cb, m_electrodes, pane.electrodeOffset);

        // Issue MRI slice draws after opaque surfaces but before the holographic
        // brain.  Background voxels are discarded in the shader; remaining anatomy
        // alpha-blends into the framebuffer.  The subsequent holographic additive
        // pass (SrcAlpha + One) adds its glow on top without being dimmed.
        // depthTest=true, depthWrite=false keeps slices behind opaque geometry.
        for (int i = 0; i < kMaxSliceSlots; ++i) {
            m_renderer->issueSliceDraw(cb, i, pane.sliceOffsets[i]);
        }

        // The video overlay goes right after its target surface, or last
        bool videoOverlayDrawn = false;
        for (const auto& item : pane.transparentDraws) {
            m_renderer->issueSurfaceDraw(cb, item.surface, item.mode, item.uniformOffset);
            if (item.surface == videoOverlayTargetSurface && !videoOverlayDrawn) {
                m_renderer->issueVideoOverlayDraw(cb, m_videoOverlay.get(), videoOverlayTargetSurface, pane.videoOverlayOffset);
                videoOverlayDrawn = true;
            }
        }
        if (!videoOverlayDrawn) {
            m_renderer->issueVideoOverlayDraw(cb, m_videoOverlay.get(), videoOverlayTargetSurface, pane.videoOverlayOffset);
        }

        for (const auto& [dipoles, offset] : pane.dipoleDraws)
            m_renderer->issueDipoleDraw(cb, dipoles, offset);
        m_renderer->issueNetworkDraw(cb, m_network.get(), pane.networkOffset);
        m_renderer->issuePolylineDraw(cb, m_headPath.get(), pane.headPathOffset);

        m_renderer->issueSurfaceDraw(cb, m_debugPointerSurface.get(), mne::viz::Holographic, pane.pointerOffset);
    }

    // Labels and pane separators on top of every pane
    m_renderer->issueScreenOverlayDraw(cb);

    m_renderer->endPass(cb);
    m_inRender = false;
}

//=============================================================================================================

void BrainViewCore::handleMousePress(QMouseEvent* event)
{
    syncInteraction();
    m_interaction.handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Press));
}

//=============================================================================================================

void BrainViewCore::handleMouseMove(QMouseEvent* event)
{
    syncInteraction();
    m_interaction.handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Move));
}

//=============================================================================================================

void BrainViewCore::handleMouseRelease(QMouseEvent* event)
{
    syncInteraction();
    m_interaction.handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Release));
}

//=============================================================================================================

bool BrainViewCore::handleMouseDoubleClick(QMouseEvent* event)
{
    syncInteraction();
    return m_interaction.handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::DoubleClick));
}

//=============================================================================================================

void BrainViewCore::handleWheel(QWheelEvent* event)
{
    syncInteraction();
    m_interaction.handle_wheel(toSceneWheelEvent(*event));
}

//=============================================================================================================

void BrainViewCore::handleKeyPress(QKeyEvent* event)
{
    // Only printable keys have a meaning here; Qt's codes of those are their Latin-1 characters
    viz::KeyEvent key;
    if (event->key() >= Qt::Key_Space && event->key() <= Qt::Key_ydiaeresis) {
        key.key = viz::Key::Character;
        key.character = static_cast<char32_t>(event->key());
    }
    key.timestamp_ms = sceneTimestampMs();
    syncInteraction();
    m_interaction.handle_key(key);
}

//=============================================================================================================

bool BrainViewCore::loadSourceEstimate(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath)
{
    return m_sourceManager.load(lhPath, rhPath, m_surfaces, m_activeSurfaceType);
}

//=============================================================================================================

void BrainViewCore::onSourceEstimateLoaded(int numTimePoints)
{
    setVisualizationMode("Source Estimate");
    emit sourceEstimateLoaded(numTimePoints);
    setTimePoint(0);
}

//=============================================================================================================

void BrainViewCore::setTimePoint(int index)
{
    m_sourceManager.setTimePoint(index, m_surfaces, m_singleView, m_subViews);
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setSourceColormap(const std::string& name)
{
    m_sourceManager.setColormap(name);
    setTimePoint(m_sourceManager.currentTimePoint());
}

//=============================================================================================================

void BrainViewCore::setSourceThresholds(float min, float mid, float max)
{
    m_sourceManager.setThresholds(min, mid, max);
    setTimePoint(m_sourceManager.currentTimePoint());
}

//=============================================================================================================

void BrainViewCore::startRealtimeStreaming()
{
    setVisualizationMode("Source Estimate");
    m_sourceManager.startStreaming(m_surfaces, m_singleView, m_subViews);
}

//=============================================================================================================

void BrainViewCore::stopRealtimeStreaming()
{
    m_sourceManager.stopStreaming();
}

//=============================================================================================================

bool BrainViewCore::isRealtimeStreaming() const
{
    return m_sourceManager.isStreaming();
}

//=============================================================================================================

void BrainViewCore::pushRealtimeSourceData(const Eigen::VectorXd& matData)
{
    m_sourceManager.pushData(matData);
}

//=============================================================================================================

void BrainViewCore::setRealtimeInterval(int msec)
{
    m_sourceManager.setInterval(msec);
}

//=============================================================================================================

void BrainViewCore::setRealtimeLooping(bool enabled)
{
    m_sourceManager.setLooping(enabled);
}

//=============================================================================================================

void BrainViewCore::onRealtimeColorsAvailable(const std::vector<uint32_t>& colorsLh,
                                              const std::vector<uint32_t>& colorsRh)
{
    // Apply colors to all brain surfaces matching active surface types
    std::set<std::string> activeTypes;
    activeTypes.insert(m_singleView.surfaceType);
    for (const SubView& subView : m_subViews) {
        activeTypes.insert(subView.surfaceType);
    }

    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (!it->second || it->second->tissueType() != BrainSurface::TissueBrain)
            continue;

        for (const std::string& type : activeTypes) {
            if (it->first.ends_with(type)) {
                int hemi = it->second->hemi();
                const std::vector<uint32_t>& colors = (hemi == 0) ? colorsLh : colorsRh;
                if (!colors.empty()) {
                    it->second->applySourceEstimateColors(colors);
                }
                break;
            }
        }
    }

    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

bool BrainViewCore::loadSensorField(const std::filesystem::path& evokedPath, int aveIndex)
{
    auto evoked = DataLoader::loadEvoked(evokedPath, aveIndex);
    if (evoked.empty())
        return false;

    // Preserve the current time point when switching between evoked sets
    // that share the same sensor configuration (same file, different condition).
    const int previousTimePoint = m_fieldMapper.timePoint();
    const bool canReuse = m_fieldMapper.hasMappingFor(evoked);

    m_fieldMapper.setEvoked(evoked);

    if (!canReuse) {
        // Sensor config changed — full rebuild required (also precomputes global range)
        if (!m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans)) {
            m_fieldMapper.setEvoked(mne::FiffEvoked()); // Clear state on failure
            return false;
        }
    } else {
        // Mapping reused — recompute normalization for new evoked data
        m_fieldMapper.computeNormRange();
    }

    // Clamp preserved time point to the range of the new evoked data
    const int numTimes = static_cast<int>(m_fieldMapper.evoked().times.size());
    const int tp = std::clamp(previousTimePoint, 0, std::max(0, numTimes - 1));

    emit sensorFieldLoaded(numTimes, tp);
    setSensorFieldTimePoint(tp);
    return true;
}

//=============================================================================================================

std::vector<std::string> BrainViewCore::probeEvokedSets(const std::filesystem::path& evokedPath)
{
    return DataLoader::probeEvokedSets(evokedPath);
}

//=============================================================================================================

void BrainViewCore::setSensorFieldTimePoint(int index)
{
    if (!m_fieldMapper.isLoaded() || m_fieldMapper.evoked().empty()) {
        return;
    }

    int maxIdx = static_cast<int>(m_fieldMapper.evoked().times.size()) - 1;
    if (maxIdx < 0) {
        return;
    }

    m_fieldMapper.setTimePoint(std::clamp(index, 0, maxIdx));
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    emit sensorFieldTimePointChanged(m_fieldMapper.timePoint(), m_fieldMapper.evoked().times(m_fieldMapper.timePoint()));
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setSensorFieldVisible(const std::string& type, bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (type == "MEG") {
        profile.megFieldMap = visible;
    } else if (type == "EEG") {
        profile.eegFieldMap = visible;
    } else {
        return;
    }

    saveMultiViewSettings();
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setSensorFieldContourVisible(const std::string& type, bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (type == "MEG") {
        profile.megFieldContours = visible;
    } else if (type == "EEG") {
        profile.eegFieldContours = visible;
    } else {
        return;
    }

    saveMultiViewSettings();
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setMegFieldMapOnHead(bool useHead)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    if (profile.megFieldMapOnHead == useHead && m_fieldMapper.megFieldMapOnHead() == useHead) {
        return;
    }

    profile.megFieldMapOnHead = useHead;
    m_fieldMapper.setMegFieldMapOnHead(useHead);
    saveMultiViewSettings();
    if (m_fieldMapper.isLoaded()) {
        m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::setSensorFieldColormap(const std::string& name)
{
    if (m_fieldMapper.colormap() == name) {
        return;
    }
    m_fieldMapper.setColormap(name);
    m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

float BrainViewCore::stcStep() const
{
    return m_sourceManager.tstep();
}

//=============================================================================================================

float BrainViewCore::stcTmin() const
{
    return m_sourceManager.tmin();
}

//=============================================================================================================

int BrainViewCore::stcNumTimePoints() const
{
    return m_sourceManager.numTimePoints();
}

//=============================================================================================================

int BrainViewCore::closestSensorFieldIndex(float timeSec) const
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

int BrainViewCore::closestStcIndex(float timeSec) const
{
    return m_sourceManager.closestIndex(timeSec);
}

//=============================================================================================================

bool BrainViewCore::sensorFieldTimeRange(float& tmin, float& tmax) const
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

void BrainViewCore::startRealtimeSensorStreaming(const std::string& modality)
{
    m_sensorStreamManager.startStreaming(modality, m_fieldMapper, m_surfaces);
}

//=============================================================================================================

void BrainViewCore::stopRealtimeSensorStreaming()
{
    m_sensorStreamManager.stopStreaming();
}

//=============================================================================================================

bool BrainViewCore::isRealtimeSensorStreaming() const
{
    return m_sensorStreamManager.isStreaming();
}

//=============================================================================================================

void BrainViewCore::pushRealtimeSensorData(const Eigen::VectorXf& vecData)
{
    m_sensorStreamManager.pushData(vecData);
}

//=============================================================================================================

void BrainViewCore::setRealtimeSensorInterval(int msec)
{
    m_sensorStreamManager.setInterval(msec);
}

//=============================================================================================================

void BrainViewCore::setRealtimeSensorLooping(bool enabled)
{
    m_sensorStreamManager.setLooping(enabled);
}

//=============================================================================================================

void BrainViewCore::setRealtimeSensorAverages(int numAvr)
{
    m_sensorStreamManager.setAverages(numAvr);
}

//=============================================================================================================

void BrainViewCore::setRealtimeSensorColormap(const std::string& name)
{
    m_sensorStreamManager.setColormap(name);
}

//=============================================================================================================

void BrainViewCore::onSensorStreamColorsAvailable(const std::string& surfaceKey,
                                                  const std::vector<uint32_t>& colors)
{
    const auto it = m_surfaces.find(surfaceKey);
    if (surfaceKey.empty() || it == m_surfaces.end()) {
        return;
    }

    auto surface = it->second;
    if (surface && !colors.empty()) {
        surface->applySourceEstimateColors(colors);
    }

    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

bool BrainViewCore::loadSensors(const std::filesystem::path& fifPath)
{
    if (!m_model)
        return false;

    auto r = DataLoader::loadSensors(fifPath, m_megHelmetOverridePath);
    if (!r.hasInfo && !r.hasDigitizer)
        return false;

    // Store Device→Head transform for later helmet surface reloads
    m_devHeadTrans = r.devHeadTrans;
    m_hasDevHead = r.hasDevHead;

    if (!r.megGradItems.empty())
        m_model->addSensors("MEG/Grad", std::move(r.megGradItems));
    if (!r.megMagItems.empty())
        m_model->addSensors("MEG/Mag", std::move(r.megMagItems));
    if (!r.eegItems.empty())
        m_model->addSensors("EEG", std::move(r.eegItems));

    if (r.helmetSurface) {
        m_surfaces["sens_surface_meg"] = r.helmetSurface;
    } else {
        mne::Log::warning("BrainView::loadSensors: NO helmet surface returned from DataLoader!");
    }

    if (!r.digitizerPoints.empty())
        m_model->addDigitizerData(r.digitizerPoints.points());

    // Extract cardinal dig points for later fiducial queries
    m_cardinalDigPoints = r.digitizerPoints.pick_types({FIFFV_POINT_CARDINAL});

    return true;
}

//=============================================================================================================

std::map<int, QVector3D> BrainViewCore::cardinalFiducialsInMri() const
{
    std::map<int, QVector3D> result;
    if (m_cardinalDigPoints.empty())
        return result;

    // Cardinal points are in Head coordinates; transform to MRI if available
    const bool haveTrans = (m_headToMriTrans.from != 0 || m_headToMriTrans.to != 0);
    QMatrix4x4 headToMri;
    headToMri.setToIdentity();
    if (haveTrans)
        headToMri = mne::viz::qt::surface_keys::toQMatrix4x4(m_headToMriTrans.trans);

    for (const auto& dp : m_cardinalDigPoints) {
        const QVector3D posHead(dp.r[0], dp.r[1], dp.r[2]);
        result[dp.ident] = haveTrans ? headToMri.map(posHead) : posHead;
    }
    return result;
}

//=============================================================================================================

bool BrainViewCore::bemTopVertexInMri(QVector3D& pos) const
{
    auto it = m_surfaces.find("bem_head");
    if (it == m_surfaces.end() || !it->second)
        return false;

    const BrainSurface* surf = it->second.get();
    Eigen::Vector3f bminEigen, bmaxEigen;
    surf->boundingBox(bminEigen, bmaxEigen);
    QVector3D bmin = toQVector3D(bminEigen), bmax = toQVector3D(bmaxEigen);

    // Top of head = max Z in MRI/surface-RAS (Z = superior)
    pos = QVector3D((bmin.x() + bmax.x()) * 0.5f,
                    (bmin.y() + bmax.y()) * 0.5f,
                    bmax.z());
    return true;
}

//=============================================================================================================

bool BrainViewCore::loadMegHelmetSurface(const std::filesystem::path& helmetFilePath)
{
    auto surface = DataLoader::loadHelmetSurface(helmetFilePath, m_devHeadTrans, m_hasDevHead);
    if (!surface) {
        mne::Log::warning("BrainView::loadMegHelmetSurface: DataLoader returned nullptr!");
        return false;
    }

    m_surfaces["sens_surface_meg"] = surface;
    refreshSensorTransforms();
    updateSceneBounds();
    m_sceneDirty = true;
    requestUpdate();
    return true;
}

//=============================================================================================================

bool BrainViewCore::loadDipoles(const std::filesystem::path& dipPath)
{
    if (!m_model)
        return false;

    auto ecdSet = DataLoader::loadDipoles(dipPath);
    if (ecdSet.size() == 0)
        return false;
    m_model->addDipoles(ecdSet);
    return true;
}

//=============================================================================================================

bool BrainViewCore::loadNetwork(const mne::Network& network, const QString& name)
{
    if (network.nodes().empty())
        return false;

    m_network = std::make_unique<NetworkObject>();
    m_network->load(network);
    m_network->setVisible(true);

    // Also register in the tree model
    if (m_model)
        m_model->addNetwork(network, name);

    m_sceneDirty = true;
    requestUpdate();
    return true;
}

//=============================================================================================================

void BrainViewCore::setNetworkVisible(bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.network = visible;
    m_networkVisible = visible;
    if (m_network)
        m_network->setVisible(visible);
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setNetworkThreshold(double threshold)
{
    if (m_network) {
        m_network->setThreshold(threshold);
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

void BrainViewCore::setNetworkColormap(const std::string& name)
{
    if (m_network) {
        m_network->setColormap(name);
        m_sceneDirty = true;
        requestUpdate();
    }
}

//=============================================================================================================

bool BrainViewCore::loadSourceSpace(const std::filesystem::path& fwdPath)
{
    if (!m_model)
        return false;

    auto srcSpace = DataLoader::loadSourceSpace(fwdPath);
    if (srcSpace.empty())
        return false;
    m_model->addSourceSpace(srcSpace);
    return true;
}

//=============================================================================================================

void BrainViewCore::setSourceSpaceVisible(bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.sourceSpace = visible;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

bool BrainViewCore::loadTransformation(const std::filesystem::path& transPath)
{
    FiffCoordTrans trans;
    if (!DataLoader::loadHeadToMriTransform(transPath, trans))
        return false;

    m_headToMriTrans = trans;
    refreshSensorTransforms();
    return true;
}

//==============================================================================

const QStandardItem* BrainViewCore::itemForSurface(const std::shared_ptr<BrainSurface>& surface) const
{
    const auto it = std::ranges::find_if(m_itemSurfaceMap, [&](const auto& entry) { return entry.second == surface; });
    return it == m_itemSurfaceMap.end() ? nullptr : it->first;
}

//==============================================================================

QMatrix4x4 BrainViewCore::itemTransform(const QStandardItem* item) const
{
    const auto* absItem = dynamic_cast<const AbstractTreeItem*>(item);
    if (!absItem)
        return {};
    QMatrix4x4 trans = absItem->transform();
    // Sensors, digitizer points and dipoles are in head coordinates
    const int type = absItem->type();
    const bool inHead = type == AbstractTreeItem::itemTypeId(AbstractTreeItem::SensorItem) || type == AbstractTreeItem::itemTypeId(AbstractTreeItem::DigitizerItem) || type == AbstractTreeItem::itemTypeId(AbstractTreeItem::DipoleItem);
    if (inHead && m_applySensorTrans && !m_headToMriTrans.empty())
        trans = mne::viz::qt::surface_keys::toQMatrix4x4(m_headToMriTrans.trans) * trans;
    return trans;
}

//==============================================================================

void BrainViewCore::refreshSensorTransforms()
{
    QMatrix4x4 qmat;
    if (m_applySensorTrans && !m_headToMriTrans.empty()) {
        qmat = mne::viz::qt::surface_keys::toQMatrix4x4(m_headToMriTrans.trans);
    }

    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if ((it->first.starts_with("sens_") || it->first.starts_with("dig_")) && it->second) {
            const QStandardItem* item = itemForSurface(it->second);
            it->second->applyTransform(toEigen(item ? itemTransform(item) : qmat));
        }
    }
    for (auto it = m_itemDipoleMap.cbegin(); it != m_itemDipoleMap.cend(); ++it) {
        it->second->applyTransform(itemTransform(it->first));
    }

    if (m_fieldMapper.isLoaded()) {
        m_fieldMapper.buildMapping(m_surfaces, m_headToMriTrans, m_applySensorTrans);
        m_fieldMapper.apply(m_surfaces, m_singleView, m_subViews);
    }
}

//=============================================================================================================

void BrainViewCore::castRay(const QPoint& pos)
{
    // Same camera and pane layout as the render, in logical pixels
    const viz::PixelSize outputSize = toPixelSize(hostSize());
    const auto enabledViewports = enabledViewportIndices();
    const int numEnabled = static_cast<int>(enabledViewports.size());
    const viz::Point point{static_cast<double>(pos.x()), static_cast<double>(pos.y())};

    int activeSlot = 0;
    viz::PixelRect activePane{0, 0, outputSize.width, outputSize.height};
    bool hasValidPane = true;
    if (m_viewMode == MultiView && numEnabled > 1) {
        hasValidPane = false;
        for (int slot = 0; slot < numEnabled; ++slot) {
            const viz::PixelRect pane = m_interaction.layout().slot_rect(slot, numEnabled, outputSize);
            if (pane.contains(point)) {
                activeSlot = slot;
                activePane = pane;
                hasValidPane = true;
                break;
            }
        }
    }

    const int vp = (m_viewMode == MultiView) ? enabledViewports[activeSlot] : 0;
    const SubView& sv = (m_viewMode == MultiView) ? m_subViews[vp] : m_singleView;

    m_camera.set_scene_center(toEigen(m_cameraFocusOverride ? m_cameraFocusCenter : m_sceneCenter));
    m_camera.set_scene_size(m_cameraFocusOverride ? m_cameraFocusSize : m_sceneSize);
    const float aspect = float(std::max(1, activePane.width)) / float(std::max(1, activePane.height));
    const viz::CameraMatrices cam = (m_viewMode == MultiView)
        ? m_camera.pane_view(sv, aspect)
        : m_camera.single_view(m_singleView.rotation, m_singleView.zoom, aspect);

    // ── Unproject screen position to world-space ray ───────────────────
    const std::optional<viz::Ray> ray = viz::ray_from_pixel(point, activePane.to_rect(), cam.pvm());
    if (!ray)
        return;
    const QVector3D rayOrigin = toQVector3D(ray->origin);
    const QVector3D rayDir = toQVector3D(ray->direction);

    // ── Pick against all scene geometry ────────────────────────────────
    RayHit pickResult;
    if (hasValidPane) {
        pickResult = RayPicker::pick(rayOrigin, rayDir, sv, m_surfaces, m_itemSurfaceMap, m_itemDipoleMap);
    }
    m_hasIntersection = pickResult.hit;
    if (pickResult.hit) {
        m_lastIntersectionPoint = pickResult.hitPoint;
    }

    // Electrode contacts in front of everything else take the hover
    int hitContact = -1;
    if (hasValidPane && m_electrodes && m_electrodes->isVisible()) {
        float contactDist = 0.0f;
        hitContact = m_electrodes->intersect(rayOrigin, rayDir, contactDist);
        if (hitContact >= 0 && pickResult.hit && pickResult.distance < contactDist)
            hitContact = -1;
        if (hitContact >= 0) {
            pickResult = RayHit();
            m_hasIntersection = true;
            m_lastIntersectionPoint = rayOrigin + contactDist * rayDir;
        }
        m_electrodes->setHoveredContact(hitContact);
    }

    QStandardItem* hitItem = pickResult.item;
    int hitIndex = pickResult.vertexIndex;

    // ── Build hover label ──────────────────────────────────────────────
    const QString displayLabel = QString::fromStdString(hitContact >= 0 ? m_electrodes->contactLabel(hitContact)
                                                                        : RayPicker::buildLabel(pickResult, m_itemSurfaceMap, m_surfaces));
    const std::string& hitKey = pickResult.surfaceKey;
    int currentRegionId = pickResult.regionId;

    if (displayLabel != m_hoveredRegion) {
        m_hoveredRegion = displayLabel;
        emit hoveredRegionChanged(m_hoveredRegion);
        m_overlay.set_region(m_hoveredRegion.toStdString());
        markOverlayDirty();
    }

    std::string hoveredSurfaceKey;
    if (hitKey.starts_with("sens_surface_meg")) {
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
        if (const auto hovered = m_surfaces.find(m_hoveredSurfaceKey); !m_hoveredSurfaceKey.empty() && hovered != m_surfaces.end()) {
            hovered->second->setSelected(false);
            hovered->second->setSelectedRegion(-1);
            hovered->second->setSelectedVertexRange(-1, 0);
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
                    (absHitSel->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::DigitizerItem));

                if (isDigitizer && m_hoveredIndex >= 0) {
                    const int vertsPerSphere = MeshFactory::sphereVertexCount();
                    int sphereIdx = m_hoveredIndex / vertsPerSphere;
                    m_itemSurfaceMap[m_hoveredItem]->setSelectedVertexRange(
                        sphereIdx * vertsPerSphere, vertsPerSphere);
                } else if (currentRegionId != -1) {
                    m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
                    // Keep the surface selected so the shader gold glow
                    // activates.  The old CPU vertex-color region highlight
                    // was removed to avoid buffer re-uploads on WASM.
                    m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
                } else {
                    m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
                    m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
                }
            } else if (m_itemDipoleMap.contains(m_hoveredItem)) {
                m_itemDipoleMap[m_hoveredItem]->setSelected(m_hoveredIndex, true);
            }
        } else if (!m_hoveredSurfaceKey.empty() && m_surfaces.contains(m_hoveredSurfaceKey)) {
            m_surfaces[m_hoveredSurfaceKey]->setSelected(true);
            m_surfaces[m_hoveredSurfaceKey]->setSelectedRegion(-1);
        }
    } else if (m_hoveredItem && m_itemSurfaceMap.contains(m_hoveredItem)) {
        AbstractTreeItem* absHitUpd = dynamic_cast<AbstractTreeItem*>(m_hoveredItem);
        bool isDigitizer = absHitUpd &&
            (absHitUpd->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::DigitizerItem));

        if (isDigitizer && m_hoveredIndex >= 0) {
            const int vertsPerSphere = MeshFactory::sphereVertexCount();
            int sphereIdx = m_hoveredIndex / vertsPerSphere;
            m_itemSurfaceMap[m_hoveredItem]->setSelectedVertexRange(
                sphereIdx * vertsPerSphere, vertsPerSphere);
        } else if (currentRegionId != -1) {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(currentRegionId);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
        } else {
            m_itemSurfaceMap[m_hoveredItem]->setSelectedRegion(-1);
            m_itemSurfaceMap[m_hoveredItem]->setSelected(true);
        }
    } else if (!m_hoveredSurfaceKey.empty() && m_surfaces.contains(m_hoveredSurfaceKey)) {
        m_surfaces[m_hoveredSurfaceKey]->setSelected(true);
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================
// Data removal
//=============================================================================================================

void BrainViewCore::removeSurfacesByPrefix(std::initializer_list<std::string_view> prefixes)
{
    const auto matches = [&prefixes](std::string_view key) {
        return std::ranges::any_of(prefixes, [key](std::string_view prefix) { return key.starts_with(prefix); });
    };

    // Remove the tree rows first: they are found through the surfaces being removed
    for (auto it = m_itemSurfaceMap.begin(); it != m_itemSurfaceMap.end();) {
        const auto surface = std::ranges::find_if(m_surfaces, [&it](const auto& entry) { return entry.second == it->second; });
        const bool remove = matches(surface == m_surfaces.end() ? std::string_view() : std::string_view(surface->first));
        if (!remove) {
            ++it;
            continue;
        }
        if (m_model) {
            QStandardItem* item = const_cast<QStandardItem*>(it->first);
            if (item->parent())
                item->parent()->removeRow(item->row());
            else
                m_model->removeRow(item->row());
        }
        it = m_itemSurfaceMap.erase(it);
    }
    for (auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if (matches(it->first))
            it = m_surfaces.erase(it);
        else
            ++it;
    }
    updateSceneBounds();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearSurfaces()
{
    m_activeSurface.reset();
    m_activeSurfaceType.clear();
    removeSurfacesByPrefix({"lh_", "rh_"});
}

//=============================================================================================================

void BrainViewCore::clearBem()
{
    removeSurfacesByPrefix({"bem_"});
}

//=============================================================================================================

void BrainViewCore::setElectrodeValues(const std::map<std::string, float>& values)
{
    if (!m_electrodes)
        return;
    m_electrodes->setContactValues(values);

    const auto surfaceOrNull = [this](const std::string& key) -> std::shared_ptr<BrainSurface> {
        const auto it = m_surfaces.find(key);
        return it == m_surfaces.end() ? nullptr : it->second;
    };
    for (const std::string hemi : {"lh", "rh"}) {
        std::shared_ptr<BrainSurface> anatomy = surfaceOrNull(hemi + "_pial");
        if (!anatomy)
            anatomy = surfaceOrNull(hemi + "_white");
        const std::vector<uint32_t> colors = anatomy ? m_electrodes->surfaceColors(anatomy->vertexPositions()) : std::vector<uint32_t>();
        for (auto it = m_surfaces.cbegin(); it != m_surfaces.cend(); ++it) {
            if (!it->first.starts_with(hemi + "_") || !it->second || it->second->tissueType() != BrainSurface::TissueBrain)
                continue;
            if (colors.size() == it->second->vertexCount())
                it->second->applySourceEstimateColors(colors);
            else
                it->second->clearSourceEstimateColors();
        }
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearSourceEstimate()
{
    m_sourceManager.stopStreaming();
    for (auto it = m_surfaces.begin(); it != m_surfaces.end(); ++it) {
        if (it->first.starts_with("lh_") || it->first.starts_with("rh_")) {
            it->second->clearSourceEstimateColors();
        }
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearDipoles()
{
    m_dipoles.reset();

    // Remove dipole items from model and maps
    for (auto it = m_itemDipoleMap.begin(); it != m_itemDipoleMap.end();) {
        if (m_model) {
            QStandardItem* mutableItem = const_cast<QStandardItem*>(it->first);
            if (mutableItem->parent())
                mutableItem->parent()->removeRow(mutableItem->row());
            else
                m_model->removeRow(mutableItem->row());
        }
        it = m_itemDipoleMap.erase(it);
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearSourceSpace()
{
    removeSurfacesByPrefix({"srcsp_"});
}

//=============================================================================================================

void BrainViewCore::setLiveMarkers(const std::vector<LiveMarker>& markers)
{
    // Remove previous live surfaces (no scene-bounds update).
    for (auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if (it->first.starts_with("dig_live_"))
            it = m_surfaces.erase(it);
        else
            ++it;
    }

    for (std::size_t i = 0; i < markers.size(); ++i) {
        auto surf = MeshFactory::createBatchedSpheres(
            {toEigen(markers[i].position)}, markers[i].radius, toColor(markers[i].color));
        surf->setVisible(true);
        m_surfaces[std::format("{}{}", markers[i].transparent ? "dig_live_t_" : "dig_live_", i)] = surf;
    }

    // Intentionally NO updateSceneBounds() — the camera stays locked.
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearLiveMarkers()
{
    bool removed = false;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if (it->first.starts_with("dig_live_")) {
            it = m_surfaces.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (removed) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setLiveRay(const QVector3D& from, const QVector3D& to,
                               const QColor& color, float radius)
{
    // Remove previous ray surface
    m_surfaces.erase("dig_ray_0");

    auto surf = MeshFactory::createCylinder(toEigen(from), toEigen(to), radius, toColor(color));
    surf->setVisible(true);
    m_surfaces["dig_ray_0"] = surf;

    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearLiveRay()
{
    if (m_surfaces.erase("dig_ray_0")) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setProbeVisualization(const QVector3D& tip, const QVector3D& direction,
                                          float length, const QColor& color,
                                          const QColor& glowColor,
                                          const QQuaternion& orientation)
{
    // Remove previous probe surfaces
    m_surfaces.erase("dig_probe_shaft");
    m_surfaces.erase("dig_probe_tip");
    m_surfaces.erase("dig_probe_tipglow");
    m_surfaces.erase("dig_probe_axis_x");
    m_surfaces.erase("dig_probe_axis_y");
    m_surfaces.erase("dig_probe_axis_z");
    m_surfaces.erase("dig_probe_axis_x_tip");
    m_surfaces.erase("dig_probe_axis_y_tip");
    m_surfaces.erase("dig_probe_axis_z_tip");
    m_surfaces.erase("dig_probe_axis_xn_tip");
    m_surfaces.erase("dig_probe_axis_yn_tip");
    m_surfaces.erase("dig_probe_axis_zn_tip");
    m_surfaces.erase("dig_probe_cross_x");
    m_surfaces.erase("dig_probe_cross_y");
    m_surfaces.erase("dig_probe_cross_z");

    // Shaft: only drawn when length > 0
    if (length > 0.0f && !direction.isNull()) {
        const QVector3D shaftEnd = tip - direction * length;
        constexpr float kShaftRadius = 0.0015f;
        auto shaft = MeshFactory::createCylinder(toEigen(tip), toEigen(shaftEnd), kShaftRadius, toColor(color));
        shaft->setVisible(true);
        m_surfaces["dig_probe_shaft"] = shaft;
    }

    // Tip: the focal point of the probe
    constexpr float kTipRadius = 0.0010f; // 1.0 mm
    auto tipSurf = MeshFactory::createBatchedSpheres({toEigen(tip)}, kTipRadius, toColor(color));
    tipSurf->setVisible(true);
    m_surfaces["dig_probe_tip"] = tipSurf;

    // Glow aura: subtle halo around the tip
    if (glowColor.alpha() > 0) {
        constexpr float kGlowTipRadius = 0.003f; // 3 mm glow
        auto tipGlow = MeshFactory::createBatchedSpheres({toEigen(tip)}, kGlowTipRadius, toColor(glowColor));
        tipGlow->setVisible(true);
        m_surfaces["dig_probe_tipglow"] = tipGlow;
    }

    // --- Crosshair rotated with probe orientation (X=red, Y=green, Z=blue) ---
    if (!orientation.isNull()) {
        constexpr float kCrossLen = 0.008f;     // 8 mm per arm
        constexpr float kCrossRadius = 0.0002f; // 0.2 mm — hair-thin

        const QVector3D xDir = orientation.rotatedVector(QVector3D(1, 0, 0)).normalized();
        const QVector3D yDir = orientation.rotatedVector(QVector3D(0, 1, 0)).normalized();
        const QVector3D zDir = orientation.rotatedVector(QVector3D(0, 0, 1)).normalized();

        struct CrossDef
        {
            QVector3D dir;
            QColor color;
            const char* key;
        };
        const CrossDef cross[] = {
            {xDir, QColor(255, 50, 50, 160), "dig_probe_cross_x"},
            {yDir, QColor(50, 220, 50, 160), "dig_probe_cross_y"},
            {zDir, QColor(80, 140, 255, 160), "dig_probe_cross_z"},
        };
        for (const auto& c : cross) {
            const QVector3D from = tip - c.dir * kCrossLen;
            const QVector3D to = tip + c.dir * kCrossLen;
            auto cyl = MeshFactory::createCylinder(toEigen(from), toEigen(to), kCrossRadius, toColor(c.color));
            cyl->setVisible(true);
            m_surfaces[c.key] = cyl;
        }
    }

    // --- Debug coordinate frame (X=red, Y=green, Z=blue) ---
    if (!orientation.isNull()) {
        constexpr float kAxisLen = 0.012f;     // 12 mm per axis arm
        constexpr float kAxisRadius = 0.0004f; // 0.4 mm — thin axis lines
        constexpr float kPosTipR = 0.0012f;    // 1.2 mm — positive-end sphere
        constexpr float kNegTipR = 0.0006f;    // 0.6 mm — negative-end dot (smaller)

        const QVector3D xDir = orientation.rotatedVector(QVector3D(1, 0, 0)).normalized();
        const QVector3D yDir = orientation.rotatedVector(QVector3D(0, 1, 0)).normalized();
        const QVector3D zDir = orientation.rotatedVector(QVector3D(0, 0, 1)).normalized();

        struct AxisDef
        {
            QVector3D dir;
            QColor color;
            const char* cylKey;
            const char* posTipKey;
            const char* negTipKey;
        };
        const AxisDef axes[] = {
            {xDir, QColor(255, 50, 50), "dig_probe_axis_x",
             "dig_probe_axis_x_tip", "dig_probe_axis_xn_tip"},
            {yDir, QColor(50, 220, 50), "dig_probe_axis_y",
             "dig_probe_axis_y_tip", "dig_probe_axis_yn_tip"},
            {zDir, QColor(80, 140, 255), "dig_probe_axis_z",
             "dig_probe_axis_z_tip", "dig_probe_axis_zn_tip"},
        };

        for (const auto& a : axes) {
            const QVector3D posEnd = tip + a.dir * kAxisLen;
            const QVector3D negEnd = tip - a.dir * kAxisLen;

            // Full axis cylinder from -axis to +axis through the tip
            auto cyl = MeshFactory::createCylinder(toEigen(negEnd), toEigen(posEnd), kAxisRadius, toColor(a.color));
            cyl->setVisible(true);
            m_surfaces[a.cylKey] = cyl;

            // Positive tip: larger sphere (the "+" end)
            auto posTip = MeshFactory::createBatchedSpheres({toEigen(posEnd)}, kPosTipR, toColor(a.color));
            posTip->setVisible(true);
            m_surfaces[a.posTipKey] = posTip;

            // Negative tip: smaller dot (the "-" end)
            QColor dimColor = a.color;
            dimColor.setAlpha(120);
            auto negTip = MeshFactory::createBatchedSpheres({toEigen(negEnd)}, kNegTipR, toColor(dimColor));
            negTip->setVisible(true);
            m_surfaces[a.negTipKey] = negTip;
        }
    }

    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearProbeVisualization()
{
    std::size_t nRemoved = 0;
    for (const char* key : {"dig_probe_shaft",
                            "dig_probe_tip",
                            "dig_probe_tipglow",
                            "dig_probe_axis_x",
                            "dig_probe_axis_y",
                            "dig_probe_axis_z",
                            "dig_probe_axis_x_tip",
                            "dig_probe_axis_y_tip",
                            "dig_probe_axis_z_tip",
                            "dig_probe_axis_xn_tip",
                            "dig_probe_axis_yn_tip",
                            "dig_probe_axis_zn_tip",
                            "dig_probe_cross_x",
                            "dig_probe_cross_y",
                            "dig_probe_cross_z"}) {
        nRemoved += m_surfaces.erase(key);
    }

    const bool removed = nRemoved > 0;
    if (removed) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setStaticMarkers(const std::vector<LiveMarker>& markers)
{
    for (auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if (it->first.starts_with("dig_static_"))
            it = m_surfaces.erase(it);
        else
            ++it;
    }

    for (std::size_t i = 0; i < markers.size(); ++i) {
        auto surf = MeshFactory::createBatchedSpheres(
            {toEigen(markers[i].position)}, markers[i].radius, toColor(markers[i].color));
        surf->setVisible(true);
        m_surfaces[std::format("dig_static_{}", i)] = surf;
    }

    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearStaticMarkers()
{
    bool removed = false;
    for (auto it = m_surfaces.begin(); it != m_surfaces.end();) {
        if (it->first.starts_with("dig_static_")) {
            it = m_surfaces.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (removed) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setCameraFocusOverride(const QVector3D& center, float size)
{
    m_cameraFocusOverride = true;
    m_cameraFocusCenter = center;
    m_cameraFocusSize = size;
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearCameraFocusOverride()
{
    if (!m_cameraFocusOverride)
        return;
    m_cameraFocusOverride = false;
    updateSceneBounds();
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::clearSensors()
{
    m_devHeadTrans = QMatrix4x4();
    m_hasDevHead = false;
    removeSurfacesByPrefix({"sens_", "dig_"});
}

//=============================================================================================================

void BrainViewCore::clearEvoked()
{
    m_fieldMapper.setEvoked(mne::FiffEvoked());
    m_sensorStreamManager.stopStreaming();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearTransformation()
{
    m_headToMriTrans = mne::FiffCoordTrans();
    refreshSensorTransforms();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::clearNetwork()
{
    m_network.reset();
    m_networkVisible = false;

    // Remove network items from model
    if (m_model) {
        for (int r = m_model->rowCount() - 1; r >= 0; --r) {
            QStandardItem* item = m_model->item(r);
            if (item && item->text() == "Networks") {
                m_model->removeRow(r);
                break;
            }
        }
    }
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::stopAllStreaming()
{
    m_sourceManager.cancelLoading();
    m_sourceManager.stopStreaming();
    m_sensorStreamManager.stopStreaming();
}

//=============================================================================================================
// Video overlay public API
//=============================================================================================================

void BrainViewCore::setVideoOverlayEnabled(bool enabled)
{
    if (!m_videoOverlay)
        return;
    if (m_videoOverlay->isEnabled() == enabled)
        return;
    m_videoOverlay->setEnabled(enabled);
    m_sceneDirty = true;
    requestUpdate();
}

bool BrainViewCore::isVideoOverlayEnabled() const
{
    return m_videoOverlay && m_videoOverlay->isEnabled();
}

void BrainViewCore::setVideoOverlayFocusPosition(const QVector3D& position)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setFocusPosition(position);
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoOverlayUpHint(const QVector3D& dir)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setUpHint(dir);
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoOverlaySize(float meters)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setSizeMeters(std::max(0.001f, meters));
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoOverlayOpacity(float opacity)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setOpacity(std::clamp(opacity, 0.0f, 1.0f));
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::pushVideoOverlayFrame(const QImage& frame)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setFrame(frame);
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoDepthEnabled(bool enabled)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setDepthEnabled(enabled);
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoDepthScale(float scale)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setDepthScale(std::clamp(scale, 0.0f, 1.0f));
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::setVideoDepthSteps(int steps)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setDepthSteps(std::clamp(steps, 8, 64));
    if (m_videoOverlay->isEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

void BrainViewCore::pushVideoDepthFrame(const QImage& depthFrame)
{
    if (!m_videoOverlay)
        return;
    m_videoOverlay->setDepthFrame(depthFrame);
    if (m_videoOverlay->isEnabled() && m_videoOverlay->isDepthEnabled()) {
        m_sceneDirty = true;
        requestUpdate();
    }
}

bool BrainViewCore::intersectWorldRay(const QVector3D& origin, const QVector3D& direction, QVector3D& hitPoint) const
{
    // Test against ALL surfaces (no SubView visibility filter) so
    // the optical ray always finds the head even if it's hidden in
    // the first SubView of a multi-view layout.
    float closestDist = std::numeric_limits<float>::max();
    bool found = false;

    // Always test the BEM head surface first (even when hidden) — the
    // optical projection cares about the outermost physical shell.
    auto headIt = m_surfaces.find("bem_head");
    if (headIt != m_surfaces.cend() && headIt->second) {
        float dist = 0.0f;
        int vertexIdx = -1;
        if (headIt->second->intersects(toEigen(origin), toEigen(direction), dist, vertexIdx) && dist < closestDist) {
            closestDist = dist;
            hitPoint = origin + dist * direction;
            found = true;
        }
    }

    // Fall back to any other visible surface if the head wasn't hit.
    if (!found) {
        for (auto it = m_surfaces.cbegin(); it != m_surfaces.cend(); ++it) {
            const auto& surf = it->second;
            if (!surf || !surf->isVisible())
                continue;
            float dist = 0.0f;
            int vertexIdx = -1;
            if (surf->intersects(toEigen(origin), toEigen(direction), dist, vertexIdx) && dist < closestDist) {
                closestDist = dist;
                hitPoint = origin + dist * direction;
                found = true;
            }
        }
    }
    return found;
}

//=============================================================================================================
// MRI slice rendering
//=============================================================================================================

void BrainViewCore::setSlice(int slotIndex, mne::viz::qt::SliceObject* slice)
{
    if (slotIndex < 0 || slotIndex >= kMaxSliceSlots)
        return;
    m_slices[slotIndex] = slice;
    m_sceneDirty = true;
    requestUpdate();
}

void BrainViewCore::setElectrodes(mne::viz::qt::ElectrodeObject* electrodes)
{
    if (m_electrodes)
        m_electrodes->setHoveredContact(-1);
    m_electrodes = electrodes;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setSliceVisible(int slotIndex, bool visible)
{
    if (slotIndex < 0 || slotIndex >= kMaxSliceSlots)
        return;
    m_sliceVisible[slotIndex] = visible;
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================

void BrainViewCore::setMriSlicesVisible(bool visible)
{
    auto& profile = visibilityProfileForTarget(m_visualizationEditTarget);
    profile.mriSlices = visible;
    saveMultiViewSettings();
    m_sceneDirty = true;
    requestUpdate();
}

//=============================================================================================================
// Camera and overlay state
//=============================================================================================================

QQuaternion BrainViewCore::cameraRotation() const
{
    return toQQuaternion(m_singleView.rotation);
}

//=============================================================================================================

float BrainViewCore::zoomForTarget(int target) const
{
    return subViewForTarget(target).zoom;
}

//=============================================================================================================

namespace
{

mne::viz::BrainViewOverlay::Item toSceneItem(BrainViewCore::OverlayItem item)
{
    switch (item) {
        case BrainViewCore::FpsLabel:
            return viz::BrainViewOverlay::Item::FpsLabel;
        case BrainViewCore::SingleViewInfoLabel:
            return viz::BrainViewOverlay::Item::SingleViewInfoLabel;
        case BrainViewCore::RegionLabel:
            return viz::BrainViewOverlay::Item::RegionLabel;
        case BrainViewCore::ViewportNameLabel:
            return viz::BrainViewOverlay::Item::ViewportNameLabel;
        case BrainViewCore::ViewportInfoLabel:
            return viz::BrainViewOverlay::Item::ViewportInfoLabel;
        case BrainViewCore::VerticalSeparator:
            return viz::BrainViewOverlay::Item::VerticalSeparator;
        case BrainViewCore::HorizontalSeparator:
            break;
    }
    return viz::BrainViewOverlay::Item::HorizontalSeparator;
}

} // namespace

//=============================================================================================================

QString BrainViewCore::overlayText(OverlayItem item, int viewport) const
{
    if (item == VerticalSeparator || item == HorizontalSeparator) {
        return {};
    }
    const viz::BrainViewOverlay::Label* label = m_overlay.item(toSceneItem(item), viewport);
    return label ? QString::fromStdString(label->text) : QString();
}

//=============================================================================================================

QRect BrainViewCore::overlayGeometry(OverlayItem item, int viewport) const
{
    const viz::BrainViewOverlay::Label* label = m_overlay.item(toSceneItem(item), viewport);
    return label ? toQRect(label->geometry) : QRect();
}

//=============================================================================================================

bool BrainViewCore::isOverlayItemVisible(OverlayItem item, int viewport) const
{
    const viz::BrainViewOverlay::Label* label = m_overlay.item(toSceneItem(item), viewport);
    return label && label->visible;
}

//=============================================================================================================

QImage BrainViewCore::overlayImage(const QSize& pixelSize, qreal devicePixelRatio) const
{
    if (pixelSize.isEmpty()) {
        return {};
    }

    const qreal dpr = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    // The overlay is laid out in logical pixels; the image's device pixel ratio scales it
    const QFont baseFont = overlayBaseFont();
    const QtTextMetrics metrics(baseFont);
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    paintScene(painter, m_overlay.draw(qCeil(pixelSize.width() / dpr), qCeil(pixelSize.height() / dpr), metrics), baseFont);
    return image;
}

} // namespace mne::viz::qt
