//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    BrainView definition: the QRhiWidget host that forwards events, frames and its API to BrainViewCore.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainview.h"

#include <rhi/qrhi.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief Connects a BrainViewCore to its BrainView widget.
 */
class BrainView::Host : public BrainViewHost
{
public:
    explicit Host(BrainView& view)
    : m_view(view)
    {
    }

    void requestUpdate() override
    {
        m_view.update();
    }

    QSize logicalSize() const override
    {
        return m_view.size();
    }

    qreal devicePixelRatio() const override
    {
        return m_view.devicePixelRatioF();
    }

    bool isVisible() const override
    {
        return m_view.isVisible();
    }

    void setCursorShape(Qt::CursorShape shape) override
    {
        if (shape == Qt::ArrowCursor) {
            m_view.unsetCursor();
        } else {
            m_view.setCursor(shape);
        }
    }

    QImage grabFrame() override
    {
        // QRhiWidget::grabFramebuffer() renders an offscreen frame and reads the texture back
        return m_view.grabFramebuffer();
    }

    void requestViewportPresetMenu(int viewport, const QPoint& pos) override
    {
        BrainViewCore* core = m_view.core();
        if (viewport < 0 || viewport > 3) {
            return;
        }

        static constexpr std::array<const char*, 7> kPresetNames = {"Top", "Perspective", "Front", "Left", "Bottom", "Back", "Right"};
        QMenu menu;
        const int currentPreset = std::clamp(core->viewportCameraPreset(viewport), 0, 6);
        std::array<QAction*, kPresetNames.size()> actions{};
        for (int preset = 0; preset < std::ssize(kPresetNames); ++preset) {
            if (preset == 4) {
                menu.addSeparator();
            }
            actions[preset] = menu.addAction(QString::fromLatin1(kPresetNames[preset]));
            actions[preset]->setCheckable(true);
            actions[preset]->setChecked(preset == currentPreset);
        }

        QAction* selected = menu.exec(m_view.mapToGlobal(pos));
        const auto chosen = std::ranges::find(actions, selected);
        if (selected && chosen != actions.end()) {
            core->setViewportCameraPreset(viewport, static_cast<int>(chosen - actions.begin()));
        }
    }

    bool beginOffscreenCapture(const QSize& size) override
    {
        // Realise the widget off-screen: QRhi init still runs because the widget is
        // technically "shown", but no window-manager surface is created.
        m_view.setAttribute(Qt::WA_DontShowOnScreen, true);
        m_view.resize(size);
        m_view.show();
        return true;
    }

    void endOffscreenCapture() override
    {
        m_view.hide();
        m_view.setAttribute(Qt::WA_DontShowOnScreen, false);
    }

private:
    BrainView& m_view;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainView::BrainView(QWidget* parent)
: QRhiWidget(parent)
, m_pHost(std::make_unique<Host>(*this))
{
    setMinimumSize(800, 600);
    setSampleCount(1);
    setAutoRenderTarget(false); // The renderer manages its own dual render targets

#if defined(WASMBUILD) || defined(__EMSCRIPTEN__)
    setApi(Api::OpenGL); // WebGL 2 (OpenGL ES 3.0) on WASM
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    setApi(Api::Metal);
#elif defined(Q_OS_WIN)
    setApi(Api::Direct3D11);
#else
    setApi(Api::OpenGL);
#endif

    setMouseTracking(true); // Enable hover events

    m_pCore = std::make_unique<BrainViewCore>(m_pHost.get());

    // Re-emit the core's signals so connections to BrainView keep working
    BrainViewCore* core = m_pCore.get();
    connect(core, &BrainViewCore::timePointChanged, this, &BrainView::timePointChanged);
    connect(core, &BrainViewCore::sourceEstimateLoaded, this, &BrainView::sourceEstimateLoaded);
    connect(core, &BrainViewCore::sensorFieldLoaded, this, &BrainView::sensorFieldLoaded);
    connect(core, &BrainViewCore::sourceThresholdsUpdated, this, &BrainView::sourceThresholdsUpdated);
    connect(core, &BrainViewCore::stcLoadingProgress, this, &BrainView::stcLoadingProgress);
    connect(core, &BrainViewCore::sensorFieldTimePointChanged, this, &BrainView::sensorFieldTimePointChanged);
    connect(core, &BrainViewCore::hoveredRegionChanged, this, &BrainView::hoveredRegionChanged);
    connect(core, &BrainViewCore::electrodeContactClicked, this, &BrainView::electrodeContactClicked);
    connect(core, &BrainViewCore::visualizationEditTargetChanged, this, &BrainView::visualizationEditTargetChanged);
    connect(core, &BrainViewCore::viewCountChanged, this, &BrainView::viewCountChanged);
    connect(core, &BrainViewCore::shaderModeChanged, this, &BrainView::shaderModeChanged);
    connect(core, &BrainViewCore::surfacePointClicked, this, &BrainView::surfacePointClicked);
    connect(core, &BrainViewCore::surfacePointDoubleClicked, this, &BrainView::surfacePointDoubleClicked);
}

//=============================================================================================================

BrainView::~BrainView()
{
    // The core persists the layout and frees the GPU resources while the widget's QRhi still exists
    m_pCore.reset();
}

//=============================================================================================================

BrainViewCore* BrainView::core() const
{
    return m_pCore.get();
}

//=============================================================================================================

BrainView::ViewMode BrainView::viewMode() const
{
    return m_pCore->viewMode() == BrainViewCore::MultiView ? MultiView : SingleView;
}

//=============================================================================================================

void BrainView::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb);
    // colorTexture() is only valid inside initialize()/render()
    m_pCore->initialize(rhi(), colorTexture());
}

