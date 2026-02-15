//=============================================================================================================
/**
 * @file     brainview.h
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
 * @brief    BrainView class declaration.
 *
 */

#ifndef BRAINVIEW_H
#define BRAINVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrenderer.h"
#include "multiviewlayout.h"
#include "../core/viewstate.h"
#include "../input/cameracontroller.h"
#include "../input/raypicker.h"
#include "../scene/sensorfieldmapper.h"
#include "../geometry/meshfactory.h"
#include "../renderable/brainsurface.h"
#include "../renderable/dipoleobject.h"
#include "../renderable/sourceestimateoverlay.h"
#include "../model/braintreemodel.h"
#include "../model/items/surfacetreeitem.h"

#include <mne/mne_bem.h>
#include <mne/mne_sourcespace.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <QWidget>
#include <QRhiWidget>
#include <QMap>
#include <QElapsedTimer>
#include <QStandardItem>
#include <memory>
#include <QQuaternion>
#include <QThread>
#include <Eigen/Sparse>
#include <QRect>

class QLabel;
class QTimer;
class StcLoadingWorker;
class QFrame;
class RtSourceDataController;

//=============================================================================================================
/**
 * BrainView is the main widget for the 3D brain visualization. It handles user interaction,
 * surface loading, and coordinates with the BrainRenderer.
 *
 * @brief    BrainView class.
 */
