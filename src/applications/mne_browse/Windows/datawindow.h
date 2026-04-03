//=============================================================================================================
/**
 * @file     datawindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the DataWindow class.
 *
 */

#ifndef DATAWINDOW_H
#define DATAWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "mainwindow.h"
#include "../Utils/datamarker.h"
#include "ui_datawindowdock.h"
#include "../Delegates/rawdelegate.h"
#include "../Models/rawmodel.h"
#include "../Models/fiffblockreader.h"
#include "../Utils/sessionfilter.h"
#include "../Views/rawview.h"

#include <disp/viewers/channeldataview.h>
#include <disp/viewers/helpers/channelrhiview.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QByteArray>
#include <QResizeEvent>
#include <QPainter>
#include <QColor>
#include <QGesture>
#include <QLabel>
#include <QScroller>
#include <QSet>
#include <QVector>

#include <Eigen/Core>

#include <memory>
#include <limits>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow;
class RawDelegate;
struct VirtualChannelDefinition;

/**
 * DECLARE CLASS DataWindow
 *
 * @brief The DataWindow class provides the data dock window.
 */
class DataWindow : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a DataWindow dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new DataWindow becomes a window. If parent is another widget, DataWindow becomes a child window inside parent. DataWindow is deleted when its parent is deleted.
     */
    DataWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DataWindow.
     * All DataWindow's children are deleted first. The application exits if DataWindow is the main widget.
     */
    ~DataWindow();

    //=========================================================================================================
    /**
     * Initialises this window.
     */
    void init();

    //=========================================================================================================
    /**
     * Setup the model view controller of the data window
     */
    void initMVCSettings();

    //=========================================================================================================
    /**
     * Load a raw FIFF file into the ChannelDataView.
     * Only the file header is read immediately; data is demand-paged as the user scrolls.
     *
     * @param[in] path  Absolute path to the .fif file.
     * @return true on success.
     */
    bool loadFiffFile(const QString &path);

    //=========================================================================================================
    /**
     * Load raw FIFF content from memory.
     * Used by the browser/WASM file-picker path where no persistent filesystem path exists.
     *
     * @param[in] data         Raw FIFF file contents.
     * @param[in] displayName  Friendly file name shown in the UI.
     * @return true on success.
     */
    bool loadFiffBuffer(const QByteArray &data, const QString &displayName);

    //=========================================================================================================
    /**
     * Returns true when the FiffBlockReader has a file open.
     */
    bool isFiffFileLoaded() const;

    //=========================================================================================================
    /**
     * Returns the duration of the loaded file in seconds, or 0 if none loaded.
     */
    double fiffFileDurationSeconds() const;

    //=========================================================================================================
    /**
     * Returns the measurement info of the currently loaded FIFF file, or null if none loaded.
     */
    QSharedPointer<FIFFLIB::FiffInfo> fiffInfo() const;

    //=========================================================================================================
    /**
     * Returns the first sample of the currently loaded FIFF file, or 0 if none loaded.
     */
    int firstSample() const;

    //=========================================================================================================
    /**
     * Returns the last sample of the currently loaded FIFF file, or 0 if none loaded.
     */
    int lastSample() const;

    //=========================================================================================================
    /**
     * Returns the base file name of the currently loaded FIFF file, or empty string if none.
     */
    QString fiffFileName() const;

    //=========================================================================================================
    /**
     * Returns the data QTableView of this window
     */
    QTableView* getDataTableView();

    //=========================================================================================================
    /**
     * Returns the undocked data QTableView of this window
     */
    QTableView* getUndockedDataTableView();

    //=========================================================================================================
    /**
     * Returns the RawModel of this window
     */
    RawModel* getDataModel();

    //=========================================================================================================
    /**
     * Returns the RawModel of this window
     */
    RawDelegate* getDataDelegate();

    //=========================================================================================================
    /**
     * Scales the data according to scaleMap
     *
     * @param [in] scaleMap map with all channel types and their current scaling value
     */
    void scaleData(const QMap<QString,double> &scaleMap);

    //=========================================================================================================
    /**
     * Recompute SSP projectors from FiffInfo::projs and reload visible data.
     */
    void updateProjections();

    //=========================================================================================================
    /**
     * Updates the data table views
     */
    void updateDataTableViews();

    //=========================================================================================================
    /**
     * Only shows the channels defined in the QStringList selectedChannels
     *
     * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
     */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
     * Change the channel plot height in the data views to the double value heigt
     */
    void changeRowHeight(int height);

    //=========================================================================================================
    /**
     * hide all bad channels
     */
    void hideBadChannels(bool hideChannels);

    //=========================================================================================================
    /**
     * Returns the GPU-accelerated ChannelDataView (may be nullptr before loadFiffFile).
     */
    DISPLIB::ChannelDataView* getChannelDataView() { return m_pChannelDataView; }

    //=========================================================================================================
    /**
     * Returns the application-level raw browser wrapper.
     */
    RawView* getRawView() { return m_pChannelDataView; }

    //=========================================================================================================
    /**
     * Set the browser annotation spans shown in the GPU raw view.
     */
    void setAnnotations(const QVector<DISPLIB::ChannelRhiView::AnnotationSpan> &annotations);

    //=========================================================================================================
    /**
     * Enable or disable Shift-drag annotation creation in the raw browser.
     */
    void setAnnotationSelectionEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Set browser-level virtual bipolar channels appended to the raw view.
     *
     * @param[in] virtualChannels  Virtual channel definitions to apply.
     * @param[in] reloadIfOpen     True to refresh the current browser immediately.
     */
    void setVirtualChannels(const QVector<VirtualChannelDefinition> &virtualChannels,
                            bool reloadIfOpen = true);

    //=========================================================================================================
    /**
     * Apply a shared session filter to newly loaded raw blocks and offline workflows.
     *
     * @param[in] filterDefinition  Filter definition to apply. Pass a null pointer to clear filtering.
     */
    void setUserDefinedFilter(const QSharedPointer<SessionFilter>& filterDefinition);

    //=========================================================================================================
    /**
     * Clear any browser/session filter and reload the current view unfiltered.
     */
    void clearUserDefinedFilter();

    //=========================================================================================================
    /**
     * Returns the currently active session filter used by preview and offline processing.
     */
    QSharedPointer<SessionFilter> activeSessionFilter() const { return m_pUserDefinedFilter; }

