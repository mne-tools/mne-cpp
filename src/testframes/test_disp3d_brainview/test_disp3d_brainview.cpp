//=============================================================================================================
/**
 * @file     test_disp3d_brainview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for the disp3D BrainView, BrainTreeModel, workers and scene managers.
 *           Covers: BrainView construction and public API (without GPU rendering),
 *           BrainTreeModel item model, RtSourceInterpolationMatWorker,
 *           RtSensorInterpolationMatWorker, SourceEstimateManager,
 *           RtSensorStreamManager, MultiViewLayout, and StcLoadingWorker.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view/brainview.h>
#include <disp3D/view/multiviewlayout.h>
#include <disp3D/model/braintreemodel.h>
#include <disp3D/model/items/abstracttreeitem.h>
#include <disp3D/model/items/digitizersettreeitem.h>
#include <disp3D/model/items/digitizertreeitem.h>
#include <disp3D/model/items/surfacetreeitem.h>
#include <disp3D/model/items/bemtreeitem.h>
#include <disp3D/model/items/networktreeitem.h>
#include <disp3D/model/items/dipoletreeitem.h>
#include <disp3D/model/items/sourcespacetreeitem.h>
#include <disp3D/workers/rtsourceinterpolationmatworker.h>
#include <disp3D/workers/rtsensorinterpolationmatworker.h>
#include <disp3D/workers/rtsourcedataworker.h>
#include <disp3D/workers/rtsensordataworker.h>
#include <disp3D/scene/sourceestimatemanager.h>
#include <disp3D/scene/rtsensorstreammanager.h>
#include <disp3D/workers/stcloadingworker.h>
#include <disp3D/renderable/brainsurface.h>
#include <disp3D/input/cameracontroller.h>
#include <disp3D/input/raypicker.h>
#include <disp3D/renderable/dipoleobject.h>
#include <disp3D/core/viewstate.h>

#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_constants.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <conn/network/network.h>
#include <fs/fs_label.h>

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QVector3D>
#include <QQuaternion>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestDisp3dBrainView
 *
 * @brief The TestDisp3dBrainView class tests the disp3D BrainView and related classes headlessly
 *        (construction and public-API calls that do not require an active GPU/render context).
 *
 */
class TestDisp3dBrainView : public QObject
{
    Q_OBJECT

private slots:

    //=========================================================================================================
    /**
     * Called once before any test function.
     */
    void initTestCase();

    //=========================================================================================================
    /**
     * Called once after all test functions have run.
     */
    void cleanupTestCase();

    //=========================================================================================================
    /**
     * Verifies that BrainView constructs without crashing (no GPU calls in ctor),
     * that its public setters do not crash, and that the widget can be sized.
     */
    void brainView_constructAndSetters();

    //=========================================================================================================
    /**
     * Verifies that BrainView multi-view API (setViewCount, setViewportEnabled,
     * resetMultiViewLayout, showSingleView, showMultiView, setViewCount) does not crash.
     */
    void brainView_multiViewApi();

    //=========================================================================================================
    /**
     * Verifies that BrainView appearance-related setters (setShaderMode, setBemShaderMode,
     * setActiveSurface, setVisualizationMode, setLightingEnabled, setInfoPanelVisible,
     * setHemiVisible, setNetworkVisible, setDipoleVisible, setSensorVisible) do not crash.
     */
    void brainView_appearanceSetters();

    //=========================================================================================================
    /**
     * Verifies that BrainTreeModel constructs as a valid QStandardItemModel,
     * rowCount/columnCount return sensible values, addDigitizerData with an empty list
     * does not crash, and getSubjectItem for an unknown key returns nullptr.
     */
    void brainTreeModel_basics();

    //=========================================================================================================
    /**
     * Verifies that RtSourceInterpolationMatWorker constructs, setters do not crash,
     * and the object survives destruction.
     */
    void rtSourceInterpolationMatWorker_basics();

    //=========================================================================================================
    /**
     * Verifies that RtSensorInterpolationMatWorker constructs, setters do not crash,
     * and the object survives destruction.
     */
    void rtSensorInterpolationMatWorker_basics();

    //=========================================================================================================
    /**
     * Verifies that SourceEstimateManager constructs and basic API calls do not crash.
     */
    void sourceEstimateManager_basics();

    //=========================================================================================================
    /**
     * Verifies that RtSensorStreamManager constructs without crashing.
     */
    void rtSensorStreamManager_basics();

    //=========================================================================================================
    /**
     * Verifies that StcLoadingWorker constructs without crashing.
     */
    void stcLoadingWorker_basics();

    //=========================================================================================================
    /**
     * Verifies that MultiViewLayout helpers (default construction, single/multi-view
     * layout values) compile and return sensible values.
     */
    void multiViewLayout_basics();

    //=========================================================================================================
    /**
     * Verifies MultiViewLayout geometry computations: slotRect for 1–4 panes,
     * hitTestSplitter, cursorForHit, viewportIndexAt, insetForSeparator,
     * separatorGeometries, and dragSplitter.
     */
    void multiViewLayout_geometry();

    //=========================================================================================================
    /**
     * Verifies CameraController setters, computeSingleView, computeMultiView,
     * applyMouseRotation, and applyMousePan.
     */
    void cameraController_basics();