//=============================================================================================================

void BrainView::render(QRhiCommandBuffer* cb)
{
    m_pCore->render(rhi(), colorTexture(), sampleCount(), cb);
}

//=============================================================================================================

void BrainView::releaseResources()
{
    if (m_pCore) {
        m_pCore->releaseResources();
    }
}

//=============================================================================================================

void BrainView::mousePressEvent(QMouseEvent* event)
{
    m_pCore->handleMousePress(event);
}

//=============================================================================================================

void BrainView::mouseMoveEvent(QMouseEvent* event)
{
    m_pCore->handleMouseMove(event);
}

//=============================================================================================================

void BrainView::mouseReleaseEvent(QMouseEvent* event)
{
    m_pCore->handleMouseRelease(event);
}

//=============================================================================================================

void BrainView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pCore->handleMouseDoubleClick(event)) {
        QRhiWidget::mouseDoubleClickEvent(event);
    }
}

//=============================================================================================================

void BrainView::wheelEvent(QWheelEvent* event)
{
    m_pCore->handleWheel(event);
}

//=============================================================================================================

void BrainView::keyPressEvent(QKeyEvent* event)
{
    m_pCore->handleKeyPress(event);
}

//=============================================================================================================

void BrainView::resizeEvent(QResizeEvent* event)
{
    QRhiWidget::resizeEvent(event);
    m_pCore->handleResize();
}

//=============================================================================================================
// Forwarded API
//=============================================================================================================

void BrainView::setModel(BrainTreeModel* model)
{
    m_pCore->setModel(model);
}

//=============================================================================================================

void BrainView::setInitialCameraRotation(const QQuaternion& rotation)
{
    m_pCore->setInitialCameraRotation(rotation);
}

//=============================================================================================================

void BrainView::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    m_pCore->onRowsInserted(parent, first, last);
}

//=============================================================================================================

void BrainView::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
{
    m_pCore->onDataChanged(topLeft, bottomRight, roles);
}

//=============================================================================================================

void BrainView::setActiveSurface(const std::string& type)
{
    m_pCore->setActiveSurface(type);
}

//=============================================================================================================

void BrainView::setShaderMode(const std::string& mode)
{
    m_pCore->setShaderMode(mode);
}

//=============================================================================================================

void BrainView::setBemShaderMode(const std::string& mode)
{
    m_pCore->setBemShaderMode(mode);
}

//=============================================================================================================

void BrainView::syncBemShadersToBrainShaders()
{
    m_pCore->syncBemShadersToBrainShaders();
}

//=============================================================================================================

void BrainView::setVisualizationMode(const std::string& mode)
{
    m_pCore->setVisualizationMode(mode);
}

//=============================================================================================================

void BrainView::setHemiVisible(int hemiIdx, bool visible)
{
    m_pCore->setHemiVisible(hemiIdx, visible);
}

//=============================================================================================================

void BrainView::setBemVisible(const std::string& name, bool visible)
{
    m_pCore->setBemVisible(name, visible);
}

//=============================================================================================================