private:
    struct PersistentMarker;

    //=========================================================================================================
    /**
     * Setup the tool bar of the data window.
     */
    void initToolBar();

    //=========================================================================================================
    /**
     * Setup the sample labels of the data window
     */
    void initLabels();

    //=========================================================================================================
    /**
     * Add a persistent sample marker at the requested absolute sample.
     *
     * @param[in] sample  Absolute sample index.
     */
    void addMarkerAtSample(int sample);

    //=========================================================================================================
    /**
     * Remove the marker with the requested internal id.
     *
     * @param[in] markerId  Internal marker id.
     */
    void removeMarker(int markerId);

    //=========================================================================================================
    /**
     * Remove the marker nearest to the requested sample.
     *
     * @param[in] sample  Absolute sample index near the marker to remove.
     */
    void removeMarkerNearSample(int sample);

    //=========================================================================================================
    /**
     * Remove all persistent sample markers.
     */
    void clearMarkers();

    //=========================================================================================================
    /**
     * Make the requested marker the active one.
     *
     * @param[in] markerId  Internal marker id.
     */
    void setActiveMarker(int markerId);

    //=========================================================================================================
    /**
     * Returns the active marker sample or the first visible sample when no marker exists.
     */
    int activeMarkerSample() const;

    //=========================================================================================================
    /**
     * Sync all marker widgets and labels with the current viewport and scroll position.
     */
    void syncMarkersToViewport();

    //=========================================================================================================
    /**
     * Rebuild the ruler marker summary from the current persistent markers.
     */
    void updateTimeRulerMarkers();

    //=========================================================================================================
    /**
     * Find a marker entry by its internal id.
     *
     * @param[in] markerId  Internal marker id.
     * @return Pointer to the marker entry, or nullptr when not found.
     */
    struct PersistentMarker* findMarker(int markerId);
    const struct PersistentMarker* findMarker(int markerId) const;

    //=========================================================================================================
    /**
     * Build channel-view metadata for the currently configured virtual channels.
     */
    void rebuildVirtualChannels();

    //=========================================================================================================
    /**
     * Append the currently active virtual channels to a raw data block.
     */
    Eigen::MatrixXd appendVirtualChannels(const Eigen::MatrixXd &data) const;

    //=========================================================================================================
    /**
     * Apply the currently active session filter to a loaded raw block.
     */
    Eigen::MatrixXd applyUserDefinedFilter(const Eigen::MatrixXd &data) const;

    //=========================================================================================================
    /**
     * Reinitialize the browser view and resume block loading at the requested sample.
     */
    void restartChannelView(int initialSample, bool clearAnnotations);

    //=========================================================================================================
    /**
     * Compute auto-scale values from the first loaded data window and apply to the GPU model.
     * Call once right after file open, before restartChannelView.
     */
    void computeAutoScale();

    //=========================================================================================================
    /**
     * Setup the marker of the data window
     */
    void initMarker();

    //=========================================================================================================
    /**
     * resizeEvent reimplemented virtual function to handle resize events of the data dock window
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     * keyPressEvent reimplemented virtual function to handle key press events of the data dock window
     */
    void keyPressEvent(QKeyEvent* event);

    //=========================================================================================================
    /**
     * Installed event filter.
     */
    bool eventFilter(QObject *object, QEvent *event);

    //=========================================================================================================
    /**
     * gestureEvent processes gesture events
     */
    bool gestureEvent(QGestureEvent *event);

    //=========================================================================================================
    /**
     * pinchTriggered processes pinch gesture events
     */
    bool pinchTriggered(QPinchGesture *gesture);

    std::unique_ptr<Ui::DataWindowDockWidget> ui;           /**< Pointer to the qt designer generated ui class.*/

    MainWindow*     m_pMainWindow;                  /**< pointer to the main window (parent). */

    QSettings       m_qSettings;                    /**< QSettings variable used to write or read from independent application sessions. */

    struct PersistentMarker {
        int         id = -1;                        /**< Internal marker id. */
        int         sample = 0;                     /**< Absolute sample position the marker is anchored to. */
        QColor      color;                          /**< Marker colour used for the line and label. */
        DataMarker* line = nullptr;                 /**< Draggable line widget rendered over the signal viewport. */
        QLabel*     label = nullptr;                /**< Small floating label with sample/time information. */
    };

    QVector<PersistentMarker> m_dataMarkers;        /**< All persistent sample markers in the raw browser. */
    int             m_iNextMarkerId = 1;            /**< Next internal marker id. */
    int             m_iActiveMarkerId = -1;         /**< Active marker used for event insertion. */

    RawDelegate*    m_pRawDelegate;                 /**< the QAbstractDelegate being part of the raw model/view framework of Qt. */
    RawModel*       m_pRawModel;                    /**< the QAbstractTable model being part of the model/view framework of Qt. */

    QScroller*      m_pKineticScroller;             /**< the kinetic scroller of the QTableView. */

    QStringList     m_slSelectedChannels;           /**< the currently selected channels from the selection manager window. */

    bool            m_bHideBadChannels;             /**< hide bad channels flag. */

    // ── ChannelDataView (GPU-accelerated renderer) ─────────────────────
    RawView*                 m_pChannelDataView     = nullptr; /**< Application-level raw browser view wrapper. */
    FiffBlockReader*         m_pFiffReader          = nullptr; /**< Demand-paging FIFF reader. */
    int                      m_iNextLoadSample      = 0;       /**< First sample of the next forward block to load. */
    int                      m_iCurrentScrollSample = 0;       /**< Last known scroll position (absolute sample). */
    bool                     m_bLoadingBlock        = false;   /**< Async load in progress. */
    QString                  m_sFiffFilePath;                  /**< Path of the currently open FIFF file. */

    // ── STIM event cache ───────────────────────────────────────────────
    QVector<DISPLIB::ChannelRhiView::EventMarker> m_stimEvents; /**< Accumulated STIM-channel events across loaded blocks. */
    QMap<int, QColor>                             m_eventTypeColors; /**< Per-type colour palette (built on demand). */
    QSet<quint64>                                 m_seenStimEventKeys; /**< De-duplicates events when blocks are revisited. */
    int                                           m_iStimChannel       = -1; /**< Selected trigger channel index (prefer STI 014). */
    int                                           m_iStimLastSample    = std::numeric_limits<int>::min(); /**< Last scanned sample on the trigger channel. */
    int                                           m_iStimLastValue     = 0; /**< Trigger value at m_iStimLastSample. */

    struct ResolvedVirtualChannel {
        QString                     name;
        VirtualChannelKind          kind = VirtualChannelKind::Bipolar;
        int                         primaryChannel = -1;
        QVector<int>                referenceChannels;
        QVector<double>             referenceWeights;
        DISPLIB::ChannelDisplayInfo displayInfo;
    };

    QVector<VirtualChannelDefinition>             m_virtualChannelDefinitions; /**< Requested virtual-channel definitions. */
    QVector<ResolvedVirtualChannel>               m_resolvedVirtualChannels; /**< Definitions resolved against the current FIFF header. */
    QSharedPointer<SessionFilter>                 m_pUserDefinedFilter; /**< Optional session filter applied directly in the QRHI browser path. */

    // ── Buffer sizing ──────────────────────────────────────────────────
    // Each block is 60 s.  Buffer holds kMaxBlocks = 10 blocks = 10 min.
    // Prefetch is triggered whenever the unloaded frontier is within
    // kLookaheadBlocks × kBlockSeconds ahead of the current scroll position,
    // so loading chains greedily and keeps at least ~3 min of data ahead.
    static constexpr float kBlockSeconds    = 60.f;  /**< Seconds of data per demand-load block. */
    static constexpr int   kMaxBlocks       = 10;    /**< Ring-buffer depth in blocks (10 × 60 s = 10 min). */
    static constexpr float kLookaheadBlocks = 3.f;   /**< Keep this many blocks loaded ahead of scroll. */

