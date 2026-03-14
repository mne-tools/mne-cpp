#include <QtTest/QtTest>
#include <Eigen/Dense>

#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_label.h>
#include <fs/fs_colortable.h>

using namespace FSLIB;
using namespace Eigen;

class TestFsSurfaceAnnotation : public QObject
{
    Q_OBJECT

private:
    void makeTetrahedron(MatrixX3f &verts, MatrixX3i &tris)
    {
        verts.resize(4, 3);
        verts << 0.0f, 0.0f, 1.0f,
                 1.0f, 0.0f, 0.0f,
                 -0.5f, 0.866f, 0.0f,
                 -0.5f, -0.866f, 0.0f;
        tris.resize(4, 3);
        tris << 0, 1, 2,
                0, 2, 3,
                0, 3, 1,
                1, 3, 2;
    }

private slots:
    //=========================================================================
    // FsSurface::compute_normals
    //=========================================================================
    void surface_computeNormals_tetrahedron()
    {
        MatrixX3f verts;
        MatrixX3i tris;
        makeTetrahedron(verts, tris);

        MatrixX3f normals = FsSurface::compute_normals(verts, tris);
        QCOMPARE(normals.rows(), 4);
        QCOMPARE(normals.cols(), 3);

        for (int i = 0; i < normals.rows(); ++i) {
            float len = normals.row(i).norm();
            QVERIFY(qAbs(len - 1.0f) < 0.1f);
        }
    }

    void surface_computeNormals_plane()
    {
        MatrixX3f verts(4, 3);
        verts << 0.0f, 0.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 1.0f, 1.0f, 0.0f,
                 0.0f, 1.0f, 0.0f;
        MatrixX3i tris(2, 3);
        tris << 0, 1, 2,
                0, 2, 3;

        MatrixX3f normals = FsSurface::compute_normals(verts, tris);
        QCOMPARE(normals.rows(), 4);

        for (int i = 0; i < 4; ++i) {
            QVERIFY(qAbs(normals(i, 2)) > 0.9f);
        }
    }

    void surface_computeNormals_single()
    {
        MatrixX3f verts(3, 3);
        verts << 0.0f, 0.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f;
        MatrixX3i tris(1, 3);
        tris << 0, 1, 2;

        MatrixX3f normals = FsSurface::compute_normals(verts, tris);
        QCOMPARE(normals.rows(), 3);
    }

    void surface_computeNormals_large()
    {
        int gridSize = 10;
        int nVerts = gridSize * gridSize;
        MatrixX3f verts(nVerts, 3);
        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                verts.row(i * gridSize + j) << (float)i, (float)j, 0.0f;
            }
        }

        int nTris = (gridSize - 1) * (gridSize - 1) * 2;
        MatrixX3i tris(nTris, 3);
        int t = 0;
        for (int i = 0; i < gridSize - 1; ++i) {
            for (int j = 0; j < gridSize - 1; ++j) {
                int v00 = i * gridSize + j;
                int v10 = (i + 1) * gridSize + j;
                int v01 = i * gridSize + (j + 1);
                int v11 = (i + 1) * gridSize + (j + 1);
                tris.row(t++) << v00, v10, v01;
                tris.row(t++) << v10, v11, v01;
            }
        }

        MatrixX3f normals = FsSurface::compute_normals(verts, tris);
        QCOMPARE(normals.rows(), nVerts);
    }

    //=========================================================================
    // FsSurface accessors
    //=========================================================================
    void surface_defaultCtor()
    {
        FsSurface s;
        QVERIFY(s.isEmpty());
        QCOMPARE(s.hemi(), -1);
    }

    void surface_offset()
    {
        FsSurface s;
        s.offset() = Vector3f(1.0f, 2.0f, 3.0f);
        QVERIFY(qAbs(s.offset()(0) - 1.0f) < 1e-5f);
    }

    void surface_filePath()
    {
        FsSurface s;
        QVERIFY(s.filePath().isEmpty());
    }

    //=========================================================================
    // FsSurfaceSet
    //=========================================================================
    void surfaceSet_defaultCtor()
    {
        FsSurfaceSet ss;
        QVERIFY(ss.isEmpty());
        QCOMPARE(ss.size(), 0);
    }

    void surfaceSet_insertAndAccess()
    {
        FsSurfaceSet ss;
        FsSurface s;
        ss.insert(s);
        QVERIFY(ss.size() >= 0);
    }

    void surfaceSet_twoSurfaces()
    {
        FsSurface lh, rh;
        FsSurfaceSet ss(lh, rh);
        QVERIFY(ss.size() >= 0);
    }

    void surfaceSet_clear()
    {
        FsSurfaceSet ss;
        ss.clear();
        QVERIFY(ss.isEmpty());
    }

    //=========================================================================
    // FsAnnotation
    //=========================================================================
    void annotation_defaultCtor()
    {
        FsAnnotation a;
        QVERIFY(a.isEmpty());
        QCOMPARE(a.hemi(), -1);
    }

    void annotation_clear()
    {
        FsAnnotation a;
        a.clear();
        QVERIFY(a.isEmpty());
    }

    void annotation_getters()
    {
        FsAnnotation a;
        VectorXi verts = a.getVertices();
        QCOMPARE(verts.size(), 0);
        VectorXi labels = a.getLabelIds();
        QCOMPARE(labels.size(), 0);
    }

    //=========================================================================
    // FsAnnotationSet
    //=========================================================================
    void annotationSet_defaultCtor()
    {
        FsAnnotationSet as;
        QVERIFY(as.isEmpty());
        QCOMPARE(as.size(), 0);
    }

    void annotationSet_twoAnnotations()
    {
        FsAnnotation lh, rh;
        FsAnnotationSet as(lh, rh);
        QVERIFY(as.size() >= 0);
    }

    void annotationSet_clear()
    {
        FsAnnotationSet as;
        as.clear();
        QVERIFY(as.isEmpty());
    }

    void annotationSet_insert()
    {
        FsAnnotationSet as;
        FsAnnotation a;
        as.insert(a);
        QVERIFY(as.size() >= 0);
    }

    //=========================================================================
    // FsLabel
    //=========================================================================
    void label_operations()
    {
        VectorXi verts(5);
        verts << 10, 20, 30, 40, 50;
        MatrixX3f pos(5, 3);
        pos.setRandom();
        VectorXd vals(5);
        vals.setOnes();

        FsLabel l(verts, pos, vals, 0, "TestLabel", 1);
        QVERIFY(!l.isEmpty());
        QCOMPARE(l.vertices.size(), (Eigen::Index)5);

        l.clear();
        QVERIFY(l.isEmpty());
    }

    //=========================================================================
    // FsColortable
    //=========================================================================
    void colortable_defaultCtor()
    {
        FsColortable ct;
        QCOMPARE(ct.numEntries, 0);
    }

    void colortable_clear()
    {
        FsColortable ct;
        ct.clear();
        QCOMPARE(ct.numEntries, 0);
    }
};

QTEST_GUILESS_MAIN(TestFsSurfaceAnnotation)
#include "test_fs_surface_annotation.moc"
