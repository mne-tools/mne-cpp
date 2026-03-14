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

class TestFsAnnotationLabels : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FsLabel
    //=========================================================================
    void label_defaultCtor()
    {
        FsLabel l;
        QVERIFY(l.isEmpty());
        QCOMPARE(l.hemi, -1);
    }

    void label_paramCtor()
    {
        VectorXi verts(3);
        verts << 0, 1, 2;
        MatrixX3f pos(3, 3);
        pos << 0, 0, 0,  1, 0, 0,  0, 1, 0;
        VectorXd vals = VectorXd::Ones(3);

        FsLabel l(verts, pos, vals, 0, "test-lh", 42);
        QVERIFY(!l.isEmpty());
        QCOMPARE(l.hemi, 0);
        QCOMPARE(l.name, QString("test-lh"));
        QCOMPARE(l.label_id, 42);
        QCOMPARE(l.vertices.size(), (Eigen::Index)3);
    }

    void label_clear()
    {
        VectorXi verts(2);
        verts << 0, 1;
        MatrixX3f pos(2, 3);
        pos << 0, 0, 0,  1, 0, 0;
        VectorXd vals = VectorXd::Ones(2);

        FsLabel l(verts, pos, vals, 0, "test", 1);
        QVERIFY(!l.isEmpty());
        l.clear();
        QVERIFY(l.isEmpty());
    }

    void label_selectTrisMatrix()
    {
        // Create label with vertices {0, 2}
        VectorXi verts(2);
        verts << 0, 2;
        MatrixX3f pos(2, 3);
        pos << 0, 0, 0,  0, 1, 0;
        VectorXd vals = VectorXd::Ones(2);
        FsLabel l(verts, pos, vals, 0, "test", 1);

        // triangles: only tri(0,1,2) contains both label vertices
        MatrixX3i tris(2, 3);
        tris << 0, 1, 2,
                3, 4, 5;

        MatrixX3i sel = l.selectTris(tris);
        // At least one triangle should be selected (0,1,2 includes vertex 0 and 2)
        QVERIFY(sel.rows() >= 1);
    }

    //=========================================================================
    // FsSurface - synthetic data
    //=========================================================================
    void surface_defaultCtor()
    {
        FsSurface s;
        QVERIFY(s.isEmpty());
        QCOMPARE(s.hemi(), -1);
    }

    void surface_computeNormals()
    {
        // Simple triangle: three vertices, one triangle
        MatrixX3f rr(3, 3);
        rr << 0, 0, 0,
              1, 0, 0,
              0, 1, 0;
        MatrixX3i tris(1, 3);
        tris << 0, 1, 2;

        MatrixX3f nn = FsSurface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)3);
        QCOMPARE(nn.cols(), (Eigen::Index)3);

        // All normals for this flat triangle should be roughly (0,0,1)
        for (int i = 0; i < 3; ++i) {
            QVERIFY(std::abs(nn(i, 2)) > 0.9f);
        }
    }

    void surface_computeNormals_quad()
    {
        // Two triangles forming a quad
        MatrixX3f rr(4, 3);
        rr << 0, 0, 0,
              1, 0, 0,
              1, 1, 0,
              0, 1, 0;
        MatrixX3i tris(2, 3);
        tris << 0, 1, 2,
                0, 2, 3;

        MatrixX3f nn = FsSurface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)4);
        // All normals should point in Z direction
        for (int i = 0; i < 4; ++i) {
            QVERIFY(std::abs(nn(i, 2)) > 0.9f);
        }
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
        FsSurface s;
        // An empty surface has hemi == -1, so insert will skip it
        FsSurfaceSet ss;
        ss.insert(s);
        // Should still be empty because the surface is empty/invalid
        QCOMPARE(ss.size(), 0);
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

    void annotation_getters()
    {
        FsAnnotation a;
        VectorXi& verts = a.getVertices();
        QCOMPARE(verts.size(), (Eigen::Index)0);

        VectorXi& labels = a.getLabelIds();
        QCOMPARE(labels.size(), (Eigen::Index)0);
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

    void annotationSet_insertEmpty()
    {
        FsAnnotationSet as;
        FsAnnotation a; // empty
        as.insert(a);
        // Should remain empty because the annotation is empty
        QCOMPARE(as.size(), 0);
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
        ct.numEntries = 10;
        ct.clear();
        QCOMPARE(ct.numEntries, 0);
    }

    //=========================================================================
    // FsAnnotation::toLabels with synthetic data
    //=========================================================================
    void annotation_toLabels_emptyAnnotation()
    {
        FsAnnotation a;
        FsSurface s;
        QList<FsLabel> labels;
        QList<RowVector4i> rgbas;

        bool ok = a.toLabels(s, labels, rgbas);
        // Should fail or return empty gracefully on empty annotation
        Q_UNUSED(ok);
        // Just verify no crash
        QVERIFY(true);
    }

    //=========================================================================
    // FsSurface - file based (QSKIP if not available)
    //=========================================================================
    void surface_readFile()
    {
        QString testData = QCoreApplication::applicationDirPath()
                           + "/../resources/data/mne-cpp-test-data/subjects/sample/surf/lh.white";
        if (!QFileInfo::exists(testData)) {
            QSKIP("Test data not available");
        }
        FsSurface s(testData);
        QVERIFY(!s.isEmpty());
        QVERIFY(s.rr().rows() > 0);
        QVERIFY(s.tris().rows() > 0);
    }

    void annotation_readFile()
    {
        QString testData = QCoreApplication::applicationDirPath()
                           + "/../resources/data/mne-cpp-test-data/subjects/sample/label/lh.aparc.annot";
        if (!QFileInfo::exists(testData)) {
            QSKIP("Test data not available");
        }
        FsAnnotation a(testData);
        QVERIFY(!a.isEmpty());
    }

    //=========================================================================
    // FsSurface combined: read + toLabels
    //=========================================================================
    void annotation_toLabels_withFile()
    {
        QString surfFile = QCoreApplication::applicationDirPath()
                           + "/../resources/data/mne-cpp-test-data/subjects/sample/surf/lh.white";
        QString annotFile = QCoreApplication::applicationDirPath()
                            + "/../resources/data/mne-cpp-test-data/subjects/sample/label/lh.aparc.annot";
        if (!QFileInfo::exists(surfFile) || !QFileInfo::exists(annotFile)) {
            QSKIP("Test data not available");
        }

        FsSurface surf(surfFile);
        FsAnnotation annot(annotFile);

        QList<FsLabel> labels;
        QList<RowVector4i> rgbas;
        bool ok = annot.toLabels(surf, labels, rgbas);
        QVERIFY(ok);
        QVERIFY(labels.size() > 0);
        QCOMPARE(labels.size(), rgbas.size());
    }
};

QTEST_GUILESS_MAIN(TestFsAnnotationLabels)
#include "test_fs_annotation_labels.moc"
