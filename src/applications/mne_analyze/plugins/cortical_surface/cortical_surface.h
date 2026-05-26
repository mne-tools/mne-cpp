//=============================================================================================================
/**
 * @file     cortical_surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    CorticalSurface plugin — FreeSurfer cortical-surface loader
 *           and visualiser for mne_analyze.
 *
 *           This first slice ships the plugin shell, FreeSurfer subject
 *           browser, hemisphere/surface-type selection, and a `MultimodalScene`
 *           registration of the loaded surfaces. The QRhi rendering, vertex
 *           picking dock, and STC overlay loading land in subsequent slices
 *           with STC overlays, vertex picking, inverse dispatch and labels.
 *
 */

#ifndef MNEANALYZE_CORTICAL_SURFACE_H
#define MNEANALYZE_CORTICAL_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cortical_surface_global.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>
#include <QtCore/QtPlugin>

#include <inv/inv_source_estimate.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QCheckBox;
class QColor;
class QComboBox;
class QDockWidget;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QPushButton;
class QSlider;
class QTimer;
class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;

namespace ANSHAREDLIB {
    class Communicator;
    class Event;
}

namespace DISP3DLIB {
    class MultimodalScene;
}

namespace FSLIB {
    class FsSurface;
    class FsLabel;
}

//=============================================================================================================
// DEFINE NAMESPACE CORTICALSURFACEPLUGIN
//=============================================================================================================

