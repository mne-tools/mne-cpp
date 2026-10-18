//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp3d_brainsurface.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for BrainSurface — creation from raw data, geometry access,
 *           visibility, hemisphere/tissue type, visualization modes, and
 *           MeshFactory sphere/plate/barbell creation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/renderable/brainsurface.h>
#include <disp3D/geometry/meshfactory.h>
#include <disp3D/core/rendertypes.h>
#include <fs/fs_annotation.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QVector3D>
#include <QColor>
#include <QMatrix4x4>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <cmath>

using namespace DISP3DLIB;

//=============================================================================================================
/**
 * @brief Tests for BrainSurface creation, geometry, and MeshFactory primitives.
 */
class TestDisp3dBrainSurface : public QObject
{
    Q_OBJECT

private slots:
    // ── BrainSurface construction ─────────────────────────────────────
    void testDefaultConstructor();
    void testCreateFromData();
    void testCreateFromDataWithNormals();

    // ── Visibility ────────────────────────────────────────────────────
    void testVisibility();

    // ── Hemisphere and tissue type ────────────────────────────────────
    void testHemisphere();
    void testTissueType();

    // ── Geometry accessors ────────────────────────────────────────────
    void testVertexPositions();
    void testVertexNormals();
    void testTriangleIndices();
    void testBoundingBox();
    void testVertexCount();

    // ── Source estimate colors ────────────────────────────────────────
    void testApplySourceEstimateColors();

    // Selection highlight
    void testHighlightTouchesNoStream();
    void testElementLabels();

    // Ray picking
    void testIntersectsSphere();

    // ── MeshFactory ───────────────────────────────────────────────────
    void testMeshFactorySphere();
    void testMeshFactoryPlate();
    void testMeshFactoryBarbell();

private:
    // Helper: create a minimal triangle mesh (a single triangle)
    void createMinimalSurface(BrainSurface& surf);
    // Helper: create a quad (two triangles)
    void createQuadSurface(BrainSurface& surf);
};

//=============================================================================================================
// Helpers
//=============================================================================================================

void TestDisp3dBrainSurface::createMinimalSurface(BrainSurface& surf)
{
    Eigen::MatrixX3f verts(3, 3);
    verts << 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f;

    Eigen::MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    surf.createFromData(verts, tris, Qt::gray);
}

void TestDisp3dBrainSurface::createQuadSurface(BrainSurface& surf)
{
    Eigen::MatrixX3f verts(4, 3);
    verts << 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f;

    Eigen::MatrixX3i tris(2, 3);
    tris << 0, 1, 2,
        0, 2, 3;

    surf.createFromData(verts, tris, Qt::blue);
}

//=============================================================================================================
// BrainSurface construction
//=============================================================================================================

void TestDisp3dBrainSurface::testDefaultConstructor()
{
    BrainSurface surf;
    QCOMPARE(surf.vertexCount(), (uint32_t)0);
    QCOMPARE(surf.indexCount(), (uint32_t)0);
    QVERIFY(surf.isVisible());
}

//=============================================================================================================

void TestDisp3dBrainSurface::testCreateFromData()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    QCOMPARE(surf.vertexCount(), (uint32_t)3);
    QCOMPARE(surf.indexCount(), (uint32_t)3);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testCreateFromDataWithNormals()
{
    Eigen::MatrixX3f verts(3, 3);
    verts << 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f;

    Eigen::MatrixX3f normals(3, 3);
    normals << 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f;

    Eigen::MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    BrainSurface surf;
    surf.createFromData(verts, normals, tris, Qt::red);

    QCOMPARE(surf.vertexCount(), (uint32_t)3);

    // Normals should match what we provided
    Eigen::MatrixX3f readNormals = surf.vertexNormals();
    QCOMPARE(readNormals.rows(), 3);
    for (int i = 0; i < 3; ++i) {
        QVERIFY(qAbs(readNormals(i, 2) - 1.0f) < 1e-5f);
    }
}

//=============================================================================================================
// Visibility
//=============================================================================================================

void TestDisp3dBrainSurface::testVisibility()
{
    BrainSurface surf;
    QVERIFY(surf.isVisible()); // Default is visible

    surf.setVisible(false);
    QVERIFY(!surf.isVisible());

    surf.setVisible(true);
    QVERIFY(surf.isVisible());
}