void BrainView::setBemHighContrast(bool enabled)
{
    m_pCore->setBemHighContrast(enabled);
}

//=============================================================================================================

void BrainView::setSensorVisible(const std::string& type, bool visible)
{
    m_pCore->setSensorVisible(type, visible);
}

//=============================================================================================================

void BrainView::setSensorTransEnabled(bool enabled)
{
    m_pCore->setSensorTransEnabled(enabled);
}

//=============================================================================================================

void BrainView::setMegHelmetOverride(const std::filesystem::path& path)
{
    m_pCore->setMegHelmetOverride(path);
}

//=============================================================================================================

bool BrainView::loadMegHelmetSurface(const std::filesystem::path& helmetFilePath)
{
    return m_pCore->loadMegHelmetSurface(helmetFilePath);
}

//=============================================================================================================

void BrainView::setDipoleVisible(bool visible)
{
    m_pCore->setDipoleVisible(visible);
}

//=============================================================================================================

bool BrainView::loadNetwork(const mne::Network& network, const QString& name)
{
    return m_pCore->loadNetwork(network, name);
}

//=============================================================================================================

void BrainView::setNetworkVisible(bool visible)
{
    m_pCore->setNetworkVisible(visible);
}

//=============================================================================================================

void BrainView::setNetworkThreshold(double threshold)
{
    m_pCore->setNetworkThreshold(threshold);
}

//=============================================================================================================

void BrainView::setNetworkColormap(const std::string& name)
{
    m_pCore->setNetworkColormap(name);
}

//=============================================================================================================

void BrainView::setLightingEnabled(bool enabled)
{
    m_pCore->setLightingEnabled(enabled);
}

//=============================================================================================================

void BrainView::setAutomatedRotation(bool enabled)
{
    m_pCore->setAutomatedRotation(enabled);
}

//=============================================================================================================

bool BrainView::automatedRotation() const
{
    return m_pCore->automatedRotation();
}

//=============================================================================================================

void BrainView::setHeadMovementPath(const std::vector<Eigen::Vector3f>& vecPositions)
{
    m_pCore->setHeadMovementPath(vecPositions);
}

//=============================================================================================================

void BrainView::clearHeadMovementPath()
{
    m_pCore->clearHeadMovementPath();
}

//=============================================================================================================

bool BrainView::saveSnapshot()
{
    return m_pCore->saveSnapshot();
}

//=============================================================================================================

bool BrainView::takeScreenshot(const std::filesystem::path& fileName)
{
    return m_pCore->takeScreenshot(fileName);
}

//=============================================================================================================

bool BrainView::savePng(const std::filesystem::path& path, int width, int height, const std::string& surfaceType)
{
    return m_pCore->savePng(path, width, height, surfaceType);
}

//=============================================================================================================

void BrainView::showSingleView()
{
    m_pCore->showSingleView();
}

//=============================================================================================================

void BrainView::showMultiView()
{
    m_pCore->showMultiView();
}

//=============================================================================================================

void BrainView::setViewCount(int count)
{
    m_pCore->setViewCount(count);
}

//=============================================================================================================

int BrainView::viewCount() const
{
    return m_pCore->viewCount();
}

//=============================================================================================================

void BrainView::setViewportEnabled(int index, bool enabled)
{
    m_pCore->setViewportEnabled(index, enabled);
}

//=============================================================================================================

void BrainView::setViewportCameraPreset(int index, int preset)
{
    m_pCore->setViewportCameraPreset(index, preset);
}

//=============================================================================================================

void BrainView::resetSingleViewCameraState()
{
    m_pCore->resetSingleViewCameraState();
}

//=============================================================================================================

void BrainView::resetViewportCameraState(int index)
{
    m_pCore->resetViewportCameraState(index);
}

//=============================================================================================================

void BrainView::resetAllSubViewState()
{
    m_pCore->resetAllSubViewState();
}

//=============================================================================================================

int BrainView::viewportCameraPreset(int index) const
{
    return m_pCore->viewportCameraPreset(index);
}

//=============================================================================================================

void BrainView::resetMultiViewLayout()
{
    m_pCore->resetMultiViewLayout();
}

//=============================================================================================================

void BrainView::setVisualizationEditTarget(int target)
{
    m_pCore->setVisualizationEditTarget(target);
}

//=============================================================================================================