namespace CORTICALSURFACEPLUGIN
{

//=============================================================================================================
/**
 * @brief Hemisphere selection for the cortical surface plugin.
 */
enum class HemisphereChoice {
    LeftOnly = 0,
    RightOnly,
    Both
};

//=============================================================================================================
/**
 * @brief Cortical surface variant requested by the user.
 *
 * Maps directly to the FreeSurfer surface filename suffix
 * (`lh.<name>` / `rh.<name>`).
 */
enum class CorticalSurfaceType {
    Inflated = 0,    /**< `lh.inflated` / `rh.inflated` */
    Pial,            /**< `lh.pial` / `rh.pial` */
    White            /**< `lh.white` / `rh.white` */
};

//=============================================================================================================
/**
 * @brief Options for @ref CorticalSurface::runComputeSourceEstimate.
 *
 * The method string selects which inverse algorithm is dispatched:
 *   - "MNE", "dSPM", "sLORETA", "eLORETA" → @ref INVLIB::InvMinimumNorm
 *   - "MxNE"                              → @ref INVLIB::InvMxne
 *   - "Gamma-MAP"                         → @ref INVLIB::InvGammaMap
 *   - "CMNE"                              → @ref INVLIB::InvCMNE
 */
struct CORTICAL_SURFACE_SHARED_EXPORT ComputeSourceEstimateOptions
{
    QString method  = QStringLiteral("MNE");
    double  snr     = 3.0;     /**< Linear regularisation: lambda² = 1 / SNR². */
    double  alpha   = 1.0;     /**< Sparsity weight (MxNE / Gamma-MAP). */
    double  loose   = 0.2;     /**< Loose orientation factor in [0, 1]. */
    double  depth   = 0.8;     /**< Depth weighting exponent in [0, 1]. */
    QString onnxPath;          /**< Optional CMNE LSTM model. */
};

//=============================================================================================================
/**
 * @brief One vertex picked from the cortex via @ref CorticalSurface::pickVertex.
 */
struct CORTICAL_SURFACE_SHARED_EXPORT CorticalPickedVertex
{
    int        hemi    = -1;          /**< 0 = lh, 1 = rh. */
    int        vertex  = -1;          /**< Surface-local vertex index. */
    QVector3D  world;                  /**< World position used to look up the vertex. */
    QString    name;                   /**< Auto-generated or user-renamed label. */
    QColor     color   = QColor("steelblue"); /**< Plot colour. */
};

//=============================================================================================================
/**
 * @brief mne_analyze plugin that loads and visualises FreeSurfer cortical surfaces.
 *
 * Workflow:
 *   1. User invokes "Load Surface…" from the plugin menu.
 *   2. The plugin asks for the FreeSurfer `SUBJECTS_DIR` and lists the
 *      subjects found there.
 *   3. The user picks subject + hemisphere set + surface type.
 *   4. The plugin loads the requested @ref FSLIB::FsSurface objects and
 *      registers them as @c BrainSurface layers on the shared
 *      @ref DISP3DLIB::MultimodalScene.
 *   5. The hemisphere toggle and surface-type combo box let the user
 *      switch what is shown without re-opening the dialog.
 *
 * The plugin emits the standard `MODEL_ADDED` event so other plugins
 * (STC overlay, vertex picking) can subscribe.
 */
class CORTICAL_SURFACE_SHARED_EXPORT CorticalSurface : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "cortical_surface.json")
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    CorticalSurface();
    ~CorticalSurface() override;

    // AbstractPlugin contract
    QSharedPointer<AbstractPlugin> clone() const override;
    void                           init() override;
    void                           unload() override;
    QString                        getName() const override;

    QMenu*                         getMenu() override;
    QDockWidget*                   getControl() override;
    QWidget*                       getView() override;
    QString                        getBuildInfo() override;

    void                           handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

    //=========================================================================================================
    /**
     * @return Pointer to the shared MultimodalScene used by this plugin.
     *         Owned by the plugin; consumers must not delete it.
     */
    DISP3DLIB::MultimodalScene* scene() const;

    //=========================================================================================================
    /**
     * Force-load the configured surfaces from a given FreeSurfer subject
     * directory. Public so unit tests can drive the loader without a GUI.
     *
     * @param[in] subjectsDir   Path to the SUBJECTS_DIR.
     * @param[in] subjectId     Subject id (sub-directory name in @p subjectsDir).
     * @param[in] hemi          Which hemispheres to load.
     * @param[in] type          Surface variant.
     *
     * @return true if at least one surface was loaded successfully.
     */
    bool loadSurfaces(const QString& subjectsDir,
                      const QString& subjectId,
                      HemisphereChoice hemi,
                      CorticalSurfaceType type);

    //=========================================================================================================
    /**
     * @return Map subject id -> (lh.inflated exists, rh.inflated exists)
     *         enumerated from @p subjectsDir. Used by the load dialog and
     *         by tests.
     */
    static QStringList enumerateSubjects(const QString& subjectsDir);

    //=========================================================================================================
    // STC overlay
    //=========================================================================================================

    /**
     * Read an STC file (FreeSurfer / MNE-Python compatible binary `.stc`)
     * and install the resulting source estimate as the active overlay.
     *
     * @param[in] path   Absolute path to the `.stc` file.
     * @return true if the file was loaded and a non-empty overlay was set.
     */
    bool loadSourceEstimate(const QString& path);

    /**
     * Install an already-constructed source estimate as the active overlay.
     * Resets the time slider to sample 0 and re-publishes the overlay layer
     * on the shared @ref DISP3DLIB::MultimodalScene.
     */
    void setSourceEstimate(const INVLIB::InvSourceEstimate& stc);

    /** @return Current overlay (empty when none loaded). */
    const INVLIB::InvSourceEstimate& sourceEstimate() const;

    /** @return Number of vertex rows in the overlay (0 if none). */
    int overlayRowCount() const;

    /** @return Number of time samples in the overlay (0 if none). */
    int totalTimeSamples() const;

    /** @return Vertex count of the loaded surface(s) (left + right), or 0. */
    int surfaceVertexCount() const;

    /** @return true when overlay vertex count matches the loaded surface
     *  vertex count, or there is no surface to compare against. */
    bool overlayMatchesSurface() const;

    /** @return Current time sample index, or -1 when no overlay is loaded. */
    int currentTimeSample() const;

    /**
     * Set the current time sample. Values are clamped to
     * `[0, totalTimeSamples()-1]` (or -1 if no overlay). Emits
     * @ref timeSampleChanged and updates the slider + the scene's shared
     * timeline cursor.
     */
    void setCurrentTimeSample(int sample);

    /**
     * Set the three colormap thresholds at once. The values are normalised
     * so the invariant `fthresh <= fmid <= fmax` always holds, regardless
     * of the order they are supplied in (the implementation sorts them).
     * Negative values are allowed; a `fmax == fthresh` overlay collapses
     * to a step function.
     */
    void setColormapThresholds(float fthresh, float fmid, float fmax);

    float fThresh() const;
    float fMid()    const;
    float fMax()    const;

    /** @return true when the playback timer is running. */
    bool isPlaying() const;

    /** Start playback at the configured FPS (no-op when already playing
     *  or when no overlay is loaded). */
    void play();

    /** Stop playback. */
    void pause();

    /** Convenience for the toolbar play/pause button. */
    void togglePlayPause();

    /** @return Configured playback frame-rate (samples per second). */
    double playbackFps() const;

    /** Set the playback frame-rate. Values <= 0 are clamped to 1.0 fps. */
    void setPlaybackFps(double fps);

    //=========================================================================================================
    // Vertex picking + Time Course dock
    //=========================================================================================================

    /** @return The "Source Time Course" dock (built on first call). */
    QDockWidget* getTimeCourseDock();

    /**
     * Pick the vertex of the currently loaded cortical surface closest to
     * @p worldPoint. On success the pick is appended to the Time Course
     * Manager and @ref vertexPicked is emitted.
     *
     * @return true if a vertex was identified.
     */
    bool pickVertex(const QVector3D& worldPoint);

    /** @return Number of picks stored in the Time Course Manager. */
    int pickedVertexCount() const;

    /** @return Snapshot of all currently stored picks. */
    QVector<CorticalPickedVertex> pickedVertices() const;

    /**
     * Look up the STC time course at a specific (hemisphere, vertex).
     * Returns an empty vector if no overlay is loaded or the vertex is
     * not represented in the overlay's vertex list.
     */
    QVector<double> timeCourseAt(int hemi, int vertex) const;

    /**
     * Export the currently stored picks (one column per pick, one row per
     * time sample) to @p path in CSV format. Emits @ref timeCoursesExported.
     */
    bool exportTimeCoursesCsv(const QString& path) const;

    //=========================================================================================================
    // Inverse computation (Compute Source Estimate dialog)
    //=========================================================================================================

    /**
     * Compute a source estimate from disk inputs and install it as the
     * active overlay (calls @ref setSourceEstimate on success).
     *
     * @param[in] fwdPath     `-fwd.fif` forward solution.
     * @param[in] covPath     `-cov.fif` noise covariance.
     * @param[in] evokedPath  `-ave.fif` evoked response.
     * @param[in] opts        Method + regularisation parameters.
     *
     * @return true on success.
     */
    bool runComputeSourceEstimate(const QString& fwdPath,
                                  const QString& covPath,
                                  const QString& evokedPath,
                                  const ComputeSourceEstimateOptions& opts);

    //=========================================================================================================
    // Labels dock (FreeSurfer annotation / atlas browser)
    //=========================================================================================================

    /** @return The "Labels" dock (built on first call). */
    QDockWidget* getLabelsDock();

    /** @return Snapshot of all labels currently loaded in the labels dock. */
    QList<FSLIB::FsLabel> labels() const;

    /**
     * Load a FreeSurfer `.label` file. The label is added to the labels
     * dock under its hemisphere root.
     *
     * @return Number of labels added (0 or 1).
     */
    int loadLabel(const QString& path);

    /**
     * Load a FreeSurfer `.annot` annotation and explode it into one
     * @ref FSLIB::FsLabel per parcel.
     *
     * @return Number of labels added.
     */
    int loadAnnot(const QString& path);

    /**
     * Extract an ROI time course from the loaded overlay using one of
     * "mean", "mean_flip", "pca_flip", "max", "auto" and append the trace
     * to the Time Course Manager.
     */
    bool extractLabelTimeCourse(const FSLIB::FsLabel& lbl, const QString& mode);

    /** Write @p lbl to @p path in FreeSurfer `.label` format. */
    bool saveLabel(const FSLIB::FsLabel& lbl, const QString& path);

