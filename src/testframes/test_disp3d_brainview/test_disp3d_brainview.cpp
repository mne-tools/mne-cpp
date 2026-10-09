//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp3d_brainview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for the disp3D BrainView, BrainTreeModel, workers and scene managers.
 *           Covers: BrainView construction and public API (without GPU rendering),
 *           BrainTreeModel item model, RtSourceInterpolationMatWorker,
 *           RtSensorInterpolationMatWorker, SourceEstimateManager,
 *           RtSensorStreamManager, MultiViewLayout, and StcLoadingWorker.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view/brainview.h>
#include <disp3D/view/brainrenderer.h>
#include <rhi/qrhi.h>
#include <disp3D/renderable/polylineobject.h>
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
#include <disp3D/workers/rtsourcedatacontroller.h>
#include <disp3D/workers/rtsensorinterpolationmatworker.h>
#include <disp3D/workers/rtsourcedataworker.h>
#include <disp3D/workers/rtsensordataworker.h>
#include <disp3D/workers/rtsensordatacontroller.h>
#include <disp3D/scene/sourceestimatemanager.h>
#include <disp3D/scene/rtsensorstreammanager.h>
#include <disp3D/workers/stcloadingworker.h>
#include <disp3D/renderable/brainsurface.h>
#include <disp3D/input/cameracontroller.h>
#include <disp3D/input/raypicker.h>
#include <disp3D/renderable/dipoleobject.h>
#include <disp3D/renderable/networkobject.h>
#include <disp3D/renderable/sourceestimateoverlay.h>
#include <disp3D/renderable/sliceobject.h>
#include <disp3D/renderable/videooverlay.h>
#include <disp3D/scene/sensorfieldmapper.h>
#include <fiff/fiff_evoked.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_field_map.h>
#include <disp/plots/helpers/colormap.h>
#include <inv/inv_source_estimate.h>
#include <disp3D/core/viewstate.h>

#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans_set.h>
#include <fiff/fiff_stream.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <connectivity/network/network.h>
#include <connectivity/network/networknode.h>
#include <connectivity/network/networkedge.h>
#include <fs/fs_label.h>
#include <fs/fs_surface.h>
#include <mne/mne_bem.h>

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
     * Verifies that the HPI head movement path accepts a position list, ignores
     * degenerate input and can be cleared, without requiring a GPU.
     */
    void brainView_headMovementPath();
    void polylineObject_segments();

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
     * pick with empty surface/dipole maps, and displayLabel on a RayHit.
     */
    void rayPicker_basics();

    //=========================================================================================================
    /**
     * Verifies VideoOverlay ignores null frames and counts video and depth frames separately.
     */
    void videoOverlay_basics();

    //=========================================================================================================
    /**
     * Verifies live ray, live/static markers, probe and video overlay setters on BrainView.
     */
    void brainView_liveOverlays();

    //=========================================================================================================
    /**
     * Drives BrainRenderer through whole frames on the Null QRhi backend: render targets, every
     * renderable, the uniform slot budget and recovery after it overflows.
     */
    void brainRenderer_nullRhi();

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
     * Each BrainView::clear* removes exactly its objects and tree rows; loading again works.
     */
    void brainView_clearRemovesObjectsAndRows();

    //=========================================================================================================
    /**
     * A left click reports the surface point under the cursor; releasing after a rotate or pan drag does not.
     */
    void brainView_clickVersusDrag();

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

    //=========================================================================================================
    /**
     * Verifies NetworkObject geometry and the node/edge instances kept by the threshold.
     */
    void networkObject_thresholdAndInstances();

    //=========================================================================================================
    /**
     * Verifies SourceEstimateOverlay thresholds, time access and per-vertex colours on a surface.
     */
    void sourceEstimateOverlay_colorsSurface();

    //=========================================================================================================
    /**
     * Verifies SliceObject corner placement for each orientation and its quad buffers.
     */
    void sliceObject_cornersAndQuad();
};

//=============================================================================================================

