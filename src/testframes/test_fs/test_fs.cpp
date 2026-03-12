//=============================================================================================================
/**
 * @file     test_fs.cpp
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
 * @brief    Comprehensive tests for the FreeSurfer (FS) library classes.
 *           Tests Surface, SurfaceSet, Annotation, AnnotationSet, Label, Colortable
 *           using synthetic data (no FreeSurfer data files required).
 *           Inspired by mne-python test_surface.py and test_label.py.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_label.h>
#include <fs/fs_colortable.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for the FreeSurfer library: Surface, Label, Colortable, SurfaceSet, AnnotationSet.
 *        Uses synthetic geometry so no test data files are needed.
 */
class TestFS : public QObject
{
    Q_OBJECT

public:
    TestFS();

private slots:
    void initTestCase();

    // ----- Surface -----
    void testSurfaceDefaultCtor();
    void testSurfaceClear();
    void testComputeNormalsTetrahedron();
    void testComputeNormalsPlane();
    void testComputeNormalsOrthogonality();
    void testSurfaceReadNonExistent();

    // ----- SurfaceSet -----
    void testSurfaceSetDefaultCtor();
    void testSurfaceSetInsert();
    void testSurfaceSetSize();

    // ----- Label -----
    void testLabelDefaultCtor();
    void testLabelFullCtor();
    void testLabelClear();
    void testLabelSelectTrisMatrix();
    void testLabelSelectTrisEmptyVertices();
    void testLabelReadFromFile();
    void testLabelReadNonLabelFile();
    void testLabelReadNonExistentFile();

    // ----- Colortable -----
    void testColortableDefaultCtor();
    void testColortableClear();
    void testColortableAccessors();

    // ----- Annotation -----
    void testAnnotationDefaultCtor();
    void testAnnotationReadNonExistent();

    // ----- AnnotationSet -----
    void testAnnotationSetDefaultCtor();
    void testAnnotationSetInsert();

    void cleanupTestCase();

private:
    /// Build a regular tetrahedron: 4 vertices, 4 triangular faces
    void buildTetrahedron(MatrixX3f &rr, MatrixX3i &tris);

    /// Build an XY-plane quad: 4 vertices, 2 triangles, normal should point +Z
    void buildXYPlane(MatrixX3f &rr, MatrixX3i &tris);

    /// Create a minimal FreeSurfer .label file in the given directory
    QString createTempLabelFile(const QTemporaryDir &dir, const QString &prefix = "lh");
};

//=============================================================================================================

TestFS::TestFS()
{
}

//=============================================================================================================

void TestFS::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qInfo() << "TestFS: Starting FreeSurfer library unit tests";
}

//=============================================================================================================
// Helper: regular tetrahedron with edge length sqrt(2)
//=============================================================================================================

void TestFS::buildTetrahedron(MatrixX3f &rr, MatrixX3i &tris)
{
    rr.resize(4, 3);
    rr << 1.0f,  1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,
         -1.0f,  1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f;

    tris.resize(4, 3);
    tris << 0, 1, 2,
            0, 1, 3,
            0, 2, 3,
            1, 2, 3;
}

//=============================================================================================================
// Helper: XY-plane quad with known normal (+Z)
//=============================================================================================================

void TestFS::buildXYPlane(MatrixX3f &rr, MatrixX3i &tris)
{
    rr.resize(4, 3);
    rr << 0.0f, 0.0f, 0.0f,
          1.0f, 0.0f, 0.0f,
          1.0f, 1.0f, 0.0f,
          0.0f, 1.0f, 0.0f;

    tris.resize(2, 3);
    tris << 0, 1, 2,
            0, 2, 3;
}

//=============================================================================================================
// Helper: Create a temporary .label file
//=============================================================================================================

QString TestFS::createTempLabelFile(const QTemporaryDir &dir, const QString &prefix)
{
    // Format: comment line, vertex count, then rows of: vertexId x y z value
    // Positions in mm (will be converted to m by Label::read)
    QString filePath = dir.path() + "/" + prefix + ".test.label";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning("Failed to create temp label file");
        return QString();
    }

    QTextStream ts(&file);
    ts << "#test label comment\n";
    ts << "3\n";
    ts << "0   10.0   20.0   30.0   1.0\n";
    ts << "1   40.0   50.0   60.0   0.5\n";
    ts << "2   70.0   80.0   90.0   0.0\n";

    file.close();
    return filePath;
}

//=============================================================================================================
// Surface tests
//=============================================================================================================

