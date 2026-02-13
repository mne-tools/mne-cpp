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

private slots:
    //=========================================================================================================
    /**
     * Called when async STC loading finishes.
     *
     * @param[in] success    True if loading succeeded.
     */
    void onStcLoadingFinished(bool success);

private:
    /**
     * Per-object visibility flags for a single view.
     */
    struct ViewVisibilityProfile {
        bool lh = true;
        bool rh = true;
        bool bemHead = true;
        bool bemOuterSkull = true;
        bool bemInnerSkull = true;
        bool sensMeg = false;
        bool sensMegGrad = false;
        bool sensMegMag = false;
        bool sensMegHelmet = false;
        bool sensEeg = false;
        bool dig = false;
        bool digCardinal = false;
        bool digHpi = false;
        bool digEeg = false;
        bool digExtra = false;
        bool megFieldMap = false;
        bool eegFieldMap = false;
        bool megFieldContours = false;
        bool eegFieldContours = false;
        bool dipoles = true;
        bool sourceSpace = false;
        bool megFieldMapOnHead = false;
    };

    /**
     * Encapsulates all per-view state for a single viewport (single view or
     * one pane in multi-view).  The class stores five instances: one for the
     * single-view and four for the multi-view panes.
     */
    struct SubView {
        QString                         surfaceType      = "pial";
        BrainRenderer::ShaderMode       brainShader      = BrainRenderer::Standard;
        BrainRenderer::ShaderMode       bemShader        = BrainRenderer::Standard;
        BrainSurface::VisualizationMode overlayMode      = BrainSurface::ModeSurface;
        ViewVisibilityProfile           visibility;
        float                           zoom             = 0.0f;
        QVector2D                       pan;
        QQuaternion                     perspectiveRotation;
        int                             preset           = 1;   // 0=Top,1=Perspective,2=Front,3=Left,4=Bottom,5=Back,6=Right
        bool                            enabled          = true;
    };

    /** Return the SubView for a given target (-1 = single, 0..3 = multi). */
    SubView&       subViewForTarget(int target);
    const SubView& subViewForTarget(int target) const;

    static bool objectVisibleFromProfile(const ViewVisibilityProfile &profile, const QString &object);
    static void setObjectVisibleInProfile(ViewVisibilityProfile &profile, const QString &object, bool visible);
    ViewVisibilityProfile& visibilityProfileForTarget(int target);
    const ViewVisibilityProfile& visibilityProfileForTarget(int target) const;
    bool shouldRenderSurfaceForView(const QString &key, const ViewVisibilityProfile &profile) const;

    void refreshSensorTransforms();
    bool buildSensorFieldMapping();
    void applySensorFieldMap();
    void updateContourSurfaces(const QString &prefix,
                               const BrainSurface &surface,
                               const QVector<float> &values,
                               float step,
                               bool visible);
    float contourStep(float minVal, float maxVal, int targetTicks) const;
    QString findHeadSurfaceKey() const;
    QString findHelmetSurfaceKey() const;

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void hoveredRegionChanged(const QString &regionName);
    void visualizationEditTargetChanged(int target);