//=============================================================================================================
// Hemisphere and tissue type
//=============================================================================================================

void TestDisp3dBrainSurface::testHemisphere()
{
    BrainSurface surf;

    surf.setHemi(0);
    QCOMPARE(surf.hemi(), 0); // LH

    surf.setHemi(1);
    QCOMPARE(surf.hemi(), 1); // RH
}

//=============================================================================================================

void TestDisp3dBrainSurface::testTissueType()
{
    BrainSurface surf;

    surf.setTissueType(BrainSurface::TissueBrain);
    QCOMPARE(surf.tissueType(), BrainSurface::TissueBrain);

    surf.setTissueType(BrainSurface::TissueSkin);
    QCOMPARE(surf.tissueType(), BrainSurface::TissueSkin);

    surf.setTissueType(BrainSurface::TissueOuterSkull);
    QCOMPARE(surf.tissueType(), BrainSurface::TissueOuterSkull);

    surf.setTissueType(BrainSurface::TissueInnerSkull);
    QCOMPARE(surf.tissueType(), BrainSurface::TissueInnerSkull);

    surf.setTissueType(BrainSurface::TissueUnknown);
    QCOMPARE(surf.tissueType(), BrainSurface::TissueUnknown);
}

//=============================================================================================================
// Geometry accessors
//=============================================================================================================

void TestDisp3dBrainSurface::testVertexPositions()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    Eigen::MatrixX3f pos = surf.vertexPositions();
    QCOMPARE(pos.rows(), 3);
    QCOMPARE(pos.cols(), 3);

    // Check first vertex is at origin
    QVERIFY(qAbs(pos(0, 0)) < 1e-5f);
    QVERIFY(qAbs(pos(0, 1)) < 1e-5f);
    QVERIFY(qAbs(pos(0, 2)) < 1e-5f);

    // Second vertex at (1.0, 0.0, 0.0)
    QVERIFY(qAbs(pos(1, 0) - 1.0f) < 1e-5f);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testVertexNormals()
{
    // createFromData without explicit normals computes spherical normals
    // (position-based), so a triangle at z=0 won't have Z-pointing normals.
    // Test with explicit normals to verify round-trip.
    Eigen::MatrixX3f verts(3, 3);
    verts << 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f;

    Eigen::MatrixX3f normals(3, 3);
    normals << 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f;

    Eigen::MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    BrainSurface surf;
    surf.createFromData(verts, normals, tris, Qt::gray);

    Eigen::MatrixX3f readNormals = surf.vertexNormals();
    QCOMPARE(readNormals.rows(), 3);
    QCOMPARE(readNormals.cols(), 3);

    // For a triangle in the XY plane with explicit Z normals
    for (int i = 0; i < 3; ++i) {
        float nz = readNormals(i, 2);
        QVERIFY(qAbs(nz) > 0.5f); // Should have significant Z component
    }
}

//=============================================================================================================