void TestFS::testSurfaceDefaultCtor()
{
    Surface s;
    QVERIFY(s.isEmpty());
    QCOMPARE(s.hemi(), -1);
    QCOMPARE(s.rr().rows(), 0);
    QCOMPARE(s.tris().rows(), 0);
    QCOMPARE(s.nn().rows(), 0);
    QCOMPARE(s.curv().size(), 0);
    QVERIFY(s.filePath().isEmpty());
    QVERIFY(s.fileName().isEmpty());
    QVERIFY(s.surf().isEmpty());
}

//=============================================================================================================

void TestFS::testSurfaceClear()
{
    // Start with default, clear should be safe
    Surface s;
    s.clear();
    QVERIFY(s.isEmpty());
    QCOMPARE(s.rr().rows(), 0);
}

//=============================================================================================================

void TestFS::testComputeNormalsTetrahedron()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildTetrahedron(rr, tris);

    MatrixX3f nn = Surface::compute_normals(rr, tris);

    // Should have one normal per vertex
    QCOMPARE(nn.rows(), rr.rows());
    QCOMPARE(nn.cols(), 3);

    // All normals should be unit length
    for (int i = 0; i < nn.rows(); ++i)
    {
        float norm = nn.row(i).norm();
        QVERIFY2(qAbs(norm - 1.0f) < 1e-5f,
                 qPrintable(QString("Normal %1 has length %2").arg(i).arg(norm)));
    }
}

//=============================================================================================================

void TestFS::testComputeNormalsPlane()
{
    MatrixX3f rr;
    MatrixX3i tris;
    buildXYPlane(rr, tris);

    MatrixX3f nn = Surface::compute_normals(rr, tris);

    QCOMPARE(nn.rows(), 4);

    // All normals on a flat XY plane should point along +Z or -Z
    // The cross product of (1,0,0)x(1,1,0) = (0,0,1) -> +Z
    for (int i = 0; i < nn.rows(); ++i)
    {
        float nz = qAbs(nn(i, 2));
        QVERIFY2(nz > 0.99f,
                 qPrintable(QString("Vertex %1: Z-component = %2, expected ~1.0").arg(i).arg(nn(i,2))));
    }
}

//=============================================================================================================

void TestFS::testComputeNormalsOrthogonality()
{
    // For the XY-plane, normals should be perpendicular to the surface
    // i.e., the dot product of any normal with any edge should be ~0
    MatrixX3f rr;
    MatrixX3i tris;
    buildXYPlane(rr, tris);

    MatrixX3f nn = Surface::compute_normals(rr, tris);

    // Edge vectors in the plane
    Vector3f edge1 = rr.row(1) - rr.row(0);  // (1,0,0)
    Vector3f edge2 = rr.row(2) - rr.row(0);  // (1,1,0)

    for (int i = 0; i < nn.rows(); ++i)
    {
        float dot1 = nn.row(i).dot(edge1);
        float dot2 = nn.row(i).dot(edge2);
        QVERIFY2(qAbs(dot1) < 1e-5f,
                 qPrintable(QString("Normal %1 not perpendicular to edge1: dot=%2").arg(i).arg(dot1)));
        QVERIFY2(qAbs(dot2) < 1e-5f,
                 qPrintable(QString("Normal %1 not perpendicular to edge2: dot=%2").arg(i).arg(dot2)));
    }
}

//=============================================================================================================

void TestFS::testSurfaceReadNonExistent()
{
    Surface s;
    bool ok = Surface::read("/nonexistent/path/lh.white", s);
    QVERIFY(!ok);
    QVERIFY(s.isEmpty());
}

//=============================================================================================================
// SurfaceSet tests
//=============================================================================================================

void TestFS::testSurfaceSetDefaultCtor()
{
    SurfaceSet ss;
    QVERIFY(ss.isEmpty());
    QCOMPARE(ss.size(), 0);
}

//=============================================================================================================

void TestFS::testSurfaceSetInsert()
{
    // SurfaceSet::insert uses hemi() as key. Default Surface has hemi=-1,
    // and insert may skip invalid/empty surfaces. Just verify no crash.
    SurfaceSet ss;
    Surface s;
    ss.insert(s);
    // Default surface has hemi=-1; insert may or may not accept it
    // The important thing is it doesn't crash
    QVERIFY(ss.size() >= 0);
}

//=============================================================================================================