void TestDisp3dBrainView::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::cleanupTestCase()
{
}

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

    // Loaders register their data in the tree model: without a model they refuse instead of crashing,
    // a network still renders without its tree row
    const QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/");
    QTemporaryDir dir;
    INVLIB::InvEcd ecd;
    ecd.valid = true;
    ecd.time = 0.1f;
    ecd.rd = Eigen::Vector3f(0.0f, 0.0f, 0.05f);
    ecd.Q = Eigen::Vector3f(1e-8f, 0.0f, 0.0f);
    ecd.good = 0.9f;
    INVLIB::InvEcdSet dipoles;
    dipoles << ecd;
    const QString dipPath = dir.filePath(QStringLiteral("one.dip"));
    QVERIFY(dipoles.save_dipoles_dip(dipPath));
    CONNECTIVITYLIB::Network network(QStringLiteral("Coherence"));
    QList<CONNECTIVITYLIB::NetworkNode::SPtr> nodes;
    for (int i = 0; i < 2; ++i) {
        nodes.append(CONNECTIVITYLIB::NetworkNode::SPtr::create(static_cast<qint16>(i), Eigen::RowVectorXf::Constant(3, 0.01f * static_cast<float>(i))));
        network.append(nodes.last());
    }
    auto edge = CONNECTIVITYLIB::NetworkEdge::SPtr::create(0, 1, Eigen::MatrixXd::Constant(1, 1, 0.5));
    network.append(edge);
    nodes[0]->append(edge);
    nodes[1]->append(edge);
    const QString avePath = dataDir + QStringLiteral("MEG/sample/sample_audvis-ave.fif");
    const QString fwdPath = dataDir + QStringLiteral("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QVERIFY(!view.loadSensors(avePath));
    QVERIFY(!view.loadDipoles(dipPath));
    QVERIFY(!view.loadSourceSpace(fwdPath));
    QVERIFY(view.loadNetwork(network, QStringLiteral("Coherence")));

    BrainTreeModel model;
    BrainView modelView;
    modelView.setModel(&model);
    QVERIFY(modelView.loadDipoles(dipPath));
    QVERIFY(modelView.loadSensors(avePath));
    QVERIFY(modelView.loadNetwork(network, QStringLiteral("Coherence")));
    QVERIFY(model.rowCount() >= 3);

    // Cardinal fiducials come in head coordinates, in MRI ones once the head <-> MRI transform is loaded
    // (MNE-Python: apply_trans(invert_transform(mri_head_t), dig[k]["r"]))
    QMap<int, QVector3D> fiducials = modelView.cardinalFiducialsInMri();
    QCOMPARE(fiducials.size(), 3);
    QVERIFY(std::fabs(fiducials[FIFFV_POINT_LPA].x() + 0.07137661f) < 1e-6f);
    QVERIFY(!modelView.loadTransformation(dir.filePath(QStringLiteral("missing-trans.fif"))));
    QVERIFY(modelView.loadTransformation(dataDir + QStringLiteral("MEG/sample/all-trans.fif")));
    // The dipole (head coordinates) follows the transform like the sensors: a ray through the view centre
    // picks it only where MNE-Python's apply_trans puts head (0, 0, 50) mm, 41 mm from the untransformed spot
    modelView.showSingleView();
    modelView.setDipoleVisible(true);
    QSignalSpy hovered(&modelView, &BrainView::hoveredRegionChanged);
    const auto dipoleHitAt = [&](const QVector3D& centre) {
        modelView.setCameraFocusOverride(centre, 0.05f);
        hovered.clear();
        modelView.castRay(modelView.rect().center());
        return !hovered.isEmpty() && !hovered.last().at(0).toString().isEmpty();
    };
    QVERIFY(dipoleHitAt(QVector3D(0.00381462f, -0.01784828f, 0.0130306f)));
    QVERIFY(!dipoleHitAt(QVector3D(0.0f, 0.0f, 0.05f)));
    modelView.clearTransformation();
    QVERIFY(dipoleHitAt(QVector3D(0.0f, 0.0f, 0.05f)));
    modelView.clearCameraFocusOverride();
    QVERIFY(modelView.loadTransformation(dataDir + QStringLiteral("MEG/sample/all-trans.fif")));

    fiducials = modelView.cardinalFiducialsInMri();
    const QMap<int, QVector3D> expected{{FIFFV_POINT_LPA, QVector3D(-0.06925742f, 0.01058946f, -0.02500086f)},
                                        {FIFFV_POINT_NASION, QVector3D(0.00337909f, 0.09465942f, 0.03225918f)},
                                        {FIFFV_POINT_RPA, QVector3D(0.07728562f, 0.01205367f, -0.03024882f)}};
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        QVERIFY2((fiducials[it.key()] - it.value()).length() < 1e-6f, qPrintable(QString::number(it.key())));
    }
    modelView.clearTransformation();
    QVERIFY(std::fabs(modelView.cardinalFiducialsInMri()[FIFFV_POINT_LPA].x() + 0.07137661f) < 1e-6f);

    // The head <-> MRI transform is found wherever it sits in the file; a file without one is refused
    FIFFLIB::FiffCoordTransSet allTrans;
    QVERIFY(allTrans.read(dataDir + QStringLiteral("MEG/sample/all-trans.fif")) > 0);
    const FIFFLIB::FiffCoordTrans devHead(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, Eigen::Matrix3f::Identity(), Eigen::Vector3f(0.0f, 0.0f, 0.04f));
    const QString reordered = dir.filePath(QStringLiteral("mri-head-first-trans.fif"));
    const QString devOnly = dir.filePath(QStringLiteral("dev-head-trans.fif"));
    for (const QString& path : {reordered, devOnly}) {
        QFile out(path);
        auto stream = FIFFLIB::FiffStream::start_file(out);
        QVERIFY(stream);
        if (path == reordered) {
            stream->write_coord_trans(allTrans.head_surf_RAS_t.inverted());
        }
        stream->write_coord_trans(devHead);
        stream->end_file();
        out.close();
    }
    QVERIFY(modelView.loadTransformation(reordered));
    fiducials = modelView.cardinalFiducialsInMri();
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        QVERIFY2((fiducials[it.key()] - it.value()).length() < 1e-6f, qPrintable(QString::number(it.key())));
    }
    QVERIFY(!modelView.loadTransformation(devOnly));

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

    // Dragging the vertical splitter of the 2x2 layout from the centre to 500 px moves the pane boundary:
    // x = 450 then lies in the first pane instead of the second, also in a view opened afterwards
    view.setViewportEnabled(0, true);
    view.resize(800, 600);
    view.resetMultiViewLayout();
    view.setViewCount(4);
    const auto send = [](BrainView& target, QEvent::Type type, const QPoint& pos, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, target.mapToGlobal(pos), type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons, Qt::NoModifier);
        QApplication::sendEvent(&target, &event);
    };
    const auto paneAt = [&send](BrainView& target, const QPoint& pos) {
        send(target, QEvent::MouseButtonPress, pos, Qt::LeftButton);
        send(target, QEvent::MouseButtonRelease, pos, Qt::NoButton);
        return target.visualizationEditTarget();
    };
    QCOMPARE(paneAt(view, QPoint(450, 100)), 1);
    QSignalSpy clicked(&view, &BrainView::surfacePointClicked);
    send(view, QEvent::MouseButtonPress, QPoint(400, 100), Qt::LeftButton);
    send(view, QEvent::MouseMove, QPoint(500, 100), Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, QPoint(500, 100), Qt::NoButton);
    QVERIFY(clicked.isEmpty());
    BrainView reopened;
    reopened.resize(800, 600);
    QCOMPARE(paneAt(reopened, QPoint(450, 100)), 0);
    QCOMPARE(paneAt(view, QPoint(450, 100)), 0);
    view.resetMultiViewLayout();
    QCOMPARE(paneAt(view, QPoint(450, 100)), 1);
    view.showSingleView();

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

    // Without a rendered frame (no QRhi offscreen) a screenshot is refused and nothing is written
    QTemporaryDir shots;
    const QString shot = shots.filePath(QStringLiteral("sub/view.png"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("No rendered frame"));
    QVERIFY(!view.takeScreenshot(shot));
    QVERIFY(QDir(shots.path()).isEmpty());
    const QString cwd = QDir::currentPath();
    QDir::setCurrent(shots.path());
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("No rendered frame"));
    QVERIFY(!view.saveSnapshot());
    QVERIFY(QDir(shots.path()).isEmpty());
    QDir::setCurrent(cwd);

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

    QSharedPointer<Eigen::SparseMatrix<float>> lhMat;
    int nLh = 0;
    QObject::connect(&worker, &RtSourceInterpolationMatWorker::newInterpolationMatrixLeftAvailable,
                     [&](QSharedPointer<Eigen::SparseMatrix<float>> mat) {
                         lhMat = mat;
                         ++nLh;
                     });

    // Annotation mode: 6 vertices in labels 10 (vertices 0-2) and 20 (3-5), sources at vertices 0, 1 and 4.
    // Every vertex shows the mean of its label's sources.
    Eigen::VectorXi labelIds(6);
    labelIds << 10, 10, 10, 20, 20, 20;
    FSLIB::FsLabel a;
    a.label_id = 10;
    a.vertices = Eigen::VectorXi::LinSpaced(3, 0, 2);
    FSLIB::FsLabel b;
    b.label_id = 20;
    b.vertices = Eigen::VectorXi::LinSpaced(3, 3, 5);
    Eigen::VectorXi sources(3);
    sources << 0, 1, 4;
    worker.setAnnotationInfoLeft(labelIds, {a, b}, sources);
    worker.setVisualizationType(RtSourceInterpolationMatWorker::AnnotationBased);
    worker.computeInterpolationMatrix();
    QCOMPARE(nLh, 1);
    QVERIFY(lhMat);
    QCOMPARE(lhMat->rows(), Eigen::Index(6));
    QCOMPARE(lhMat->cols(), Eigen::Index(3));
    const Eigen::VectorXf values = *lhMat * Eigen::Vector3f(2.0f, 4.0f, 7.0f);
    for (int v = 0; v < 3; ++v) {
        QVERIFY(std::fabs(values(v) - 3.0f) < 1e-6f);
        QVERIFY(std::fabs(values(v + 3) - 7.0f) < 1e-6f);
    }

    // Labels need not cover the surface (e.g. an unlabelled medial wall): one row per surface vertex
    // regardless, unlabelled vertices stay at zero
    b.vertices = Eigen::VectorXi::LinSpaced(2, 4, 5);
    labelIds(3) = 0;
    worker.setAnnotationInfoLeft(labelIds, {a, b}, sources);
    worker.computeInterpolationMatrix();
    QCOMPARE(lhMat->rows(), Eigen::Index(6));
    const Eigen::VectorXf partial = *lhMat * Eigen::Vector3f(2.0f, 4.0f, 7.0f);
    QCOMPARE(partial(3), 0.0f);
    QVERIFY(std::fabs(partial(5) - 7.0f) < 1e-6f);

    // Interpolation mode on a line of 3 vertices with sources at both ends
    Eigen::MatrixX3f rr(3, 3);
    rr << 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.02f, 0.0f, 0.0f;
    std::vector<Eigen::VectorXi> neighbors(3);
    neighbors[0] = (Eigen::VectorXi(1) << 1).finished();
    neighbors[1] = (Eigen::VectorXi(2) << 0, 2).finished();
    neighbors[2] = (Eigen::VectorXi(1) << 1).finished();
    Eigen::VectorXi ends(2);
    ends << 0, 2;
    worker.setInterpolationInfoLeft(rr, neighbors, ends);
    worker.setVisualizationType(RtSourceInterpolationMatWorker::InterpolationBased);
    worker.setInterpolationFunction(QStringLiteral("linear"));
    const int nBefore = nLh;
    worker.computeInterpolationMatrix();
    QCOMPARE(nLh, nBefore + 1);
    QCOMPARE(lhMat->rows(), Eigen::Index(3));
    QCOMPARE(lhMat->cols(), Eigen::Index(2));
    // Source vertices keep their own value; the middle one lies between both
    const Eigen::VectorXf line = *lhMat * Eigen::Vector2f(1.0f, 3.0f);
    QVERIFY(std::fabs(line(0) - 1.0f) < 1e-6f);
    QVERIFY(std::fabs(line(2) - 3.0f) < 1e-6f);
    QVERIFY(line(1) > 1.0f && line(1) < 3.0f);

    // The controller computes the matrices on its worker thread and streams one colour per vertex of each hemisphere
    RtSourceDataController controller;
    int nCtrlLh = 0;
    int nCtrlRh = 0;
    QVector<uint32_t> colorsLh;
    QVector<uint32_t> colorsRh;
    Eigen::VectorXd rawLh;
    connect(&controller, &RtSourceDataController::newInterpolationMatrixLeftAvailable,
            [&](const QSharedPointer<Eigen::SparseMatrix<float>>&) { ++nCtrlLh; });
    connect(&controller, &RtSourceDataController::newInterpolationMatrixRightAvailable,
            [&](const QSharedPointer<Eigen::SparseMatrix<float>>&) { ++nCtrlRh; });
    connect(&controller, &RtSourceDataController::newSmoothedDataAvailable,
            [&](const QVector<uint32_t>& lh, const QVector<uint32_t>& rh) {
                colorsLh = lh;
                colorsRh = rh;
            });
    connect(&controller, &RtSourceDataController::newRawDataAvailable,
            [&](const Eigen::VectorXd& lh, const Eigen::VectorXd&) { rawLh = lh; });
    b.vertices = Eigen::VectorXi::LinSpaced(3, 3, 5);
    labelIds(3) = 20;
    controller.setVisualizationType(RtSourceInterpolationMatWorker::AnnotationBased);
    controller.setAnnotationInfoLeft(labelIds, {a, b}, sources);
    controller.setAnnotationInfoRight(labelIds, {a, b}, sources);
    controller.recomputeInterpolation();
    QTRY_COMPARE(nCtrlLh, 1);
    QTRY_COMPARE(nCtrlRh, 1);
    controller.setThresholds(0.0, 0.5, 1.0);
    controller.setTimeInterval(10);
    Eigen::VectorXd data(6);
    data << 0.0, 0.2, 1.0, 0.0, 0.2, 1.0;
    controller.addData(data);
    controller.setStreamingState(true);
    QTRY_COMPARE(colorsLh.size(), 6);
    QCOMPARE(colorsRh.size(), 6);
    // Vertices of one label share its mean, so they share a colour; the two labels differ
    QCOMPARE(colorsLh[1], colorsLh[0]);
    QCOMPARE(colorsLh[5], colorsLh[3]);
    QVERIFY(colorsLh[0] != colorsLh[3]);
    QCOMPARE(colorsRh, colorsLh);
    controller.setStreamSmoothedData(false);
    controller.addData(data);
    QTRY_COMPARE(rawLh.size(), Eigen::Index(3));
    QCOMPARE(rawLh(2), 1.0);
    controller.setStreamingState(false);

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

    int eegCount = 0;
    QString eegKey;
    std::shared_ptr<Eigen::MatrixXf> mapping;
    QVector<int> pick;
    QObject::connect(&worker, &RtSensorInterpolationMatWorker::newEegMappingAvailable,
                     [&](const QString& key, std::shared_ptr<Eigen::MatrixXf> mat, const QVector<int>& picked) {
                         ++eegCount;
                         eegKey = key;
                         mapping = std::move(mat);
                         pick = picked;
                     });

    // Without evoked data nothing is computed
    worker.computeMapping();
    QCOMPARE(eegCount, 0);

    // EEG of the sample data onto a small scalp patch: one mapping row per vertex, one column per picked EEG channel
    QFile file(QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"));
    const FIFFLIB::FiffEvoked evoked(file, 0);
    QVERIFY(!evoked.isEmpty());
    Eigen::MatrixX3f scalp(6, 3);
    scalp << 0.09f, 0.0f, 0.04f, -0.09f, 0.0f, 0.04f, 0.0f, 0.09f, 0.04f, 0.0f, -0.09f, 0.04f, 0.0f, 0.0f, 0.13f, 0.05f, 0.05f, 0.1f;
    worker.setEvoked(evoked);
    worker.setTransform(FIFFLIB::FiffCoordTrans(), false);
    worker.setEegSurface(QStringLiteral("bem_head"), scalp);
    worker.computeMapping();
    QCOMPARE(eegCount, 1);
    QCOMPARE(eegKey, QStringLiteral("bem_head"));
    QVERIFY(mapping && mapping->rows() == scalp.rows() && mapping->cols() == pick.size());
    // The file's own bad channels (EEG 053) are left out, as in SensorFieldMapper and MNE-Python
    QVERIFY(!evoked.info.bads.isEmpty());
    for (int k : pick) {
        QCOMPARE(evoked.info.chs[k].kind, FIFFV_EEG_CH);
        QVERIFY(!evoked.info.bads.contains(evoked.info.chs[k].ch_name));
    }

    // Same mapping as FwdFieldMap with the origin fitted to the head shape (MNE-Python's make_field_map origin="auto")
    QList<FIFFLIB::FiffChInfo> eegChs;
    for (int k : pick) {
        eegChs.append(evoked.info.chs[k]);
    }
    auto coils = FWDLIB::FwdCoilSet::create_eeg_els(eegChs, eegChs.size(), FIFFLIB::FiffCoordTrans());
    const auto expected = FWDLIB::FwdFieldMap::computeEegMapping(*coils, scalp, SensorFieldMapper::fitSphereOrigin(evoked.info), 0.06f, 1e-3f);
    QVERIFY((*mapping - *expected).cwiseAbs().maxCoeff() <= 1e-5f * expected->cwiseAbs().maxCoeff());

    const auto nEeg = pick.size();

    // Channels passed as bad are left out
    const QString firstEeg = evoked.info.chs[pick.first()].ch_name;
    worker.setBadChannels({firstEeg});
    worker.computeMapping();
    QCOMPARE(eegCount, 2);
    for (int k : pick) {
        QVERIFY(evoked.info.chs[k].ch_name != firstEeg);
    }
    worker.setBadChannels({});

    // The controller computes the same mapping on its worker thread and streams colours onto that surface
    RtSensorDataController controller;
    QString controllerKey;
    int controllerPicks = 0;
    QString colorKey;
    QVector<uint32_t> colors;
    Eigen::VectorXf raw;
    connect(&controller, &RtSensorDataController::newEegMappingAvailable,
            [&](const QString& key, const std::shared_ptr<Eigen::MatrixXf>&, const QVector<int>& picked) {
                controllerKey = key;
                controllerPicks = picked.size();
            });
    connect(&controller, &RtSensorDataController::newSensorColorsAvailable,
            [&](const QString& key, const QVector<uint32_t>& vertexColors) {
                colorKey = key;
                colors = vertexColors;
            });
    connect(&controller, &RtSensorDataController::newRawSensorDataAvailable,
            [&](const Eigen::VectorXf& data) { raw = data; });
    controller.setEvoked(evoked);
    controller.setTransform(FIFFLIB::FiffCoordTrans(), false);
    controller.setEegSurface(QStringLiteral("bem_head"), scalp);
    controller.recomputeMapping();
    QTRY_COMPARE(controllerKey, QStringLiteral("bem_head"));
    QCOMPARE(controllerPicks, nEeg);
    controller.setTimeInterval(10);
    controller.addData(Eigen::VectorXf::Constant(nEeg, 1e-6f));
    controller.setStreamingState(true);
    QTRY_COMPARE(colors.size(), static_cast<int>(scalp.rows()));
    QCOMPARE(colorKey, QStringLiteral("bem_head"));
    controller.setStreamSmoothedData(false);
    controller.addData(Eigen::VectorXf::Constant(nEeg, 1e-6f));
    QTRY_COMPARE(raw.size(), static_cast<Eigen::Index>(nEeg));
    controller.setStreamingState(false);

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
    QCOMPARE(manager.closestIndex(0.0f), -1);

    // No brain surface: loading is refused
    QMap<QString, std::shared_ptr<BrainSurface>> surfaces;
    QVERIFY(!manager.load(QStringLiteral("a-lh.stc"), QString(), surfaces, QStringLiteral("pial")));

    // Left STC: 2 sources on a 4-vertex "lh_pial" surface, 3 time points from -10 ms in 5 ms steps
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString lhPath = dir.filePath(QStringLiteral("test-lh.stc"));
    Eigen::MatrixXd data(2, 3);
    data << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    Eigen::VectorXi vertices(2);
    vertices << 0, 3;
    {
        QFile out(lhPath);
        QVERIFY(INVLIB::InvSourceEstimate(data, vertices, -0.01f, 0.005f).write(out));
    }
    Eigen::MatrixX3f rr(4, 3);
    rr << 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.01f, 0.01f, 0.0f;
    Eigen::MatrixX3i tris(2, 3);
    tris << 0, 1, 2, 1, 3, 2;
    auto lh = std::make_shared<BrainSurface>();
    lh->createFromData(rr, tris, Qt::gray);
    lh->setHemi(0);
    surfaces.insert(QStringLiteral("lh_pial"), lh);

    QSignalSpy loadedSpy(&manager, &SourceEstimateManager::loaded);
    QSignalSpy thresholdSpy(&manager, &SourceEstimateManager::thresholdsUpdated);
    QVERIFY(manager.load(lhPath, QString(), surfaces, QStringLiteral("pial")));
    QVERIFY(manager.isLoading());
    QVERIFY(!manager.load(lhPath, QString(), surfaces, QStringLiteral("pial")));
    QTRY_COMPARE(loadedSpy.size(), 1);
    QVERIFY(!manager.isLoading());
    QVERIFY(manager.isLoaded());
    QCOMPARE(loadedSpy.at(0).at(0).toInt(), 3);
    QCOMPARE(thresholdSpy.size(), 1);
    QCOMPARE(thresholdSpy.at(0).at(2).toFloat(), 6.0f);
    QCOMPARE(manager.numTimePoints(), 3);
    QCOMPARE(manager.tmin(), -0.01f);
    QCOMPARE(manager.tstep(), 0.005f);
    QCOMPARE(manager.closestIndex(-0.0004f), 2);
    QCOMPARE(manager.closestIndex(-0.0071f), 1);
    QCOMPARE(manager.closestIndex(1.0f), 2);

    // Stepping to a time point colours the active surface type and reports the time
    QSignalSpy timeSpy(&manager, &SourceEstimateManager::timePointChanged);
    const uint32_t before = lh->vertexDataRef()[3].color;
    SubView view;
    view.surfaceType = QStringLiteral("pial");
    manager.setTimePoint(5, surfaces, view, {});
    QCOMPARE(manager.currentTimePoint(), 2);
    QCOMPARE(timeSpy.size(), 1);
    QVERIFY(std::fabs(timeSpy.at(0).at(1).toFloat() - 0.0f) < 1e-6f);
    QVERIFY(lh->vertexDataRef()[3].color != before);

    // A finished load leaves the manager safe to reload, cancel and destroy (the worker used to be
    // freed behind its back; this faults under DYLD_INSERT_LIBRARIES=libgmalloc / glibc heap checks)
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(manager.load(lhPath, QString(), surfaces, QStringLiteral("pial")));
    QTRY_COMPARE(loadedSpy.size(), 2);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    manager.cancelLoading();
    QVERIFY(!manager.isLoading());
    QVERIFY(manager.load(lhPath, QString(), surfaces, QStringLiteral("pial")));
    manager.cancelLoading();
    QVERIFY(!manager.isLoading());

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::rtSensorStreamManager_basics()
{
    RtSensorStreamManager manager;

    // stopStreaming is safe to call even with no active stream
    manager.stopStreaming();

    // Without evoked data or a mapping nothing can be streamed
    SensorFieldMapper mapper;
    QMap<QString, std::shared_ptr<BrainSurface>> surfaces;
    QVERIFY(!manager.startStreaming(QStringLiteral("EEG"), mapper, surfaces));
    QVERIFY(!manager.isStreaming());

    // EEG of the sample data mapped onto a small scalp surface
    QFile file(QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"));
    mapper.setEvoked(FIFFLIB::FiffEvoked(file, 0));
    Eigen::MatrixX3f rr(6, 3);
    rr << 0.09f, 0.0f, 0.04f, -0.09f, 0.0f, 0.04f, 0.0f, 0.09f, 0.04f, 0.0f, -0.09f, 0.04f, 0.0f, 0.0f, 0.13f, 0.05f, 0.05f, 0.1f;
    Eigen::MatrixX3i tris(4, 3);
    tris << 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0, 4;
    auto head = std::make_shared<BrainSurface>();
    head->createFromData(rr, tris, Qt::gray);
    surfaces.insert(QStringLiteral("bem_head"), head);
    QVERIFY(mapper.buildMapping(surfaces, FIFFLIB::FiffCoordTrans(), false));
    QVERIFY(!manager.startStreaming(QStringLiteral("ECoG"), mapper, surfaces));

    // Streaming sends colours for every vertex of the mapped surface, addressed by its key
    QString key;
    QVector<uint32_t> colors;
    connect(&manager, &RtSensorStreamManager::colorsAvailable,
            [&](const QString& surfaceKey, const QVector<uint32_t>& vertexColors) {
                key = surfaceKey;
                colors = vertexColors;
            });
    manager.setInterval(10);
    QVERIFY(manager.startStreaming(QStringLiteral("EEG"), mapper, surfaces));
    QVERIFY(manager.isStreaming());
    QVERIFY(!manager.startStreaming(QStringLiteral("EEG"), mapper, surfaces));
    QTRY_COMPARE(colors.size(), 6);
    QCOMPARE(key, QStringLiteral("bem_head"));
    manager.stopStreaming();
    QVERIFY(!manager.isStreaming());

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

    // Neither file exists: two read errors, then failure
    QSignalSpy errorSpy(&worker, &StcLoadingWorker::error);
    QSignalSpy finishedSpy(&worker, &StcLoadingWorker::finished);
    worker.process();
    QCOMPARE(errorSpy.size(), 3);
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.at(0).at(0).toBool());

    // A left-hemisphere STC with sources at two vertices of a 4-vertex surface; the right one is missing
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString lhPath = dir.filePath(QStringLiteral("test-lh.stc"));
    Eigen::MatrixXd data(2, 3);
    data << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    Eigen::VectorXi vertices(2);
    vertices << 0, 3;
    {
        QFile out(lhPath);
        QVERIFY(INVLIB::InvSourceEstimate(data, vertices, 0.0f, 0.001f).write(out));
    }
    Eigen::MatrixX3f rr(4, 3);
    rr << 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.01f, 0.01f, 0.0f;
    Eigen::MatrixX3i tris(2, 3);
    tris << 0, 1, 2, 1, 3, 2;
    lhSurface.createFromData(rr, tris, Qt::gray);
    StcLoadingWorker loader(lhPath, QString(), &lhSurface, &rhSurface, 0.05);
    QSignalSpy loadedSpy(&loader, &StcLoadingWorker::finished);
    QSignalSpy progressSpy(&loader, &StcLoadingWorker::progress);
    loader.process();
    QCOMPARE(loadedSpy.size(), 1);
    QVERIFY(loadedSpy.at(0).at(0).toBool());
    QVERIFY(loader.hasLh());
    QVERIFY(!loader.hasRh());
    QCOMPARE(loader.stcLh().data, data);
    // One row per surface vertex, one column per source; each source vertex takes its own value
    const auto mat = loader.interpolationMatLh();
    QVERIFY(mat);
    QCOMPARE(mat->rows(), Eigen::Index(4));
    QCOMPARE(mat->cols(), Eigen::Index(2));
    QVERIFY(std::fabs(mat->coeff(0, 0) - 1.0f) < 1e-6f);
    QVERIFY(std::fabs(mat->coeff(3, 1) - 1.0f) < 1e-6f);
    QVERIFY(progressSpy.size() > 2);
    QCOMPARE(progressSpy.last().at(0).toInt(), 100);

    // Cancelled before it starts: no matrix, failure
    StcLoadingWorker cancelled(lhPath, QString(), &lhSurface, &rhSurface, 0.05);
    QSignalSpy cancelledSpy(&cancelled, &StcLoadingWorker::finished);
    cancelled.requestCancel();
    QVERIFY(cancelled.isCancelled());
    cancelled.process();
    QCOMPARE(cancelledSpy.size(), 1);
    QVERIFY(!cancelledSpy.at(0).at(0).toBool());
    QVERIFY(cancelled.interpolationMatLh().isNull());

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
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::None), Qt::ArrowCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Vertical), Qt::SizeHorCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Horizontal), Qt::SizeVerCursor);
    QCOMPARE(MultiViewLayout::cursorForHit(SplitterHit::Both), Qt::SizeAllCursor);

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
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Vertical, sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Horizontal, sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::Both, sz);
    layout.dragSplitter(QPoint(300, 200), SplitterHit::None, sz);

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
    cardinal.kind = FIFFV_POINT_CARDINAL;
    cardinal.ident = FIFFV_POINT_NASION;
    cardinal.r[0] = 0.0f;
    cardinal.r[1] = 0.08f;
    cardinal.r[2] = 0.0f;
    pts << cardinal;

    FIFFLIB::FiffDigPoint lpa;
    lpa.kind = FIFFV_POINT_CARDINAL;
    lpa.ident = FIFFV_POINT_LPA;
    lpa.r[0] = -0.07f;
    lpa.r[1] = 0.0f;
    lpa.r[2] = 0.0f;
    pts << lpa;

    FIFFLIB::FiffDigPoint hpi;
    hpi.kind = FIFFV_POINT_HPI;
    hpi.ident = 1;
    hpi.r[0] = 0.01f;
    hpi.r[1] = 0.01f;
    hpi.r[2] = 0.06f;
    pts << hpi;

    FIFFLIB::FiffDigPoint eeg;
    eeg.kind = FIFFV_POINT_EEG;
    eeg.ident = 1;
    eeg.r[0] = 0.02f;
    eeg.r[1] = 0.02f;
    eeg.r[2] = 0.07f;
    pts << eeg;

    FIFFLIB::FiffDigPoint extra;
    extra.kind = FIFFV_POINT_EXTRA;
    extra.ident = 1;
    extra.r[0] = 0.03f;
    extra.r[1] = 0.03f;
    extra.r[2] = 0.05f;
    pts << extra;

    DigitizerSetTreeItem item("Digitizer", pts);

    // 5 points across categories
    QCOMPARE(item.totalPointCount(), 5);

    // categoryItem uses DigitizerTreeItem::PointKind (Cardinal=0, HPI=1, EEG=2, Extra=3),
    // not the FIFF constants (FIFFV_POINT_CARDINAL=1, etc.)
    QVERIFY(item.categoryItem(DigitizerTreeItem::Cardinal) != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::HPI) != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::EEG) != nullptr);
    QVERIFY(item.categoryItem(DigitizerTreeItem::Extra) != nullptr);

    // Non-existent category returns nullptr
    QVERIFY(item.categoryItem(999) == nullptr);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_liveOverlays()
{
    // Live ray, probe and markers are pickable scene surfaces; each clear call removes exactly its own kind
    BrainView view;
    const auto hitAtZ = [&](float z) {
        QVector3D hit;
        return view.intersectWorldRay(QVector3D(-1.0f, 0.0f, z), QVector3D(1.0f, 0.0f, 0.0f), hit) && std::fabs(hit.z() - z) < 1e-3f;
    };
    QVERIFY(!hitAtZ(0.15f));
    view.setLiveRay(QVector3D(0.0f, 0.0f, 0.1f), QVector3D(0.0f, 0.0f, 0.2f), Qt::yellow, 0.002f);
    QVERIFY(hitAtZ(0.15f));
    view.setLiveRay(QVector3D(0.0f, 0.0f, 0.3f), QVector3D(0.0f, 0.0f, 0.4f), Qt::yellow, 0.002f);
    QVERIFY(!hitAtZ(0.15f));
    QVERIFY(hitAtZ(0.35f));
    view.clearLiveRay();
    QVERIFY(!hitAtZ(0.35f));

    LiveMarker marker;
    marker.position = QVector3D(0.0f, 0.0f, 0.5f);
    marker.color = Qt::red;
    marker.radius = 0.01f;
    view.setLiveMarkers({marker});
    marker.position = QVector3D(0.0f, 0.0f, 0.6f);
    view.setStaticMarkers({marker});
    QVERIFY(hitAtZ(0.5f) && hitAtZ(0.6f));
    view.clearLiveMarkers();
    QVERIFY(!hitAtZ(0.5f) && hitAtZ(0.6f));
    view.clearStaticMarkers();
    QVERIFY(!hitAtZ(0.6f));

    view.setProbeVisualization(QVector3D(0.0f, 0.0f, 0.7f), QVector3D(0.0f, 0.0f, 1.0f), 0.05f, Qt::blue, Qt::cyan, QQuaternion());
    QVERIFY(hitAtZ(0.68f)); // the shaft runs back from the tip against the direction
    view.clearProbeVisualization();
    QVERIFY(!hitAtZ(0.68f));

    // Video overlay settings only take effect when enabled and are clamped to sane ranges
    QVERIFY(!view.isVideoOverlayEnabled());
    view.setVideoOverlayEnabled(true);
    QVERIFY(view.isVideoOverlayEnabled());
    view.setVideoOverlaySize(-1.0f);
    view.setVideoOverlayOpacity(2.0f);
    view.setVideoDepthScale(-0.5f);
    view.setVideoDepthSteps(1000);
    view.setVideoDepthEnabled(true);
    view.setVideoOverlayFocusPosition(QVector3D(0.0f, 0.0f, 0.1f));
    view.setVideoOverlayUpHint(QVector3D(0.0f, 1.0f, 0.0f));
    QImage frame(8, 4, QImage::Format_RGB32);
    frame.fill(Qt::green);
    view.pushVideoOverlayFrame(frame);
    view.pushVideoDepthFrame(QImage(8, 4, QImage::Format_Grayscale8));
    view.setVideoOverlayEnabled(false);
    QVERIFY(!view.isVideoOverlayEnabled());
}