signals:
    /** Emitted after @ref setSourceEstimate succeeds. */
    void sourceEstimateLoaded(int nVertices, int nTimes);

    /** Emitted when @ref setCurrentTimeSample actually changes the sample. */
    void timeSampleChanged(int sample);

    /** Emitted after @ref setColormapThresholds reorders/applies values. */
    void colormapThresholdsChanged(float fthresh, float fmid, float fmax);

    /** Emitted when @ref play / @ref pause toggle the playback state. */
    void playStateChanged(bool playing);

    /** Emitted after @ref pickVertex succeeds. */
    void vertexPicked(int hemi, int vertex);

    /** Emitted after @ref exportTimeCoursesCsv succeeds. */
    void timeCoursesExported(QString path);

    /** Emitted whenever the labels-dock contents change. */
    void labelsChanged();

    /** Emitted after @ref runComputeSourceEstimate succeeds. */
    void sourceEstimateComputed(QString method);

private slots:
    void onLoadSurfaceTriggered();
    void onHemisphereChoiceChanged(int index);
    void onSurfaceTypeChanged(int index);
    void onLoadStcTriggered();
    void onFThreshSpinChanged(double value);
    void onFMidSpinChanged(double value);
    void onFMaxSpinChanged(double value);
    void onTimeSliderChanged(int sample);
    void onPlayPauseClicked();
    void onPlaybackFpsChanged(double fps);
    void onPlaybackTick();
    void onTimeCourseManagerSelectionChanged();
    void onTimeCourseRenameClicked();
    void onTimeCourseColorClicked();
    void onTimeCourseRemoveClicked();
    void onTimeCourseExportClicked();
    void onComputeSourceEstimateTriggered();
    void onLoadLabelTriggered();
    void onLoadAnnotTriggered();
    void onSaveLabelTriggered();
    void onCreateLabelFromMarkedTriggered();
    void onLabelItemChanged(QTreeWidgetItem* item, int column);
    void onLabelTreeContextMenu(const QPoint& pos);

