//=============================================================================================================
/**
 * @file     test_disp3d_brainsurface.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
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

    // ── Visualization mode ────────────────────────────────────────────
    void testSetVisualizationMode();

    // ── Source estimate colors ────────────────────────────────────────
    void testApplySourceEstimateColors();

    // ── MeshFactory ───────────────────────────────────────────────────
    void testMeshFactorySphere();
    void testMeshFactoryPlate();
    void testMeshFactoryBarbell();

private:
    // Helper: create a minimal triangle mesh (a single triangle)
    void createMinimalSurface(BrainSurface &surf);
    // Helper: create a quad (two triangles)
    void createQuadSurface(BrainSurface &surf);
};

//=============================================================================================================
// Helpers
//=============================================================================================================

void TestDisp3dBrainSurface::createMinimalSurface(BrainSurface &surf)
{
    Eigen::MatrixX3f verts(3, 3);
    verts << 0.0f, 0.0f, 0.0f,
             1.0f, 0.0f, 0.0f,
             0.5f, 1.0f, 0.0f;

    Eigen::MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    surf.createFromData(verts, tris, Qt::gray);
}

void TestDisp3dBrainSurface::createQuadSurface(BrainSurface &surf)
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
    BrainSurface surf;
    createMinimalSurface(surf);

    Eigen::MatrixX3f normals = surf.vertexNormals();
    QCOMPARE(normals.rows(), 3);
    QCOMPARE(normals.cols(), 3);

    // For a triangle in the XY plane, normal should point in Z direction
    for (int i = 0; i < 3; ++i) {
        float nz = normals(i, 2);
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
}

//=============================================================================================================
// Visualization mode
//=============================================================================================================

void TestDisp3dBrainSurface::testSetVisualizationMode()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    // Should not crash for any mode
    surf.setVisualizationMode(ModeSurface);
    surf.setVisualizationMode(ModeAnnotation);
    surf.setVisualizationMode(ModeScientific);
    surf.setVisualizationMode(ModeSourceEstimate);
    QVERIFY(true);
}

//=============================================================================================================
// Source estimate colors
//=============================================================================================================

void TestDisp3dBrainSurface::testApplySourceEstimateColors()
{
    BrainSurface surf;
    createMinimalSurface(surf);

    // Apply 3 vertex colors (one per vertex)
    QVector<uint32_t> colors;
    colors.append(packABGR(255, 0, 0));    // Red
    colors.append(packABGR(0, 255, 0));    // Green
    colors.append(packABGR(0, 0, 255));    // Blue

    surf.applySourceEstimateColors(colors);
    QVERIFY(true); // Should not crash
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