private:
    enum class SplitterHit {
        None,
        Vertical,
        Horizontal,
        Both
    };

    int enabledViewportCount() const;
    int viewportIndexAt(const QPoint& pos) const;
    QRect multiViewSlotRect(int slot, int numEnabled, const QSize& outputSize) const;
    SplitterHit hitTestSplitter(const QPoint& pos, int numEnabled, const QSize& outputSize) const;
    void updateSplitterCursor(const QPoint& pos);
    void updateViewportSeparators();
    void updateOverlayLayout();
    void updateViewportLabelHighlight();
    void showViewportPresetMenu(int viewport, const QPoint &globalPos);
    void logPerspectiveRotation(const QString& context) const;
    void loadMultiViewSettings();
    void saveMultiViewSettings() const;
    void updateInflatedSurfaceTransforms();

    std::unique_ptr<BrainRenderer> m_renderer;
    BrainTreeModel* m_model = nullptr;
    
    // Map Model Items to Render Resources
    QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> m_itemSurfaceMap;
    QMap<const QStandardItem*, std::shared_ptr<DipoleObject>> m_itemDipoleMap;

    // Legacy/Helper maps (refactoring TODO: remove if fully replaced by model mapping)
    QMap<QString, std::shared_ptr<BrainSurface>> m_surfaces; 
    std::shared_ptr<BrainSurface> m_activeSurface;
    QString m_activeSurfaceType;

    // ── SubView state ──────────────────────────────────────────────────
    SubView m_singleView;                           // single-view state
    SubView m_subViews[4];                          // multi-view panes 0..3
    int m_visualizationEditTarget = -1;             // -1=single, 0..3=multi
    
    // Sensors (Lists of surfaces/meshes for coils/electrodes)
    QList<std::shared_ptr<BrainSurface>> m_megSensors;
    QList<std::shared_ptr<BrainSurface>> m_eegSensors;
    QList<std::shared_ptr<BrainSurface>> m_digitizers;
    
    // ── Active (runtime) copies — kept in sync with selected SubView ──
    BrainRenderer::ShaderMode m_brainShaderMode = BrainRenderer::Standard;
    BrainRenderer::ShaderMode m_bemShaderMode = BrainRenderer::Standard;
    BrainSurface::VisualizationMode m_currentVisMode = BrainSurface::ModeSurface;
    bool m_lightingEnabled = true;
    
    QQuaternion m_cameraRotation;
    QVector3D m_sceneCenter = QVector3D(0,0,0);
    float m_sceneSize = 0.3f; // Default ~30cm
    float m_zoom = 0.0f;      // single-view zoom (kept separate from SubView for now)
    QPoint m_lastMousePos;
    
    int m_frameCount = 0;
    QElapsedTimer m_fpsTimer;
    QLabel *m_fpsLabel = nullptr;
    QTimer *m_updateTimer = nullptr;
    int m_snapshotCounter = 0;
    bool m_infoPanelVisible = true;
    
    std::unique_ptr<SourceEstimateOverlay> m_sourceOverlay;
    std::unique_ptr<DipoleObject> m_dipoles;
    int m_currentTimePoint = 0;
    
    //=========================================================================================================
    /**
     * Update the scene bounding box based on visible objects.
     */
    void updateSceneBounds();
    
    FIFFLIB::FiffCoordTrans m_headToMriTrans;
    bool m_applySensorTrans = true;
    QString m_megHelmetOverridePath;
    bool m_dipolesVisible = true;
     
    QStandardItem* m_hoveredItem = nullptr;
    int m_hoveredIndex = -1;
    QString m_hoveredRegion;
    QString m_hoveredSurfaceKey;
    QLabel* m_regionLabel = nullptr;
    QLabel* m_viewportNameLabels[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel* m_viewportInfoLabels[4] = {nullptr, nullptr, nullptr, nullptr};
     
    // Debug Intersection Pointer
    std::shared_ptr<BrainSurface> m_debugPointerSurface;
    QVector3D m_lastIntersectionPoint;
    bool m_hasIntersection = false;
     
    // Async loading
    QThread* m_loadingThread = nullptr;
    StcLoadingWorker* m_stcWorker = nullptr;
    bool m_isLoadingStc = false;
     
    // Multi-view support
    ViewMode m_viewMode = SingleView;
    QQuaternion m_multiViewCameras[4]; // Top, Perspective, Front, Left views
    float m_multiSplitX = 0.5f;
    float m_multiSplitY = 0.5f;
    bool m_isDraggingSplitter = false;
    SplitterHit m_activeSplitter = SplitterHit::None;
    int m_splitterHitTolerancePx = 6;
    int m_splitterMinPanePx = 80;
    int m_separatorLinePx = 2;
    QFrame* m_verticalSeparator = nullptr;
    QFrame* m_horizontalSeparator = nullptr;
    bool m_perspectiveRotatedSincePress = false;

    // Sensor field mapping
    FIFFLIB::FiffEvoked m_sensorEvoked;
    bool m_sensorFieldLoaded = false;
    int m_sensorFieldTimePoint = 0;
    bool m_megFieldMapOnHead = false;
    QString m_sensorFieldColormap = "MNE";
    QString m_megFieldSurfaceKey;
    QString m_eegFieldSurfaceKey;
    QString m_megFieldContourPrefix = "sens_contour_meg";
    QString m_eegFieldContourPrefix = "sens_contour_eeg";
    QVector<int> m_megFieldPick;
    QVector<int> m_eegFieldPick;
    QVector<Eigen::Vector3f> m_megFieldPositions;
    QVector<Eigen::Vector3f> m_eegFieldPositions;
    QSharedPointer<Eigen::MatrixXf> m_megFieldMapping;
    QSharedPointer<Eigen::MatrixXf> m_eegFieldMapping;
};

#endif // BRAINVIEW_H
