//=============================================================================================================
/**
 * @file     test_tool_forward_generation.cpp
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
 * @brief    Tests for forward tool functions (icosahedron, icosphere, source space).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_make_source_space ---
#define main _make_src_main_unused
#include "../../tools/forward/mne_make_source_space/main.cpp"
#undef main

// --- mne_make_sphere_bem ---
#define main _sphere_bem_main_unused
#include "../../tools/forward/mne_make_sphere_bem/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>

//=============================================================================================================

class TestToolForwardGeneration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- makeIcosahedron tests ---
    void testIcosahedronGrade0();
    void testIcosahedronGrade1();
    void testIcosahedronGrade2();
    void testIcosahedronGrade3();
    void testIcosahedronGrade4();
    void testIcosahedronOnUnitSphere();
    void testIcosahedronVertexCount();

    // --- selectVerticesIco tests ---
    void testSelectVerticesIcoBasic();
    void testSelectVerticesIcoCount();
    void testSelectVerticesIcoInuseVector();

    // --- makeIcosphere tests ---
    void testIcosphereGrade0();
    void testIcosphereGrade1();
    void testIcosphereGrade2();
    void testIcosphereGrade3();
    void testIcosphereOnUnitSphere();
    void testIcosphereTriangleCount();
    void testIcosphereClosedSurface();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
};

//=============================================================================================================

void TestToolForwardGeneration::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================
// makeIcosahedron tests
//=============================================================================================================

void TestToolForwardGeneration::testIcosahedronGrade0()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(0, verts));
    QCOMPARE(verts.rows(), (Eigen::Index)12);  // Base icosahedron
}

void TestToolForwardGeneration::testIcosahedronGrade1()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(1, verts));
    // After 1 subdivision: 12 + 30 = 42 vertices
    QCOMPARE(verts.rows(), (Eigen::Index)42);
}

void TestToolForwardGeneration::testIcosahedronGrade2()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(2, verts));
    // After 2 subdivisions: 162 vertices
    QCOMPARE(verts.rows(), (Eigen::Index)162);
}

void TestToolForwardGeneration::testIcosahedronGrade3()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(3, verts));
    // After 3 subdivisions: 642 vertices
    QCOMPARE(verts.rows(), (Eigen::Index)642);
}

void TestToolForwardGeneration::testIcosahedronGrade4()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(4, verts));
    // After 4 subdivisions: 2562 vertices
    QCOMPARE(verts.rows(), (Eigen::Index)2562);
}

void TestToolForwardGeneration::testIcosahedronOnUnitSphere()
{
    MatrixX3f verts;
    QVERIFY(makeIcosahedron(2, verts));

    // All vertices should be on the unit sphere
    for (int i = 0; i < verts.rows(); i++) {
        float norm = verts.row(i).norm();
        QVERIFY(qAbs(norm - 1.0f) < 1e-4f);
    }
}

void TestToolForwardGeneration::testIcosahedronVertexCount()
{
    // Formula: V = 10 * 4^grade + 2
    for (int grade = 0; grade <= 4; grade++) {
        MatrixX3f verts;
        QVERIFY(makeIcosahedron(grade, verts));
        int expected = 10 * (int)std::pow(4, grade) + 2;
        QCOMPARE(verts.rows(), (Eigen::Index)expected);
    }
}

//=============================================================================================================
// selectVerticesIco tests
//=============================================================================================================

void TestToolForwardGeneration::testSelectVerticesIcoBasic()
{
    // Create a surface and icosphere, select vertices
    MatrixX3f surfVerts;
    QVERIFY(makeIcosahedron(3, surfVerts));  // 642 surface vertices

    MatrixX3f icoVerts;
    QVERIFY(makeIcosahedron(1, icoVerts));  // 42 ico points

    VectorXi inuse;
    VectorXi vertno;
    int nSelected = selectVerticesIco(surfVerts, icoVerts, inuse, vertno);

    QVERIFY(nSelected > 0);
    QVERIFY(nSelected <= icoVerts.rows());
    QCOMPARE(vertno.size(), (Eigen::Index)nSelected);
    QCOMPARE(inuse.size(), surfVerts.rows());
}

void TestToolForwardGeneration::testSelectVerticesIcoCount()
{
    MatrixX3f surfVerts;
    QVERIFY(makeIcosahedron(4, surfVerts));  // 2562 surface vertices

    MatrixX3f icoVerts;
    QVERIFY(makeIcosahedron(2, icoVerts));  // 162 ico points

    VectorXi inuse;
    VectorXi vertno;
    int nSelected = selectVerticesIco(surfVerts, icoVerts, inuse, vertno);

    // Should select roughly as many as ico points
    QVERIFY(nSelected >= icoVerts.rows() * 0.8);  // Some may overlap
    QVERIFY(nSelected <= icoVerts.rows());
}

void TestToolForwardGeneration::testSelectVerticesIcoInuseVector()
{
    MatrixX3f surfVerts;
    QVERIFY(makeIcosahedron(3, surfVerts));

    MatrixX3f icoVerts;
    QVERIFY(makeIcosahedron(0, icoVerts));  // 12 ico points

    VectorXi inuse;
    VectorXi vertno;
    int nSelected = selectVerticesIco(surfVerts, icoVerts, inuse, vertno);

    // inuse should be binary (0 or 1)
    for (int i = 0; i < inuse.size(); i++) {
        QVERIFY(inuse(i) == 0 || inuse(i) == 1);
    }

    // Sum of inuse should equal nSelected
    QCOMPARE(inuse.sum(), nSelected);

    // vertno entries should correspond to 1s in inuse
    for (int i = 0; i < vertno.size(); i++) {
        QCOMPARE(inuse(vertno(i)), 1);
    }
}

//=============================================================================================================
// makeIcosphere tests
//=============================================================================================================

void TestToolForwardGeneration::testIcosphereGrade0()
{
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(0, verts, tris);
    QCOMPARE(verts.rows(), (Eigen::Index)12);
    QCOMPARE(tris.rows(), (Eigen::Index)20);
}

void TestToolForwardGeneration::testIcosphereGrade1()
{
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(1, verts, tris);
    QCOMPARE(verts.rows(), (Eigen::Index)42);
    QCOMPARE(tris.rows(), (Eigen::Index)80);
}

void TestToolForwardGeneration::testIcosphereGrade2()
{
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(2, verts, tris);
    QCOMPARE(verts.rows(), (Eigen::Index)162);
    QCOMPARE(tris.rows(), (Eigen::Index)320);
}

void TestToolForwardGeneration::testIcosphereGrade3()
{
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(3, verts, tris);
    QCOMPARE(verts.rows(), (Eigen::Index)642);
    QCOMPARE(tris.rows(), (Eigen::Index)1280);
}

void TestToolForwardGeneration::testIcosphereOnUnitSphere()
{
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(3, verts, tris);

    for (int i = 0; i < verts.rows(); i++) {
        float norm = verts.row(i).norm();
        QVERIFY(qAbs(norm - 1.0f) < 1e-4f);
    }
}

void TestToolForwardGeneration::testIcosphereTriangleCount()
{
    // Formula: F = 20 * 4^level
    for (int level = 0; level <= 4; level++) {
        MatrixX3f verts;
        MatrixX3i tris;
        makeIcosphere(level, verts, tris);
        int expectedFaces = 20 * (int)std::pow(4, level);
        QCOMPARE(tris.rows(), (Eigen::Index)expectedFaces);
    }
}

void TestToolForwardGeneration::testIcosphereClosedSurface()
{
    // Euler formula for closed surface: V - E + F = 2
    // For triangulated sphere: E = 3*F/2 (each edge shared by 2 faces)
    MatrixX3f verts;
    MatrixX3i tris;
    makeIcosphere(2, verts, tris);

    int V = verts.rows();
    int F = tris.rows();
    int E = 3 * F / 2;

    QCOMPARE(V - E + F, 2);
}

//=============================================================================================================

void TestToolForwardGeneration::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolForwardGeneration)
#include "test_tool_forward_generation.moc"
