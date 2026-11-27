//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainviewcore.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Widgets-free core of the 3-D brain view: scene wiring, camera, multi-view, picking, overlay and frame rendering.
 *
 * BrainViewCore holds everything of the interactive 3-D brain scene that
 * does not need Qt Widgets. It connects @ref mne::viz::qt::BrainTreeModel data
 * changes to scene rebuilds, translates QtGui mouse / wheel / key events for
 * the toolkit-neutral @ref mne::viz::BrainViewInteraction (orbit, pan, zoom,
 * pane selection and splitters), takes the camera matrices from
 * @ref mne::viz::OrbitCamera, paints the 2-D overlay (FPS, per-pane info,
 * pane names, hovered region, pane separators) of
 * @ref mne::viz::BrainViewOverlay with paintScene() into a QImage and
 * records a frame through @ref mne::viz::qt::BrainRenderer into a QRhi render
 * target.
 *
 * A host surface implements @ref mne::viz::qt::BrainViewHost and forwards its
 * events and frames: @ref mne::viz::qt::BrainView (a QRhiWidget) and
 * @ref mne::viz::qt::BrainWindow (a QWindow owning its own QRhi).
 */

#ifndef DISP3D_BRAINVIEWCORE_H
#define DISP3D_BRAINVIEWCORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "../../../../brain/rendertypes.h"
#include "multiviewlayout.h"
#include "../../../../brain/viewstate.h"
#include "../input/cameracontroller.h"
#include "../../../../brain/sensorfieldmapper.h"
#include "../scene/sourceestimatemanager.h"
#include "../scene/rtsensorstreammanager.h"
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_dig_point_set.h>
#include <mne/viz/views/brain_view_interaction.h>
#include <mne/viz/views/brain_view_overlay.h>
#include <mne/viz/views/camera.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QImage>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QStandardItem;
class QTimer;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QRhiTexture;
namespace mne
{
class Network;
}

namespace mne::viz
{
class BrainSurface;
} // namespace mne::viz

namespace mne::viz
{
class PolylineObject;
class NetworkObject;
class ElectrodeObject;
class DipoleObject;
} // namespace mne::viz

namespace mne::viz::qt
{

class BrainTreeModel;
class BrainRenderer;
class VideoOverlay;
class SliceObject;

//=============================================================================================================
/**
 * @brief Lightweight live overlay marker rendered as a single sphere.
 *
 * Used by @ref BrainViewCore::setLiveMarkers to draw transient markers
 * (e.g. digitizer tracker tip, orientation axes) without going through
 * the model pipeline or recalculating scene bounds.
 *
 * @snippet ex_disp3d_scene/main.cpp brain_view_usage
 */
struct VIZQTSHARED_EXPORT LiveMarker
{
    QVector3D position;
    QColor color;
    float radius = 0.003f;
    bool transparent = false; ///< If true, rendered with alpha blending.
};

//=============================================================================================================
/**
 * @brief Surface that shows a @ref BrainViewCore, such as a QRhiWidget, a QWindow or a test double.
 *
 * The core asks its host for repaints, the logical size, the device pixel ratio and the cursor, and
 * lets it grab the presented frame. The optional hooks let a host pop up the pane preset menu and
 * realise itself off-screen for @ref BrainViewCore::savePng.
 */
class VIZQTSHARED_EXPORT BrainViewHost
{
public:
    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~BrainViewHost();

    //=========================================================================================================
    /**
     * Schedule a new frame; several requests before the next frame coalesce into one.
     */
    virtual void requestUpdate() = 0;

    //=========================================================================================================
    /**
     * @return Size of the surface in logical (device-independent) pixels; event positions use the same units.
     */
    virtual QSize logicalSize() const = 0;

    //=========================================================================================================
    /**
     * @return Ratio of physical to logical pixels of the surface.
     */
    virtual qreal devicePixelRatio() const = 0;

    //=========================================================================================================
    /**
     * @return Whether the surface is shown; hidden surfaces do not open the pane preset menu.
     */
    virtual bool isVisible() const = 0;

    //=========================================================================================================
    /**
     * Show a cursor over the surface.
     *
     * @param[in] shape      Cursor shape; Qt::ArrowCursor restores the default cursor.
     */
    virtual void setCursorShape(Qt::CursorShape shape) = 0;

    //=========================================================================================================
    /**
     * Render the current scene and read it back.
     *
     * @return The frame, or a null image when nothing can be rendered.
     */
    virtual QImage grabFrame() = 0;

    //=========================================================================================================
    /**
     * Called when the user clicks the name label of a multi-view pane. The host may offer the camera
     * presets and apply the choice with @ref BrainViewCore::setViewportCameraPreset. Does nothing by default.
     *
     * @param[in] viewport   Index of the pane (0..3).
     * @param[in] pos        Click position in logical surface coordinates.
     */
    virtual void requestViewportPresetMenu(int viewport, const QPoint& pos);

    //=========================================================================================================
    /**
     * Realise the surface off-screen at a size so that frames are rendered without a visible window.
     * Not supported by default.
     *
     * @param[in] size       Logical size to render at.
     * @return Whether the host renders off-screen now; endOffscreenCapture() is called in any case.
     */
    virtual bool beginOffscreenCapture(const QSize& size);

    //=========================================================================================================
    /**
     * Undo beginOffscreenCapture(). Does nothing by default.
     */
    virtual void endOffscreenCapture();
};

//=============================================================================================================
/**
 * BrainViewCore is the Widgets-free part of the 3-D brain view. It owns the scene state and the
 * renderer and is driven by a @ref BrainViewHost that forwards QtGui input events and frames.
 *
 * Embed it through @ref BrainView in a widget hierarchy, or through @ref BrainWindow, a QWindow that
 * widget applications embed with a window container (createWindowContainer()).
 *
 * @brief Widgets-free 3-D brain scene: model wiring, camera, multi-view, picking, overlay and QRhi frame recording.
 */
class VIZQTSHARED_EXPORT BrainViewCore : public QObject
{
    Q_OBJECT

public:
    /**
     * View mode for single or multi-viewport display.
     */
    enum ViewMode
    {
        SingleView, /**< Single viewport with interactive camera */
        MultiView   /**< Three viewports with fixed cameras (top, left, front) */
    };

public:
    //=========================================================================================================
    /**
     * Items of the 2-D overlay that BrainView used to show as child widgets.
     */
    enum OverlayItem
    {
        FpsLabel,            /**< Frame rate and visible vertex count, bottom right. */
        SingleViewInfoLabel, /**< Shader / surface / overlay info of the single view, top right. */
        RegionLabel,         /**< Name of the region under the pointer, top left. */
        ViewportNameLabel,   /**< Camera preset name of a multi-view pane, top left of the pane. */
        ViewportInfoLabel,   /**< Shader / surface / overlay info of a multi-view pane, top right of the pane. */
        VerticalSeparator,   /**< Line between the left and right panes. */
        HorizontalSeparator  /**< Line between the top and bottom panes. */
    };