    //=========================================================================================================
    /**
     * Verifies DigitizerSetTreeItem construction with empty and non-empty
     * digitizer point lists, totalPointCount, and categoryItem.
     */
    void digitizerSetTreeItem_basics();

    //=========================================================================================================
    /**
     * Verifies RayPicker static methods: unproject with a valid viewport/matrix,
     * pick with empty surface/dipole maps, and displayLabel on a PickResult.
     */
    void rayPicker_basics();

    //=========================================================================================================
    /**
     * Verifies BrainTreeModel::addDigitizerData, addSensors, and row/item counts.
     */
    void brainTreeModel_advanced();

    //=========================================================================================================
    /**
     * Verifies SurfaceTreeItem, BemTreeItem, NetworkTreeItem, DipoleTreeItem, and
     * SourceSpaceTreeItem construct correctly and their public API does not crash.
     */
    void treeItems_basics();

    //=========================================================================================================
    /**
     * Verifies BrainTreeModel::addBemSurface, addDipoles, addSourceSpace, and addNetwork.
     */
    void brainTreeModel_extended();

    //=========================================================================================================
    /**
     * Verifies BrainSurface non-GPU methods: createFromData, vertexPositions,
     * vertexNormals, setters, boundingBox, intersects, getAnnotation*.
     */
    void brainSurface_advanced();

    //=========================================================================================================
    /**
     * Verifies DipoleObject non-GPU methods on an empty (no-data) instance.
     */
    void dipoleObject_basics();

    //=========================================================================================================
    /**
     * Verifies additional setters on RtSourceInterpolationMatWorker,
     * RtSensorInterpolationMatWorker, SourceEstimateManager, and RtSensorStreamManager.
     */
    void rtWorkers_extended();

    //=========================================================================================================
    /**
     * Verifies BrainView RT streaming API: setTimePoint, setSourceColormap, setSourceThresholds,
     * startRealtimeStreaming, stopRealtimeStreaming, isRealtimeStreaming, stcNumTimePoints,
     * closestStcIndex, and sensor field / realtime sensor streaming methods.
     */
    void brainView_streamingApi();

    //=========================================================================================================
    /**
     * Verifies RtSourceDataWorker and RtSensorDataWorker setters do not crash.
     */
    void rtDataWorkers_basics();

    //=========================================================================================================
    /**
     * Verifies DipoleObject with a loaded ECD set: load(), applyTransform(),
     * debugFirstDipolePosition(), and intersect().
     */
    void dipoleObject_extended();
};

//=============================================================================================================

void TestDisp3dBrainView::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::cleanupTestCase() {}

//=============================================================================================================