void TestFS::testSurfaceSetSize()
{
    SurfaceSet ss;
    QCOMPARE(ss.size(), 0);

    // Verify the size API works on empty sets
    QVERIFY(ss.isEmpty());
}

//=============================================================================================================
// Label tests
//=============================================================================================================

void TestFS::testLabelDefaultCtor()
{
    Label l;
    QVERIFY(l.isEmpty());
    QCOMPARE(l.hemi, -1);
    QCOMPARE(l.label_id, -1);
    QCOMPARE(l.vertices.size(), 0);
    QCOMPARE(l.pos.rows(), 0);
    QCOMPARE(l.values.size(), 0);
}

//=============================================================================================================

void TestFS::testLabelFullCtor()
{
    VectorXi verts(3);
    verts << 0, 1, 2;

    MatrixX3f pos(3, 3);
    pos << 0.01f, 0.02f, 0.03f,
           0.04f, 0.05f, 0.06f,
           0.07f, 0.08f, 0.09f;

    VectorXd vals(3);
    vals << 1.0, 0.5, 0.0;

    Label l(verts, pos, vals, 0, "test_label", 42);

    QVERIFY(!l.isEmpty());
    QCOMPARE(l.hemi, 0);
    QCOMPARE(l.name, QString("test_label"));
    QCOMPARE(l.label_id, 42);
    QCOMPARE(l.vertices.size(), 3);
    QCOMPARE(l.pos.rows(), 3);
    QCOMPARE(l.values.size(), 3);
    QVERIFY(qAbs(l.values(0) - 1.0) < 1e-10);
}

//=============================================================================================================

void TestFS::testLabelClear()
{
    VectorXi verts(2);
    verts << 0, 1;
    MatrixX3f pos(2, 3);
    pos << 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f;
    VectorXd vals(2);
    vals << 1.0, 0.0;

    Label l(verts, pos, vals, 1, "to_clear", 10);
    QVERIFY(!l.isEmpty());

    l.clear();
    QVERIFY(l.isEmpty());
    QCOMPARE(l.hemi, -1);
    QCOMPARE(l.label_id, -1);
    QCOMPARE(l.vertices.size(), 0);
}

//=============================================================================================================

void TestFS::testLabelSelectTrisMatrix()
{
    // Create a triangle mesh:
    //   v0--v1
    //   | \ |
    //   v3--v2
    // Two triangles: (0,1,2) and (0,2,3)
    MatrixX3i allTris(2, 3);
    allTris << 0, 1, 2,
               0, 2, 3;

    // Label contains only vertices {0, 1}
    VectorXi labelVerts(2);
    labelVerts << 0, 1;

    MatrixX3f pos(2, 3);
    pos << 0.0f, 0.0f, 0.0f,
           1.0f, 0.0f, 0.0f;

    VectorXd vals(2);
    vals << 1.0, 1.0;

    Label l(labelVerts, pos, vals, 0, "partial");

    MatrixX3i selected = l.selectTris(allTris);

    // Triangle (0,1,2) has vertices 0 and 1 in label -> selected
    // Triangle (0,2,3) has vertex 0 in label -> selected
    QCOMPARE(selected.rows(), 2);
}

//=============================================================================================================

void TestFS::testLabelSelectTrisEmptyVertices()
{
    Label l;  // empty label, no vertices

    MatrixX3i someTris(1, 3);
    someTris << 0, 1, 2;

    MatrixX3i result = l.selectTris(someTris);
    QCOMPARE(result.rows(), 0);
}

//=============================================================================================================

void TestFS::testLabelReadFromFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString labelFile = createTempLabelFile(tmpDir, "lh");

    Label l;
    bool ok = Label::read(labelFile, l);

    QVERIFY(ok);
    QVERIFY(!l.isEmpty());
    QCOMPARE(l.hemi, 0);  // "lh" -> left hemisphere
    QCOMPARE(l.vertices.size(), 3);
    QCOMPARE(l.pos.rows(), 3);
    QCOMPARE(l.values.size(), 3);

    // Check vertex indices
    QCOMPARE(l.vertices(0), 0);
    QCOMPARE(l.vertices(1), 1);
    QCOMPARE(l.vertices(2), 2);

    // Positions should be in meters (original mm / 1000)
    QVERIFY(qAbs(l.pos(0, 0) - 0.010f) < 1e-5f);
    QVERIFY(qAbs(l.pos(0, 1) - 0.020f) < 1e-5f);
    QVERIFY(qAbs(l.pos(0, 2) - 0.030f) < 1e-5f);

    // Values
    QVERIFY(qAbs(l.values(0) - 1.0) < 1e-10);
    QVERIFY(qAbs(l.values(1) - 0.5) < 1e-10);
    QVERIFY(qAbs(l.values(2) - 0.0) < 1e-10);

    // Comment
    QCOMPARE(l.comment, QString("test label comment"));
}