private:
    /** Build the controls dock (hemisphere combo, surface-type combo,
     *  "Load Surface…" button, status label). Call once from @ref init. */
    void buildControlDock();

    /** Translate (subject_id, hemi, type) into a FreeSurfer file path. */
    static QString surfaceFilePath(const QString& subjectsDir,
                                   const QString& subjectId,
                                   int            hemiCode,    // 0 = lh, 1 = rh
                                   CorticalSurfaceType type);

    /** Map a plugin enum to the on-disk surface filename suffix. */
    static QString surfaceTypeSuffix(CorticalSurfaceType type);

    /** Re-register the loaded surfaces on the scene with current visibility. */
    void refreshSceneLayers();

    /** (Re)publish the source estimate overlay layer on the shared scene. */
    void refreshOverlayLayer();

    /** Refresh time-slider tooltip + status label to reflect current sample. */
    void updateTimeReadout();

    /** Sync the three spin-boxes from the stored thresholds without
     *  re-emitting changed signals. */
    void syncThresholdSpins();

    /** Build the Time Course dock (chart + manager list) on first request. */
    void buildTimeCourseDock();

    /** Build the Labels dock on first request. */
    void buildLabelsDock();

    /** Refresh the manager list, plot widget and current selection state. */
    void refreshTimeCourseManager();

    /** @return The "Source Time Course" plot widget (lazily constructed). */
    class TimeCoursePlotter* plotter() const;

    /** Add a label to the labels dock under the appropriate hemi root. */
    QTreeWidgetItem* addLabelItem(const FSLIB::FsLabel& label);

    /** Append a raw trace (used by both vertex picks and label extracts). */
    void appendTrace(const QString& name,
                     const QColor& color,
                     const QVector<double>& trace,
                     int hemi   = -1,
                     int vertex = -1);

    /** Find the row in m_stc.vertices that matches a (hemi, vertex) tuple. */
    int findStcRowFor(int hemi, int vertex) const;

    /** Render the current plotter cursor + traces. */
    void refreshPlotter();

    /** Resolve the FsLabel cached as data on a tree item. */
    static FSLIB::FsLabel labelFromItem(QTreeWidgetItem* item);

    /** Write @p label to disk in FreeSurfer `.label` format. */
    static bool writeLabelFile(const FSLIB::FsLabel& label, const QString& path);

    ANSHAREDLIB::Communicator*           m_pCommu = nullptr;
    QPointer<QMenu>                      m_pMenu;
    QPointer<QAction>                    m_pLoadSurfaceAction;
    QPointer<QAction>                    m_pLoadStcAction;
    QPointer<QDockWidget>                m_pControlDock;
    QPointer<QComboBox>                  m_pHemiCombo;
    QPointer<QComboBox>                  m_pSurfaceTypeCombo;

    // Overlay controls
    QPointer<QDoubleSpinBox>             m_pFThreshSpin;
    QPointer<QDoubleSpinBox>             m_pFMidSpin;
    QPointer<QDoubleSpinBox>             m_pFMaxSpin;
    QPointer<QSlider>                    m_pTimeSlider;
    QPointer<QLabel>                     m_pTimeReadout;
    QPointer<QPushButton>                m_pPlayButton;
    QPointer<QDoubleSpinBox>             m_pFpsSpin;
    QPointer<QTimer>                     m_pPlaybackTimer;

    HemisphereChoice                     m_hemi = HemisphereChoice::Both;
    CorticalSurfaceType                  m_surfaceType = CorticalSurfaceType::Inflated;

    /** Loaded surfaces keyed by hemisphere index (0 = lh, 1 = rh). */
    QSharedPointer<FSLIB::FsSurface>     m_pSurfaceLh;
    QSharedPointer<FSLIB::FsSurface>     m_pSurfaceRh;

    /** Per-plugin scene; a future slice will share this with the
     *  MNE Inspect application via the AnalyzeData store. */
    QScopedPointer<DISP3DLIB::MultimodalScene> m_pScene;

    QString                              m_lastSubjectsDir;
    QString                              m_lastSubjectId;
    QString                              m_lastStcDir;

    // Overlay state
    INVLIB::InvSourceEstimate            m_stc;
    int                                  m_currentSample = -1;
    float                                m_fThresh = 0.0f;
    float                                m_fMid    = 0.5f;
    float                                m_fMax    = 1.0f;
    double                               m_fps = 10.0;
    bool                                 m_playing = false;

    //=========================================================================================================
    // Time Course dock (per-vertex STC trace)
    //=========================================================================================================

    QPointer<QDockWidget>                m_pTimeCourseDock;
    QPointer<QListWidget>                m_pTimeCourseList;
    class TimeCoursePlotter*             m_pPlotter = nullptr;
    QPointer<QPushButton>                m_pTcRenameBtn;
    QPointer<QPushButton>                m_pTcColorBtn;
    QPointer<QPushButton>                m_pTcRemoveBtn;
    QPointer<QPushButton>                m_pTcExportBtn;

    struct TraceEntry {
        QString          name;
        QColor           color;
        QVector<double>  data;
        int              hemi   = -1;
        int              vertex = -1;
        bool             isPick = false;     /**< true if produced by pickVertex(); false for label extracts. */
    };
    QVector<TraceEntry>                  m_traces;

    //=========================================================================================================
    // Inverse dialog state
    //=========================================================================================================

    QPointer<QAction>                    m_pComputeSourceEstimateAction;
    QString                              m_lastFwdDir;
    QString                              m_lastCovDir;
    QString                              m_lastAveDir;

    //=========================================================================================================
    // Labels dock state
    //=========================================================================================================

    QPointer<QDockWidget>                m_pLabelsDock;
    QPointer<QTreeWidget>                m_pLabelTree;
    QTreeWidgetItem*                     m_pLabelRootLh = nullptr;
    QTreeWidgetItem*                     m_pLabelRootRh = nullptr;
    QPointer<QAction>                    m_pLoadLabelAction;
    QPointer<QAction>                    m_pLoadAnnotAction;
    QPointer<QAction>                    m_pSaveLabelAction;
    QPointer<QAction>                    m_pCreateLabelAction;
    QString                              m_lastLabelDir;
};

} // namespace CORTICALSURFACEPLUGIN

#endif // MNEANALYZE_CORTICAL_SURFACE_H