    //=========================================================================================================
    /**
     * Constructor. Restores the persisted view layout (QSettings group "BrainView").
     *
     * @param[in] host       Surface showing the view (not owned), or nullptr until setHost() is called.
     * @param[in] parent     Parent object.
     */
    explicit BrainViewCore(BrainViewHost* host = nullptr, QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Persists the view layout.
     */
    ~BrainViewCore() override;

    //=========================================================================================================
    /**
     * Set the surface showing this view.
     *
     * @param[in] host       Host (not owned), or nullptr to detach.
     */
    void setHost(BrainViewHost* host);

    //=========================================================================================================
    /**
     * @return The surface showing this view, or nullptr.
     */
    BrainViewHost* host() const;

    //=========================================================================================================
    /**
     * (Re)create the renderer and its render targets on a colour texture, e.g. from
     * QRhiWidget::initialize(). Earlier GPU resources are dropped.
     *
     * @param[in] rhi            QRhi the texture belongs to.
     * @param[in] colorTexture   Texture rendered into.
     */
    void initialize(QRhi* rhi, QRhiTexture* colorTexture);

    //=========================================================================================================
    /**
     * Record one frame (3-D scene and 2-D overlay) into a colour texture. The renderer is created on
     * first use and keeps a clearing and a preserving render target on the texture.
     *
     * @param[in] rhi            QRhi recording the frame.
     * @param[in] colorTexture   Texture rendered into.
     * @param[in] sampleCount    Sample count of the pipelines.
     * @param[in] cb             Command buffer of the current frame.
     */
    void render(QRhi* rhi, QRhiTexture* colorTexture, int sampleCount, QRhiCommandBuffer* cb);

    //=========================================================================================================
    /**
     * Record one frame (3-D scene and 2-D overlay) into a render target owned by the caller, such as
     * the current target of a QRhiSwapChain. The renderer is created on first use.
     *
     * @param[in] rhi            QRhi recording the frame.
     * @param[in] renderTarget   Target to clear and draw into; it must have a depth-stencil buffer.
     * @param[in] cb             Command buffer of the current frame.
     */
    void render(QRhi* rhi, QRhiRenderTarget* renderTarget, QRhiCommandBuffer* cb);

    //=========================================================================================================
    /**
     * Free all GPU resources, e.g. from QRhiWidget::releaseResources() or before the QRhi is destroyed.
     */
    void releaseResources();

    //=========================================================================================================
    /**
     * @return Whether a renderer (and thus GPU resources) exists.
     */
    bool hasRenderer() const;

    //=========================================================================================================
    /**
     * Handle a mouse press: pane selection, splitter drag start or pane preset menu.
     *
     * @param[in] event      Event in logical surface coordinates.
     */
    void handleMousePress(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handle a mouse move: camera rotation / panning, splitter drags and hover picking.
     *
     * @param[in] event      Event in logical surface coordinates.
     */
    void handleMouseMove(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handle a mouse release: ends drags and emits the click signals.
     *
     * @param[in] event      Event in logical surface coordinates.
     */
    void handleMouseRelease(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handle a double click on a surface.
     *
     * @param[in] event      Event in logical surface coordinates.
     * @return Whether the event was consumed; otherwise the host should pass it on.
     */
    bool handleMouseDoubleClick(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handle a wheel event: zooms the single view or the pane under the pointer.
     *
     * @param[in] event      Event in logical surface coordinates.
     */
    void handleWheel(QWheelEvent* event);

    //=========================================================================================================
    /**
     * Handle a key press: S saves a snapshot, R resets the camera rotation.
     *
     * @param[in] event      Key event.
     */
    void handleKeyPress(QKeyEvent* event);

    //=========================================================================================================
    /**
     * Relayout the overlay and the pane separators after the host surface was resized.
     */
    void handleResize();

    //=========================================================================================================
    /**
     * @return Camera rotation of the single view.
     */
    QQuaternion cameraRotation() const;

    //=========================================================================================================
    /**
     * Zoom of the single view or of one multi-view pane.
     *
     * @param[in] target     -1=Single, 0..3 = multi-view pane.
     * @return Zoom steps (one wheel notch is 1).
     */
    float zoomForTarget(int target) const;

    //=========================================================================================================
    /**
     * Text of an overlay label.
     *
     * @param[in] item       Overlay item.
     * @param[in] viewport   Pane index for the per-pane items, ignored otherwise.
     * @return The text, empty for separators and invalid panes.
     */
    QString overlayText(OverlayItem item, int viewport = 0) const;

    //=========================================================================================================
    /**
     * Geometry of an overlay item in logical surface coordinates.
     *
     * @param[in] item       Overlay item.
     * @param[in] viewport   Pane index for the per-pane items, ignored otherwise.
     * @return The rectangle, empty for invalid panes.
     */
    QRect overlayGeometry(OverlayItem item, int viewport = 0) const;

    //=========================================================================================================
    /**
     * Whether an overlay item is drawn.
     *
     * @param[in] item       Overlay item.
     * @param[in] viewport   Pane index for the per-pane items, ignored otherwise.
     * @return True if the item is shown.
     */
    bool isOverlayItemVisible(OverlayItem item, int viewport = 0) const;

    //=========================================================================================================
    /**
     * Paint the 2-D overlay (labels and pane separators) that is blended over the 3-D scene.
     *
     * @param[in] pixelSize          Size of the image in physical pixels.
     * @param[in] devicePixelRatio   Ratio of physical to logical pixels.
     * @return Premultiplied ARGB image, transparent where nothing is drawn.
     */
    QImage overlayImage(const QSize& pixelSize, qreal devicePixelRatio) const;

    //=========================================================================================================
    /**
     * Set the data model and show the objects it already holds. Objects of a previous model are removed
     * from the view (the previous model is not modified).
     *
     * @param[in] model      Pointer to BrainTreeModel, or nullptr to detach.
     */
    void setModel(BrainTreeModel* model);

    //=========================================================================================================
    /**
     * Set the initial camera rotation for this view.
     * Each view can start from a different orientation (e.g., top, front, left).
     *
     * @param[in] rotation   Initial camera rotation quaternion.
     */
    void setInitialCameraRotation(const QQuaternion& rotation);

public slots:
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);
    //=========================================================================================================
    /**
     * Set the active surface type to search for (e.g. "pial").
     *
     * @param[in] type       The surface type to activate.
     */
    void setActiveSurface(const std::string& type);

    //=========================================================================================================
    /**
     * Set the shader mode (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setShaderMode(const std::string& mode);

    //=========================================================================================================
    /**
     * Set the shader mode for BEM surfaces (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setBemShaderMode(const std::string& mode);

    /**
     * Synchronize all BEM shader targets to their respective brain shader targets.
     */
    void syncBemShadersToBrainShaders();

    //=========================================================================================================
    /**
     * Set the overlay mode (FsSurface, FsAnnotation, Scientific).
     *
     * @param[in] mode       The visualization mode to set.
     */
    void setVisualizationMode(const std::string& mode);

    //=========================================================================================================
    /**
     * Toggle visibility of a hemisphere.
     *
     * @param[in] hemiIdx    0 for LH, 1 for RH.
     * @param[in] visible    Visibility state.
     */
    void setHemiVisible(int hemiIdx, bool visible);

    //=========================================================================================================
    /**
     * Toggle visibility of a BEM surface layer.
     *
     * @param[in] name       "head", "outer_skull", or "inner_skull".
     * @param[in] visible    Visibility state.
     */
    void setBemVisible(const std::string& name, bool visible);

    //=========================================================================================================
    /**
     * Set whether BEM surfaces should use their standard (colorful) definition or white.
     *
     * @param[in] enabled    True to use standard colors (Red/Green/Blue), False for White.
     */
    void setBemHighContrast(bool enabled);

    //=========================================================================================================
    /**
     * Toggle visibility of sensor groups.
     *
     * @param[in] type       "MEG", "EEG", or "Digitizer".
     * @param[in] visible    Visibility state.
     */
    void setSensorVisible(const std::string& type, bool visible);

    //=========================================================================================================
    /**
     * Set whether sensor transformations (if loaded) should be applied.
     *
     * @param[in] enabled    True to apply transformations, False to show original positions.
     */
    void setSensorTransEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Override the auto-selected MEG helmet surface path.
     *
     * @param[in] path       Absolute path to a helmet surface file. Empty to use auto selection.
     */
    void setMegHelmetOverride(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Load a standalone MEG helmet surface, replacing the current one.
     *
     * The Device→Head transform from the last loadSensors() call is applied
     * automatically when available.
     *
     * @param[in] helmetFilePath   Absolute path to a helmet BEM FIF file.
     * @return True if the helmet surface was loaded successfully.
     */
    bool loadMegHelmetSurface(const std::filesystem::path& helmetFilePath);

    //=========================================================================================================
    /**
     * Toggle visibility of dipoles.
     *
     * @param[in] visible    Visibility state.
     */
    void setDipoleVisible(bool visible);

    //=========================================================================================================
    /**
     * Load a connectivity network for visualization.
     *
     * @param[in] network    The connectivity network to visualize.
     * @param[in] name       Display name for the network.
     * @return True if successful.
     */
    bool loadNetwork(const mne::Network& network, const QString& name = "Network");

    //=========================================================================================================
    /**
     * Toggle visibility of connectivity network.
     *
     * @param[in] visible    Visibility state.
     */
    void setNetworkVisible(bool visible);

    //=========================================================================================================
    /**
     * Set the threshold for connectivity network edge filtering.
     *
     * @param[in] threshold  Threshold value (0.0–1.0).
     */
    void setNetworkThreshold(double threshold);

    //=========================================================================================================
    /**
     * Set the colormap for connectivity network visualization.
     *
     * @param[in] name       Colormap name ("Hot", "Jet", etc.).
     */
    void setNetworkColormap(const std::string& name);

    //=========================================================================================================
    /**
     * Enable or disable lighting for the scene.
     *
     * @param[in] enabled    True to enable lighting, false to disable.
     */
    void setLightingEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Enable or disable continuous automated rotation of the scene.
     *
     * The rotation is applied to the camera, not to the individual object
     * transformation matrices, so head, sensor and all other surfaces keep
     * their relative alignment and their coregistration transforms.
     *
     * @param[in] enabled    True to start rotating, false to stop.
     */
    void setAutomatedRotation(bool enabled);

    //=========================================================================================================
    /**
     * Whether automated rotation is currently active.
     *
     * @return True if the scene is being rotated automatically.
     */
    bool automatedRotation() const;

    //=========================================================================================================
    /**
     * Show the path the subject's head travelled as a connected line.
     *
     * Intended for HPI fitting: pass the head positions recorded over the
     * course of a measurement (see RealTimeHpiResult::headPositionHistory)
     * and they are drawn as a polyline, oldest to newest, so the study
     * conductor can judge how much the subject moved.
     *
     * Passing fewer than two positions clears the path.
     *
     * @param[in] vecPositions   Head positions in metres, oldest first.
     */
    void setHeadMovementPath(const std::vector<Eigen::Vector3f>& vecPositions);

    //=========================================================================================================
    /**
     * Remove the head movement path from the scene.
     */
    void clearHeadMovementPath();

    //=========================================================================================================
    /**
     * Save the current view as ./Screenshots/&lt;date&gt;-&lt;time&gt;-3DView.png, like the other MNE-CPP views.
     * The host grabs the 3-D scene without the 2-D overlay labels.
     *
     * @return false if there is no rendered frame or the file cannot be written.
     */
    bool saveSnapshot();

    //=========================================================================================================
    /**
     * Save the current rendered frame to an image file; missing parent directories are created.
     * The host grabs the 3-D scene without the 2-D overlay labels.
     *
     * @param[in] fileName   Output file; the format follows the suffix (e.g. .png, .jpg).
     * @return false if there is no rendered frame or the file cannot be written.
     */
    bool takeScreenshot(const std::filesystem::path& fileName);

    //=========================================================================================================
    /**
     * Render the scene offscreen and save to a PNG file.
     *
     * The host realises its surface off-screen through
     * BrainViewHost::beginOffscreenCapture() (BrainView uses
     * Qt::WA_DontShowOnScreen, so it works on headless CI runners), the
     * event loop runs until a frame has been recorded, then
     * BrainViewHost::grabFrame() reads the scene back without the overlay.
     *
     * @param[in] path        Output PNG file path.
     * @param[in] width       Render width in pixels (default 1200).
     * @param[in] height      Render height in pixels (default 800).
     * @param[in] surfaceType Surface type to match (default "pial").
     * @return true on success, false on failure.
     */
    bool savePng(const std::filesystem::path& path, int width = 1200, int height = 800,
                 const std::string& surfaceType = "pial");

    //=========================================================================================================
    /**
     * Switch to single-view mode (one viewport, interactive camera).
     */
    void showSingleView();

    //=========================================================================================================
    /**
        * Switch to multi-view mode (four viewports: top, perspective, front, left).
     */
    void showMultiView();

    //=========================================================================================================
    /**
     * Set the number of visible viewport panes (1–4).
     *
     * @param[in] count  Number of panes: 1 = single, 2 = side-by-side,
     *                   3 = two + one, 4 = 2×2 grid.
     */
    void setViewCount(int count);

    /**
     * @return Current number of visible viewport panes (1–4).
     */
    int viewCount() const
    {
        return m_viewCount;
    }

    //=========================================================================================================
    /**
     * Enable or disable a specific viewport in multi-view mode.
     *
        * @param[in] index      Viewport index (0=Top, 1=Perspective, 2=Front, 3=Left).
     * @param[in] enabled    True to enable, false to disable.
     */
    void setViewportEnabled(int index, bool enabled);

    /**
     * Set the camera preset for a specific viewport.
     *
     * @param[in] index      Viewport index (0–3).
     * @param[in] preset     Camera preset (0=Top, 1=Perspective, 2=Front, 3=Left, 4=Bottom, 5=Back, 6=Right).
     */
    void setViewportCameraPreset(int index, int preset);

    /**
     * Reset single-view camera state (rotation and zoom) to defaults.
     */
    void resetSingleViewCameraState();

    /**
     * Reset one multi-view pane's camera state (zoom, pan, perspective rotation).
     *
     * @param[in] index      Viewport index (0–3).
     */
    void resetViewportCameraState(int index);

    /**
     * Reset all per-pane SubView state (camera, visibility, shaders, surface
     * type, overlay) to its index-specific factory defaults and reset the
     * single-view state too. Use this when an embedding application wants
     * a clean slate that ignores any settings persisted by previous runs
     * or other hosts of BrainViewCore.
     */
    void resetAllSubViewState();

    /**
     * Get the camera preset for a specific viewport.
     *
     * @param[in] index      Viewport index (0–3).
     * @return               Camera preset index (0–6), or -1 if invalid.
     */
    int viewportCameraPreset(int index) const;

    /**
     * Reset the multi-view splitter layout to equal-sized panes.
     */
    void resetMultiViewLayout();

    /**
     * Select which view's visualization settings are edited by UI controls.
     *
     * @param[in] target     -1=Single, 0=Top, 1=Perspective, 2=Front, 3=Left.
     */
    void setVisualizationEditTarget(int target);

    /**
     * Get current visualization edit target.
     *
     * @return               -1=Single, 0=Top, 1=Perspective, 2=Front, 3=Left.
     */
    int visualizationEditTarget() const;

    /**
     * Get configured surface type for a target view.
     */
    std::string activeSurfaceForTarget(int target) const;

    /**
     * Get configured shader mode name for a target view.
     */
    std::string shaderModeForTarget(int target) const;

    /**
     * Get configured BEM shader mode name for a target view.
     */
    std::string bemShaderModeForTarget(int target) const;

    /**
     * Get configured overlay mode name for a target view.
     */
    std::string overlayModeForTarget(int target) const;

    /**
     * Check object visibility for a given target view.
     *
     * @param[in] object     Object key (e.g. "lh", "bem_head", "sens_meg").
     * @param[in] target     -1=Single, 0=Top, 1=Perspective, 2=Front, 3=Left.
     * @return               True if visible, false otherwise.
     */
    bool objectVisibleForTarget(const std::string& object, int target) const;

    /**
     * Check whether MEG field mapping uses head surface for a target.
     *
     * @param[in] target     -1=Single, 0=Top, 1=Perspective, 2=Front, 3=Left.
     * @return               True if MEG map uses head surface.
     */
    bool megFieldMapOnHeadForTarget(int target) const;

    /**
     * Check if a multi-view viewport is enabled.
     *
     * @param[in] index      Viewport index (0=Top, 1=Perspective, 2=Front, 3=Left).
     * @return               True if enabled, false otherwise.
     */
    bool isViewportEnabled(int index) const;

    //=========================================================================================================
    /**
     * Show or hide the info panel (FPS, vertices, shader).
     *
     * @param[in] visible    True to show, false to hide.
     */
    void setInfoPanelVisible(bool visible);

    //=========================================================================================================
    /**
     * Whether the info panel is currently visible.
     *
     * @return True if visible.
     */
    bool isInfoPanelVisible() const
    {
        return m_infoPanelVisible;
    }

    //=========================================================================================================
    /**
     * Get the current view mode.
     *
     * @return Current ViewMode.
     */
    ViewMode viewMode() const
    {
        return m_viewMode;
    }

    //=========================================================================================================
    /**
     * Load source estimate files (.stc) for both hemispheres.
     *
     * @param[in] lhPath     Path to left hemisphere .stc file.
     * @param[in] rhPath     Path to right hemisphere .stc file.
     * @return True if successful.
     */
    bool loadSourceEstimate(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath);

    //=========================================================================================================
    /**
     * Load sensors (MEG/EEG/Digitizers) from a FIF file.
     *
     * @param[in] fifPath    Path to the FIF file.
     * @return True if successful; false without a tree model (see setModel()).
     */
    bool loadSensors(const std::filesystem::path& fifPath);

    /**
     * @brief Return cardinal fiducials (NAS/LPA/RPA) transformed to MRI coordinates.
     *
     * Returns an empty map when no cardinal dig points are loaded or when no
     * head-to-MRI transform is available.  Keys are the FIFF ident values
     * (FIFFV_POINT_LPA=1, FIFFV_POINT_NASION=2, FIFFV_POINT_RPA=3).
     */
    std::map<int, QVector3D> cardinalFiducialsInMri() const;

    /**
     * @brief Return the highest-Z vertex of the BEM head surface in MRI coords.
     *
     * This is the top-of-head (vertex/CZ) reference used for orientation
     * validation during Polhemus coregistration.
     *
     * @param[out] pos  The vertex position (metres, MRI/surface-RAS).
     * @return True if a BEM head surface is loaded; false otherwise.
     */
    bool bemTopVertexInMri(QVector3D& pos) const;

    //=========================================================================================================
    /**
     * Load dipoles from a .dip or .bdip file.
     *
     * @param[in] dipPath    Path to the dipole file.
     * @return True if successful; false without a tree model (see setModel()).
     */
    bool loadDipoles(const std::filesystem::path& dipPath);

    //=========================================================================================================
    /**
     * Load source space from a FIF file (forward solution or source space file).
     *
     * @param[in] fwdPath    Path to the FIF file containing source space data.
     * @return True if successful; false without a tree model (see setModel()).
     */
    bool loadSourceSpace(const std::filesystem::path& fwdPath);

    //=========================================================================================================
    /**
     * Probe evoked data sets in a FIF file.
     *
     * Returns a list of descriptive strings for each evoked set found
     * (e.g. "0: Left Auditory (Average, nave=55)"). The list is empty
     * if the file cannot be read.
     *
     * @param[in] evokedPath   Path to the evoked/average FIF file.
     * @return Descriptive labels for each evoked set.
     */
    static std::vector<std::string> probeEvokedSets(const std::filesystem::path& evokedPath);

    /**
     * Load sensor measurements from an evoked/average FIF file.
     *
     * @param[in] evokedPath   Path to the evoked/average FIF file.
     * @param[in] aveIndex     Dataset index to load from the file.
     * @return True if successful.
     */
    bool loadSensorField(const std::filesystem::path& evokedPath, int aveIndex = 0);

    //=========================================================================================================
    /**
     * Toggle visibility of source space points.
     *
     * @param[in] visible    Visibility state.
     */
    void setSourceSpaceVisible(bool visible);

    //=========================================================================================================
    /**
     * Load a coordinate transformation from a FIF file.
     *
     * @param[in] transPath  Path to the transformation file.
     * @return True if successful.
     */
    bool loadTransformation(const std::filesystem::path& transPath);

    //=========================================================================================================
    /**
     * Set the current time point for source estimate visualization.
     *
     * @param[in] index      Time sample index.
     */
    void setTimePoint(int index);

    //=========================================================================================================
    /**
     * Cast rays from screen position to find intersected objects.
     *
     * @param[in] pos        2D mouse position.
     */
    void castRay(const QPoint& pos);

    //=========================================================================================================
    /**
     * Set the colormap for source estimate visualization.
     *
     * @param[in] name       Colormap name ("Hot", "Jet", etc.).
     */
    void setSourceColormap(const std::string& name);

    //=========================================================================================================
    /**
     * Set threshold values for source estimate visualization.
     *
     * @param[in] min        Minimum threshold.
     * @param[in] mid        Mid-point threshold.
     * @param[in] max        Maximum threshold.
     */
    void setSourceThresholds(float min, float mid, float max);

    //=========================================================================================================
    /**
     * Set the time point for sensor field maps.
     *
     * @param[in] index      Time sample index.
     */
    void setSensorFieldTimePoint(int index);

    //=========================================================================================================
    /**
     * Set visibility for sensor field maps.
     *
     * @param[in] type       "MEG" or "EEG".
     * @param[in] visible    Visibility state.
     */
    void setSensorFieldVisible(const std::string& type, bool visible);

    //=========================================================================================================
    /**
     * Set contour visibility for sensor field maps.
     *
     * @param[in] type       "MEG" or "EEG".
     * @param[in] visible    Visibility state.
     */
    void setSensorFieldContourVisible(const std::string& type, bool visible);

    //=========================================================================================================
    /**
     * Choose MEG mapping surface (helmet or head).
     *
     * @param[in] useHead    True to map MEG on head surface.
     */
    void setMegFieldMapOnHead(bool useHead);

    //=========================================================================================================
    /**
     * Set colormap for sensor field maps.
     *
     * @param[in] name       Colormap name.
     */
    void setSensorFieldColormap(const std::string& name);

    //=========================================================================================================
    /**
     * Get the time step of the loaded source estimate.
     *
     * @return Time step in seconds. Returns 0 if not loaded.
     */
    float stcStep() const;

    //=========================================================================================================
    /**
     * Get the start time of the loaded source estimate.
     *
     * @return Start time in seconds. Returns 0 if not loaded.
     */
    float stcTmin() const;

    //=========================================================================================================
    /**
     * Get the number of time points in the loaded source estimate.
     *
     * @return Number of time points. Returns 0 if not loaded.
     */
    int stcNumTimePoints() const;

    //=========================================================================================================
    /**
     * Find the closest sensor field time index for a given time in seconds.
     *
     * @param[in] timeSec    Time in seconds.
     * @return Closest sample index, or -1 if no sensor field data loaded.
     */
    int closestSensorFieldIndex(float timeSec) const;

    //=========================================================================================================
    /**
     * Find the closest STC time index for a given time in seconds.
     *
     * @param[in] timeSec    Time in seconds.
     * @return Closest sample index, or -1 if no STC data loaded.
     */
    int closestStcIndex(float timeSec) const;

    //=========================================================================================================
    /**
     * Start real-time source data streaming.
     * Creates the controller (if needed), feeds the loaded STC data column-by-column,
     * and begins the timer-driven streaming loop.
     */
    void startRealtimeStreaming();

    //=========================================================================================================
    /**
     * Stop real-time source data streaming.
     */
    void stopRealtimeStreaming();

    //=========================================================================================================
    /**
     * Check whether real-time streaming is active.
     *
     * @return True if streaming.
     */
    bool isRealtimeStreaming() const;

    //=========================================================================================================
    /**
     * Push a single source-data vector into the real-time pipeline.
     * The vector should contain source values for all sources (LH + RH concatenated).
     *
     * @param[in] matData       Source activity vector.
     */
    void pushRealtimeSourceData(const Eigen::VectorXd& matData);

    //=========================================================================================================
    /**
     * Set the streaming speed (interval between frames).
     *
     * @param[in] msec       Interval in milliseconds.
     */
    void setRealtimeInterval(int msec);

    //=========================================================================================================
    /**
     * Enable or disable looping for real-time streaming.
     *
     * @param[in] enabled    True to loop.
     */
    void setRealtimeLooping(bool enabled);

    //=========================================================================================================
    /**
     * Get the time range of the loaded sensor field (evoked) data.
     *
     * @param[out] tmin      Start time in seconds.
     * @param[out] tmax      End time in seconds.
     * @return True if sensor field data is loaded, false otherwise.
     */
    bool sensorFieldTimeRange(float& tmin, float& tmax) const;

    //=========================================================================================================
    // ── Real-time sensor data streaming ────────────────────────────────
    //=========================================================================================================

    /**
     * Start real-time sensor data streaming for the specified modality.
     * Creates the controller (if needed), feeds the loaded evoked data
     * column-by-column, and begins the timer-driven streaming loop.
     *
     * Requires that sensor field data (evoked) has been loaded and
     * the mapping matrix is built.
     *
     * @param[in] modality   "MEG" or "EEG".
     */
    void startRealtimeSensorStreaming(const std::string& modality = "MEG");

    //=========================================================================================================
    /**
     * Stop real-time sensor data streaming.
     */
    void stopRealtimeSensorStreaming();

    //=========================================================================================================
    /**
     * Check whether real-time sensor streaming is active.
     *
     * @return True if streaming.
     */
    bool isRealtimeSensorStreaming() const;

    //=========================================================================================================
    /**
     * Push a single sensor measurement vector into the real-time sensor pipeline.
     * The vector should contain one value per picked channel
     * (matching the active mapping matrix column count).
     *
     * @param[in] vecData       Sensor measurement vector.
     */
    void pushRealtimeSensorData(const Eigen::VectorXf& vecData);

    //=========================================================================================================
    /**
     * Set the streaming speed for sensor streaming (interval between frames).
     *
     * @param[in] msec       Interval in milliseconds.
     */
    void setRealtimeSensorInterval(int msec);

    //=========================================================================================================
    /**
     * Enable or disable looping for real-time sensor streaming.
     *
     * @param[in] enabled    True to loop.
     */
    void setRealtimeSensorLooping(bool enabled);

    //=========================================================================================================
    /**
     * Set the number of averages for sensor streaming.
     *
     * @param[in] numAvr     Number of averages.
     */
    void setRealtimeSensorAverages(int numAvr);

    //=========================================================================================================
    /**
     * Set the colormap used for sensor streaming visualization.
     *
     * @param[in] name       Colormap name.
     */
    void setRealtimeSensorColormap(const std::string& name);

    // ── Data removal ───────────────────────────────────────────────────

    /**
     * Remove all brain surfaces (pial, white, inflated) and their annotations.
     */
    void clearSurfaces();

    /**
     * Remove all BEM surfaces from the scene.
     */
    void clearBem();

    /**
     * Remove source estimate data and restore surface colours.
     */
    void clearSourceEstimate();

    /**
     * Remove dipole objects from the scene.
     */
    void clearDipoles();

    /**
     * Remove source space points from the scene.
     */
    void clearSourceSpace();

    /**
     * Remove sensor geometry, digitizer points, and the MEG helmet surface.
     */
    void clearSensors();

    // ── Live marker overlay (lightweight, no model, no scene-bounds) ───

    /**
     * Replace the set of live overlay markers.
     *
     * Renders each marker as a small sphere without touching the model
     * or recalculating scene bounds, so the camera stays locked.
     */
    void setLiveMarkers(const std::vector<LiveMarker>& markers);

    /** Remove all live overlay markers. */
    void clearLiveMarkers();

    /**
     * Set a semi-transparent ray cylinder between two world-space points.
     *
     * @param[in] from     Ray origin (metres).
     * @param[in] to       Ray end point (metres).
     * @param[in] color    Ray color (alpha channel controls transparency).
     * @param[in] radius   Cylinder radius (metres, default 0.001 = 1 mm).
     */
    void setLiveRay(const QVector3D& from, const QVector3D& to,
                    const QColor& color, float radius = 0.001f);

    /** Remove the live ray from the scene. */
    void clearLiveRay();

    /**
     * Show a catheter-style probe visualization (cylinder shaft + sphere tip
     * with an optional pulsing glow aura).
     *
     * @param[in] tip         Probe tip position (metres, model space).
     * @param[in] direction   Probe forward axis (unit vector, model space).
     * @param[in] length      Visible shaft length behind the tip (metres).
     * @param[in] color       Probe core color.
     * @param[in] glowColor   Outer glow aura color (default: transparent = no glow).
     * @param[in] orientation Full probe orientation for drawing a debug
     *                        coordinate frame (X=red, Y=green, Z=blue).
     *                        Pass a null quaternion to skip axes.
     */
    void setProbeVisualization(const QVector3D& tip, const QVector3D& direction,
                               float length, const QColor& color,
                               const QColor& glowColor = QColor(0, 0, 0, 0),
                               const QQuaternion& orientation = QQuaternion(0, 0, 0, 0));

    /** Remove the probe visualization from the scene. */
    void clearProbeVisualization();

    /**
     * Replace the set of static overlay markers (fiducials, acquired points).
     * These persist across frames and are always visible.
     */
    void setStaticMarkers(const std::vector<LiveMarker>& markers);

    /** Remove all static overlay markers. */
    void clearStaticMarkers();

    /**
     * Override the auto-computed scene center and size.
     * While active, updateSceneBounds() results are ignored for camera framing.
     */
    void setCameraFocusOverride(const QVector3D& center, float size);

    /** Remove the camera focus override; revert to auto-computed bounds. */
    void clearCameraFocusOverride();

    /**
     * Remove evoked / sensor field mapping data.
     */
    void clearEvoked();

    /**
     * Remove the head-to-MRI coordinate transformation.
     */
    void clearTransformation();

    /**
     * Remove the connectivity network from the scene.
     */
    void clearNetwork();

    /**
     * Stop all active background streaming (source estimate + sensor).
     * Call before destruction to avoid blocking thread joins in destructors.
     */
    void stopAllStreaming();

    //=========================================================================================================
    // ── Video overlay ──────────────────────────────────────────────────
    //=========================================================================================================

    /**
     * Enable or disable the generic live video overlay decal that is
     * drawn on top of the head surface at the current focus position.
     */
    void setVideoOverlayEnabled(bool enabled);

    /** @return Whether the video overlay is currently enabled. */
    bool isVideoOverlayEnabled() const;

    /** Set the world-space focus position (metres) of the overlay quad. */
    void setVideoOverlayFocusPosition(const QVector3D& position);

    /** Set the orientation hint for the overlay quad (world-space direction). */
    void setVideoOverlayUpHint(const QVector3D& dir);

    /** Set the side length of the overlay quad (metres). Default ≈ 0.06 m. */
    void setVideoOverlaySize(float meters);

    /** Set the overall opacity of the overlay [0..1]. */
    void setVideoOverlayOpacity(float opacity);

    /** Push a new video frame to the overlay (e.g. from QVideoSink). */
    void pushVideoOverlayFrame(const QImage& frame);

    /** Enable or disable depth-based parallax relief on the video overlay. */
    void setVideoDepthEnabled(bool enabled);

    /** Set the parallax occlusion displacement scale [0..1]. */
    void setVideoDepthScale(float scale);

    /** Set the number of POM ray-march steps (quality vs. performance). */
    void setVideoDepthSteps(int steps);

    /** Push a monocular depth map corresponding to the current video frame. */
    void pushVideoDepthFrame(const QImage& depthFrame);

    /**
     * @brief Intersect a world-space ray with loaded scene geometry.
     *
     * Tests the ray against all visible surfaces (BEM, cortical, sensors)
     * and returns the closest hit point.
     *
     * @param[in]  origin     Ray origin in world (model) space (metres).
     * @param[in]  direction  Ray direction (unit vector).
     * @param[out] hitPoint   Intersection point in world (model) space.
     * @return @c true if the ray hits a surface; @c false otherwise.
     */
    bool intersectWorldRay(const QVector3D& origin, const QVector3D& direction, QVector3D& hitPoint) const;

    //=========================================================================================================
    // ── MRI slice rendering ────────────────────────────────────────────
    //=========================================================================================================

    /**
     * Set an MRI slice to be rendered in the 3-D scene.
     *
     * @param[in] slotIndex  Slot index (0=axial, 1=coronal, 2=sagittal).
     * @param[in] slice      Pointer to SliceObject, or nullptr to hide.
     */
    void setSlice(int slotIndex, mne::viz::qt::SliceObject* slice);

    /**
     * Toggle visibility of an MRI slice slot.
     *
     * @param[in] slotIndex  Slot index (0=axial, 1=coronal, 2=sagittal).
     * @param[in] visible    Whether this slice should be drawn.
     */
    void setSliceVisible(int slotIndex, bool visible);

    /**
     * Toggle MRI slice visibility for the current edit-target viewport.
     *
     * @param[in] visible  Whether MRI slices should be drawn in the active viewport.
     */
    void setMriSlicesVisible(bool visible);

    /**
     * Set the sEEG / ECoG electrodes to draw, picked under the pointer like the other scene objects.
     *
     * @param[in] electrodes  Non-owning pointer; it must outlive this view or be reset to nullptr.
     */
    void setElectrodes(mne::viz::ElectrodeObject* electrodes);

    /**
     * Show one value per contact (e.g. high-gamma power) on the contacts and the cortex around
     * them, as MNE-Python's stc_near_sensors. Weights come from each hemisphere's pial (or
     * white) surface and colour every surface of that hemisphere. This replaces a source
     * estimate overlay; an empty map clears both.
     *
     * @param[in] values  Map of contact name -> value.
     */
    void setElectrodeValues(const std::map<std::string, float>& values);

signals:
    //=========================================================================================================
    /**
     * Emitted when the time point changes.
     *
     * @param[in] index      Time sample index.
     * @param[in] time       Time in seconds.
     */
    void timePointChanged(int index, float time);

    //=========================================================================================================
    /**
     * Emitted when a source estimate is loaded.
     *
     * @param[in] numTimePoints     Number of time samples in the source estimate.
     */
    void sourceEstimateLoaded(int numTimePoints);

    //=========================================================================================================
    /**
     * Emitted when sensor field data is loaded.
     *
     * @param[in] numTimePoints     Number of time samples.
     */
    void sensorFieldLoaded(int numTimePoints, int initialTimePoint = 0);

    //=========================================================================================================
    /**
     * Emitted when auto-thresholds are set from loaded source estimate data.
     *
     * @param[in] min     Minimum threshold.
     * @param[in] mid     Mid threshold.
     * @param[in] max     Maximum threshold.
     */
    void sourceThresholdsUpdated(float min, float mid, float max);

    //=========================================================================================================
    /**
     * Emitted to report STC loading progress.
     *
     * @param[in] percent        Progress percentage (0-100).
     * @param[in] message        Status message.
     */
    void stcLoadingProgress(int percent, const QString& message);

    //=========================================================================================================
    /**
     * Emitted when the sensor field time point changes.
     *
     * @param[in] index      Time sample index.
     * @param[in] time       Time in seconds.
     */
    void sensorFieldTimePointChanged(int index, float time);

    //=========================================================================================================
    /**
     * Emitted when the hovered brain region changes.
     *
     * @param[in] regionName  Name of the region under the cursor.
     */
    void hoveredRegionChanged(const QString& regionName);

    /**
     * Emitted when a click lands on an electrode contact.
     *
     * @param[in] contactName  Name of the clicked contact.
     * @param[in] position     Contact position.
     */
    void electrodeContactClicked(const QString& contactName, const QVector3D& position);

    //=========================================================================================================
    /**
     * Emitted when the active visualization edit target changes.
     *
     * @param[in] target  New target index (-1 = single, 0..N-1 = multi pane).
     */
    void visualizationEditTargetChanged(int target);

    //=========================================================================================================
    /**
     * Emitted whenever the number of viewports changes — either via
     * @ref setViewCount or after restoring persisted state in
     * @c BrainViewCore::loadMultiViewSettings "loadMultiViewSettings". UI consumers (e.g. host toolbars)
     * should connect to this to keep their controls in sync.
     *
     * @param[in] count  Number of viewports (1..kDefaultViewportCount).
     */
    void viewCountChanged(int count);

    //=========================================================================================================
    /**
     * Emitted whenever the active brain shader mode changes — either
     * via @ref setShaderMode or after restoring persisted state in
     * @c BrainViewCore::loadMultiViewSettings "loadMultiViewSettings".
     *
     * @param[in] modeName  Shader name as accepted by @ref setShaderMode.
     */
    void shaderModeChanged(const QString& modeName);

    /**
     * Emitted on left-click when the click hits a surface.
     * @param[in] worldPos  Hit point in world (model) coordinates.
     */
    void surfacePointClicked(const QVector3D& worldPos);

    /**
     * Emitted on double-click when the click hits a surface.
     * @param[in] worldPos  Hit point in world (model) coordinates.
     */
    void surfacePointDoubleClicked(const QVector3D& worldPos);

private slots:
    //=========================================================================================================
    /**
     * Called when the SourceEstimateManager finishes loading.
     * Switches visualisation mode and emits sourceEstimateLoaded().
     *
     * @param[in] numTimePoints   Number of loaded time points.
     */
    void onSourceEstimateLoaded(int numTimePoints);

    //=========================================================================================================
    /**
     * Called when the real-time streaming pipeline produces new per-vertex
     * colours.  Applies them to the matching brain surfaces.
     *
     * @param[in] colorsLh   Per-vertex ABGR colors for the left hemisphere.
     * @param[in] colorsRh   Per-vertex ABGR colors for the right hemisphere.
     */
    void onRealtimeColorsAvailable(const std::vector<uint32_t>& colorsLh,
                                   const std::vector<uint32_t>& colorsRh);

    //=========================================================================================================
    /** Repaint after sensor streaming produces new colours. */
    void onSensorStreamColorsAvailable(const std::string& surfaceKey,
                                       const std::vector<uint32_t>& colors);

private:
    // Note: ViewVisibilityProfile and SubView are defined in core/viewstate.h.
    // SplitterHit is defined in view/multiviewlayout.h.

    /** Return the SubView for a given target (-1 = single, 0..3 = multi). */
    SubView& subViewForTarget(int target);
    const SubView& subViewForTarget(int target) const;

    ViewVisibilityProfile& visibilityProfileForTarget(int target);
    const ViewVisibilityProfile& visibilityProfileForTarget(int target) const;

    void refreshSensorTransforms();
    QMatrix4x4 itemTransform(const QStandardItem* item) const;
    const QStandardItem* itemForSurface(const std::shared_ptr<BrainSurface>& surface) const;
    void removeSurfacesByPrefix(std::initializer_list<std::string_view> prefixes);

private:
    // ── Layout helpers (delegate to viz::PaneLayout) ────────────
    std::vector<int> enabledViewportIndices() const;

    // ── Helpers ────────────────────────────────────────────────────────
    void setupInteraction();
    void syncInteraction();
    void cameraChanged();
    void updateOverlayLayout();
    void requestUpdate();
    void markOverlayDirty();
    QSize hostSize() const;
    void setHostCursor(viz::CursorShape shape);
    QImage grabSceneFrame();
    void renderFrame(QRhi* rhi, int sampleCount, QRhiCommandBuffer* cb);
    void loadMultiViewSettings();
    void saveMultiViewSettings() const;
    void updateInflatedSurfaceTransforms();

    // ── Rendering ───────────────────────────────────────────────────────
    std::unique_ptr<BrainRenderer> m_renderer; /**< GPU renderer for all surfaces/dipoles. */
    BrainTreeModel* m_model = nullptr;         /**< Data model driving the scene graph (not owned). */

    std::map<const QStandardItem*, std::shared_ptr<BrainSurface>> m_itemSurfaceMap; /**< Model-item → renderable surface. */
    std::map<const QStandardItem*, std::shared_ptr<DipoleObject>> m_itemDipoleMap;  /**< Model-item → dipole renderable. */

    std::map<std::string, std::shared_ptr<BrainSurface>> m_surfaces; /**< Key → surface lookup (e.g. "lh_pial", "bem_head"). */
    std::shared_ptr<BrainSurface> m_activeSurface;                   /**< Currently selected brain surface for stats. */
    std::string m_activeSurfaceType;                                 /**< Type name of the active surface (e.g. "pial"). */

    // ── SubView state ──────────────────────────────────────────────────
    static constexpr int kDefaultViewportCount = 4; /**< Default number of multi-view panes. */
    SubView m_singleView;                           /**< Render state for single-view mode. */
    std::vector<SubView> m_subViews;                /**< Per-pane render state for multi-view mode. */
    int m_visualizationEditTarget = -1;             /**< Active pane for UI edits (-1 = single, 0..N-1 = multi). */

    // ── Active (runtime) copies — kept in sync with selected SubView ──
    ShaderMode m_brainShaderMode = Standard;          /**< Current brain shader mode. */
    ShaderMode m_bemShaderMode = Standard;            /**< Current BEM shader mode. */
    VisualizationMode m_currentVisMode = ModeSurface; /**< Current overlay mode. */
    bool m_lightingEnabled = true;                    /**< Whether per-fragment lighting is active. */

    // ── Extracted components ───────────────────────────────────────────
    viz::OrbitCamera m_camera;                   /**< Camera maths. */
    viz::BrainViewInteraction m_interaction;     /**< Mouse / wheel / key state machine and pane layout. */
    SensorFieldMapper m_fieldMapper;             /**< Sensor → surface field mapping helper. */
    SourceEstimateManager m_sourceManager;       /**< Source estimate lifecycle (load, stream, navigate). */
    RtSensorStreamManager m_sensorStreamManager; /**< Real-time sensor streaming lifecycle. */

    // ── Camera state (the single-view rotation is m_singleView.rotation) ──
    QVector3D m_sceneCenter = QVector3D(0, 0, 0); /**< Bounding-box centre of the scene. */
    float m_sceneSize = 0.3f;                     /**< Bounding-box diagonal of the scene (metres). */
    bool m_cameraFocusOverride = false;           /**< When true, m_sceneCenter/m_sceneSize are user-set. */
    QVector3D m_cameraFocusCenter;                /**< Override center (valid when m_cameraFocusOverride). */
    float m_cameraFocusSize = 0.3f;               /**< Override size   (valid when m_cameraFocusOverride). */
    QTimer* m_pAutoRotateTimer = nullptr;         /**< Drives the automated rotation animation. */

    // ── UI overlays ────────────────────────────────────────────────────
    int m_frameCount = 0;                             /**< Frames rendered since last FPS sample. */
    std::chrono::steady_clock::time_point m_fpsStart; /**< Start of the current FPS measurement window. */
    bool m_sceneDirty = true;                         /**< Set when scene needs redraw; cleared after render. */
    qint64 m_cachedVertexCount = 0;                   /**< Cached visible vertex count (invalidated on surface changes). */
    bool m_vertexCountDirty = true;                   /**< Recount vertices on next FPS update when true. */
    bool m_infoPanelVisible = true;                   /**< Whether the info overlay panel is shown. */

    // ── Scene objects ──────────────────────────────────────────────────
    std::unique_ptr<DipoleObject> m_dipoles;                    /**< Standalone dipole set (loaded via file). */
    std::unique_ptr<NetworkObject> m_network;                   /**< Connectivity network visualization. */
    std::unique_ptr<PolylineObject> m_headPath;                 /**< Head movement path over the course of a measurement. */
    std::unique_ptr<mne::viz::qt::VideoOverlay> m_videoOverlay; /**< Live RGB video overlay decal. */

    // ── MRI slices ─────────────────────────────────────────────────────
    static constexpr int kMaxSliceSlots = 3;
    mne::viz::ElectrodeObject* m_electrodes = nullptr;        /**< Non-owning. */
    mne::viz::qt::SliceObject* m_slices[kMaxSliceSlots] = {}; /**< Non-owning pointers to slice data. */
    bool m_sliceVisible[kMaxSliceSlots] = {true, true, true}; /**< Per-slot visibility flags. */

    /** Update the scene bounding box based on visible objects. */
    void updateSceneBounds();

    // ── Coordinate transforms ──────────────────────────────────────────
    mne::FiffCoordTrans m_headToMriTrans;                         /**< Head-to-MRI coordinate transform. */
    mne::FiffDigPointSet m_cardinalDigPoints;                     /**< Cardinal dig points from last sensor load. */
    bool m_applySensorTrans = true;                               /**< Whether to apply the transform to sensors/digitizers. */
    std::filesystem::path m_megHelmetOverridePath;                /**< Optional override path for MEG helmet surface. */
    Eigen::Matrix4f m_devHeadTrans = Eigen::Matrix4f::Identity(); /**< Device→Head transformation from last sensor load. */
    bool m_hasDevHead = false;                                    /**< Whether a valid Device→Head transform is available. */
    bool m_dipolesVisible = true;                                 /**< Whether dipoles are rendered. */
    bool m_networkVisible = false;                                /**< Whether the connectivity network is rendered. */

    // ── Ray-pick hover state ───────────────────────────────────────────
    QStandardItem* m_hoveredItem = nullptr; /**< Model item currently under the cursor. */
    int m_hoveredIndex = -1;                /**< Vertex index at the hover point. */
    QString m_hoveredRegion;                /**< Atlas region name at the hover point. */
    std::string m_hoveredSurfaceKey;        /**< FsSurface map key at the hover point. */

    // ── Debug intersection ─────────────────────────────────────────────
    std::shared_ptr<BrainSurface> m_debugPointerSurface; /**< Semi-transparent sphere at hit point. */
    QVector3D m_lastIntersectionPoint;                   /**< World-space position of last ray hit. */
    bool m_hasIntersection = false;                      /**< Whether the last ray cast produced a hit. */


    // ── Multi-view support ─────────────────────────────────────────────
    ViewMode m_viewMode = SingleView; /**< Current view mode (single or multi). */
    int m_viewCount = 1;              /**< Number of viewports to show (1..kDefaultViewportCount). */

    // ── Host and 2-D overlay ───────────────────────────────────────────
    BrainViewHost* m_host = nullptr;  /**< Surface showing the view (not owned). */
    viz::BrainViewOverlay m_overlay;  /**< Labels and pane separators, in logical surface coordinates. */
    mutable QImage m_overlayCache;    /**< Last painted overlay. */
    bool m_overlayDirty = true;       /**< Overlay must be repainted. */
    bool m_inRender = false;          /**< True while a frame is recorded. */
    bool m_overlaySuppressed = false; /**< Screenshot frames leave the overlay out. */
};

} // namespace mne::viz::qt

#endif // DISP3D_BRAINVIEWCORE_H