//=============================================================================================================

void TestDisp3dBrainView::videoOverlay_basics()
{
    VideoOverlay overlay;
    QVERIFY(!overlay.isEnabled());
    QVERIFY(!overlay.hasFrame());
    QVERIFY(!overlay.hasDepthFrame());

    // Null frames are ignored; each real frame bumps only its own generation so the renderer re-uploads it
    overlay.setFrame(QImage());
    overlay.setDepthFrame(QImage());
    QCOMPARE(overlay.frameGeneration(), quint64(0));
    QCOMPARE(overlay.depthFrameGeneration(), quint64(0));
    QImage video(4, 2, QImage::Format_RGBA8888);
    video.fill(Qt::red);
    overlay.setFrame(video);
    overlay.setFrame(video);
    QVERIFY(overlay.hasFrame());
    QCOMPARE(overlay.frame().pixelColor(3, 1), QColor(Qt::red));
    QCOMPARE(overlay.frameGeneration(), quint64(2));
    QCOMPARE(overlay.depthFrameGeneration(), quint64(0));
    QImage depth(4, 2, QImage::Format_Grayscale8);
    depth.fill(128);
    overlay.setDepthFrame(depth);
    QVERIFY(overlay.hasDepthFrame());
    QCOMPARE(overlay.depthFrame().size(), QSize(4, 2));
    QCOMPARE(overlay.depthFrameGeneration(), quint64(1));
    QCOMPARE(overlay.frameGeneration(), quint64(2));
}