class BrainView : public QRhiWidget
{
    Q_OBJECT

public:
    /**
     * View mode for single or multi-viewport display.
     */
    enum ViewMode {
        SingleView,     /**< Single viewport with interactive camera */
        MultiView       /**< Three viewports with fixed cameras (top, left, front) */
    };

public:
    //=========================================================================================================
    /**
     * Constructor
     *
     * @param[in] parent     Parent widget.
     */
    explicit BrainView(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~BrainView();

    //=========================================================================================================
    /**
     * Set the data model.
     *
     * @param[in] model      Pointer to BrainTreeModel.
     */
    void setModel(BrainTreeModel *model);

    //=========================================================================================================
    /**
     * Set the initial camera rotation for this view.
     * Each view can start from a different orientation (e.g., top, front, left).
     *
     * @param[in] rotation   Initial camera rotation quaternion.
     */
    void setInitialCameraRotation(const QQuaternion &rotation);

public slots:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    //=========================================================================================================
    /**
     * Set the active surface type to search for (e.g. "pial").
     *
     * @param[in] type       The surface type to activate.
     */
    void setActiveSurface(const QString &type);

    //=========================================================================================================
    /**
     * Set the shader mode (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setShaderMode(const QString &mode);

    //=========================================================================================================
    /**
     * Set the shader mode for BEM surfaces (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setBemShaderMode(const QString &mode);

    /**
     * Synchronize all BEM shader targets to their respective brain shader targets.
     */
    void syncBemShadersToBrainShaders();

    //=========================================================================================================
    /**
     * Set the overlay mode (Surface, Annotation, Scientific).
     *
     * @param[in] mode       The visualization mode to set.
     */
    void setVisualizationMode(const QString &mode);

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
    void setBemVisible(const QString &name, bool visible);

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
    void setSensorVisible(const QString &type, bool visible);

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
    void setMegHelmetOverride(const QString &path);

    //=========================================================================================================
    /**
     * Toggle visibility of dipoles.
     *
     * @param[in] visible    Visibility state.
     */
    void setDipoleVisible(bool visible);

    //=========================================================================================================
    /**
     * Enable or disable lighting for the scene.
     *
     * @param[in] enabled    True to enable lighting, false to disable.
     */
    void setLightingEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Save a snapshot of the current view to a file.
     */
    void saveSnapshot();

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
    int  viewCount() const { return m_viewCount; }

    //=========================================================================================================
    /**
     * Enable or disable a specific viewport in multi-view mode.
     *
        * @param[in] index      Viewport index (0=Top, 1=Perspective, 2=Front, 3=Left).
     * @param[in] enabled    True to enable, false to disable.
     */
    void setViewportEnabled(int index, bool enabled);

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
    QString activeSurfaceForTarget(int target) const;

    /**
     * Get configured shader mode name for a target view.
     */
    QString shaderModeForTarget(int target) const;

    /**
     * Get configured BEM shader mode name for a target view.
     */
    QString bemShaderModeForTarget(int target) const;

    /**
     * Get configured overlay mode name for a target view.
     */
    QString overlayModeForTarget(int target) const;

    /**
     * Check object visibility for a given target view.
     *
     * @param[in] object     Object key (e.g. "lh", "bem_head", "sens_meg").
     * @param[in] target     -1=Single, 0=Top, 1=Perspective, 2=Front, 3=Left.
     * @return               True if visible, false otherwise.
     */
    bool objectVisibleForTarget(const QString &object, int target) const;

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
     * Get the current view mode.
     *
     * @return Current ViewMode.
     */
    ViewMode viewMode() const { return m_viewMode; }

    //=========================================================================================================
    /**
     * Load source estimate files (.stc) for both hemispheres.
     *
     * @param[in] lhPath     Path to left hemisphere .stc file.
     * @param[in] rhPath     Path to right hemisphere .stc file.
     * @return True if successful.
     */
    bool loadSourceEstimate(const QString &lhPath, const QString &rhPath);

    //=========================================================================================================
    /**
     * Load sensors (MEG/EEG/Digitizers) from a FIF file.
     *
     * @param[in] fifPath    Path to the FIF file.
     * @return True if successful.
     */
    bool loadSensors(const QString &fifPath);

    //=========================================================================================================
    /**
     * Load dipoles from a .dip or .bdip file.
     *
     * @param[in] dipPath    Path to the dipole file.
     * @return True if successful.
     */
    bool loadDipoles(const QString &dipPath);

    //=========================================================================================================
    /**
     * Load source space from a FIF file (forward solution or source space file).
     *
     * @param[in] fwdPath    Path to the FIF file containing source space data.
     * @return True if successful.
     */
    bool loadSourceSpace(const QString &fwdPath);

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
    static QStringList probeEvokedSets(const QString &evokedPath);

    /**
     * Load sensor measurements from an evoked/average FIF file.
     *
     * @param[in] evokedPath   Path to the evoked/average FIF file.
     * @param[in] aveIndex     Dataset index to load from the file.
     * @return True if successful.
     */
    bool loadSensorField(const QString &evokedPath, int aveIndex = 0);

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
    bool loadTransformation(const QString &transPath);

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
    void castRay(const QPoint &pos);

    //=========================================================================================================
    /**
     * Set the colormap for source estimate visualization.
     *
     * @param[in] name       Colormap name ("Hot", "Jet", etc.).
     */
    void setSourceColormap(const QString &name);

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
    void setSensorFieldVisible(const QString &type, bool visible);

    //=========================================================================================================
    /**
     * Set contour visibility for sensor field maps.
     *
     * @param[in] type       "MEG" or "EEG".
     * @param[in] visible    Visibility state.
     */
    void setSensorFieldContourVisible(const QString &type, bool visible);

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
    void setSensorFieldColormap(const QString &name);

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
     * @param[in] data       Source activity vector.
     */
    void pushRealtimeSourceData(const Eigen::VectorXd &data);

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
    bool sensorFieldTimeRange(float &tmin, float &tmax) const;

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
    void sensorFieldLoaded(int numTimePoints);

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
    void stcLoadingProgress(int percent, const QString &message);

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
    void hoveredRegionChanged(const QString &regionName);

    //=========================================================================================================
    /**
     * Emitted when the active visualization edit target changes.
     *
     * @param[in] target  New target index (-1 = single, 0..N-1 = multi pane).
     */
    void visualizationEditTargetChanged(int target);

private slots:
    //=========================================================================================================
    /**
     * Called when async STC loading finishes.
     *
     * @param[in] success    True if loading succeeded.
     */
    void onStcLoadingFinished(bool success);

    //=========================================================================================================
    /**
     * Called when new per-vertex colors arrive from the real-time streaming controller.
     *
     * @param[in] colorsLh   Per-vertex ABGR colors for the left hemisphere.
     * @param[in] colorsRh   Per-vertex ABGR colors for the right hemisphere.
     */
    void onRealtimeColorsAvailable(const QVector<uint32_t> &colorsLh,
                                   const QVector<uint32_t> &colorsRh);

private:
    // Note: ViewVisibilityProfile and SubView are defined in core/viewstate.h.
    // SplitterHit is defined in view/multiviewlayout.h.

    /** Return the SubView for a given target (-1 = single, 0..3 = multi). */
    SubView&       subViewForTarget(int target);
    const SubView& subViewForTarget(int target) const;

    ViewVisibilityProfile& visibilityProfileForTarget(int target);
    const ViewVisibilityProfile& visibilityProfileForTarget(int target) const;

    void refreshSensorTransforms();

    // ── Sensor field mapping (delegated to SensorFieldMapper) ──────────
    bool buildSensorFieldMapping();
    void applySensorFieldMap();
    QString findHeadSurfaceKey() const;
    QString findHelmetSurfaceKey() const;

    // ── Contour helpers (TODO: migrate to SensorFieldMapper) ──────────
    float contourStep(float minVal, float maxVal, int targetTicks) const;
    void updateContourSurfaces(const QString &prefix,
                               const BrainSurface &surface,
                               const QVector<float> &values,
                               float step,
                               bool visible);

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // ── Layout helpers (delegate to MultiViewLayout) ───────────────────
    QVector<int> enabledViewportIndices() const;
    int enabledViewportCount() const;
    int viewportIndexAt(const QPoint& pos) const;
    QRect multiViewSlotRect(int slot, int numEnabled, const QSize& outputSize) const;
    SplitterHit hitTestSplitter(const QPoint& pos, int numEnabled, const QSize& outputSize) const;

    // ── Helpers ────────────────────────────────────────────────────────
    void updateSplitterCursor(const QPoint& pos);
    void updateViewportSeparators();
    void updateOverlayLayout();
    void updateViewportLabelHighlight();
    void showViewportPresetMenu(int viewport, const QPoint &globalPos);
    void logPerspectiveRotation(const QString& context) const;
    void loadMultiViewSettings();
    void saveMultiViewSettings() const;
    void updateInflatedSurfaceTransforms();

    // ── Rendering ───────────────────────────────────────────────────────
    std::unique_ptr<BrainRenderer> m_renderer;      /**< GPU renderer for all surfaces/dipoles. */
    BrainTreeModel* m_model = nullptr;              /**< Data model driving the scene graph (not owned). */

    QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> m_itemSurfaceMap;  /**< Model-item → renderable surface. */
    QMap<const QStandardItem*, std::shared_ptr<DipoleObject>> m_itemDipoleMap;   /**< Model-item → dipole renderable. */

    QMap<QString, std::shared_ptr<BrainSurface>> m_surfaces;  /**< Key → surface lookup (e.g. "lh_pial", "bem_head"). */
    std::shared_ptr<BrainSurface> m_activeSurface;            /**< Currently selected brain surface for stats. */
    QString m_activeSurfaceType;                              /**< Type name of the active surface (e.g. "pial"). */

    // ── SubView state ──────────────────────────────────────────────────
    static constexpr int kDefaultViewportCount = 4; /**< Default number of multi-view panes. */
    SubView m_singleView;                           /**< Render state for single-view mode. */
    QVector<SubView> m_subViews;                    /**< Per-pane render state for multi-view mode. */
    int m_visualizationEditTarget = -1;             /**< Active pane for UI edits (-1 = single, 0..N-1 = multi). */

    // ── Active (runtime) copies — kept in sync with selected SubView ──
    BrainRenderer::ShaderMode m_brainShaderMode = BrainRenderer::Standard;      /**< Current brain shader mode. */
    BrainRenderer::ShaderMode m_bemShaderMode = BrainRenderer::Standard;        /**< Current BEM shader mode. */
    BrainSurface::VisualizationMode m_currentVisMode = BrainSurface::ModeSurface; /**< Current overlay mode. */
    bool m_lightingEnabled = true;                  /**< Whether per-fragment lighting is active. */

    // ── Extracted components ───────────────────────────────────────────
    CameraController   m_camera;                    /**< Camera maths helper. */
    MultiViewLayout    m_layout;                    /**< Multi-view pane geometry and splitter logic. */
    SensorFieldMapper  m_fieldMapper;               /**< Sensor → surface field mapping helper. */

    // ── Camera state ───────────────────────────────────────────────────
    QQuaternion m_cameraRotation;                   /**< Global camera orientation quaternion. */
    QVector3D m_sceneCenter = QVector3D(0, 0, 0);   /**< Bounding-box centre of the scene. */
    float m_sceneSize = 0.3f;                       /**< Bounding-box diagonal of the scene (metres). */
    float m_zoom = 0.0f;                            /**< Zoom level for single-view mode. */
    QPoint m_lastMousePos;                          /**< Previous mouse position for drag deltas. */

    // ── UI overlays ────────────────────────────────────────────────────
    int m_frameCount = 0;                           /**< Frames rendered since last FPS sample. */
    QElapsedTimer m_fpsTimer;                       /**< Timer for FPS measurement. */
    QLabel *m_fpsLabel = nullptr;                   /**< Overlay label showing FPS and vertex count. */
    QLabel *m_singleViewInfoLabel = nullptr;        /**< Overlay label for single-view shader/surface info. */
    QTimer *m_updateTimer = nullptr;                /**< Periodic repaint timer (~60 Hz). */
    int m_snapshotCounter = 0;                      /**< Sequential counter for snapshot filenames. */
    bool m_infoPanelVisible = true;                 /**< Whether the info overlay panel is shown. */

    // ── Source estimate ────────────────────────────────────────────────
    std::unique_ptr<SourceEstimateOverlay> m_sourceOverlay; /**< Active source estimate overlay data. */
    std::unique_ptr<DipoleObject> m_dipoles;        /**< Standalone dipole set (loaded via file). */
    int m_currentTimePoint = 0;                     /**< Current source estimate time sample index. */

    /** Update the scene bounding box based on visible objects. */
    void updateSceneBounds();

    // ── Coordinate transforms ──────────────────────────────────────────
    FIFFLIB::FiffCoordTrans m_headToMriTrans;       /**< Head-to-MRI coordinate transform. */
    bool m_applySensorTrans = true;                 /**< Whether to apply the transform to sensors/digitizers. */
    QString m_megHelmetOverridePath;                /**< Optional override path for MEG helmet surface. */
    bool m_dipolesVisible = true;                   /**< Whether dipoles are rendered. */

    // ── Ray-pick hover state ───────────────────────────────────────────
    QStandardItem* m_hoveredItem = nullptr;         /**< Model item currently under the cursor. */
    int m_hoveredIndex = -1;                        /**< Vertex index at the hover point. */
    QString m_hoveredRegion;                        /**< Atlas region name at the hover point. */
    QString m_hoveredSurfaceKey;                    /**< Surface map key at the hover point. */
    QLabel* m_regionLabel = nullptr;                /**< Overlay label showing the hovered region name. */
    QVector<QLabel*> m_viewportNameLabels;          /**< Per-viewport name labels (e.g. "Top", "Front"). */
    QVector<QLabel*> m_viewportInfoLabels;          /**< Per-viewport info labels (shader, surface info). */

    // ── Debug intersection ─────────────────────────────────────────────
    std::shared_ptr<BrainSurface> m_debugPointerSurface; /**< Semi-transparent sphere at hit point. */
    QVector3D m_lastIntersectionPoint;              /**< World-space position of last ray hit. */
    bool m_hasIntersection = false;                 /**< Whether the last ray cast produced a hit. */

    // ── Async STC loading ──────────────────────────────────────────────
    QThread* m_loadingThread = nullptr;             /**< Background thread for STC file loading. */
    StcLoadingWorker* m_stcWorker = nullptr;        /**< Worker object performing the STC load. */
    bool m_isLoadingStc = false;                    /**< True while an async STC load is in progress. */

    // ── Real-time streaming ─────────────────────────────────────────────
    std::unique_ptr<RtSourceDataController> m_rtController; /**< Real-time source data streaming controller. */
    bool m_isRtStreaming = false;                   /**< True while real-time streaming is active. */

    // ── Multi-view support ─────────────────────────────────────────────
    ViewMode m_viewMode = SingleView;               /**< Current view mode (single or multi). */
    int m_viewCount = 1;                            /**< Number of viewports to show (1..kDefaultViewportCount). */
    float m_multiSplitX = 0.5f;                     /**< Horizontal splitter position (0..1). */
    float m_multiSplitY = 0.5f;                     /**< Vertical splitter position (0..1). */
    bool m_isDraggingSplitter = false;              /**< True while the user is dragging a splitter. */
    SplitterHit m_activeSplitter = SplitterHit::None; /**< Which splitter is being dragged. */
    int m_splitterHitTolerancePx = 6;               /**< Pixel tolerance for splitter hit testing. */
    int m_splitterMinPanePx = 80;                   /**< Minimum pane size in pixels. */
    int m_separatorLinePx = 2;                      /**< Separator line thickness in pixels. */
    QFrame* m_verticalSeparator = nullptr;          /**< Visual separator between left/right panes. */
    QFrame* m_horizontalSeparator = nullptr;        /**< Visual separator between top/bottom panes. */
    bool m_perspectiveRotatedSincePress = false;    /**< True if mouse drag rotated a perspective pane. */

    // ── Sensor field mapping state ─────────────────────────────────────
    FIFFLIB::FiffEvoked m_sensorEvoked;             /**< Loaded evoked data for field mapping. */
    bool m_sensorFieldLoaded = false;               /**< Whether evoked sensor data has been loaded. */
    int m_sensorFieldTimePoint = 0;                 /**< Current time sample index for field display. */
    bool m_megFieldMapOnHead = false;               /**< Whether MEG field map is projected onto head surface. */
    QString m_sensorFieldColormap = QStringLiteral("MNE"); /**< Colormap name for field visualisation. */
    QString m_megFieldSurfaceKey;                   /**< Surface key used for MEG field mapping. */
    QString m_eegFieldSurfaceKey;                   /**< Surface key used for EEG field mapping. */
    QString m_megFieldContourPrefix = QStringLiteral("sens_contour_meg"); /**< Key prefix for MEG contour surfaces. */
    QString m_eegFieldContourPrefix = QStringLiteral("sens_contour_eeg"); /**< Key prefix for EEG contour surfaces. */
    QVector<int> m_megFieldPick;                    /**< Channel indices picked for MEG field mapping. */
    QVector<int> m_eegFieldPick;                    /**< Channel indices picked for EEG field mapping. */
    QVector<Eigen::Vector3f> m_megFieldPositions;   /**< 3D positions of MEG sensor coils. */
    QVector<Eigen::Vector3f> m_eegFieldPositions;   /**< 3D positions of EEG electrode locations. */
    QSharedPointer<Eigen::MatrixXf> m_megFieldMapping; /**< Interpolation matrix: MEG sensors → surface. */
    QSharedPointer<Eigen::MatrixXf> m_eegFieldMapping; /**< Interpolation matrix: EEG sensors → surface. */
};

#endif // BRAINVIEW_H