int BrainView::visualizationEditTarget() const
{
    return m_pCore->visualizationEditTarget();
}

//=============================================================================================================

std::string BrainView::activeSurfaceForTarget(int target) const
{
    return m_pCore->activeSurfaceForTarget(target);
}

//=============================================================================================================

std::string BrainView::shaderModeForTarget(int target) const
{
    return m_pCore->shaderModeForTarget(target);
}

//=============================================================================================================

std::string BrainView::bemShaderModeForTarget(int target) const
{
    return m_pCore->bemShaderModeForTarget(target);
}

//=============================================================================================================

std::string BrainView::overlayModeForTarget(int target) const
{
    return m_pCore->overlayModeForTarget(target);
}

//=============================================================================================================

bool BrainView::objectVisibleForTarget(const std::string& object, int target) const
{
    return m_pCore->objectVisibleForTarget(object, target);
}

//=============================================================================================================

bool BrainView::megFieldMapOnHeadForTarget(int target) const
{
    return m_pCore->megFieldMapOnHeadForTarget(target);
}

//=============================================================================================================

bool BrainView::isViewportEnabled(int index) const
{
    return m_pCore->isViewportEnabled(index);
}

//=============================================================================================================

void BrainView::setInfoPanelVisible(bool visible)
{
    m_pCore->setInfoPanelVisible(visible);
}

//=============================================================================================================

bool BrainView::isInfoPanelVisible() const
{
    return m_pCore->isInfoPanelVisible();
}

//=============================================================================================================

bool BrainView::loadSourceEstimate(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath)
{
    return m_pCore->loadSourceEstimate(lhPath, rhPath);
}

//=============================================================================================================

bool BrainView::loadSensors(const std::filesystem::path& fifPath)
{
    return m_pCore->loadSensors(fifPath);
}

//=============================================================================================================

std::map<int, QVector3D> BrainView::cardinalFiducialsInMri() const
{
    return m_pCore->cardinalFiducialsInMri();
}

//=============================================================================================================

bool BrainView::bemTopVertexInMri(QVector3D& pos) const
{
    return m_pCore->bemTopVertexInMri(pos);
}

//=============================================================================================================

bool BrainView::loadDipoles(const std::filesystem::path& dipPath)
{
    return m_pCore->loadDipoles(dipPath);
}

//=============================================================================================================

bool BrainView::loadSourceSpace(const std::filesystem::path& fwdPath)
{
    return m_pCore->loadSourceSpace(fwdPath);
}

//=============================================================================================================

std::vector<std::string> BrainView::probeEvokedSets(const std::filesystem::path& evokedPath)
{
    return BrainViewCore::probeEvokedSets(evokedPath);
}

//=============================================================================================================

bool BrainView::loadSensorField(const std::filesystem::path& evokedPath, int aveIndex)
{
    return m_pCore->loadSensorField(evokedPath, aveIndex);
}

//=============================================================================================================

void BrainView::setSourceSpaceVisible(bool visible)
{
    m_pCore->setSourceSpaceVisible(visible);
}

//=============================================================================================================

bool BrainView::loadTransformation(const std::filesystem::path& transPath)
{
    return m_pCore->loadTransformation(transPath);
}

//=============================================================================================================

void BrainView::setTimePoint(int index)
{
    m_pCore->setTimePoint(index);
}

//=============================================================================================================

void BrainView::castRay(const QPoint& pos)
{
    m_pCore->castRay(pos);
}

//=============================================================================================================

void BrainView::setSourceColormap(const std::string& name)
{
    m_pCore->setSourceColormap(name);
}

//=============================================================================================================

void BrainView::setSourceThresholds(float min, float mid, float max)
{
    m_pCore->setSourceThresholds(min, mid, max);
}

//=============================================================================================================

void BrainView::setSensorFieldTimePoint(int index)
{
    m_pCore->setSensorFieldTimePoint(index);
}

//=============================================================================================================

void BrainView::setSensorFieldVisible(const std::string& type, bool visible)
{
    m_pCore->setSensorFieldVisible(type, visible);
}

//=============================================================================================================

void BrainView::setSensorFieldContourVisible(const std::string& type, bool visible)
{
    m_pCore->setSensorFieldContourVisible(type, visible);
}

//=============================================================================================================

void BrainView::setMegFieldMapOnHead(bool useHead)
{
    m_pCore->setMegFieldMapOnHead(useHead);
}

//=============================================================================================================