void TestDisp3dBrainSurface::testTriangleIndices()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    QVector<uint32_t> indices = surf.triangleIndices();
    QCOMPARE(indices.size(), 3); // 1 triangle = 3 indices
    QCOMPARE(indices[0], (uint32_t)0);
    QCOMPARE(indices[1], (uint32_t)1);
    QCOMPARE(indices[2], (uint32_t)2);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testBoundingBox()
{
    BrainSurface surf;
    createQuadSurface(surf);

    QVector3D minPt, maxPt;
    surf.boundingBox(minPt, maxPt);

    // Quad spans [0,1] x [0,1] x [0,0]
    QVERIFY(qAbs(minPt.x()) < 1e-5f);
    QVERIFY(qAbs(minPt.y()) < 1e-5f);
    QVERIFY(qAbs(maxPt.x() - 1.0f) < 1e-5f);
    QVERIFY(qAbs(maxPt.y() - 1.0f) < 1e-5f);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testVertexCount()
{
    BrainSurface surf;
    QCOMPARE(surf.vertexCount(), (uint32_t)0);

    createQuadSurface(surf);
    QCOMPARE(surf.vertexCount(), (uint32_t)4);
    QCOMPARE(surf.indexCount(), (uint32_t)6); // 2 triangles = 6 indices
    // Without explicit normals the face normals are used: the quad lies in z = 0
    QCOMPARE(surf.vertexDataRef()[2].norm, QVector3D(0.0f, 0.0f, 1.0f));
}

// Source estimate colors
//=============================================================================================================

void TestDisp3dBrainSurface::testApplySourceEstimateColors()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    // Apply 3 vertex colors (one per vertex)
    QVector<uint32_t> colors;
    colors.append(packABGR(255, 0, 0)); // Red
    colors.append(packABGR(0, 255, 0)); // Green
    colors.append(packABGR(0, 0, 255)); // Blue

    surf.applySourceEstimateColors(colors);
    QCOMPARE(surf.colorsRef()[0] & 0x00FFFFFFu, packABGR(255, 0, 0) & 0x00FFFFFFu);

    // The shader draws only RGB, so overlay alpha must be blended over the cortex grey (0xAA without curvature).
    surf.applySourceEstimateColors({packABGR(255, 0, 0, 0), packABGR(255, 0, 0, 51), packABGR(0, 0, 255)});
    QCOMPARE(surf.colorsRef()[0] & 0x00FFFFFFu, packABGR(0xAA, 0xAA, 0xAA) & 0x00FFFFFFu);
    QCOMPARE(surf.colorsRef()[1] & 0x00FFFFFFu, packABGR(187, 136, 136) & 0x00FFFFFFu);
    QCOMPARE(surf.colorsRef()[2] & 0x00FFFFFFu, packABGR(0, 0, 255) & 0x00FFFFFFu);
    surf.setUseDefaultColor(true); // rebuilds the vertex colours from the stored overlay
    QCOMPARE(surf.colorsRef()[1] & 0x00FFFFFFu, packABGR(187, 136, 136) & 0x00FFFFFFu);
}

//=============================================================================================================
// Selection highlight
//=============================================================================================================