//=============================================================================================================

void TestDisp3dBrainView::brainRenderer_nullRhi()
{
    QRhiNullInitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &params));
    QVERIFY(rhi);
    std::unique_ptr<QRhiTexture> color(rhi->newTexture(QRhiTexture::RGBA8, QSize(64, 48), 1, QRhiTexture::RenderTarget));
    QVERIFY(color->create());

    BrainRenderer renderer;
    renderer.ensureRenderTargets(rhi.get(), color.get(), QSize(64, 48));
    QVERIFY(renderer.rtClear() && renderer.rtPreserve());
    QCOMPARE(renderer.rtClear()->pixelSize(), QSize(64, 48));
    QRhiRenderTarget* const firstClear = renderer.rtClear();
    renderer.ensureRenderTargets(rhi.get(), color.get(), QSize(64, 48));
    QCOMPARE(renderer.rtClear(), firstClear);
    renderer.initialize(rhi.get(), renderer.rtClear()->renderPassDescriptor(), 1);

    // Scene: a tetrahedron, a dipole, a two-node network, a polyline, an MRI slice and a video overlay
    Eigen::MatrixX3f rr(4, 3);
    rr << 0.0f, 0.0f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.05f;
    Eigen::MatrixX3i tris(4, 3);
    tris << 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3;
    BrainSurface surface;
    surface.createFromData(rr, tris, Qt::gray);
    INVLIB::InvEcd ecd;
    ecd.rd = Eigen::Vector3f(0.0f, 0.0f, 0.05f);
    ecd.Q = Eigen::Vector3f(1e-8f, 0.0f, 0.0f);
    INVLIB::InvEcdSet ecds;
    ecds << ecd;
    DipoleObject dipoles;
    dipoles.load(ecds);
    CONNECTIVITYLIB::Network network(QStringLiteral("Coherence"));
    QList<CONNECTIVITYLIB::NetworkNode::SPtr> nodes;
    for (int i = 0; i < 2; ++i) {
        nodes.append(CONNECTIVITYLIB::NetworkNode::SPtr::create(static_cast<qint16>(i), Eigen::RowVectorXf::Constant(3, 0.02f * static_cast<float>(i))));
        network.append(nodes.last());
    }
    auto edge = CONNECTIVITYLIB::NetworkEdge::SPtr::create(0, 1, Eigen::MatrixXd::Constant(1, 1, 0.5));
    network.append(edge);
    nodes[0]->append(edge);
    nodes[1]->append(edge);
    NetworkObject networkObject;
    networkObject.load(network, QStringLiteral("Jet"));
    networkObject.setThreshold(0.0);
    PolylineObject path;
    path.setPoints({Eigen::Vector3f(0.0f, 0.0f, 0.0f), Eigen::Vector3f(0.01f, 0.0f, 0.0f), Eigen::Vector3f(0.01f, 0.01f, 0.0f)});
    QImage mri(4, 3, QImage::Format_Grayscale8);
    mri.fill(128);
    SliceObject slice;
    slice.setSlice(mri, SliceOrientation::Axial, 1, Eigen::Matrix4d::Identity());
    VideoOverlay video;
    video.setEnabled(true);
    QImage frame(8, 4, QImage::Format_RGB32);
    frame.fill(Qt::green);
    video.setFrame(frame);
    video.setDepthEnabled(true);
    QImage depth(8, 4, QImage::Format_Grayscale8);
    depth.fill(64);
    video.setDepthFrame(depth);

    BrainRenderer::SceneData scene;
    scene.mvp.ortho(-0.1f, 0.1f, -0.1f, 0.1f, -1.0f, 1.0f);
    scene.cameraPos = QVector3D(0.0f, 0.0f, 1.0f);
    scene.lightDir = QVector3D(0.0f, 0.0f, -1.0f);
    scene.lightingEnabled = true;
    scene.viewportW = 64;
    scene.viewportH = 48;
    scene.scissorW = 64;
    scene.scissorH = 48;

    // The uniform buffer holds one slot per draw (8192); a whole frame must stay usable when it overflows
    QTest::failOnWarning(QRegularExpression(QStringLiteral("exhausted|uniform buffer")));
    for (int frameIndex = 0; frameIndex < 2; ++frameIndex) {
        QRhiCommandBuffer* cb = nullptr;
        QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);
        QRhiResourceUpdateBatch* upload = rhi->nextResourceUpdateBatch();
        QVERIFY(upload);
        surface.updateBuffers(rhi.get(), upload);
        renderer.prepareVideoOverlay(rhi.get(), upload, &video);
        renderer.prepareSlice(rhi.get(), upload, &slice, 0);
        renderer.prepareSlice(rhi.get(), upload, nullptr, 1);
        renderer.prepareMergedSurfaces(rhi.get(), upload, {&surface}, QStringLiteral("default"));
        QVERIFY(renderer.hasMergedContent(QStringLiteral("default")));
        cb->resourceUpdate(upload);

        renderer.beginFrame(cb);
        for (const auto mode : {BrainRenderer::Standard, BrainRenderer::Holographic, BrainRenderer::Anatomical, BrainRenderer::XRay, BrainRenderer::ShowNormals}) {
            renderer.renderSurface(cb, rhi.get(), scene, &surface, mode);
        }
        renderer.drawMergedSurfaces(cb, rhi.get(), scene, BrainRenderer::Standard, QStringLiteral("default"));
        renderer.renderDipoles(cb, rhi.get(), scene, &dipoles);
        renderer.renderNetwork(cb, rhi.get(), scene, &networkObject);
        renderer.renderPolyline(cb, rhi.get(), scene, &path);
        renderer.renderVideoOverlay(cb, rhi.get(), scene, &video);
        renderer.renderVideoOverlayOnSurface(cb, rhi.get(), scene, &video, &surface);
        renderer.endPass(cb);
        renderer.beginPreservingPass(cb);

        // Slots continue after the draws above and are aligned; every slot of the buffer is usable
        QRhiResourceUpdateBatch* slotBatch = rhi->nextResourceUpdateBatch();
        const int sliceOffset = renderer.prepareSliceDraw(slotBatch, scene, 0);
        QCOMPARE(sliceOffset, 0);
        QCOMPARE(renderer.prepareSliceDraw(slotBatch, scene, 1), -1);
        int used = 10; // 5 surface modes, merged group, dipoles, network nodes + edges, polyline
        int last = -1;
        // One warning per frame, however many draws are skipped
        QTest::ignoreMessage(QtWarningMsg, "BrainRenderer: uniform buffer full at prepareSurfaceDraw; further draws this frame are skipped");
        for (int offset = renderer.prepareSurfaceDraw(slotBatch, scene, &surface); offset >= 0; offset = renderer.prepareSurfaceDraw(slotBatch, scene, &surface)) {
            QCOMPARE(offset % rhi->ubufAlignment(), 0);
            QVERIFY(offset > last);
            last = offset;
            ++used;
        }
        QCOMPARE(used, 8192);
        cb->resourceUpdate(slotBatch);
        renderer.issueSliceDraw(cb, 0, sliceOffset);
        renderer.issueSurfaceDraw(cb, &surface, BrainRenderer::Standard, last);
        // Overflowing draws are skipped without exhausting QRhi's pool of 64 update batches
        for (int i = 0; i < 100; ++i) {
            renderer.renderSurface(cb, rhi.get(), scene, &surface, BrainRenderer::Standard);
            renderer.renderDipoles(cb, rhi.get(), scene, &dipoles);
            renderer.renderPolyline(cb, rhi.get(), scene, &path);
            renderer.renderVideoOverlay(cb, rhi.get(), scene, &video);
            renderer.renderVideoOverlayOnSurface(cb, rhi.get(), scene, &video, &surface);
        }
        renderer.endPass(cb);
        QCOMPARE(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    }

    // A new colour texture rebuilds the targets; an invalidated merged group is rebuilt from the new surface list
    std::unique_ptr<QRhiTexture> smaller(rhi->newTexture(QRhiTexture::RGBA8, QSize(32, 24), 1, QRhiTexture::RenderTarget));
    QVERIFY(smaller->create());
    renderer.ensureRenderTargets(rhi.get(), smaller.get(), QSize(32, 24));
    QCOMPARE(renderer.rtClear()->pixelSize(), QSize(32, 24));
    renderer.invalidateMergedGroup(QStringLiteral("default"));
    QRhiResourceUpdateBatch* rebuild = rhi->nextResourceUpdateBatch();
    renderer.prepareMergedSurfaces(rhi.get(), rebuild, {}, QStringLiteral("default"));
    rebuild->release();
    QVERIFY(!renderer.hasMergedContent(QStringLiteral("default")));
}