void BrainView::setSensorFieldColormap(const std::string& name)
{
    m_pCore->setSensorFieldColormap(name);
}

//=============================================================================================================

float BrainView::stcStep() const
{
    return m_pCore->stcStep();
}

//=============================================================================================================

float BrainView::stcTmin() const
{
    return m_pCore->stcTmin();
}

//=============================================================================================================

int BrainView::stcNumTimePoints() const
{
    return m_pCore->stcNumTimePoints();
}

//=============================================================================================================

int BrainView::closestSensorFieldIndex(float timeSec) const
{
    return m_pCore->closestSensorFieldIndex(timeSec);
}

//=============================================================================================================

int BrainView::closestStcIndex(float timeSec) const
{
    return m_pCore->closestStcIndex(timeSec);
}

//=============================================================================================================

void BrainView::startRealtimeStreaming()
{
    m_pCore->startRealtimeStreaming();
}

//=============================================================================================================

void BrainView::stopRealtimeStreaming()
{
    m_pCore->stopRealtimeStreaming();
}

//=============================================================================================================

bool BrainView::isRealtimeStreaming() const
{
    return m_pCore->isRealtimeStreaming();
}

//=============================================================================================================

void BrainView::pushRealtimeSourceData(const Eigen::VectorXd& matData)
{
    m_pCore->pushRealtimeSourceData(matData);
}

//=============================================================================================================

void BrainView::setRealtimeInterval(int msec)
{
    m_pCore->setRealtimeInterval(msec);
}

//=============================================================================================================

void BrainView::setRealtimeLooping(bool enabled)
{
    m_pCore->setRealtimeLooping(enabled);
}

//=============================================================================================================

bool BrainView::sensorFieldTimeRange(float& tmin, float& tmax) const
{
    return m_pCore->sensorFieldTimeRange(tmin, tmax);
}

//=============================================================================================================

void BrainView::startRealtimeSensorStreaming(const std::string& modality)
{
    m_pCore->startRealtimeSensorStreaming(modality);
}

//=============================================================================================================

void BrainView::stopRealtimeSensorStreaming()
{
    m_pCore->stopRealtimeSensorStreaming();
}

//=============================================================================================================

bool BrainView::isRealtimeSensorStreaming() const
{
    return m_pCore->isRealtimeSensorStreaming();
}

//=============================================================================================================

void BrainView::pushRealtimeSensorData(const Eigen::VectorXf& vecData)
{
    m_pCore->pushRealtimeSensorData(vecData);
}

//=============================================================================================================

void BrainView::setRealtimeSensorInterval(int msec)
{
    m_pCore->setRealtimeSensorInterval(msec);
}

//=============================================================================================================

void BrainView::setRealtimeSensorLooping(bool enabled)
{
    m_pCore->setRealtimeSensorLooping(enabled);
}

//=============================================================================================================

void BrainView::setRealtimeSensorAverages(int numAvr)
{
    m_pCore->setRealtimeSensorAverages(numAvr);
}

//=============================================================================================================

void BrainView::setRealtimeSensorColormap(const std::string& name)
{
    m_pCore->setRealtimeSensorColormap(name);
}

//=============================================================================================================

void BrainView::clearSurfaces()
{
    m_pCore->clearSurfaces();
}

//=============================================================================================================

void BrainView::clearBem()
{
    m_pCore->clearBem();
}

//=============================================================================================================

void BrainView::clearSourceEstimate()
{
    m_pCore->clearSourceEstimate();
}

//=============================================================================================================

void BrainView::clearDipoles()
{
    m_pCore->clearDipoles();
}

//=============================================================================================================

void BrainView::clearSourceSpace()
{
    m_pCore->clearSourceSpace();
}

//=============================================================================================================

void BrainView::clearSensors()
{
    m_pCore->clearSensors();
}

//=============================================================================================================

void BrainView::setLiveMarkers(const std::vector<LiveMarker>& markers)
{
    m_pCore->setLiveMarkers(markers);
}

//=============================================================================================================

void BrainView::clearLiveMarkers()
{
    m_pCore->clearLiveMarkers();
}

//=============================================================================================================

void BrainView::setLiveRay(const QVector3D& from, const QVector3D& to, const QColor& color, float radius)
{
    m_pCore->setLiveRay(from, to, color, radius);
}

//=============================================================================================================