void TestDisp3dBrainSurface::testHighlightTouchesNoStream()
{
    BrainSurface surf;
    createQuadSurface(surf);

    // Two regions: vertices 0, 1 in "a" (red), 2, 3 in "b" (green); FreeSurfer ids are R + G*256 + B*65536
    FSLIB::FsAnnotation annotation;
    annotation.getVertices() = (Eigen::VectorXi(4) << 0, 1, 2, 3).finished();
    const int idA = 200;
    const int idB = 100 * 256;
    annotation.getLabelIds() = (Eigen::VectorXi(4) << idA, idA, idB, idB).finished();
    FSLIB::FsColortable& ct = annotation.getColortable();
    ct.numEntries = 2;
    ct.struct_names = {QStringLiteral("a"), QStringLiteral("b")};
    ct.table = (Eigen::MatrixXi(2, 5) << 200, 0, 0, 0, idA, 0, 100, 0, 0, idB).finished();
    surf.addAnnotation(annotation);
    QCOMPARE(surf.getAnnotationLabelId(3), idB);
    QCOMPARE(surf.getAnnotationLabel(0), QStringLiteral("a"));
    QCOMPARE(surf.annotationColorsRef()[2], packABGR(0, 100, 0));
    QVERIFY(!surf.selectedLabel());

    // Hovering only sets uniforms: no stream is touched, so nothing is re-uploaded
    const quint64 geometry = surf.geometryStream().generation;
    const quint64 colors = surf.colorStream().generation;
    const quint64 annotations = surf.annotationStream().generation;
    surf.setSelected(true);
    QVERIFY(!surf.selectedLabel());
    surf.setSelectedRegion(idB);
    QCOMPARE(surf.selectedLabel(), std::optional<uint32_t>(packABGR(0, 100, 0)));
    surf.setSelectedRegion(-1);
    surf.setSelected(false);
    QVERIFY(!surf.selectedLabel());
    QCOMPARE(surf.geometryStream().generation, geometry);
    QCOMPARE(surf.colorStream().generation, colors);
    QCOMPARE(surf.annotationStream().generation, annotations);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testElementLabels()
{
    // Two batched elements of two vertices each; a vertex range highlights one without selecting the surface
    BrainSurface surf;
    createQuadSurface(surf);
    surf.labelElements(2);
    const QVector<uint32_t>& keys = surf.annotationColorsRef();
    QCOMPARE(keys[0], keys[1]);
    QVERIFY(keys[1] != keys[2]);
    QCOMPARE(keys[2] >> 24, 0u); // transparent, so never equal to an opaque atlas colour
    surf.setSelectedVertexRange(2, 2);
    QCOMPARE(surf.selectedLabel(), std::optional<uint32_t>(keys[2]));
    surf.setSelectedVertexRange(-1, 0);
    QVERIFY(!surf.selectedLabel());
}

//=============================================================================================================
// Ray picking
//=============================================================================================================

void TestDisp3dBrainSurface::testIntersectsSphere()
{
    // 5120 triangles, radius 1 cm: every ray from above hits the near cap within the facet error
    auto sphere = MeshFactory::createSphere(QVector3D(0, 0, 0), 0.01f, Qt::red, 4);
    const QVector3D down(0.0f, 0.0f, -1.0f);
    float dist = 0.0f;
    int vertex = -1;
    for (int ix = -9; ix <= 9; ++ix) {
        for (int iy = -9; iy <= 9; ++iy) {
            const float x = 0.001f * float(ix);
            const float y = 0.001f * float(iy);
            const float r2 = x * x + y * y;
            const bool hit = sphere->intersects(QVector3D(x, y, 1.0f), down, dist, vertex);
            if (r2 > 0.0099f * 0.0099f) {
                QVERIFY(!hit || r2 < 0.0101f * 0.0101f);
                continue;
            }
            QVERIFY2(hit, qPrintable(QStringLiteral("missed at %1, %2").arg(x).arg(y)));
            QVERIFY(qAbs(dist - (1.0f - std::sqrt(0.0001f - r2))) < 3e-4f);
            // The picked vertex is a corner of the hit facet, so it lies near the hit point
            QVERIFY((sphere->vertexDataRef()[vertex].pos - QVector3D(x, y, 1.0f - dist)).length() < 1.5e-3f);
        }
    }

    // Moving the mesh rebuilds the hierarchy; a ray pointing away never hits
    QMatrix4x4 shift;
    shift.translate(0.05f, 0.0f, 0.0f);
    sphere->applyTransform(shift);
    QVERIFY(!sphere->intersects(QVector3D(0.0f, 0.0f, 1.0f), down, dist, vertex));
    QVERIFY(sphere->intersects(QVector3D(0.05f, 0.0f, 1.0f), down, dist, vertex));
    QVERIFY(qAbs(dist - 0.99f) < 3e-4f);
    QVERIFY(!sphere->intersects(QVector3D(0.05f, 0.0f, 1.0f), -down, dist, vertex));
    QCOMPARE(vertex, -1);
}

//=============================================================================================================
// MeshFactory
//=============================================================================================================

void TestDisp3dBrainSurface::testMeshFactorySphere()
{
    auto sphere = MeshFactory::createSphere(QVector3D(0, 0, 0), 0.01f, Qt::red, 1);
    QVERIFY(sphere != nullptr);
    QVERIFY(sphere->vertexCount() > 0);
    QVERIFY(sphere->indexCount() > 0);

    // Bounding box should be roughly centered at origin with radius 0.01
    QVector3D minPt, maxPt;
    sphere->boundingBox(minPt, maxPt);
    QVERIFY(maxPt.x() > 0);
    QVERIFY(minPt.x() < 0);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testMeshFactoryPlate()
{
    QMatrix4x4 identity;
    identity.setToIdentity();

    auto plate = MeshFactory::createPlate(QVector3D(0, 0, 0), identity, Qt::green, 0.01f);
    QVERIFY(plate != nullptr);
    QVERIFY(plate->vertexCount() > 0);
    QVERIFY(plate->indexCount() > 0);
}

//=============================================================================================================

void TestDisp3dBrainSurface::testMeshFactoryBarbell()
{
    QMatrix4x4 identity;
    identity.setToIdentity();

    auto barbell = MeshFactory::createBarbell(QVector3D(0, 0, 0), identity, Qt::blue, 0.01f);
    QVERIFY(barbell != nullptr);
    QVERIFY(barbell->vertexCount() > 0);
    QVERIFY(barbell->indexCount() > 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDisp3dBrainSurface)
#include "test_disp3d_brainsurface.moc"
