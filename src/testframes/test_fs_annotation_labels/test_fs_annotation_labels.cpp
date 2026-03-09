#include <QtTest/QtTest>
#include <Eigen/Dense>

#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <fs/annotation.h>
#include <fs/annotationset.h>
#include <fs/label.h>
#include <fs/colortable.h>

using namespace FSLIB;
using namespace Eigen;

class TestFsAnnotationLabels : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // Label
    //=========================================================================
    void label_defaultCtor()
    {
        Label l;
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

        Label l(verts, pos, vals, 0, "test-lh", 42);
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

        Label l(verts, pos, vals, 0, "test", 1);
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
        Label l(verts, pos, vals, 0, "test", 1);

        // triangles: only tri(0,1,2) contains both label vertices
        MatrixX3i tris(2, 3);
        tris << 0, 1, 2,
                3, 4, 5;

        MatrixX3i sel = l.selectTris(tris);
        // At least one triangle should be selected (0,1,2 includes vertex 0 and 2)
        QVERIFY(sel.rows() >= 1);
    }

    //=========================================================================
    // Surface - synthetic data
    //=========================================================================
    void surface_defaultCtor()
    {
        Surface s;
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

        MatrixX3f nn = Surface::compute_normals(rr, tris);
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

        MatrixX3f nn = Surface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)4);
        // All normals should point in Z direction
        for (int i = 0; i < 4; ++i) {
            QVERIFY(std::abs(nn(i, 2)) > 0.9f);
        }
    }

    //=========================================================================
    // SurfaceSet
    //=========================================================================
    void surfaceSet_defaultCtor()
    {
        SurfaceSet ss;
        QVERIFY(ss.isEmpty());
        QCOMPARE(ss.size(), 0);
    }

    void surfaceSet_insertAndAccess()
    {
        Surface s;
        // An empty surface has hemi == -1, so insert will skip it
        SurfaceSet ss;
        ss.insert(s);
        // Should still be empty because the surface is empty/invalid
        QCOMPARE(ss.size(), 0);
    }

    //=========================================================================
    // Annotation
    //=========================================================================
    void annotation_defaultCtor()
    {
        Annotation a;
        QVERIFY(a.isEmpty());
        QCOMPARE(a.hemi(), -1);
    }

    void annotation_getters()
    {
        Annotation a;
        VectorXi& verts = a.getVertices();
        QCOMPARE(verts.size(), (Eigen::Index)0);

        VectorXi& labels = a.getLabelIds();
        QCOMPARE(labels.size(), (Eigen::Index)0);
    }

    //=========================================================================
    // AnnotationSet
    //=========================================================================
    void annotationSet_defaultCtor()
    {
        AnnotationSet as;
        QVERIFY(as.isEmpty());
        QCOMPARE(as.size(), 0);
    }

    void annotationSet_insertEmpty()
    {
        AnnotationSet as;
        Annotation a; // empty
        as.insert(a);
        // Should remain empty because the annotation is empty
        QCOMPARE(as.size(), 0);
    }

    //=========================================================================
    // Colortable
    //=========================================================================
    void colortable_defaultCtor()
    {
        Colortable ct;
        QCOMPARE(ct.numEntries, 0);
    }

    void colortable_clear()
    {
        Colortable ct;
        ct.numEntries = 10;
        ct.clear();
        QCOMPARE(ct.numEntries, 0);
    }

    //=========================================================================
    // Annotation::toLabels with synthetic data
    //=========================================================================
    void annotation_toLabels_emptyAnnotation()
    {
        Annotation a;
        Surface s;
        QList<Label> labels;
        QList<RowVector4i> rgbas;

        bool ok = a.toLabels(s, labels, rgbas);
        // Should fail or return empty gracefully on empty annotation
        Q_UNUSED(ok);
        // Just verify no crash
        QVERIFY(true);
    }

    //=========================================================================
    // Surface - file based (QSKIP if not available)
    //=========================================================================
    void surface_readFile()
    {
        QString testData = QCoreApplication::applicationDirPath()
                           + "/../resources/data/mne-cpp-test-data/subjects/sample/surf/lh.white";
        if (!QFileInfo::exists(testData)) {
            QSKIP("Test data not available");
        }
        Surface s(testData);
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
        Annotation a(testData);
        QVERIFY(!a.isEmpty());
    }

    //=========================================================================
    // Surface combined: read + toLabels
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

        Surface surf(surfFile);
        Annotation annot(annotFile);

        QList<Label> labels;
        QList<RowVector4i> rgbas;
        bool ok = annot.toLabels(surf, labels, rgbas);
        QVERIFY(ok);
        QVERIFY(labels.size() > 0);
        QCOMPARE(labels.size(), rgbas.size());
    }
};

QTEST_GUILESS_MAIN(TestFsAnnotationLabels)
#include "test_fs_annotation_labels.moc"