void BrainView::clearLiveRay()
{
    m_pCore->clearLiveRay();
}

//=============================================================================================================

void BrainView::setProbeVisualization(const QVector3D& tip, const QVector3D& direction, float length, const QColor& color, const QColor& glowColor, const QQuaternion& orientation)
{
    m_pCore->setProbeVisualization(tip, direction, length, color, glowColor, orientation);
}

//=============================================================================================================

void BrainView::clearProbeVisualization()
{
    m_pCore->clearProbeVisualization();
}

//=============================================================================================================

void BrainView::setStaticMarkers(const std::vector<LiveMarker>& markers)
{
    m_pCore->setStaticMarkers(markers);
}

//=============================================================================================================

void BrainView::clearStaticMarkers()
{
    m_pCore->clearStaticMarkers();
}

//=============================================================================================================

void BrainView::setCameraFocusOverride(const QVector3D& center, float size)
{
    m_pCore->setCameraFocusOverride(center, size);
}

//=============================================================================================================

void BrainView::clearCameraFocusOverride()
{
    m_pCore->clearCameraFocusOverride();
}

//=============================================================================================================

void BrainView::clearEvoked()
{
    m_pCore->clearEvoked();
}

//=============================================================================================================

void BrainView::clearTransformation()
{
    m_pCore->clearTransformation();
}

//=============================================================================================================

void BrainView::clearNetwork()
{
    m_pCore->clearNetwork();
}

//=============================================================================================================

void BrainView::stopAllStreaming()
{
    m_pCore->stopAllStreaming();
}

//=============================================================================================================

void BrainView::setVideoOverlayEnabled(bool enabled)
{
    m_pCore->setVideoOverlayEnabled(enabled);
}

//=============================================================================================================

bool BrainView::isVideoOverlayEnabled() const
{
    return m_pCore->isVideoOverlayEnabled();
}

//=============================================================================================================

void BrainView::setVideoOverlayFocusPosition(const QVector3D& position)
{
    m_pCore->setVideoOverlayFocusPosition(position);
}

//=============================================================================================================

void BrainView::setVideoOverlayUpHint(const QVector3D& dir)
{
    m_pCore->setVideoOverlayUpHint(dir);
}

//=============================================================================================================

void BrainView::setVideoOverlaySize(float meters)
{
    m_pCore->setVideoOverlaySize(meters);
}

//=============================================================================================================

void BrainView::setVideoOverlayOpacity(float opacity)
{
    m_pCore->setVideoOverlayOpacity(opacity);
}

//=============================================================================================================

void BrainView::pushVideoOverlayFrame(const QImage& frame)
{
    m_pCore->pushVideoOverlayFrame(frame);
}

//=============================================================================================================

void BrainView::setVideoDepthEnabled(bool enabled)
{
    m_pCore->setVideoDepthEnabled(enabled);
}

//=============================================================================================================

void BrainView::setVideoDepthScale(float scale)
{
    m_pCore->setVideoDepthScale(scale);
}

//=============================================================================================================

void BrainView::setVideoDepthSteps(int steps)
{
    m_pCore->setVideoDepthSteps(steps);
}

//=============================================================================================================

void BrainView::pushVideoDepthFrame(const QImage& depthFrame)
{
    m_pCore->pushVideoDepthFrame(depthFrame);
}

//=============================================================================================================

bool BrainView::intersectWorldRay(const QVector3D& origin, const QVector3D& direction, QVector3D& hitPoint) const
{
    return m_pCore->intersectWorldRay(origin, direction, hitPoint);
}

//=============================================================================================================

void BrainView::setSlice(int slotIndex, mne::viz::qt::SliceObject* slice)
{
    m_pCore->setSlice(slotIndex, slice);
}

//=============================================================================================================

void BrainView::setSliceVisible(int slotIndex, bool visible)
{
    m_pCore->setSliceVisible(slotIndex, visible);
}

//=============================================================================================================

void BrainView::setMriSlicesVisible(bool visible)
{
    m_pCore->setMriSlicesVisible(visible);
}

//=============================================================================================================

void BrainView::setElectrodes(mne::viz::ElectrodeObject* electrodes)
{
    m_pCore->setElectrodes(electrodes);
}

//=============================================================================================================

void BrainView::setElectrodeValues(const std::map<std::string, float>& values)
{
    m_pCore->setElectrodeValues(values);
}

} // namespace mne::viz::qt