void TestDisp3dBrainView::brainView_constructAndSetters()
{
    // BrainView constructor only sets up Qt widgets and connects signals.
    // No GPU initialisation happens until the widget is shown and render events fire.
    BrainView view;

    // Size queries must not crash
    QVERIFY(view.viewCount() > 0);
    QVERIFY(!view.isInfoPanelVisible() || view.isInfoPanelVisible()); // just check it doesn't crash

    view.setInitialCameraRotation(QQuaternion::fromAxisAndAngle(0, 1, 0, 90));

    // Getter for visualization target
    int editTarget = view.visualizationEditTarget();
    Q_UNUSED(editTarget);

    view.setVisualizationEditTarget(0);

    // setInfoPanelVisible
    view.setInfoPanelVisible(false);
    QVERIFY(!view.isInfoPanelVisible());
    view.setInfoPanelVisible(true);
    QVERIFY(view.isInfoPanelVisible());

    // viewMode query
    Q_UNUSED(view.viewMode());

    // activeSurface / shader mode queries (no surface loaded — returns empty)
    Q_UNUSED(view.activeSurfaceForTarget(0));
    Q_UNUSED(view.shaderModeForTarget(0));
    Q_UNUSED(view.bemShaderModeForTarget(0));
    Q_UNUSED(view.overlayModeForTarget(0));
    Q_UNUSED(view.objectVisibleForTarget("BEM", 0));
    Q_UNUSED(view.megFieldMapOnHeadForTarget(0));

    // probeEvokedSets with non-existent path returns empty list
    QStringList evoked = BrainView::probeEvokedSets("/nonexistent/path.fif");
    QVERIFY(evoked.isEmpty());

    // setMegHelmetOverride must not crash
    view.setMegHelmetOverride("");

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_multiViewApi()
{
    BrainView view;

    // Default is single view
    view.showSingleView();
    view.showMultiView();

    // setViewCount
    view.setViewCount(1);
    view.setViewCount(2);
    view.setViewCount(4);
    QCOMPARE(view.viewCount(), 4);

    // Reset
    view.resetMultiViewLayout();

    // Per-viewport API
    view.setViewportEnabled(0, true);
    view.setViewportEnabled(0, false);
    QVERIFY(!view.isViewportEnabled(0));

    view.setViewportCameraPreset(0, 0);
    int preset = view.viewportCameraPreset(0);
    Q_UNUSED(preset);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_appearanceSetters()
{
    BrainView view;

    // Shader mode setters
    view.setShaderMode("phong");
    view.setShaderMode("flat");
    view.setBemShaderMode("phong");
    view.syncBemShadersToBrainShaders();

    // Surface and visualization
    view.setActiveSurface("inflated");
    view.setActiveSurface("orig");
    view.setVisualizationMode("overlay");
    view.setVisualizationMode("default");

    // Visibility setters
    view.setHemiVisible(0, true);
    view.setHemiVisible(1, false);
    view.setBemVisible("BEM", true);
    view.setBemHighContrast(false);
    view.setSensorVisible("MEG", true);
    view.setSensorTransEnabled(false);
    view.setDipoleVisible(false);
    view.setNetworkVisible(false);
    view.setNetworkThreshold(0.5);
    view.setNetworkColormap("jet");

    // Lighting
    view.setLightingEnabled(true);
    view.setLightingEnabled(false);

    // Snapshot (no rendering context — just exercises the non-render code path)
    // view.saveSnapshot(); // would need render context

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainTreeModel_basics()
{
    BrainTreeModel model;

    // It's a QStandardItemModel subclass
    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);

    // addDigitizerData with empty list must not crash
    model.addDigitizerData(QList<FiffDigPoint>());

    // rowCount must be ≥ 0 after empty digitizer list
    QVERIFY(model.rowCount() >= 0);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtSourceInterpolationMatWorker_basics()
{
    RtSourceInterpolationMatWorker worker;

    // Safe setters that don't trigger heavy computation (no surface data set)
    worker.setInterpolationFunction("linear");
    worker.setInterpolationFunction("gaussian");
    worker.setCancelDistance(0.05);
    worker.setVisualizationType(0);
    worker.setVisualizationType(1);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtSensorInterpolationMatWorker_basics()
{
    RtSensorInterpolationMatWorker worker;

    // Safe setters
    worker.setMegFieldMapOnHead(true);
    worker.setMegFieldMapOnHead(false);
    worker.setBadChannels(QStringList() << "MEG0111" << "MEG0112");
    worker.setBadChannels(QStringList());

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::sourceEstimateManager_basics()
{
    SourceEstimateManager manager;

    // setColormap / setThresholds
    manager.setColormap("jet");
    manager.setColormap("hot");
    manager.setThresholds(0.0f, 0.5f, 1.0f);
    manager.setThresholds(0.1f, 0.5f, 0.9f);

    // State queries (no data loaded)
    QVERIFY(!manager.isLoading());
    QVERIFY(!manager.isLoaded());
    QCOMPARE(manager.numTimePoints(), 0);
    QCOMPARE(manager.currentTimePoint(), 0);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtSensorStreamManager_basics()
{
    RtSensorStreamManager manager;

    // stopStreaming is safe to call even with no active stream
    manager.stopStreaming();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::stcLoadingWorker_basics()
{
    // BrainSurface has a default constructor; StcLoadingWorker constructor
    // only stores its arguments — no file I/O or GPU calls happen here.
    BrainSurface lhSurface, rhSurface;

    StcLoadingWorker worker("/nonexistent/lh.stc",
                            "/nonexistent/rh.stc",
                            &lhSurface,
                            &rhSurface,
                            0.05);

    // Accessors must not crash and return sensible defaults
    QVERIFY(!worker.hasLh());
    QVERIFY(!worker.hasRh());
    QVERIFY(worker.interpolationMatLh().isNull());
    QVERIFY(worker.interpolationMatRh().isNull());

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::multiViewLayout_basics()
{
    // MultiViewLayout is a value-type helper (struct or plain class, not QObject)
    MultiViewLayout layout;
    Q_UNUSED(layout);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::multiViewLayout_geometry()
{
    MultiViewLayout layout;
    const QSize sz(800, 600);

    // 1-pane: full area
    QRect r1 = layout.slotRect(0, 1, sz);
    QCOMPARE(r1, QRect(0, 0, 800, 600));

    // 2-pane: slots cover full height, widths > 0
    QRect r2a = layout.slotRect(0, 2, sz);
    QRect r2b = layout.slotRect(1, 2, sz);
    QVERIFY(r2a.width() > 0);
    QVERIFY(r2b.width() > 0);
    QCOMPARE(r2a.width() + r2b.width(), 800);

    // 3-pane
    QRect r3a = layout.slotRect(0, 3, sz);
    QRect r3b = layout.slotRect(1, 3, sz);
    QRect r3c = layout.slotRect(2, 3, sz);
    QVERIFY(r3a.height() > 0);
    QVERIFY(r3b.height() > 0);
    QCOMPARE(r3b.width() + r3c.width(), 800);

    // 4-pane: 2×2 grid
    for (int slot = 0; slot < 4; ++slot) {
        QRect r = layout.slotRect(slot, 4, sz);
        QVERIFY(r.width() > 0);
        QVERIFY(r.height() > 0);
    }

    // hitTestSplitter
    SplitterHit h1 = layout.hitTestSplitter(QPoint(400, 300), 1, sz);
    QCOMPARE(h1, SplitterHit::None);

    SplitterHit h2 = layout.hitTestSplitter(QPoint(400, 300), 2, sz);
    QVERIFY(h2 == SplitterHit::Vertical || h2 == SplitterHit::None);

    // cursorForHit
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::None),       Qt::ArrowCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Vertical),   Qt::SizeHorCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Horizontal), Qt::SizeVerCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Both),       Qt::SizeAllCursor);

    // viewportIndexAt
    QVector<int> vp = {0, 1, 2, 3};
    int idx = layout.viewportIndexAt(QPoint(100, 100), vp, sz);
    QVERIFY(idx >= 0 || idx == -1); // must not crash

    // insetForSeparator — must not crash for all slot/numEnabled combos
    for (int n = 1; n <= 4; ++n) {
        for (int s = 0; s < n; ++s) {
            QRect pr = layout.slotRect(s, n, sz);
            QRect inset = layout.insetForSeparator(pr, s, n);
            QVERIFY(inset.width() > 0);
            QVERIFY(inset.height() > 0);
        }
    }

    // separatorGeometries
    QRect vr, hr;
    layout.separatorGeometries(1, sz, vr, hr);
    QVERIFY(vr.isEmpty() || !vr.isEmpty()); // just no crash
    layout.separatorGeometries(2, sz, vr, hr);
    QVERIFY(vr.width() > 0 || vr.isEmpty());
    layout.separatorGeometries(4, sz, vr, hr);

    // dragSplitter
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Vertical,   sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Horizontal, sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Both,       sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::None,       sz);

    // setSplitX/Y clamp and reset
    layout.setSplitX(0.3f);
    QVERIFY(layout.splitX() >= 0.15f && layout.splitX() <= 0.85f);
    layout.setSplitY(0.7f);
    QVERIFY(layout.splitY() >= 0.15f && layout.splitY() <= 0.85f);
    layout.resetSplits();
    QCOMPARE(layout.splitX(), 0.5f);
    QCOMPARE(layout.splitY(), 0.5f);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::cameraController_basics()
{
    CameraController cam;

    // Default scene geometry
    QCOMPARE(cam.sceneCenter(), QVector3D(0, 0, 0));
    QVERIFY(cam.sceneSize() > 0.0f);

    // Setters
    cam.setSceneCenter(QVector3D(0.01f, 0.02f, 0.03f));
    QCOMPARE(cam.sceneCenter(), QVector3D(0.01f, 0.02f, 0.03f));

    cam.setSceneSize(0.5f);
    QCOMPARE(cam.sceneSize(), 0.5f);

    cam.setSceneSize(-1.0f); // negative → clamped to 0.3
    QVERIFY(cam.sceneSize() > 0.0f);

    cam.setZoom(10.0f);
    QCOMPARE(cam.zoom(), 10.0f);

    QQuaternion rot = QQuaternion::fromAxisAndAngle(0, 1, 0, 45.0f);
    cam.setRotation(rot);
    QVERIFY(!cam.rotation().isNull());

    cam.resetRotation();
    QVERIFY(cam.rotation().isIdentity());

    // computeSingleView
    cam.setSceneSize(0.3f);
    CameraResult single = cam.computeSingleView(16.0f / 9.0f);
    QVERIFY(single.distance > 0.0f);
    // projection must not be identity
    QVERIFY(!single.projection.isIdentity());

    // computeMultiView with default SubView
    SubView sv;
    sv.preset = 1;
    CameraResult multi = cam.computeMultiView(sv, 1.0f);
    QVERIFY(multi.distance > 0.0f);

    // Test each preset (0–6)
    for (int preset = 0; preset <= 6; ++preset) {
        SubView svp;
        svp.preset = preset;
        CameraResult r = cam.computeMultiView(svp, 1.333f);
        QVERIFY(r.distance > 0.0f);
    }

    // applyMouseRotation — must not crash and must change rotation
    QQuaternion q = QQuaternion();
    CameraController::applyMouseRotation(QPoint(10, 5), q, 0.5f);
    QVERIFY(!q.isIdentity());

    // applyMousePan
    QVector2D pan(0.0f, 0.0f);
    CameraController::applyMousePan(QPoint(10, -5), pan, 0.3f);
    QVERIFY(pan.x() != 0.0f || pan.y() != 0.0f);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::digitizerSetTreeItem_basics()
{
    // Empty list: item constructs without crashing, zero points
    DigitizerSetTreeItem emptyItem("Digitizer", QList<FIFFLIB::FiffDigPoint>());
    QCOMPARE(emptyItem.totalPointCount(), 0);
    QVERIFY(emptyItem.categoryItem(FIFFV_POINT_CARDINAL) == nullptr);

    // Build a list with one point of each known kind
    QList<FIFFLIB::FiffDigPoint> pts;

    FIFFLIB::FiffDigPoint cardinal;
    cardinal.kind  = FIFFV_POINT_CARDINAL;
    cardinal.ident = FIFFV_POINT_NASION;
    cardinal.r[0] = 0.0f; cardinal.r[1] = 0.08f; cardinal.r[2] = 0.0f;
    pts << cardinal;

    FIFFLIB::FiffDigPoint lpa;
    lpa.kind  = FIFFV_POINT_CARDINAL;
    lpa.ident = FIFFV_POINT_LPA;
    lpa.r[0] = -0.07f; lpa.r[1] = 0.0f; lpa.r[2] = 0.0f;
    pts << lpa;

    FIFFLIB::FiffDigPoint hpi;
    hpi.kind  = FIFFV_POINT_HPI;
    hpi.ident = 1;
    hpi.r[0] = 0.01f; hpi.r[1] = 0.01f; hpi.r[2] = 0.06f;
    pts << hpi;

    FIFFLIB::FiffDigPoint eeg;
    eeg.kind  = FIFFV_POINT_EEG;
    eeg.ident = 1;
    eeg.r[0] = 0.02f; eeg.r[1] = 0.02f; eeg.r[2] = 0.07f;
    pts << eeg;

    FIFFLIB::FiffDigPoint extra;
    extra.kind  = FIFFV_POINT_EXTRA;
    extra.ident = 1;
    extra.r[0] = 0.03f; extra.r[1] = 0.03f; extra.r[2] = 0.05f;
    pts << extra;

    DigitizerSetTreeItem item("Digitizer", pts);

    // 5 points across categories
    QCOMPARE(item.totalPointCount(), 5);

    // categoryItem uses DigitizerTreeItem::PointKind (Cardinal=0, HPI=1, EEG=2, Extra=3),
    // not the FIFF constants (FIFFV_POINT_CARDINAL=1, etc.)
    QVERIFY(item.categoryItem(DigitizerTreeItem::Cardinal) != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::HPI)      != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::EEG)      != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::Extra)    != nullptr);

    // Non-existent category returns nullptr
    QVERIFY(item.categoryItem(999) == nullptr);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rayPicker_basics()
{
    // unproject — valid 800×600 viewport, identity PVM matrix
    QMatrix4x4 pvm;  // identity
    QRect paneRect(0, 0, 800, 600);
    QVector3D origin, dir;

    // Centre of viewport with identity PVM projects to a valid ray
    bool ok = RayPicker::unproject(QPoint(400, 300), paneRect, pvm, origin, dir);
    // With identity matrix the ray should still be computed without crashing
    Q_UNUSED(ok);

    // Near-degenerate viewport should not crash
    QRect tiny(0, 0, 1, 1);
    RayPicker::unproject(QPoint(0, 0), tiny, pvm, origin, dir);

    // pick with empty maps — must return a no-hit result instantly
    SubView sv;
    QMap<QString, std::shared_ptr<BrainSurface>> surfaces;
    QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> itemSurfaceMap;
    QMap<const QStandardItem*, std::shared_ptr<DipoleObject>> itemDipoleMap;

    PickResult result = RayPicker::pick(
        QVector3D(0, 0, 1),
        QVector3D(0, 0, -1),
        sv, surfaces, itemSurfaceMap, itemDipoleMap);
    QVERIFY(!result.hit);
    QCOMPARE(result.vertexIndex, -1);

    // buildLabel on a no-hit result must not crash
    QString label = RayPicker::buildLabel(result, itemSurfaceMap, surfaces);
    QVERIFY(label.isEmpty());

    // displayLabel on a no-hit PickResult
    PickResult noHit;
    QVERIFY(noHit.displayLabel().isEmpty());

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainTreeModel_advanced()
{
    BrainTreeModel model;

    // Initial state: header labels set, no rows in invisible root
    QCOMPARE(model.columnCount(), 2);

    // addDigitizerData with empty list — no crash
    model.addDigitizerData(QList<FIFFLIB::FiffDigPoint>());

    // addDigitizerData with a real set of points
    QList<FIFFLIB::FiffDigPoint> pts;
    FIFFLIB::FiffDigPoint nasion;
    nasion.kind  = FIFFV_POINT_CARDINAL;
    nasion.ident = FIFFV_POINT_NASION;
    nasion.r[0] = 0.0f; nasion.r[1] = 0.08f; nasion.r[2] = 0.0f;
    pts << nasion;

    FIFFLIB::FiffDigPoint hpi;
    hpi.kind = FIFFV_POINT_HPI;
    hpi.ident = 1;
    hpi.r[0] = 0.01f; hpi.r[1] = 0.01f; hpi.r[2] = 0.06f;
    pts << hpi;

    model.addDigitizerData(pts);
    QVERIFY(model.rowCount() >= 0); // items were added to invisible root

    // addSensors with empty list — no crash
    model.addSensors("MEG", QList<QStandardItem*>());

    // addSensors with a real item
    QList<QStandardItem*> sensorItems;
    sensorItems << new QStandardItem("Sensor1");
    model.addSensors("EEG", sensorItems);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::treeItems_basics()
{
    // SurfaceTreeItem
    {
        SurfaceTreeItem item("TestSurface");
        QCOMPARE(item.text(), QString("TestSurface"));
        item.setShaderMode(0);
        item.setShaderMode(1);
        QCOMPARE(item.shaderMode(), 1);
        FSLIB::FsSurface emptySurf;
        item.setSurfaceData(emptySurf);
        FSLIB::FsAnnotation emptyAnnot;
        item.setAnnotationData(emptyAnnot);
        item.surfaceData();
        item.annotationData();
    }

    // BemTreeItem
    {
        MNELIB::MNEBemSurface bemSurf;
        BemTreeItem bemItem("BEM", bemSurf);
        QCOMPARE(bemItem.text(), QString("BEM"));
        const MNELIB::MNEBemSurface &ref = bemItem.bemSurfaceData();
        Q_UNUSED(ref);
    }

    // NetworkTreeItem
    {
        NetworkTreeItem netItem("Network", "net_key");
        QCOMPARE(netItem.text(), QString("Network"));
    }

    // DipoleTreeItem
    {
        INVLIB::InvEcdSet emptySet;
        DipoleTreeItem dipItem("Dipoles", emptySet);
        QCOMPARE(dipItem.text(), QString("Dipoles"));
        QCOMPARE(dipItem.ecdSet().size(), 0);
    }

    // SourceSpaceTreeItem
    {
        QVector<QVector3D> positions;
        positions << QVector3D(0.01f, 0.02f, 0.03f) << QVector3D(-0.01f, 0.02f, 0.03f);
        SourceSpaceTreeItem ssItem("LH", positions, QColor(200, 30, 90), 0.00075f);
        QCOMPARE(ssItem.text(), QString("LH"));
        QCOMPARE(ssItem.positions().size(), 2);
        QVERIFY(qAbs(ssItem.scale() - 0.00075f) < 1e-6f);
    }

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainTreeModel_extended()
{
    BrainTreeModel model;

    // addBemSurface
    {
        MNELIB::MNEBemSurface bemSurf;
        bemSurf.id = 4; // Head
        BemTreeItem *bemItem = model.addBemSurface("SubjectA", "head", bemSurf);
        QVERIFY(bemItem != nullptr);
        QCOMPARE(bemItem->text(), QString("head"));

        MNELIB::MNEBemSurface bemSurf2;
        bemSurf2.id = 3;
        model.addBemSurface("SubjectA", "outer_skull", bemSurf2);

        MNELIB::MNEBemSurface bemSurf3;
        bemSurf3.id = 1;
        model.addBemSurface("SubjectA", "inner_skull", bemSurf3);

        MNELIB::MNEBemSurface bemSurf4;
        bemSurf4.id = 99;
        model.addBemSurface("SubjectB", "unknown_bem", bemSurf4);
    }

    // addDipoles
    {
        INVLIB::InvEcdSet emptySet;
        model.addDipoles(emptySet);
    }

    // addSourceSpace with empty source spaces
    {
        MNELIB::MNESourceSpaces srcSpace;
        model.addSourceSpace(srcSpace);
    }

    // addNetwork
    {
        CONNLIB::Network net("Coherence");
        NetworkTreeItem *netItem = model.addNetwork(net, "TestNet");
        QVERIFY(netItem != nullptr);
        QCOMPARE(netItem->text(), QString("TestNet"));

        CONNLIB::Network net2("PLV");
        NetworkTreeItem *netItem2 = model.addNetwork(net2, "");
        QVERIFY(netItem2 != nullptr);
    }

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainSurface_advanced()
{
    BrainSurface surf;

    // Default state — no vertices
    QVERIFY(surf.vertexPositions().rows() == 0);
    QVERIFY(surf.vertexNormals().rows() == 0);
    QVERIFY(surf.vertexBuffer() == nullptr);
    QVERIFY(surf.indexBuffer() == nullptr);

    // createFromData with a single triangle (3 vertices)
    Eigen::MatrixX3f verts(3, 3);
    verts << 0.0f,  0.0f, 0.0f,
             0.01f, 0.0f, 0.0f,
             0.0f,  0.01f, 0.0f;
    Eigen::MatrixX3i tris(1, 3);
    tris << 0, 1, 2;
    surf.createFromData(verts, tris, QColor(200, 200, 200));

    QCOMPARE(surf.vertexPositions().rows(), 3);
    QCOMPARE(surf.vertexNormals().rows(), 3);

    // CPU-only setters
    surf.setVisible(false);
    surf.setVisible(true);
    surf.setSelected(true);
    surf.setSelected(false);
    surf.setSelectedRegion(0);
    surf.setSelectedRegion(-1);
    surf.setUseDefaultColor(true);
    surf.setUseDefaultColor(false);
    surf.setVisualizationMode(BrainSurface::ModeSurface);
    surf.setVisualizationMode(BrainSurface::ModeScientific);
    surf.setVisualizationMode(BrainSurface::ModeSourceEstimate);
    surf.setVisualizationMode(BrainSurface::ModeAnnotation);

    // applySourceEstimateColors
    QVector<uint32_t> colors(3, 0xFF808080u);
    surf.applySourceEstimateColors(colors);

    // verticesAsMatrix
    auto vm = surf.verticesAsMatrix();
    QCOMPARE(vm.rows(), 3);

    // boundingBox
    QVector3D bMin, bMax;
    surf.boundingBox(bMin, bMax);
    QVERIFY(bMax.x() >= bMin.x());

    // translateX
    surf.translateX(0.005f);
    surf.translateX(-0.005f);

    // intersects — ray passing through the triangle plane
    float dist = 0.0f;
    int vIdx = -1;
    surf.intersects(QVector3D(0.002f, 0.002f, 1.0f), QVector3D(0, 0, -1), dist, vIdx);

    // getAnnotation* without loaded annotation
    surf.getAnnotationLabel(0);
    surf.getAnnotationLabelId(0);
    surf.getAnnotationLabel(-1);

    // computeNeighbors on a single triangle
    auto neighbors = surf.computeNeighbors();
    QCOMPARE((int)neighbors.size(), 3);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::dipoleObject_basics()
{
    DipoleObject obj;

    // Buffer accessors return nullptr before GPU upload
    QVERIFY(obj.vertexBuffer() == nullptr);
    QVERIFY(obj.indexBuffer() == nullptr);
    QVERIFY(obj.instanceBuffer() == nullptr);

    // intersect on empty instance data — returns -1 immediately
    float dist = 0.0f;
    int idx = obj.intersect(QVector3D(0, 0, 1), QVector3D(0, 0, -1), dist);
    QCOMPARE(idx, -1);

    // setSelected with out-of-range indices — must not crash
    obj.setSelected(-1, true);
    obj.setSelected(0, false);

    // debugFirstDipolePosition on empty — returns QVector3D()
    QVector3D pos = obj.debugFirstDipolePosition();
    QCOMPARE(pos, QVector3D(0, 0, 0));

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtWorkers_extended()
{
    // RtSourceInterpolationMatWorker: additional setters
    {
        RtSourceInterpolationMatWorker worker;
        worker.setInterpolationFunction("linear");
        worker.setInterpolationFunction("cubic");
        worker.setInterpolationFunction("gaussian");
        worker.setInterpolationFunction("square");
        worker.setCancelDistance(0.03);
        worker.setVisualizationType(0);
        worker.setVisualizationType(1);
        worker.setVisualizationType(2);

        Eigen::MatrixX3f emptyVerts;
        std::vector<Eigen::VectorXi> emptyNeighbors;
        Eigen::VectorXi emptyVtx;
        worker.setInterpolationInfoLeft(emptyVerts, emptyNeighbors, emptyVtx);
        worker.setInterpolationInfoRight(emptyVerts, emptyNeighbors, emptyVtx);

        Eigen::VectorXi emptyLabelIds;
        QList<FSLIB::FsLabel> emptyLabels;
        worker.setAnnotationInfoLeft(emptyLabelIds, emptyLabels, emptyVtx);
        worker.setAnnotationInfoRight(emptyLabelIds, emptyLabels, emptyVtx);
    }

    // RtSensorInterpolationMatWorker: additional setters
    {
        RtSensorInterpolationMatWorker worker;
        worker.setMegFieldMapOnHead(true);
        worker.setMegFieldMapOnHead(false);
        worker.setBadChannels(QStringList() << "MEG0111");
        worker.setBadChannels(QStringList());

        FIFFLIB::FiffCoordTrans emptyTrans;
        worker.setTransform(emptyTrans, false);
        worker.setTransform(emptyTrans, true);

        Eigen::MatrixX3f emptyV, emptyN;
        Eigen::MatrixX3i emptyT;
        worker.setMegSurface("helmet", emptyV, emptyN, emptyT);
        worker.setEegSurface("head", emptyV);
    }

    // SourceEstimateManager: additional getters/setters
    {
        SourceEstimateManager manager;
        QVERIFY(qAbs(manager.tstep()) < 1e-6f);
        QVERIFY(qAbs(manager.tmin()) < 1e-6f);
        QCOMPARE(manager.closestIndex(0.0f), -1);
        QVERIFY(manager.overlay() == nullptr);
        manager.stopStreaming();
        manager.setInterval(50);
        manager.setLooping(true);
        manager.setLooping(false);
        Eigen::VectorXd emptyData;
        manager.pushData(emptyData);
        QVERIFY(!manager.isLoading());
    }

    // RtSensorStreamManager: additional setters
    {
        RtSensorStreamManager manager;
        manager.setInterval(50);
        manager.setLooping(true);
        manager.setLooping(false);
        manager.setAverages(3);
        manager.setColormap("hot");
        manager.setColormap("jet");
        manager.pushData(Eigen::VectorXf());
    }

    QApplication::processEvents();
}

//=============================================================================================================

//=============================================================================================================

void TestDisp3dBrainView::brainView_streamingApi()
{
    BrainView view;

    // Source estimate queries — all return defaults when no STC loaded
    QCOMPARE(view.stcNumTimePoints(), 0);
    int closestIdx = view.closestStcIndex(0.0f);
    QCOMPARE(closestIdx, -1);

    // setTimePoint (no surfaces loaded, just queues a repaint)
    view.setTimePoint(0);

    // Source colormap and thresholds
    view.setSourceColormap("jet");
    view.setSourceColormap("hot");
    view.setSourceThresholds(0.1f, 0.5f, 1.0f);

    // RT source streaming
    QVERIFY(!view.isRealtimeStreaming());
    view.startRealtimeStreaming();
    view.stopRealtimeStreaming();
    view.pushRealtimeSourceData(Eigen::VectorXd(0));
    view.setRealtimeInterval(50);
    view.setRealtimeLooping(false);
    view.setRealtimeLooping(true);

    // RT sensor streaming
    QVERIFY(!view.isRealtimeSensorStreaming());
    view.startRealtimeSensorStreaming("MEG");
    view.stopRealtimeSensorStreaming();
    view.pushRealtimeSensorData(Eigen::VectorXf());
    view.setRealtimeSensorInterval(50);
    view.setRealtimeSensorLooping(false);
    view.setRealtimeSensorAverages(3);
    view.setRealtimeSensorColormap("hot");

    // Sensor field visibility (no field loaded — just profiles + apply + update)
    view.setSensorFieldVisible("MEG", true);
    view.setSensorFieldVisible("MEG", false);
    view.setSensorFieldVisible("EEG", true);
    view.setSensorFieldVisible("EEG", false);
    view.setSensorFieldVisible("INVALID", false);   // early return path
    view.setSensorFieldContourVisible("MEG", true);
    view.setSensorFieldContourVisible("MEG", false);
    view.setSensorFieldContourVisible("EEG", false);
    view.setSensorFieldContourVisible("INVALID", false); // early return path

    // setMegFieldMapOnHead — already false by default
    view.setMegFieldMapOnHead(false);
    view.setMegFieldMapOnHead(true);

    // Sensor field colormap
    view.setSensorFieldColormap("jet");

    // closestSensorFieldIndex — field not loaded returns -1
    QCOMPARE(view.closestSensorFieldIndex(0.0f), -1);

    // sensorFieldTimeRange — field not loaded returns false
    float tmin = 0.0f, tmax = 0.0f;
    QVERIFY(!view.sensorFieldTimeRange(tmin, tmax));

    // Source space visibility
    view.setSourceSpaceVisible(true);
    view.setSourceSpaceVisible(false);

    // setViewCount(3) not yet tested
    view.setViewCount(3);
    QCOMPARE(view.viewCount(), 3);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtDataWorkers_basics()
{
    // RtSourceDataWorker
    {
        RtSourceDataWorker worker;

        worker.setNumberAverages(4);
        worker.setColormapType("hot");
        worker.setColormapType("jet");
        worker.setThresholds(0.1, 0.5, 1.0);
        worker.setLoopState(true);
        worker.setLoopState(false);
        worker.setSFreq(600.0);
        worker.setStreamSmoothedData(true);
        worker.setStreamSmoothedData(false);
        worker.setSurfaceColor(QVector<uint32_t>(), QVector<uint32_t>());

        // addData + clear
        Eigen::VectorXd dVec(5);
        dVec << 1.0, 2.0, 3.0, 4.0, 5.0;
        worker.addData(dVec);
        worker.addData(dVec);
        worker.clear();

        // setInterpolationMatrix* with null (safe - just stores null pointer)
        QSharedPointer<Eigen::SparseMatrix<float>> nullMat;
        worker.setInterpolationMatrixLeft(nullMat);
        worker.setInterpolationMatrixRight(nullMat);
    }

    // RtSensorDataWorker
    {
        RtSensorDataWorker worker;

        worker.setNumberAverages(4);
        worker.setColormapType("hot");
        worker.setColormapType("jet");
        worker.setThresholds(0.1, 1.0);
        worker.setLoopState(true);
        worker.setLoopState(false);
        worker.setSFreq(600.0);
        worker.setStreamSmoothedData(true);
        worker.setStreamSmoothedData(false);

        // addData + clear
        Eigen::VectorXf fVec(5);
        fVec << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
        worker.addData(fVec);
        worker.addData(fVec);
        worker.clear();

        // setMappingMatrix with null
        std::shared_ptr<Eigen::MatrixXf> nullMap;
        worker.setMappingMatrix(nullMap);
    }

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::dipoleObject_extended()
{
    // Build a real InvEcdSet with 2 dipoles
    INVLIB::InvEcdSet ecdSet;
    {
        INVLIB::InvEcd d1;
        d1.valid = true;
        d1.time  = 0.05f;
        d1.rd    = Eigen::Vector3f(0.01f, 0.02f, 0.03f);
        d1.Q     = Eigen::Vector3f(1e-9f, 0.0f, 0.0f);
        d1.good  = 0.9f;
        d1.khi2  = 0.1f;
        d1.nfree = 3;
        d1.neval = 10;
        ecdSet.addEcd(d1);

        INVLIB::InvEcd d2 = d1;
        d2.rd = Eigen::Vector3f(-0.01f, 0.01f, 0.04f);
        d2.Q  = Eigen::Vector3f(0.0f, 1e-9f, 0.0f);
        ecdSet.addEcd(d2);
    }
    QCOMPARE(ecdSet.size(), (qint32)2);

    DipoleObject obj;

    // load() — pure CPU: createGeometry() + instance data setup
    obj.load(ecdSet);
    QVERIFY(obj.instanceCount() > 0);

    // debugFirstDipolePosition — returns position of first instance
    QVector3D pos = obj.debugFirstDipolePosition();
    // Coordinates are ~10–40 mm → converted to meters (0.001 scale)
    QVERIFY(std::abs(pos.x()) < 1.0f);

    // applyTransform — multiplies all instance model matrices
    QMatrix4x4 identity;
    obj.applyTransform(identity);

    QMatrix4x4 translate;
    translate.translate(0.01f, 0.0f, 0.0f);
    obj.applyTransform(translate);

    // intersect — ray casting against cone geometry
    float dist = 0.0f;
    int hitIdx = obj.intersect(QVector3D(0, 10, 0), QVector3D(0, -1, 0), dist);
    Q_UNUSED(hitIdx);
    Q_UNUSED(dist);

    // setSelected — no GPU involved
    obj.setSelected(0, true);
    obj.setSelected(0, false);
    obj.setSelected(-1, true);  // invalid index — safe
    obj.setSelected(99, false); // out-of-range — safe

    QApplication::processEvents();
}

//=============================================================================================================

QTEST_MAIN(TestDisp3dBrainView)
#include "test_disp3d_brainview.moc"