//=============================================================================================================

void TestDisp3dBrainView::rayPicker_basics()
{
    // unproject — valid 800×600 viewport, identity PVM matrix
    QMatrix4x4 pvm; // identity
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

    RayHit result = RayPicker::pick(
        QVector3D(0, 0, 1),
        QVector3D(0, 0, -1),
        sv, surfaces, itemSurfaceMap, itemDipoleMap);
    QVERIFY(!result.hit);
    QCOMPARE(result.vertexIndex, -1);

    // buildLabel on a no-hit result must not crash
    QString label = RayPicker::buildLabel(result, itemSurfaceMap, surfaces);
    QVERIFY(label.isEmpty());

    // displayLabel on a no-hit RayHit
    RayHit noHit;
    QVERIFY(noHit.displayLabel().isEmpty());

    // A perspective camera at z = 5 looking down -z: the pane centre unprojects to the view axis,
    // and a corner ray passes through the point that projects onto that corner
    QMatrix4x4 proj;
    proj.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    QMatrix4x4 viewMat;
    viewMat.lookAt(QVector3D(0, 0, 5), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    const QMatrix4x4 camera = proj * viewMat;
    QVERIFY(RayPicker::unproject(QPoint(400, 300), paneRect, camera, origin, dir));
    QVERIFY((dir - QVector3D(0, 0, -1)).length() < 1e-4f);
    QVERIFY(std::fabs(origin.x()) < 1e-4f && std::fabs(origin.y()) < 1e-4f);
    const QVector3D target(0.5f, -0.4f, 1.0f);
    const QVector3D ndc = camera.map(target);
    const QPoint screen(qRound((ndc.x() + 1.0f) * 400.0f) + 100, qRound((1.0f - ndc.y()) * 300.0f) + 50);
    QVERIFY(RayPicker::unproject(screen, QRect(100, 50, 800, 600), camera, origin, dir));
    const QVector3D toTarget = (target - origin).normalized();
    QVERIFY(QVector3D::crossProduct(toTarget, dir).length() < 5e-3f);
    QVERIFY(!RayPicker::unproject(QPoint(0, 0), paneRect, QMatrix4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), origin, dir));

    // Two BEM planes at z = 1 and z = -1: a ray from z = 5 down -z hits the nearer one at distance 4
    const auto plane = [](float z) {
        Eigen::MatrixX3f rr(4, 3);
        rr << -1, -1, z, 1, -1, z, -1, 1, z, 1, 1, z;
        Eigen::MatrixX3i tris(2, 3);
        tris << 0, 1, 2, 1, 3, 2;
        auto surface = std::make_shared<BrainSurface>();
        surface->createFromData(rr, tris, Qt::gray);
        return surface;
    };
    surfaces.insert(QStringLiteral("bem_outer_skin"), plane(1.0f));
    surfaces.insert(QStringLiteral("bem_inner_skull"), plane(-1.0f));
    result = RayPicker::pick(QVector3D(0.2f, 0.1f, 5.0f), QVector3D(0, 0, -1), sv, surfaces, itemSurfaceMap, itemDipoleMap);
    QVERIFY(result.hit);
    QCOMPARE(result.surfaceKey, QStringLiteral("bem_outer_skin"));
    QVERIFY(std::fabs(result.distance - 4.0f) < 1e-4f);
    QVERIFY((result.hitPoint - QVector3D(0.2f, 0.1f, 1.0f)).length() < 1e-4f);
    QCOMPARE(RayPicker::buildLabel(result, itemSurfaceMap, surfaces), QStringLiteral("BEM: Outer skin"));
    QCOMPARE(result.displayLabel(), QStringLiteral("bem_outer_skin"));

    // Hidden surfaces are not picked; a ray beside the planes misses
    surfaces.value(QStringLiteral("bem_outer_skin"))->setVisible(false);
    result = RayPicker::pick(QVector3D(0.2f, 0.1f, 5.0f), QVector3D(0, 0, -1), sv, surfaces, itemSurfaceMap, itemDipoleMap);
    QCOMPARE(result.surfaceKey, QStringLiteral("bem_inner_skull"));
    QVERIFY(!RayPicker::pick(QVector3D(3.0f, 0.0f, 5.0f), QVector3D(0, 0, -1), sv, surfaces, itemSurfaceMap, itemDipoleMap).hit);

    // Labels for the other surface kinds
    RayHit hit;
    hit.hit = true;
    hit.surfaceKey = QStringLiteral("sens_surface_meg");
    QCOMPARE(RayPicker::buildLabel(hit, itemSurfaceMap, surfaces), QStringLiteral("MEG Helmet"));
    hit.surfaceKey = QStringLiteral("dig_cardinal");
    QCOMPARE(RayPicker::buildLabel(hit, itemSurfaceMap, surfaces), QStringLiteral("Digitizer (Cardinal)"));
    hit.surfaceKey = QStringLiteral("rh_pial");
    QCOMPARE(RayPicker::buildLabel(hit, itemSurfaceMap, surfaces), QStringLiteral("Right Hemisphere"));
    hit.regionName = QStringLiteral("precentral");
    QCOMPARE(RayPicker::buildLabel(hit, itemSurfaceMap, surfaces), QStringLiteral("Region: precentral (rh)"));
    QCOMPARE(hit.displayLabel(), QStringLiteral("Region: precentral (rh)"));
    hit.regionName.clear();
    hit.isDipole = true;
    hit.dipoleIndex = 3;
    QCOMPARE(RayPicker::buildLabel(hit, itemSurfaceMap, surfaces), QStringLiteral("Dipole (Dipole 3)"));
    QCOMPARE(hit.displayLabel(), QStringLiteral("Dipole 3"));

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
    nasion.kind = FIFFV_POINT_CARDINAL;
    nasion.ident = FIFFV_POINT_NASION;
    nasion.r[0] = 0.0f;
    nasion.r[1] = 0.08f;
    nasion.r[2] = 0.0f;
    pts << nasion;

    FIFFLIB::FiffDigPoint hpi;
    hpi.kind = FIFFV_POINT_HPI;
    hpi.ident = 1;
    hpi.r[0] = 0.01f;
    hpi.r[1] = 0.01f;
    hpi.r[2] = 0.06f;
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
        const MNELIB::MNEBemSurface& ref = bemItem.bemSurfaceData();
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
        BemTreeItem* bemItem = model.addBemSurface("SubjectA", "head", bemSurf);
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
        CONNECTIVITYLIB::Network net("Coherence");
        NetworkTreeItem* netItem = model.addNetwork(net, "TestNet");
        QVERIFY(netItem != nullptr);
        QCOMPARE(netItem->text(), QString("TestNet"));

        CONNECTIVITYLIB::Network net2("PLV");
        NetworkTreeItem* netItem2 = model.addNetwork(net2, "");
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
    verts << 0.0f, 0.0f, 0.0f,
        0.01f, 0.0f, 0.0f,
        0.0f, 0.01f, 0.0f;
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
    view.setSensorFieldVisible("INVALID", false); // early return path
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

    // With a scalp in the model the sample EEG field maps onto it and streams live
    {
        BrainTreeModel model;
        BrainView fieldView;
        fieldView.setModel(&model);
        MNELIB::MNEBemSurface scalp;
        scalp.id = FIFFV_BEM_SURF_ID_HEAD;
        scalp.rr.resize(6, 3);
        scalp.rr << 0.09f, 0.0f, 0.04f, -0.09f, 0.0f, 0.04f, 0.0f, 0.09f, 0.04f, 0.0f, -0.09f, 0.04f, 0.0f, 0.0f, 0.13f, 0.05f, 0.05f, 0.1f;
        scalp.itris.resize(4, 3);
        scalp.itris << 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0, 4;
        model.addBemSurface(QStringLiteral("sample"), QStringLiteral("head"), scalp);
        const QString avePath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
        QCOMPARE(BrainView::probeEvokedSets(avePath).size(), 4);
        QSignalSpy loaded(&fieldView, &BrainView::sensorFieldLoaded);
        QVERIFY(fieldView.loadSensorField(avePath, 0));
        QCOMPARE(loaded.size(), 1);
        QVERIFY(fieldView.sensorFieldTimeRange(tmin, tmax));
        QVERIFY(tmin < 0.0f && tmax > 0.0f);
        fieldView.startRealtimeSensorStreaming(QStringLiteral("EEG"));
        QVERIFY(fieldView.isRealtimeSensorStreaming());
        fieldView.stopRealtimeSensorStreaming();
        QVERIFY(!fieldView.isRealtimeSensorStreaming());

        // The scalp's top is the centre of its bounding box at maximal z; rays hit its faces
        QVector3D top;
        QVERIFY(fieldView.bemTopVertexInMri(top));
        QVERIFY((top - QVector3D(0.0f, 0.0f, 0.13f)).length() < 1e-6f);
        QVector3D hit;
        QVERIFY(fieldView.intersectWorldRay(QVector3D(0.01f, 0.01f, 1.0f), QVector3D(0.0f, 0.0f, -1.0f), hit));
        QVERIFY((hit - QVector3D(0.01f, 0.01f, 0.11f)).length() < 1e-5f);

        // Clearing the evoked stops streaming; clearing the BEM removes the scalp from the model and the scene
        fieldView.startRealtimeSensorStreaming(QStringLiteral("EEG"));
        fieldView.clearEvoked();
        QVERIFY(!fieldView.isRealtimeSensorStreaming());
        QVERIFY(!fieldView.sensorFieldTimeRange(tmin, tmax));
        fieldView.clearBem();
        QVERIFY(!fieldView.bemTopVertexInMri(top));
        QVERIFY(!fieldView.intersectWorldRay(QVector3D(0.01f, 0.01f, 1.0f), QVector3D(0.0f, 0.0f, -1.0f), hit));
        QVERIFY(model.findItems(QStringLiteral("head"), Qt::MatchRecursive).isEmpty());
    }

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
        worker.setMappingMatrix(QString(), nullMap);
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
        d1.time = 0.05f;
        d1.rd = Eigen::Vector3f(0.01f, 0.02f, 0.03f);
        d1.Q = Eigen::Vector3f(1e-9f, 0.0f, 0.0f);
        d1.good = 0.9f;
        d1.khi2 = 0.1f;
        d1.nfree = 3;
        d1.neval = 10;
        ecdSet.addEcd(d1);

        INVLIB::InvEcd d2 = d1;
        d2.rd = Eigen::Vector3f(-0.01f, 0.01f, 0.04f);
        d2.Q = Eigen::Vector3f(0.0f, 1e-9f, 0.0f);
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

    // applyTransform places the loaded dipoles; it replaces the previous transform instead of stacking on it,
    // so a transform can be re-applied whenever it changes
    QMatrix4x4 identity;
    obj.applyTransform(identity);
    QCOMPARE(obj.debugFirstDipolePosition(), pos);

    QMatrix4x4 translate;
    translate.translate(0.01f, 0.0f, 0.0f);
    obj.applyTransform(translate);
    obj.applyTransform(translate);
    QVERIFY((obj.debugFirstDipolePosition() - (pos + QVector3D(0.01f, 0.0f, 0.0f))).length() < 1e-7f);
    obj.applyTransform(identity);
    QVERIFY((obj.debugFirstDipolePosition() - pos).length() < 1e-7f);

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

void TestDisp3dBrainView::networkObject_thresholdAndInstances()
{
    NetworkObject obj;
    QVERIFY(!obj.hasData());
    QCOMPARE(obj.nodeInstanceCount(), 0);
    QCOMPARE(obj.edgeInstanceCount(), 0);

    // Chain 0 -- 1 -- 2 -- 3 with weights 0.2, 0.5, 1.0 and an isolated node 4
    CONNECTIVITYLIB::Network network(QStringLiteral("Coherence"));
    QList<CONNECTIVITYLIB::NetworkNode::SPtr> nodes;
    for (int i = 0; i < 5; ++i) {
        Eigen::RowVectorXf vert(3);
        vert << 0.01f * static_cast<float>(i), 0.0f, 0.05f;
        nodes.append(CONNECTIVITYLIB::NetworkNode::SPtr::create(static_cast<qint16>(i), vert));
        network.append(nodes.last());
    }
    const double weights[] = {0.2, 0.5, 1.0};
    for (int i = 0; i < 3; ++i) {
        Eigen::MatrixXd weight(1, 1);
        weight(0, 0) = weights[i];
        auto edge = CONNECTIVITYLIB::NetworkEdge::SPtr::create(i, i + 1, weight);
        network.append(edge);
        nodes[i]->append(edge);
        nodes[i + 1]->append(edge);
    }

    obj.load(network, QStringLiteral("Jet"));
    QVERIFY(obj.hasData());
    QVERIFY(obj.nodeIndexCount() > 0 && obj.nodeIndexCount() % 3 == 0);
    QVERIFY(obj.edgeIndexCount() > 0 && obj.edgeIndexCount() % 3 == 0);

    // Only connected nodes and kept edges are instanced; the threshold drops weak edges
    obj.setThreshold(0.0);
    QCOMPARE(obj.nodeInstanceCount(), 4);
    QCOMPARE(obj.edgeInstanceCount(), 3);
    obj.setThreshold(0.4);
    QCOMPARE(obj.edgeInstanceCount(), 2);
    QCOMPARE(obj.nodeInstanceCount(), 3);
    obj.setThreshold(0.9);
    QCOMPARE(obj.edgeInstanceCount(), 1);
    QCOMPARE(obj.nodeInstanceCount(), 2);

    // A colour map change keeps the instances
    obj.setColormap(QStringLiteral("Hot"));
    QCOMPARE(obj.edgeInstanceCount(), 1);

    obj.setVisible(false);
    QVERIFY(!obj.isVisible());
}

//=============================================================================================================

void TestDisp3dBrainView::sourceEstimateOverlay_colorsSurface()
{
    SourceEstimateOverlay overlay;
    QVERIFY(!overlay.isLoaded());
    QCOMPARE(overlay.numTimePoints(), 0);
    QVERIFY(!overlay.loadStc(QStringLiteral("/nonexistent-lh.stc"), 0));

    // Left hemisphere: sources at vertices 0 and 2 of a 4-vertex surface, two time points
    Eigen::MatrixXd data(2, 2);
    data << 1.0, -4.0,
        4.0, 2.0;
    Eigen::VectorXi vertices(2);
    vertices << 0, 2;
    overlay.setStcData(INVLIB::InvSourceEstimate(data, vertices, -0.1f, 0.01f), 0);
    QVERIFY(overlay.isLoaded());
    QCOMPARE(overlay.numTimePoints(), 2);
    QCOMPARE(overlay.tmin(), -0.1f);
    QCOMPARE(overlay.tstep(), 0.01f);
    QVERIFY(std::fabs(overlay.timeAtIndex(1) - (-0.09f)) < 1e-6f);

    // The automatic range spans the magnitudes the overlay colours: |data| from 1 to 4
    overlay.updateThresholdsFromData();
    QCOMPARE(overlay.thresholdMin(), 1.0f);
    QCOMPARE(overlay.thresholdMid(), 2.5f);
    QCOMPARE(overlay.thresholdMax(), 4.0f);

    Eigen::MatrixX3f rr(4, 3);
    rr << 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0;
    Eigen::MatrixX3i tris(2, 3);
    tris << 0, 1, 2, 1, 3, 2;
    BrainSurface surface;
    surface.createFromData(rr, tris, Qt::gray);
    surface.setHemi(0);

    // Time 0: vertex 2 is at the maximum (opaque, top of the map); vertex 0 at the minimum is transparent
    overlay.setColormap(QStringLiteral("Hot"));
    overlay.applyToSurface(&surface, 0);
    const auto& vertexData = surface.vertexDataRef();
    const QRgb top = DISPLIB::ColorMap::valueToColor(1.0, QStringLiteral("Hot"));
    const uint32_t vertex2 = vertexData[2].color;
    QCOMPARE(static_cast<int>(vertex2 & 0xFFu), qRed(top));
    QCOMPARE(static_cast<int>((vertex2 >> 8) & 0xFFu), qGreen(top));
    QCOMPARE(static_cast<int>((vertex2 >> 16) & 0xFFu), qBlue(top));
    QCOMPARE(vertexData[0].color & 0xFFFFFFu, vertexData[1].color & 0xFFFFFFu);

    // Time 1 uses |-4| = 4 for vertex 0; out-of-range indices are clamped to the last time
    overlay.applyToSurface(&surface, 99);
    QCOMPARE(surface.vertexDataRef()[0].color & 0xFFFFFFu, vertex2 & 0xFFFFFFu);

    // The data column concatenates both hemispheres
    Eigen::MatrixXd rhData(1, 2);
    rhData << 5.0, 6.0;
    Eigen::VectorXi rhVertices(1);
    rhVertices << 1;
    overlay.setStcData(INVLIB::InvSourceEstimate(rhData, rhVertices, -0.1f, 0.01f), 1);
    const Eigen::VectorXd column = overlay.sourceDataColumn(1);
    QCOMPARE(column.size(), Eigen::Index(3));
    QCOMPARE(column(0), -4.0);
    QCOMPARE(column(2), 6.0);
    double minVal = 0.0;
    double maxVal = 0.0;
    overlay.getDataRange(minVal, maxVal);
    QCOMPARE(maxVal, 6.0);

    // A surface of the other hemisphere without data stays untouched
    BrainSurface other;
    other.createFromData(rr, tris, Qt::gray);
    other.setHemi(2);
    const uint32_t before = other.vertexDataRef()[0].color;
    overlay.applyToSurface(&other, 0);
    QCOMPARE(other.vertexDataRef()[0].color, before);
    overlay.applyToSurface(nullptr, 0);
}

//=============================================================================================================

void TestDisp3dBrainView::sliceObject_cornersAndQuad()
{
    // 2 mm voxels, origin at (-10, -20, -30) mm
    Eigen::Matrix4d voxelToWorld = Eigen::Matrix4d::Identity();
    voxelToWorld.topLeftCorner<3, 3>() *= 2.0;
    voxelToWorld.topRightCorner<3, 1>() << -10.0, -20.0, -30.0;
    const QImage image(4, 3, QImage::Format_Grayscale8);

    // Quad vertex i: position (x, y, z) at floats 5i..5i+2, UV at 5i+3, 5i+4; corners 00, 10, 01, 11
    const auto corners = [](const SliceObject& slice) {
        QVector<float> vertices;
        slice.generateQuadVertices(vertices);
        QVector<QVector3D> points;
        for (int i = 0; i < 4; ++i) {
            points.append(QVector3D(vertices[5 * i], vertices[5 * i + 1], vertices[5 * i + 2]));
        }
        return points;
    };

    SliceObject slice;
    // Axial slice 5: columns along x, rows along y, at z = -30 + 2 * 5
    slice.setSlice(image, SliceOrientation::Axial, 5, voxelToWorld);
    QCOMPARE(slice.orientation(), SliceOrientation::Axial);
    QCOMPARE(slice.sliceIndex(), 5);
    QCOMPARE(slice.image().size(), QSize(4, 3));
    QVector<QVector3D> points = corners(slice);
    QCOMPARE(points[0], QVector3D(-10.0f, -20.0f, -20.0f));
    QCOMPARE(points[1], QVector3D(-2.0f, -20.0f, -20.0f));
    QCOMPARE(points[2], QVector3D(-10.0f, -14.0f, -20.0f));
    QCOMPARE(points[3], QVector3D(-2.0f, -14.0f, -20.0f));
    // The unit quad maps onto the same corners
    QCOMPARE(slice.sliceToWorld().map(QVector3D(1.0f, 1.0f, 0.0f)), points[3]);

    // Coronal: columns along x, rows along z; sagittal: columns along y, rows along z
    slice.setSlice(image, SliceOrientation::Coronal, 1, voxelToWorld);
    points = corners(slice);
    QCOMPARE(points[0], QVector3D(-10.0f, -18.0f, -30.0f));
    QCOMPARE(points[3], QVector3D(-2.0f, -18.0f, -24.0f));
    slice.setSlice(image, SliceOrientation::Sagittal, 2, voxelToWorld);
    points = corners(slice);
    QCOMPARE(points[0], QVector3D(-6.0f, -20.0f, -30.0f));
    QCOMPARE(points[3], QVector3D(-6.0f, -12.0f, -24.0f));

    // An explicit image-to-world transform places the image plane directly
    slice.setSliceToWorld(image, SliceOrientation::Axial, 0, voxelToWorld);
    points = corners(slice);
    QCOMPARE(points[3], QVector3D(-2.0f, -14.0f, -30.0f));

    QVector<float> vertices;
    slice.generateQuadVertices(vertices);
    QCOMPARE(vertices.size(), 20);
    QCOMPARE(vertices[3 * 5 + 3], 1.0f);
    QCOMPARE(vertices[3 * 5 + 4], 1.0f);
    QVector<unsigned int> indices;
    SliceObject::generateQuadIndices(indices);
    QCOMPARE(indices.size(), 6);
    for (unsigned int index : std::as_const(indices)) {
        QVERIFY(index < 4);
    }

    slice.setWindowLevel(0.4f, 0.3f);
    QCOMPARE(slice.windowCenter(), 0.4f);
    QCOMPARE(slice.windowWidth(), 0.3f);
    slice.setOpacity(0.25f);
    QCOMPARE(slice.opacity(), 0.25f);
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_clearRemovesObjectsAndRows()
{
    const QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/");
    BrainTreeModel model;
    BrainView view;
    view.setModel(&model);
    view.showSingleView();

    const auto countItems = [&model](AbstractTreeItem::ItemType type) {
        int count = 0;
        QList<QStandardItem*> stack{model.invisibleRootItem()};
        while (!stack.isEmpty()) {
            QStandardItem* item = stack.takeLast();
            count += item->type() == AbstractTreeItem::itemTypeId(type);
            for (int r = 0; r < item->rowCount(); ++r) {
                stack << item->child(r);
            }
        }
        return count;
    };
    const auto loadAll = [&] {
        for (const QString& hemi : {QStringLiteral("lh"), QStringLiteral("rh")}) {
            model.addSurface(QStringLiteral("sample"), hemi, QStringLiteral("white"),
                             FSLIB::FsSurface(dataDir + QStringLiteral("subjects/sample/surf/%1.white").arg(hemi)));
        }
        QFile bemFile(dataDir + QStringLiteral("subjects/sample/bem/sample-5120-bem.fif"));
        MNELIB::MNEBem bem(bemFile);
        QVERIFY(bem.size() > 0);
        model.addBemSurface(QStringLiteral("sample"), QStringLiteral("inner_skull"), bem[0]);
        QVERIFY(view.loadSensors(dataDir + QStringLiteral("MEG/sample/sample_audvis-ave.fif")));
        QVERIFY(view.loadSourceSpace(dataDir + QStringLiteral("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif")));
    };
    loadAll();
    const int sensors = countItems(AbstractTreeItem::SensorItem);
    const int sourceSpaces = countItems(AbstractTreeItem::SourceSpaceItem);
    QCOMPARE(countItems(AbstractTreeItem::SurfaceItem), 2);
    QCOMPARE(countItems(AbstractTreeItem::BemItem), 1);
    QVERIFY(sensors > 0 && sourceSpaces > 0);

    view.clearSurfaces();
    QCOMPARE(countItems(AbstractTreeItem::SurfaceItem), 0);
    QCOMPARE(countItems(AbstractTreeItem::BemItem), 1);
    view.clearBem();
    QCOMPARE(countItems(AbstractTreeItem::BemItem), 0);
    QCOMPARE(countItems(AbstractTreeItem::SensorItem), sensors);
    view.clearSensors();
    QCOMPARE(countItems(AbstractTreeItem::SensorItem), 0);
    QCOMPARE(countItems(AbstractTreeItem::SourceSpaceItem), sourceSpaces);
    view.clearSourceSpace();
    QCOMPARE(countItems(AbstractTreeItem::SourceSpaceItem), 0);

    // Nothing is left to pick, and the same data loads again
    QSignalSpy hovered(&view, &BrainView::hoveredRegionChanged);
    view.castRay(view.rect().center());
    QVERIFY(hovered.isEmpty() || hovered.last().at(0).toString().isEmpty());
    loadAll();
    QCOMPARE(countItems(AbstractTreeItem::SurfaceItem), 2);
    QCOMPARE(countItems(AbstractTreeItem::SensorItem), sensors);
    QCOMPARE(countItems(AbstractTreeItem::SourceSpaceItem), sourceSpaces);
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_clickVersusDrag()
{
    const QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/");
    BrainTreeModel model;
    BrainView view;
    view.setModel(&model);
    view.resize(800, 600);
    // Start from the default panes, not the layout earlier tests persisted
    view.resetAllSubViewState();
    view.resetMultiViewLayout();
    view.setViewCount(4);
    const FSLIB::FsSurface lh(dataDir + QStringLiteral("subjects/sample/surf/lh.white"));
    SurfaceTreeItem* lhItem = model.addSurface(QStringLiteral("sample"), QStringLiteral("lh"), QStringLiteral("white"), lh);
    // Aim the view centre at the hemisphere, not at the gap between hemispheres
    const Eigen::Vector3f centroid = lh.rr().colwise().mean();
    view.setCameraFocusOverride(QVector3D(centroid.x(), centroid.y(), centroid.z()), 0.1f);
    QSignalSpy clicked(&view, &BrainView::surfacePointClicked);
    const auto send = [&view](QEvent::Type type, const QPoint& pos, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, view.mapToGlobal(pos), type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons, Qt::NoModifier);
        QApplication::sendEvent(&view, &event);
    };
    QPoint centre;
    const auto dragAndRelease = [&](bool drag) {
        send(QEvent::MouseButtonPress, centre, Qt::LeftButton);
        if (drag) {
            send(QEvent::MouseMove, centre + QPoint(40, 0), Qt::LeftButton);
            send(QEvent::MouseMove, centre, Qt::LeftButton);
        }
        send(QEvent::MouseButtonRelease, centre, Qt::NoButton);
    };

    // Single view (edit target -1), then the planar top-left pane (preset Top) and the perspective
    // top-right pane of the default 2x2 layout
    for (const int pane : {-1, 0, 1}) {
        if (pane < 0) {
            view.showSingleView();
        } else {
            view.showMultiView();
        }
        view.setVisualizationEditTarget(pane);
        view.setActiveSurface(QStringLiteral("white"));
        view.setHemiVisible(0, true);
        centre = (pane < 0) ? view.rect().center() : QPoint(view.width() * (1 + 2 * pane) / 4, view.height() / 4);
        clicked.clear();
        dragAndRelease(false);
        QCOMPARE(clicked.size(), 1);
        clicked.clear();
        dragAndRelease(true);
        QVERIFY2(clicked.isEmpty(), qPrintable(QString::number(pane)));
    }

    // Unchecking the surface in the tree hides it from picking
    lhItem->setVisible(false);
    clicked.clear();
    dragAndRelease(false);
    QVERIFY(clicked.isEmpty());
    lhItem->setVisible(true);
    dragAndRelease(false);
    QCOMPARE(clicked.size(), 1);

    // A double click on the surface reports its point; off the surface it falls through to the widget
    QSignalSpy doubleClicked(&view, &BrainView::surfacePointDoubleClicked);
    send(QEvent::MouseButtonDblClick, centre, Qt::LeftButton);
    QCOMPARE(doubleClicked.size(), 1);
    QCOMPARE(doubleClicked.first().first().value<QVector3D>(), clicked.last().first().value<QVector3D>());
    send(QEvent::MouseButtonDblClick, QPoint(2, 2), Qt::LeftButton);
    QCOMPARE(doubleClicked.size(), 1);

    view.showSingleView();
    view.setVisualizationEditTarget(-1);

    // Switching models drops the old model's objects and stops following it; switching back shows them again
    centre = view.rect().center();
    BrainTreeModel emptyModel;
    view.setModel(&emptyModel);
    lhItem->setVisible(false);
    lhItem->setVisible(true);
    clicked.clear();
    dragAndRelease(false);
    QVERIFY(clicked.isEmpty());
    view.setModel(&model);
    dragAndRelease(false);
    QCOMPARE(clicked.size(), 1);

    // The single view chosen with four panes configured opens again as the single view, and multi view as multi
    QCOMPARE(BrainView().viewMode(), BrainView::SingleView);
    QCOMPARE(BrainView().viewCount(), 4);
    view.showMultiView();
    QCOMPARE(BrainView().viewMode(), BrainView::MultiView);
    view.showSingleView();

    // Wheel zoom in the single view: zooming out moves the hemisphere's right edge towards the centre,
    // and the zoom is restored like the rotation when the view is opened again
    const auto rightEdge = [&](BrainView& target) {
        QSignalSpy hits(&target, &BrainView::surfacePointClicked);
        const int y = target.height() / 2;
        for (int x = target.width() / 2; x < target.width(); x += 2) {
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(x, y), target.mapToGlobal(QPointF(x, y)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(x, y), target.mapToGlobal(QPointF(x, y)), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(&target, &press);
            QApplication::sendEvent(&target, &release);
            if (hits.isEmpty()) {
                return x;
            }
            hits.clear();
        }
        return target.width();
    };
    const int edgeBefore = rightEdge(view);
    QWheelEvent zoomOut(QPointF(400, 300), view.mapToGlobal(QPointF(400, 300)), QPoint(), QPoint(0, -720), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&view, &zoomOut);
    const int edgeAfter = rightEdge(view);
    QVERIFY2(edgeAfter < edgeBefore - 20, qPrintable(QStringLiteral("%1 -> %2").arg(edgeBefore).arg(edgeAfter)));

    BrainView reopened;
    reopened.setModel(&model);
    reopened.resize(800, 600);
    reopened.setCameraFocusOverride(QVector3D(centroid.x(), centroid.y(), centroid.z()), 0.1f);
    QCOMPARE(rightEdge(reopened), edgeAfter);
    view.resetSingleViewCameraState();
    QCOMPARE(rightEdge(view), edgeBefore);
}

//=============================================================================================================

void TestDisp3dBrainView::brainView_headMovementPath()
{
    BrainView view;

    // A path needs at least one segment, so these must be treated as "clear"
    // rather than building a degenerate network.
    view.setHeadMovementPath({});
    view.setHeadMovementPath({Eigen::Vector3f(0.0f, 0.0f, 0.0f)});

    // A realistic short drift: a few centimetres over several fits.
    QVector<Eigen::Vector3f> vecPositions;
    for (int i = 0; i < 8; ++i) {
        vecPositions.append(Eigen::Vector3f(0.001f * i, 0.002f * i, 0.05f + 0.001f * i));
    }
    view.setHeadMovementPath(vecPositions);

    // Replacing an existing path must not leak or crash.
    view.setHeadMovementPath(vecPositions);

    view.clearHeadMovementPath();
    // Clearing twice is a no-op, not a double free.
    view.clearHeadMovementPath();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDisp3dBrainView::polylineObject_segments()
{
    // The geometry and instance maths need no GPU, so they can be checked
    // directly. Driving this through BrainView only proves it does not crash.
    PolylineObject polyline;

    // Too few points to form a segment means nothing to draw.
    QVERIFY(!polyline.hasData());
    QCOMPARE(polyline.instanceCount(), 0);

    polyline.setPoints({Eigen::Vector3f(0.0f, 0.0f, 0.0f)});
    QCOMPARE(polyline.instanceCount(), 0);

    // N points must give exactly N-1 segments.
    QVector<Eigen::Vector3f> vecPoints;
    for (int i = 0; i < 8; ++i) {
        vecPoints.append(Eigen::Vector3f(0.001f * i, 0.0f, 0.0f));
    }
    polyline.setPoints(vecPoints);
    QCOMPARE(polyline.instanceCount(), 7);
    QVERIFY(polyline.hasData());

    // A stationary subject produces repeated positions. Those segments have no
    // direction to orient by and must be dropped rather than turned into a NaN
    // rotation.
    polyline.setPoints({Eigen::Vector3f(0.0f, 0.0f, 0.0f),
                        Eigen::Vector3f(0.0f, 0.0f, 0.0f),
                        Eigen::Vector3f(0.0f, 0.0f, 0.01f)});
    QCOMPARE(polyline.instanceCount(), 1);

    // A path made only of repeated positions collapses to nothing drawable.
    polyline.setPoints({Eigen::Vector3f(0.02f, 0.0f, 0.0f),
                        Eigen::Vector3f(0.02f, 0.0f, 0.0f)});
    QCOMPARE(polyline.instanceCount(), 0);
    QVERIFY(!polyline.hasData());

    // Well past the qint16 range that the previous network based
    // implementation keyed nodes by, to show the index type no longer caps the
    // path length.
    QVector<Eigen::Vector3f> vecLong;
    for (int i = 0; i < 40000; ++i) {
        vecLong.append(Eigen::Vector3f(0.0f, 0.0f, 1.0e-6f * i));
    }
    polyline.setPoints(vecLong);
    QCOMPARE(polyline.instanceCount(), 39999);

    polyline.clear();
    QCOMPARE(polyline.instanceCount(), 0);
    QVERIFY(!polyline.hasData());
}

//=============================================================================================================

QTEST_MAIN(TestDisp3dBrainView)
#include "test_disp3d_brainview.moc"
