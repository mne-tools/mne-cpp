//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Disp3D scene data: sensor-to-surface interpolation, electrodes, MRI slices, video overlay and layers.
 *
 * Runs without a GPU: it exercises the CPU side that feeds the renderer.
 * Geodesic distances on a 4 x 4 grid equal scipy.sparse.csgraph.dijkstra
 * (Manhattan distances); everything else is a closed form. Exits non-zero on
 * any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/helpers/geometryinfo.h>
#include <disp3D/helpers/interpolation.h>
#include <disp3D/renderable/brainsurface.h>
#include <disp3D/renderable/dipoleobject.h>
#include <disp3D/renderable/electrodeobject.h>
#include <disp3D/renderable/networkobject.h>
#include <disp3D/renderable/polylineobject.h>
#include <disp3D/renderable/sliceobject.h>
#include <disp3D/renderable/sourceestimateoverlay.h>
#include <disp3D/renderable/videooverlay.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/sensorfieldmapper.h>
#include <disp3D/view/brainview.h>
#include <disp3D/view/multiviewlayout.h>

#include <connectivity/network/network.h>
#include <connectivity/network/networkedge.h>
#include <connectivity/network/networknode.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/inv_source_estimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QDebug>
#include <QImage>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace CONNECTIVITYLIB;
using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv); // BrainView is a widget
    bool ok = true;

    // 4 x 4 vertex grid with 1 mm spacing, 4-connected; vertex v = 4 * row + column
    MatrixX3f vertices(16, 3);
    std::vector<VectorXi> neighbors(16);
    for (int v = 0; v < 16; ++v) {
        const int i = v % 4;
        const int j = v / 4;
        vertices.row(v) << static_cast<float>(i), static_cast<float>(j), 0.0f;
        std::vector<int> nb;
        if (i > 0)
            nb.push_back(v - 1);
        if (i < 3)
            nb.push_back(v + 1);
        if (j > 0)
            nb.push_back(v - 4);
        if (j < 3)
            nb.push_back(v + 4);
        neighbors[v] = Map<VectorXi>(nb.data(), static_cast<Index>(nb.size()));
    }

    //! [geometry_info_usage]
    MatrixX3f sensors(2, 3);
    sensors << -0.2f, 0.1f, 0.5f,                                                            // above vertex 0
        3.1f, 2.9f, 0.4f;                                                                    // above vertex 15
    VectorXi projected = GeometryInfo::projectSensors(vertices, sensors);                    // nearest vertex per sensor
    QSharedPointer<MatrixXd> distances = GeometryInfo::scdc(vertices, neighbors, projected); // geodesic: n_vertices x n_sensors
    //! [geometry_info_usage]
    // scipy.sparse.csgraph.dijkstra on the same graph: Manhattan distances to vertices 0 and 15
    ok &= expect(projected == Vector2i(0, 15) && distances->rows() == 16 && (*distances)(5, 0) == 2.0 && (*distances)(5, 1) == 4.0 && (*distances)(3, 0) == 3.0 && (*distances)(3, 1) == 3.0,
                 "GeometryInfo projects the sensors onto vertices 0 and 15; geodesic distances match scipy");

    //! [interpolation_usage]
    QSharedPointer<SparseMatrix<float>> weights = Interpolation::createInterpolationMat(projected, distances, Interpolation::linear); // inverse-distance weights
    const VectorXf atVertices = Interpolation::interpolateSignal(*weights, Vector2f(1.0f, 4.0f));                                     // one value per sensor
    //! [interpolation_usage]
    // vertex 5: weights 1/2 and 1/4, normalised to 2/3 and 1/3 -> 1 * 2/3 + 4 * 1/3 = 2; vertex 3 is equidistant -> 2.5
    ok &= expect(std::fabs(atVertices(0) - 1.0f) < 1e-6f && std::fabs(atVertices(15) - 4.0f) < 1e-6f && std::fabs(atVertices(5) - 2.0f) < 1e-6f && std::fabs(atVertices(3) - 2.5f) < 1e-6f,
                 QString("Interpolation: sensors kept, vertex 5 = %1, vertex 3 = %2").arg(atVertices(5)).arg(atVertices(3)));

    //! [electrode_object_usage]
    ElectrodeArray shaft;
    shaft.label = "LH";
    shaft.layout = ElectrodeLayout::Depth;
    for (int k = 0; k < 4; ++k) {
        ElectrodeContact contact;
        contact.name = QString("LH%1").arg(k + 1);
        contact.position = QVector3D(10.0f + 3.5f * k, -20.0f, 5.0f); // surface RAS, mm
        shaft.contacts.append(contact);
    }
    ElectrodeObject electrodes;
    electrodes.setArrays({shaft});
    electrodes.selectContact("LH3");
    QVector<float> instances; // per contact: position, radius, RGBA, selected = 9 floats
    electrodes.generateContactInstances(instances);
    //! [electrode_object_usage]
    ok &= expect(electrodes.totalContactCount() == 4 && electrodes.selectedContact() == "LH3" && instances.size() == 36 && instances[2 * 9] == 17.0f && instances[2 * 9 + 8] == 1.0f && instances[9 + 8] == 0.0f,
                 "ElectrodeObject: 4 contacts, 9 floats each, only LH3 flagged as selected");

    //! [slice_object_usage]
    QImage axial(256, 256, QImage::Format_Grayscale8);
    axial.fill(128);
    Matrix4d voxelToWorld = Matrix4d::Identity(); // 1 mm voxels, origin at the volume centre
    voxelToWorld.block<3, 1>(0, 3) = Vector3d(-128.0, -128.0, -128.0);
    SliceObject slice;
    slice.setSlice(axial, SliceOrientation::Axial, 128, voxelToWorld); // the z = 0 mm plane
    slice.setWindowLevel(100.0f, 200.0f);
    QVector<float> quad; // 4 corners x (x, y, z, u, v)
    slice.generateQuadVertices(quad);
    //! [slice_object_usage]
    ok &= expect(quad.size() == 20 && quad[0] == -128.0f && quad[1] == -128.0f && quad[2] == 0.0f && quad[15] == 128.0f && quad[16] == 128.0f && quad[17] == 0.0f && quad[18] == 1.0f && slice.windowWidth() == 200.0f,
                 "SliceObject: the axial slice 128 spans [-128, 128] mm in x and y at z = 0");

    //! [video_overlay_usage]
    VideoOverlay overlay; // CPU-side state; the renderer uploads frame() when frameGeneration() changes
    overlay.setEnabled(true);
    overlay.setFocusPosition(QVector3D(0.0f, 0.0f, 0.08f));
    overlay.setSizeMeters(0.05f);
    QImage frame(64, 48, QImage::Format_RGB32);
    frame.fill(Qt::darkGreen);
    overlay.setFrame(frame);
    overlay.setFrame(QImage()); // null frames are ignored
    //! [video_overlay_usage]
    ok &= expect(overlay.isEnabled() && overlay.hasFrame() && overlay.frameGeneration() == 1 && overlay.frame().size() == QSize(64, 48),
                 "VideoOverlay keeps the first frame and ignores the null one");

    //! [multimodal_scene_usage]
    MultimodalScene scene;
    scene.registerBoundsFn(SceneLayerKind::Electrode, [](const SceneLayer& layer, QVector3D& bbMin, QVector3D& bbMax) {
        const auto* object = static_cast<const ElectrodeObject*>(layer.payload.get());
        bbMin = bbMax = object->arrays().first().contacts.first().position;
        for (const ElectrodeContact& c : object->arrays().first().contacts) {
            bbMin = QVector3D(std::min(bbMin.x(), c.position.x()), std::min(bbMin.y(), c.position.y()), std::min(bbMin.z(), c.position.z()));
            bbMax = QVector3D(std::max(bbMax.x(), c.position.x()), std::max(bbMax.y(), c.position.y()), std::max(bbMax.z(), c.position.z()));
        }
        return true;
    });
    SceneLayer layer;
    layer.id = "seeg_LH";
    layer.kind = SceneLayerKind::Electrode;
    auto payload = std::make_shared<ElectrodeObject>(); // owns GPU buffers, so it is not copyable
    payload->setArrays(electrodes.arrays());
    layer.payload = payload;
    scene.addLayer(layer);
    scene.setOverlayThresholds(2.0f, 4.0f, 8.0f);
    QVector3D bbMin;
    QVector3D bbMax;
    scene.worldBounds(bbMin, bbMax); // union over visible layers with a bounds function
    //! [multimodal_scene_usage]
    ok &= expect(scene.layers().size() == 1 && bbMin == QVector3D(10.0f, -20.0f, 5.0f) && bbMax == QVector3D(20.5f, -20.0f, 5.0f) && scene.overlayFmid() == 4.0f,
                 "MultimodalScene bounds equal the electrode extent");
    scene.setLayerVisible("seeg_LH", false);
    scene.worldBounds(bbMin, bbMax);
    ok &= expect(bbMin == QVector3D(-1.0f, -1.0f, -1.0f) && bbMax == QVector3D(1.0f, 1.0f, 1.0f), "Hiding the only layer falls back to the unit cube");

    //! [pick_result_usage]
    // A picker reports the contact under the cursor; the host reads the most recent pick back
    const bool hitBefore = isHit(scene.lastPick());
    PickResult contactPick;
    contactPick.kind = PickKind::ElectrodeContact;
    contactPick.sourceId = "seeg_LH";
    contactPick.world = electrodes.arrays().first().contacts.first().position;
    contactPick.label = electrodes.arrays().first().contacts.first().name;
    contactPick.value = 3.5f;
    contactPick.timeSample = 120;
    scene.reportPick(contactPick);
    const PickResult& lastPick = scene.lastPick();
    //! [pick_result_usage]
    ok &= expect(!hitBefore && isHit(lastPick) && lastPick.sourceId == "seeg_LH" && lastPick.world == QVector3D(10.0f, -20.0f, 5.0f) && lastPick.value == 3.5f &&
                     lastPick.objectId == -1,
                 "MultimodalScene keeps the reported contact pick");

    //! [brain_surface_usage]
    // A 2 x 2 cm flat patch at z = 0 (metres), picked by a ray cast straight down
    MatrixX3f patchVertices(4, 3);
    patchVertices << 0.0f, 0.0f, 0.0f, 0.02f, 0.0f, 0.0f, 0.02f, 0.02f, 0.0f, 0.0f, 0.02f, 0.0f;
    MatrixX3i patchTriangles(2, 3);
    patchTriangles << 0, 1, 2, 0, 2, 3;
    BrainSurface patch;
    patch.createFromData(patchVertices, patchTriangles, Qt::gray);
    patch.setHemi(0);
    float hitDistance = 0.0f;
    int hitVertex = -1;
    const bool picked = patch.intersects(QVector3D(0.019f, 0.019f, 0.05f), QVector3D(0.0f, 0.0f, -1.0f), hitDistance, hitVertex);
    const VertexData& corner = patch.vertexDataRef().at(2); // interleaved position, normal and packed ABGR colour
    //! [brain_surface_usage]
    ok &= expect(picked && std::abs(hitDistance - 0.05f) < 1e-6f && hitVertex == 2 && corner.pos == QVector3D(0.02f, 0.02f, 0.0f) && std::abs(std::abs(corner.norm.z()) - 1.0f) < 1e-6f,
                 "BrainSurface: a downward ray hits the patch 5 cm below, nearest vertex 2");

    //! [source_estimate_overlay_usage]
    // Source estimate on vertices 0 and 2 only; the others stay cortex grey
    MatrixXd activity(2, 1);
    activity << 1.0, 0.1;
    SourceEstimateOverlay sourceOverlay;
    sourceOverlay.setStcData(InvSourceEstimate(activity, (VectorXi(2) << 0, 2).finished(), 0.0f, 0.001f), 0);
    sourceOverlay.setThresholds(0.2f, 0.5f, 1.0f);
    sourceOverlay.applyToSurface(&patch, 0); // no interpolation matrix: values land on their own vertices
    //! [source_estimate_overlay_usage]
    const uint32_t grey = packABGR(0xAA, 0xAA, 0xAA) & 0x00FFFFFFu;
    const uint32_t peak = patch.vertexDataRef().at(0).color & 0x00FFFFFFu;
    ok &= expect(sourceOverlay.numTimePoints() == 1 && peak == (packABGR(255, 255, 255) & 0x00FFFFFFu) && (patch.vertexDataRef().at(2).color & 0x00FFFFFFu) == grey &&
                     (patch.vertexDataRef().at(1).color & 0x00FFFFFFu) == grey,
                 "SourceEstimateOverlay: the peak is white on Hot, sub-threshold vertices keep the cortex grey");

    //! [dipole_object_usage]
    InvEcdSet dipoles;
    InvEcd strong;
    strong.rd = Vector3f(0.0f, 0.0f, 0.05f);
    strong.Q = Vector3f(0.0f, 0.0f, 20e-9f);
    InvEcd weak;
    weak.rd = Vector3f(0.03f, 0.0f, 0.05f);
    weak.Q = Vector3f(10e-9f, 0.0f, 0.0f);
    dipoles.addEcd(strong);
    dipoles.addEcd(weak);
    DipoleObject dipoleGlyphs;
    dipoleGlyphs.load(dipoles); // one cone per dipole, scaled by moment
    float dipoleDistance = 0.0f;
    const int pickedDipole = dipoleGlyphs.intersect(QVector3D(0.03f, 0.0f, 0.2f), QVector3D(0.0f, 0.0f, -1.0f), dipoleDistance);
    //! [dipole_object_usage]
    ok &= expect(dipoleGlyphs.instanceCount() == 2 && pickedDipole == 1 && dipoleDistance > 0.13f && dipoleDistance < 0.15f,
                 QString("DipoleObject: a ray through the second dipole picks index %1 at %2 m").arg(pickedDipole).arg(dipoleDistance));

    //! [network_object_usage]
    Network graph("coherence");
    for (int i = 0; i < 3; ++i) {
        graph.append(NetworkNode::SPtr(new NetworkNode(i, RowVectorXf::Constant(3, 0.01f * static_cast<float>(i)))));
    }
    const QList<QPair<int, int>> links = {{0, 1}, {1, 2}};
    const QList<double> strengths = {0.9, 0.3};
    for (int k = 0; k < links.size(); ++k) {
        NetworkEdge::SPtr edge(new NetworkEdge(links[k].first, links[k].second, MatrixXd::Constant(1, 1, strengths[k])));
        graph.getNodeAt(links[k].first)->append(edge);
        graph.getNodeAt(links[k].second)->append(edge);
        graph.append(edge);
    }
    NetworkObject networkGlyphs;
    networkGlyphs.load(graph, "Viridis"); // spheres for nodes, cylinders for edges
    const int edgesAll = networkGlyphs.edgeInstanceCount();
    networkGlyphs.setThreshold(0.5); // keep only the strong 0-1 link
    //! [network_object_usage]
    ok &= expect(edgesAll == 2 && networkGlyphs.edgeInstanceCount() == 1 && networkGlyphs.nodeInstanceCount() == 2,
                 "NetworkObject: thresholding at 0.5 leaves one edge and its two nodes");

    //! [polyline_object_usage]
    // Head-position track: N points draw N - 1 tube segments; repeated positions are skipped
    PolylineObject track;
    track.setPoints({Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.001f, 0.0f, 0.0f), Vector3f(0.002f, 0.001f, 0.0f)});
    track.setRadius(0.0005f);
    track.setGradient(Qt::blue, Qt::red); // oldest to newest
    //! [polyline_object_usage]
    ok &= expect(track.hasData() && track.instanceCount() == 2, "PolylineObject: four positions with one repeat give two segments");

    //! [multi_view_layout_usage]
    MultiViewLayout layout;
    layout.setSplitX(0.25f);
    const QSize canvas(800, 600);
    const QRect left = layout.slotRect(0, 2, canvas);
    const QRect bottomRight = layout.slotRect(2, 3, canvas);
    const SplitterHit onBar = layout.hitTestSplitter(QPoint(200, 300), 2, canvas);
    //! [multi_view_layout_usage]
    ok &= expect(left == QRect(0, 0, 200, 600) && bottomRight == QRect(200, 300, 600, 300) && onBar == SplitterHit::Vertical,
                 "MultiViewLayout: a 25 % split puts the bar at x = 200");

    //! [sensor_field_mapper_usage]
    // Contour spacing for a field map spanning +-120 fT with about 10 lines
    const float contourSpacing = SensorFieldMapper::contourStep(-120e-15f, 120e-15f, 10);
    QMap<QString, std::shared_ptr<BrainSurface>> sceneSurfaces;
    sceneSurfaces.insert("bem_inner_skull", std::make_shared<BrainSurface>());
    sceneSurfaces.insert("bem_head", std::make_shared<BrainSurface>());
    const QString headKey = SensorFieldMapper::findHeadSurfaceKey(sceneSurfaces); // where EEG maps are drawn
    //! [sensor_field_mapper_usage]
    ok &= expect(std::abs(contourSpacing - 50e-15f) < 1e-20f && headKey == "bem_head", "SensorFieldMapper: 50 fT contours, EEG map on bem_head");

    //! [brain_view_usage]
    BrainView view; // QRhi widget; nothing is rendered until it is shown
    view.setViewCount(2);
    const bool twoPanes = view.viewCount() == 2 && view.viewMode() == BrainView::MultiView;
    LiveMarker trackerTip;
    trackerTip.position = QVector3D(0.0f, 0.08f, 0.04f);
    trackerTip.color = Qt::yellow;
    view.setLiveMarkers({trackerTip}); // transient, does not move the camera
    view.clearLiveMarkers();
    view.setViewCount(1);
    //! [brain_view_usage]
    ok &= expect(twoPanes && view.viewCount() == 1, "BrainView switches between two panes and a single view");

    qInfo().noquote() << (ok ? "All disp3D scene checks passed." : "disp3D scene checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
