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
#include <disp3D/renderable/electrodeobject.h>
#include <disp3D/renderable/sliceobject.h>
#include <disp3D/renderable/videooverlay.h>
#include <disp3D/scene/multimodalscene.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
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
    QCoreApplication app(argc, argv);
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

    qInfo().noquote() << (ok ? "All disp3D scene checks passed." : "disp3D scene checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