signals:
    //=========================================================================================================
    /**
     * scaleChannels gets called whenever the user performed a scaling gesture (pinch)
     */
    void scaleChannels(double);

    //=========================================================================================================
    /**
     * Forwarded when the user selects a time range in annotation mode.
     */
    void annotationRangeSelected(int startSample, int endSample);

    //=========================================================================================================
    /**
     * Forwarded when the user drags an annotation boundary to a new position.
     */
    void annotationBoundaryMoved(int annotationIndex, bool isStartBoundary, int newSample);

    //=========================================================================================================
    /**
     * Emitted when the user presses T to toggle the time display format.
     */
    void timeFormatToggleRequested();

protected slots:
    //=========================================================================================================
    /**
     * @brief customContextMenuRequested
     * @param pos is the position, where the right-click occurred
     */
    void customContextMenuRequested(QPoint pos);

    //=========================================================================================================
    /**
     * Called when the ChannelDataView scroll position changes.
     * Triggers demand-loading of the next block when approaching the buffer edge.
     *
     * @param[in] sample  Current left-edge sample index.
     */
    void onChannelViewScrollChanged(int sample);

    //=========================================================================================================
    /**
     * Starts the next async block load if the loaded frontier is within the lookahead
     * window of the current scroll position.  Safe to call at any time; no-op when a
     * load is already in-flight or all data has been read.
     */
    void scheduleNextLoad();

    //=========================================================================================================
    /**
     * Called when an async block has been loaded by FiffBlockReader.
     *
     * @param[in] data         channels × samples matrix.
     * @param[in] firstSample  Absolute sample index of column 0.
     */
    void onBlockLoaded(const Eigen::MatrixXd &data, int firstSample);

    //=========================================================================================================
    /**
     * Set the range sample labels of the data window
     */
    void setRangeSampleLabels();

    //=========================================================================================================
    /**
     * Update all marker labels in the raw browser.
     */
    void setMarkerSampleLabel();

    //=========================================================================================================
    /**
     * Moves the active marker to the requested absolute sample in the visible raw viewport.
     * If no marker exists yet, a new marker is created.
     *
     * @param[in] sample  Absolute sample index to place the marker at.
     */
    void moveMarkerToSample(int sample);

    //=========================================================================================================
    /**
     * Updates the marker position
     */
    void updateMarkerPosition();

    //=========================================================================================================
    /**
     * Highlights the current selected channels in the 2D plot of selection manager
     */
    void highlightChannelsInSelectionManager();

private:
    //=========================================================================================================
    /**
     * Returns the actual signal viewport of the QRHI browser mapped into DataWindow coordinates.
     */
    QRect markerViewportRect() const;
};

} // NAMESPACE MNEBROWSE

#endif // DATAWINDOW_H
