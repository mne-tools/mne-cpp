//=============================================================================================================
/**
 * @file     test_tool_surface_math.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 * @brief    Tests for surface geometry tool functions (patch info, morph maps, smoothing, etc.).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_add_patch_info ---
#define main _patch_info_main_unused
#include "../../tools/surface/mne_add_patch_info/main.cpp"
#undef main

// --- mne_morph_labels ---
#define main _morph_labels_main_unused
#include "../../tools/surface/mne_morph_labels/main.cpp"
#undef main

// --- mne_make_morph_maps ---
#define main _morph_maps_main_unused
#include "../../tools/surface/mne_make_morph_maps/main.cpp"
#undef main

// --- mne_make_eeg_layout ---
#define main _eeg_layout_main_unused
#include "../../tools/surface/mne_make_eeg_layout/main.cpp"
#undef main

// --- mne_smooth ---
#define main _smooth_main_unused
#include "../../tools/inverse/mne_smooth/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>

//=============================================================================================================
// Helper: build an icosahedron mesh for testing
//=============================================================================================================

static void buildTestIcosahedron(MatrixX3f &rr, MatrixX3i &tris, float radius = 1.0f)
{
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    rr.resize(12, 3);
    rr.row(0)  = Vector3f(-1,  phi, 0).normalized() * radius;
    rr.row(1)  = Vector3f( 1,  phi, 0).normalized() * radius;
    rr.row(2)  = Vector3f(-1, -phi, 0).normalized() * radius;
    rr.row(3)  = Vector3f( 1, -phi, 0).normalized() * radius;
    rr.row(4)  = Vector3f(0, -1,  phi).normalized() * radius;
    rr.row(5)  = Vector3f(0,  1,  phi).normalized() * radius;
    rr.row(6)  = Vector3f(0, -1, -phi).normalized() * radius;
    rr.row(7)  = Vector3f(0,  1, -phi).normalized() * radius;
    rr.row(8)  = Vector3f( phi, 0, -1).normalized() * radius;
    rr.row(9)  = Vector3f( phi, 0,  1).normalized() * radius;
    rr.row(10) = Vector3f(-phi, 0, -1).normalized() * radius;
    rr.row(11) = Vector3f(-phi, 0,  1).normalized() * radius;

    tris.resize(20, 3);
    tris <<  0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
             1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
             3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
             4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1;
}

//=============================================================================================================

class TestToolSurfaceMath : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- buildAdjacency tests ---
    void testBuildAdjacencyIcosahedron();
    void testBuildAdjacencyVertexDegrees();
    void testBuildAdjacencyTriangle();

    // --- multiSourceDijkstra tests ---
    void testDijkstraSingleSource();
    void testDijkstraMultipleSource();
    void testDijkstraDistances();
    void testDijkstraAllVertices();

    // --- buildNearestMap tests ---
    void testBuildNearestMapIdentity();
    void testBuildNearestMapScaled();
    void testBuildNearestMapOffset();

    // --- computeMorphMap tests ---
    void testComputeMorphMapIdentity();
    void testComputeMorphMapSparse();
    void testComputeMorphMapRowSum();

    // --- azimuthalProjection tests ---
    void testAzimuthalProjectionNorthPole();
    void testAzimuthalProjectionEquator();
    void testAzimuthalProjectionSymmetry();

    // --- buildSmoothingOperator tests ---
    void testSmoothingOperatorSize();
    void testSmoothingOperatorRowSum();
    void testSmoothingOperatorConnectivity();
    void testSmoothingOperatorIteration();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
};

//=============================================================================================================

void TestToolSurfaceMath::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================
// buildAdjacency tests
//=============================================================================================================

void TestToolSurfaceMath::testBuildAdjacencyIcosahedron()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);

    auto adj = buildAdjacency(rr, tris);
    QCOMPARE((int)adj.size(), 12);  // 12 vertices

    // Each vertex of an icosahedron has exactly 5 neighbors
    for (int i = 0; i < 12; i++) {
        QCOMPARE((int)adj[i].size(), 5);
    }
}

void TestToolSurfaceMath::testBuildAdjacencyVertexDegrees()
{
    // Simple triangle: 3 vertices, 1 triangle
    MatrixX3f rr(3, 3);
    rr << 0, 0, 0,   1, 0, 0,   0, 1, 0;
    MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    auto adj = buildAdjacency(rr, tris);
    QCOMPARE((int)adj.size(), 3);
    // Each vertex connects to 2 others
    QCOMPARE((int)adj[0].size(), 2);
    QCOMPARE((int)adj[1].size(), 2);
    QCOMPARE((int)adj[2].size(), 2);
}

void TestToolSurfaceMath::testBuildAdjacencyTriangle()
{
    // Verify edge weights are Euclidean distances
    MatrixX3f rr(3, 3);
    rr << 0, 0, 0,   3, 0, 0,   0, 4, 0;
    MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    auto adj = buildAdjacency(rr, tris);
    // Edge 0→1 should have distance 3.0
    bool found01 = false;
    for (auto& [neighbor, dist] : adj[0]) {
        if (neighbor == 1) {
            QVERIFY(qAbs(dist - 3.0f) < 1e-4f);
            found01 = true;
        }
        if (neighbor == 2) {
            QVERIFY(qAbs(dist - 4.0f) < 1e-4f);
        }
    }
    QVERIFY(found01);
}

//=============================================================================================================
// multiSourceDijkstra tests
//=============================================================================================================

void TestToolSurfaceMath::testDijkstraSingleSource()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);
    auto adj = buildAdjacency(rr, tris);

    VectorXi sourceVerts(1);
    sourceVerts << 0;

    VectorXi nearest;
    VectorXf dist;
    multiSourceDijkstra(adj, sourceVerts, 12, nearest, dist);

    QCOMPARE(nearest.size(), (Eigen::Index)12);
    QCOMPARE(dist.size(), (Eigen::Index)12);
    // Source vertex should map to itself with distance 0
    QCOMPARE(nearest(0), 0);
    QVERIFY(dist(0) < 1e-6f);
    // All vertices should map to source 0
    for (int i = 0; i < 12; i++) {
        QCOMPARE(nearest(i), 0);
        QVERIFY(dist(i) >= 0.0f);
    }
}

void TestToolSurfaceMath::testDijkstraMultipleSource()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);
    auto adj = buildAdjacency(rr, tris);

    VectorXi sourceVerts(2);
    sourceVerts << 0, 3;  // Two source vertices on opposite sides

    VectorXi nearest;
    VectorXf dist;
    multiSourceDijkstra(adj, sourceVerts, 12, nearest, dist);

    // Each source vertex should map to itself
    QCOMPARE(nearest(0), 0);
    QCOMPARE(nearest(3), 3);
    // All vertices should map to either source 0 or 3
    for (int i = 0; i < 12; i++) {
        QVERIFY(nearest(i) == 0 || nearest(i) == 3);
    }
}

void TestToolSurfaceMath::testDijkstraDistances()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);
    auto adj = buildAdjacency(rr, tris);

    VectorXi sourceVerts(1);
    sourceVerts << 0;

    VectorXi nearest;
    VectorXf dist;
    multiSourceDijkstra(adj, sourceVerts, 12, nearest, dist);

    // Neighbors of vertex 0 should have shorter distance than non-neighbors
    float maxNeighborDist = 0;
    for (auto& [neighbor, edgeDist] : adj[0]) {
        maxNeighborDist = std::max(maxNeighborDist, dist(neighbor));
    }

    // Non-neighbor vertices should have distance >= max neighbor distance
    for (int i = 0; i < 12; i++) {
        bool isNeighbor = false;
        for (auto& [neighbor, edgeDist] : adj[0]) {
            if (neighbor == i) isNeighbor = true;
        }
        if (!isNeighbor && i != 0) {
            QVERIFY(dist(i) >= maxNeighborDist - 1e-5f);
        }
    }
}

void TestToolSurfaceMath::testDijkstraAllVertices()
{
    // When all vertices are sources, each maps to itself with distance 0
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);
    auto adj = buildAdjacency(rr, tris);

    VectorXi sourceVerts(12);
    for (int i = 0; i < 12; i++) sourceVerts(i) = i;

    VectorXi nearest;
    VectorXf dist;
    multiSourceDijkstra(adj, sourceVerts, 12, nearest, dist);

    for (int i = 0; i < 12; i++) {
        QCOMPARE(nearest(i), i);
        QVERIFY(dist(i) < 1e-6f);
    }
}

//=============================================================================================================
// buildNearestMap tests
//=============================================================================================================

void TestToolSurfaceMath::testBuildNearestMapIdentity()
{
    // Same source and destination → each vertex maps to itself
    MatrixX3f sphere(4, 3);
    sphere << 1, 0, 0,   0, 1, 0,   0, 0, 1,   -1, 0, 0;

    VectorXi nearestMap = buildNearestMap(sphere, sphere);
    QCOMPARE(nearestMap.size(), (Eigen::Index)4);
    for (int i = 0; i < 4; i++) {
        QCOMPARE(nearestMap(i), i);
    }
}

void TestToolSurfaceMath::testBuildNearestMapScaled()
{
    // Scaled sphere → same nearest mapping
    MatrixX3f src(3, 3);
    src << 1, 0, 0,   0, 1, 0,   0, 0, 1;

    MatrixX3f dst(3, 3);
    dst << 2, 0, 0,   0, 2, 0,   0, 0, 2;  // Same directions, different magnitudes

    VectorXi nearestMap = buildNearestMap(src, dst);
    QCOMPARE(nearestMap.size(), (Eigen::Index)3);
    for (int i = 0; i < 3; i++) {
        QCOMPARE(nearestMap(i), i);
    }
}

void TestToolSurfaceMath::testBuildNearestMapOffset()
{
    MatrixX3f src(2, 3);
    src << 1, 0, 0,   -1, 0, 0;   // Two dipolar sources

    MatrixX3f dst(3, 3);
    dst << 0.9f, 0.1f, 0,   -0.8f, 0.2f, 0,   0, 0, 1;  // 3 destinations

    VectorXi nearestMap = buildNearestMap(src, dst);
    // dst[0] is closest to src[0], dst[1] to src[1]
    QCOMPARE(nearestMap(0), 0);
    QCOMPARE(nearestMap(1), 1);
}

//=============================================================================================================
// computeMorphMap tests
//=============================================================================================================

void TestToolSurfaceMath::testComputeMorphMapIdentity()
{
    // Same src and dst on unit sphere → should morph to identity
    MatrixX3f sphere(4, 3);
    sphere << 1, 0, 0,   0, 1, 0,   0, 0, 1,   -1, 0, 0;
    sphere.rowwise().normalize();

    SparseMatrix<double> M = computeMorphMap(sphere, sphere, 3);
    QCOMPARE(M.rows(), (Eigen::Index)4);
    QCOMPARE(M.cols(), (Eigen::Index)4);

    // Diagonal should be dominant (close to 1)
    for (int i = 0; i < 4; i++) {
        QVERIFY(M.coeff(i, i) > 0.5);
    }
}

void TestToolSurfaceMath::testComputeMorphMapSparse()
{
    MatrixX3f src(6, 3);
    src << 1, 0, 0,   0, 1, 0,   0, 0, 1,
          -1, 0, 0,   0, -1, 0,  0, 0, -1;
    src.rowwise().normalize();

    MatrixX3f dst(3, 3);
    dst << 1, 0, 0,   0, 1, 0,   0, 0, 1;
    dst.rowwise().normalize();

    SparseMatrix<double> M = computeMorphMap(src, dst, 2);
    QCOMPARE(M.rows(), (Eigen::Index)3);
    QCOMPARE(M.cols(), (Eigen::Index)6);
    QVERIFY(M.nonZeros() > 0);
}

void TestToolSurfaceMath::testComputeMorphMapRowSum()
{
    // Each row of a morph map should sum to approximately 1
    MatrixX3f sphere(8, 3);
    for (int i = 0; i < 8; i++) {
        float theta = 2 * M_PI * i / 8;
        sphere.row(i) = Vector3f(cos(theta), sin(theta), 0).normalized();
    }

    SparseMatrix<double> M = computeMorphMap(sphere, sphere, 3);
    for (int i = 0; i < M.rows(); i++) {
        double rowSum = 0;
        for (SparseMatrix<double>::InnerIterator it(M, i); it; ++it) {
            rowSum += it.value();
        }
        QVERIFY(qAbs(rowSum - 1.0) < 0.1);
    }
}

//=============================================================================================================
// azimuthalProjection tests
//=============================================================================================================

void TestToolSurfaceMath::testAzimuthalProjectionNorthPole()
{
    // Point at the "north pole" of a sphere → should project to center
    Vector3f center(0, 0, 0);
    Vector3f pos(0, 0, 1);  // Top of unit sphere
    float radius = 1.0f;
    float x, y;
    azimuthalProjection(pos, center, radius, x, y);

    // North pole → center of projection
    QVERIFY(qAbs(x) < 1e-4f);
    QVERIFY(qAbs(y) < 1e-4f);
}

void TestToolSurfaceMath::testAzimuthalProjectionEquator()
{
    // Points on the equator → should project to a circle
    Vector3f center(0, 0, 0);
    float radius = 1.0f;

    float x1, y1, x2, y2;
    azimuthalProjection(Vector3f(1, 0, 0), center, radius, x1, y1);
    azimuthalProjection(Vector3f(0, 1, 0), center, radius, x2, y2);

    float dist1 = std::sqrt(x1 * x1 + y1 * y1);
    float dist2 = std::sqrt(x2 * x2 + y2 * y2);

    // Both should be at the same distance from center (equidistant projection)
    QVERIFY(qAbs(dist1 - dist2) < 0.1f);
    QVERIFY(dist1 > 0);
}

void TestToolSurfaceMath::testAzimuthalProjectionSymmetry()
{
    // Symmetric points should project symmetrically
    Vector3f center(0, 0, 0);
    float radius = 1.0f;
    float x1, y1, x2, y2;

    azimuthalProjection(Vector3f(1, 0, 0), center, radius, x1, y1);
    azimuthalProjection(Vector3f(-1, 0, 0), center, radius, x2, y2);

    // Should be symmetric about origin
    QVERIFY(qAbs(x1 + x2) < 0.1f);
    QVERIFY(qAbs(y1 + y2) < 0.1f);
}

//=============================================================================================================
// buildSmoothingOperator tests
//=============================================================================================================

void TestToolSurfaceMath::testSmoothingOperatorSize()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);

    SparseMatrix<double> S = buildSmoothingOperator(tris, 12);
    QCOMPARE(S.rows(), (Eigen::Index)12);
    QCOMPARE(S.cols(), (Eigen::Index)12);
}

void TestToolSurfaceMath::testSmoothingOperatorRowSum()
{
    // Each row should sum to 1.0 (it's an averaging operator)
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);

    SparseMatrix<double> S = buildSmoothingOperator(tris, 12);

    for (int i = 0; i < 12; i++) {
        double rowSum = 0;
        for (SparseMatrix<double>::InnerIterator it(S, i); it; ++it) {
            rowSum += it.value();
        }
        QVERIFY(qAbs(rowSum - 1.0) < 1e-10);
    }
}

void TestToolSurfaceMath::testSmoothingOperatorConnectivity()
{
    // Diagonal should be 0 (no self-contribution), off-diagonal > 0 for neighbors
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);

    SparseMatrix<double> S = buildSmoothingOperator(tris, 12);

    for (int i = 0; i < 12; i++) {
        QVERIFY(qAbs(S.coeff(i, i)) < 1e-10);  // No self-weight
    }

    // Non-zero off-diagonals should correspond to mesh edges
    QVERIFY(S.nonZeros() == 12 * 5);  // 12 vertices × 5 neighbors each
}

void TestToolSurfaceMath::testSmoothingOperatorIteration()
{
    // Apply smoothing operator to a delta function → should spread to neighbors
    MatrixX3f rr;
    MatrixX3i tris;
    buildTestIcosahedron(rr, tris);

    SparseMatrix<double> S = buildSmoothingOperator(tris, 12);

    VectorXd data = VectorXd::Zero(12);
    data(0) = 1.0;  // Delta at vertex 0

    VectorXd smoothed = S * data;

    // Vertex 0 should now be 0 (no self-weight)
    QVERIFY(qAbs(smoothed(0)) < 1e-10);
    // Neighbors of vertex 0 should have 1/5 each
    QVERIFY(smoothed.sum() > 0.99);  // Total mass preserved
}

//=============================================================================================================

void TestToolSurfaceMath::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolSurfaceMath)
#include "test_tool_surface_math.moc"