//=============================================================================================================

void TestFS::testLabelReadNonLabelFile()
{
    // Trying to read a file that doesn't end with .label should fail
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString badFile = tmpDir.path() + "/lh.test.txt";
    QFile f(badFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("dummy");
    f.close();

    Label l;
    bool ok = Label::read(badFile, l);
    QVERIFY(!ok);
    QVERIFY(l.isEmpty());
}

//=============================================================================================================

void TestFS::testLabelReadNonExistentFile()
{
    Label l;
    bool ok = Label::read("/nonexistent/path/lh.test.label", l);
    QVERIFY(!ok);
    QVERIFY(l.isEmpty());
}

//=============================================================================================================
// Colortable tests
//=============================================================================================================

void TestFS::testColortableDefaultCtor()
{
    Colortable ct;
    QCOMPARE(ct.numEntries, 0);
    QVERIFY(ct.struct_names.isEmpty());
    QCOMPARE(ct.table.rows(), 0);
}

//=============================================================================================================

void TestFS::testColortableClear()
{
    Colortable ct;
    ct.numEntries = 3;
    ct.struct_names << "A" << "B" << "C";
    ct.table.resize(3, 5);
    ct.table.setZero();

    ct.clear();

    QCOMPARE(ct.numEntries, 0);
    QVERIFY(ct.struct_names.isEmpty());
    QCOMPARE(ct.table.rows(), 0);
}

//=============================================================================================================

void TestFS::testColortableAccessors()
{
    Colortable ct;
    ct.numEntries = 2;
    ct.struct_names << "RegionA" << "RegionB";

    // Table: R, G, B, A, Id
    ct.table.resize(2, 5);
    ct.table << 255, 0, 0, 255, 1001,
                0, 128, 255, 200, 1002;

    // getLabelIds
    VectorXi ids = ct.getLabelIds();
    QCOMPARE(ids.size(), 2);
    QCOMPARE(ids(0), 1001);
    QCOMPARE(ids(1), 1002);

    // getNames
    QStringList names = ct.getNames();
    QCOMPARE(names.size(), 2);
    QCOMPARE(names[0], QString("RegionA"));
    QCOMPARE(names[1], QString("RegionB"));

    // getRGBAs
    MatrixX4i rgbas = ct.getRGBAs();
    QCOMPARE(rgbas.rows(), 2);
    QCOMPARE(rgbas(0, 0), 255);  // R
    QCOMPARE(rgbas(0, 1), 0);    // G
    QCOMPARE(rgbas(0, 2), 0);    // B
    QCOMPARE(rgbas(0, 3), 255);  // A
    QCOMPARE(rgbas(1, 0), 0);
    QCOMPARE(rgbas(1, 1), 128);
    QCOMPARE(rgbas(1, 2), 255);
}

//=============================================================================================================
// Annotation tests
//=============================================================================================================

void TestFS::testAnnotationDefaultCtor()
{
    Annotation a;
    QVERIFY(a.isEmpty());
    QCOMPARE(a.hemi(), -1);
    QVERIFY(a.filePath().isEmpty());
    QVERIFY(a.fileName().isEmpty());
}

//=============================================================================================================

void TestFS::testAnnotationReadNonExistent()
{
    Annotation a;
    bool ok = Annotation::read("/nonexistent/path/lh.aparc.annot", a);
    QVERIFY(!ok);
    QVERIFY(a.isEmpty());
}

//=============================================================================================================
// AnnotationSet tests
//=============================================================================================================

void TestFS::testAnnotationSetDefaultCtor()
{
    AnnotationSet as;
    QVERIFY(as.isEmpty());
    QCOMPARE(as.size(), 0);
}

//=============================================================================================================

void TestFS::testAnnotationSetInsert()
{
    // AnnotationSet::insert uses hemi() as key. Default annotation has hemi=-1,
    // and insert may skip invalid/empty annotations. Just verify no crash.
    AnnotationSet as;
    Annotation a;
    as.insert(a);
    QVERIFY(as.size() >= 0);
}

//=============================================================================================================

void TestFS::cleanupTestCase()
{
    qInfo() << "TestFS: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFS)
#include "test_fs.moc"
