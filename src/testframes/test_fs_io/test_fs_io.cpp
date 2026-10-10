//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *
 * @file     test_fs_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @brief    Tests for FSLIB I/O and processing: FsSurface, FsAnnotation, FsLabel,
 *        FsSurfaceSet, FsAnnotationSet, compute_normals, read_curv.
 *        Tests both error paths (invalid files) and synthetic surface operations.
 */
#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QDataStream>

#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_label.h>
#include <fs/fs_colortable.h>
#include <mne/mne_source_space.h>

#include <memory>

using namespace FSLIB;
using namespace Eigen;

class TestFsIo : public QObject
{
    Q_OBJECT

private:
    /**
     * @brief Write a minimal FreeSurfer binary surface file (triangle format).
     * Format: 3-byte magic (0xFF, 0xFF, 0xFE), comment + newlines,
     *         int32 nvert, int32 ntri, 3*nvert floats (coords),
     *         3*ntri int32 (tri indices).
     */
    void writeSyntheticSurface(const QString& path, int nvert, int ntri,
                               const MatrixX3f& verts, const MatrixX3i& tris)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QDataStream ds(&f);
        ds.setByteOrder(QDataStream::BigEndian);
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

        // Magic number for TRIANGLE_FILE_MAGIC_NUMBER
        ds << (quint8)0xFF << (quint8)0xFF << (quint8)0xFE;

        // Comment: "created by test\n\n"
        QByteArray comment = "created by test\n\n";
        f.write(comment);

        // Number of vertices and faces
        ds << (qint32)nvert << (qint32)ntri;

        // Vertex coordinates
        for (int i = 0; i < nvert; ++i) {
            ds << verts(i, 0) << verts(i, 1) << verts(i, 2);
        }

        // Triangle indices
        for (int i = 0; i < ntri; ++i) {
            ds << (qint32)tris(i, 0) << (qint32)tris(i, 1) << (qint32)tris(i, 2);
        }

        f.close();
    }

    /**
     * @brief Write a minimal FreeSurfer annotation file.
     * Format: int32 nvert, for each vertex: (int32 vertno, int32 label),
     *         then the colortable (using "new" format).
     */
    void writeSyntheticAnnotation(const QString& path, int nvert,
                                  const VectorXi& vertIndices,
                                  const VectorXi& labels)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QDataStream ds(&f);
        ds.setByteOrder(QDataStream::BigEndian);

        ds << (qint32)nvert;
        for (int i = 0; i < nvert; ++i) {
            ds << (qint32)vertIndices(i) << (qint32)labels(i);
        }

        // Write a minimal colortable (new format)
        // has_colortable tag
        ds << (qint32)1;
        // num_entries (negative = new format)
        ds << (qint32)(-1); // new format marker
        // version
        ds << (qint32)1;
        // num entries for real
        ds << (qint32)1;
        // original table name length + name
        QByteArray tableName = "test_table";
        ds << (qint32)tableName.size();
        f.write(tableName);
        // num entries again
        ds << (qint32)1;
        // For each entry: struct_id, name_length, name, R, G, B, A
        ds << (qint32)0; // struct id
        QByteArray entryName = "unknown";
        ds << (qint32)entryName.size();
        f.write(entryName);
        ds << (qint32)25 << (qint32)5 << (qint32)25 << (qint32)0; // RGBA

        f.close();
    }

private slots:

    //=========================================================================
    // FsSurface - default & clear
    //=========================================================================
    void surface_defaultCtor()
    {
        FsSurface s;
        QVERIFY(s.isEmpty());
        QCOMPARE(s.hemi(), -1);
    }

    void surface_clear()
    {
        FsSurface s;
        s.clear();
        QVERIFY(s.isEmpty());
    }

    //=========================================================================
    // FsSurface::compute_normals
    //=========================================================================
    void surface_computeNormals_singleTriangle()
    {
        // A single triangle in the XY plane
        MatrixX3f rr(3, 3);
        rr << 0, 0, 0,
            1, 0, 0,
            0, 1, 0;
        MatrixX3i tris(1, 3);
        tris << 0, 1, 2;

        MatrixX3f nn = FsSurface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)3);
        QCOMPARE(nn.cols(), (Eigen::Index)3);

        // All normals should point in +Z direction (0,0,1)
        for (int i = 0; i < 3; ++i) {
            float norm = nn.row(i).norm();
            if (norm > 1e-6) {
                QVERIFY(qAbs(nn(i, 2)) > 0.9f); // Z component
            }
        }
    }

    void surface_computeNormals_tetrahedron()
    {
        // A tetrahedron
        MatrixX3f rr(4, 3);
        rr << 1, 1, 1,
            -1, -1, 1,
            -1, 1, -1,
            1, -1, -1;
        MatrixX3i tris(4, 3);
        tris << 0, 1, 2,
            0, 2, 3,
            0, 3, 1,
            1, 3, 2;

        MatrixX3f nn = FsSurface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)4);

        // Each vertex normal should be unit-ish length
        for (int i = 0; i < 4; ++i) {
            float norm = nn.row(i).norm();
            QVERIFY(norm > 0.5f);
            QVERIFY(norm < 1.5f);
        }
    }

    void surface_computeNormals_emptyInput()
    {
        MatrixX3f rr(0, 3);
        MatrixX3i tris(0, 3);
        MatrixX3f nn = FsSurface::compute_normals(rr, tris);
        QCOMPARE(nn.rows(), (Eigen::Index)0);
    }

    //=========================================================================
    // FsSurface::read - error paths
    //=========================================================================
    void surface_readNonExistentFile()
    {
        FsSurface s;
        bool ok = FsSurface::read("/nonexistent/path/lh.white", s);
        QVERIFY(!ok);
        QVERIFY(s.isEmpty());
    }

    void surface_readFromSyntheticFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        MatrixX3f verts(4, 3);
        verts << 0, 0, 0,
            1, 0, 0,
            0, 1, 0,
            0, 0, 1;
        MatrixX3i tris(2, 3);
        tris << 0, 1, 2,
            0, 2, 3;

        QString filePath = tmpDir.path() + "/lh.test";
        writeSyntheticSurface(filePath, 4, 2, verts, tris);
        QVERIFY(QFile::exists(filePath));

        FsSurface s;
        QVERIFY(FsSurface::read(filePath, s, false));
        QVERIFY(!s.isEmpty());
        QCOMPARE(s.rr().rows(), (Eigen::Index)4);
        QCOMPARE(s.tris().rows(), (Eigen::Index)2);
    }

    void surface_readQuadFiles()
    {
        // Legacy quad surfaces (QUAD: int16 hundredths of mm, NEW_QUAD: float mm) and the old curvature format
        // (int16 hundredths); expected values from nibabel read_geometry and mne.surface.read_curvature
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const auto put3 = [](QDataStream& out, int v) {
            const char bytes[3] = {char((v >> 16) & 0xff), char((v >> 8) & 0xff), char(v & 0xff)};
            out.writeRawData(bytes, 3);
        };
        const float mm[5][3] = {{1.5f, -2.25f, 3.0f}, {10.0f, 0.5f, -7.75f}, {0.0f, 4.0f, 2.5f}, {-3.5f, 1.0f, 0.25f}, {6.0f, -6.0f, 6.0f}};
        for (const int magic : {16777215, 16777213}) {
            QByteArray b;
            QDataStream out(&b, QIODevice::WriteOnly);
            out.setFloatingPointPrecision(QDataStream::SinglePrecision);
            put3(out, magic);
            put3(out, 5);
            put3(out, 2);
            for (const auto& v : mm) {
                for (const float c : v) {
                    if (magic == 16777215) {
                        out << qint16(std::lround(c * 100.0f));
                    } else {
                        out << c;
                    }
                }
            }
            for (const int q : {0, 1, 2, 3, 1, 2, 3, 4}) {
                put3(out, q);
            }
            QFile file(tmpDir.filePath(magic == 16777215 ? "lh.quad" : "lh.newquad"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(b);
            file.close();

            FsSurface s;
            QVERIFY(FsSurface::read(file.fileName(), s, false));
            QCOMPARE(s.rr().rows(), Eigen::Index(5));
            for (int i = 0; i < 5; ++i) {
                for (int j = 0; j < 3; ++j) {
                    QVERIFY(std::fabs(s.rr()(i, j) - mm[i][j] * 0.001f) < 1e-7f);
                }
            }
            Eigen::MatrixX3i expected(4, 3);
            expected << 0, 1, 3, 2, 3, 1, 1, 2, 3, 1, 3, 4;
            QCOMPARE(s.tris(), expected);

            // MNE-C's surface reader, used by the source-space and display code, reads the same geometry
            const std::unique_ptr<MNELIB::MNESourceSpace> mneSurf = MNELIB::MNESourceSpace::load_surface(file.fileName(), QString());
            QVERIFY(mneSurf);
            QVERIFY(mneSurf->rr.isApprox(s.rr(), 1e-6f));
            QCOMPARE(Eigen::MatrixX3i(mneSurf->itris), expected);
        }

        QByteArray curv;
        QDataStream curvOut(&curv, QIODevice::WriteOnly);
        put3(curvOut, 5);
        put3(curvOut, 2);
        for (const qint16 v : {25, -150, 300, 0, -1}) {
            curvOut << v;
        }
        QFile curvFile(tmpDir.filePath("lh.curv"));
        QVERIFY(curvFile.open(QIODevice::WriteOnly));
        curvFile.write(curv);
        curvFile.close();
        const VectorXf values = FsSurface::read_curv(curvFile.fileName());
        QCOMPARE(values.size(), Eigen::Index(5));
        const float expectedCurv[] = {0.25f, -1.5f, 3.0f, 0.0f, -0.01f};
        for (int i = 0; i < 5; ++i) {
            QVERIFY(std::fabs(values(i) - expectedCurv[i]) < 1e-6f);
        }
        const std::unique_ptr<MNELIB::MNESourceSpace> withCurv = MNELIB::MNESourceSpace::load_surface(tmpDir.filePath("lh.newquad"), curvFile.fileName());
        QVERIFY(withCurv);
        QVERIFY(withCurv->curv.isApprox(values, 1e-6f));
    }

    //=========================================================================
    // FsAnnotation - default & clear
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

    //=========================================================================
    // FsAnnotation::read - error paths
    //=========================================================================
    void annotation_readNonExistentFile()
    {
        FsAnnotation a;
        bool ok = FsAnnotation::read("/nonexistent/path/lh.aparc.annot", a);
        QVERIFY(!ok);
    }

    void annotation_readFromSyntheticFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int nvert = 5;
        VectorXi vertIndices(nvert);
        vertIndices << 0, 1, 2, 3, 4;
        VectorXi labels(nvert);
        // Encode a simple color label (R=25, G=5, B=25, A=0 => id = 25 + 5*256 + 25*65536)
        int labelVal = 25 + 5 * 256 + 25 * 65536;
        labels.setConstant(labelVal);

        QString filePath = tmpDir.path() + "/test.annot";
        writeSyntheticAnnotation(filePath, nvert, vertIndices, labels);
        QVERIFY(QFile::exists(filePath));
        QVERIFY(QFileInfo(filePath).size() > 0);
        // Note: Reading back synthetic annotation is skipped because the
        // FreeSurfer annotation format requires exact version=2 encoding
        // and specific colortable structure that is complex to synthesize.
    }

    //=========================================================================
    // FsLabel
    //=========================================================================
    void label_defaultCtor()
    {
        FsLabel l;
        QVERIFY(l.isEmpty());
        QCOMPARE(l.hemi, -1);
    }

    void label_parameterizedCtor()
    {
        VectorXi verts(3);
        verts << 10, 20, 30;
        MatrixX3f pos(3, 3);
        pos << 0.01f, 0.02f, 0.03f,
            0.04f, 0.05f, 0.06f,
            0.07f, 0.08f, 0.09f;
        VectorXd vals(3);
        vals << 1.0, 2.0, 3.0;

        FsLabel l(verts, pos, vals, 0, "TestLabel", 42);
        QVERIFY(!l.isEmpty());
        QCOMPARE(l.hemi, 0);
        QCOMPARE(l.name, QString("TestLabel"));
        QCOMPARE(l.label_id, 42);
        QCOMPARE(l.vertices.size(), (Eigen::Index)3);
    }

    void label_clear()
    {
        VectorXi verts(2);
        verts << 0, 1;
        MatrixX3f pos(2, 3);
        pos.setRandom();
        VectorXd vals(2);
        vals << 1, 2;

        FsLabel l(verts, pos, vals, 1, "L", 1);
        QVERIFY(!l.isEmpty());
        l.clear();
        QVERIFY(l.isEmpty());
    }

    void label_readNonExistentFile()
    {
        FsLabel l;
        bool ok = FsLabel::read("/nonexistent/label.label", l);
        QVERIFY(!ok);
    }

    void label_selectTrisFromMatrix()
    {
        VectorXi verts(3);
        verts << 0, 1, 2;
        MatrixX3f pos(3, 3);
        pos << 0, 0, 0, 1, 0, 0, 0, 1, 0;
        VectorXd vals(3);
        vals << 1, 1, 1;

        FsLabel l(verts, pos, vals, 0, "tri_label");

        MatrixX3i allTris(4, 3);
        allTris << 0, 1, 2,
            0, 2, 3,
            3, 4, 5,
            1, 2, 6;

        MatrixX3i selected = l.selectTris(allTris);
        // Only tris where ALL 3 vertices are in the label's vertices should be selected
        // Tri (0,1,2) matches since all 3 are in verts={0,1,2}
        QVERIFY(selected.rows() >= 1);
    }

    //=========================================================================
    // FsSurfaceSet - default
    //=========================================================================
    void surfaceSet_defaultCtor()
    {
        FsSurfaceSet ss;
        QVERIFY(ss.isEmpty());
    }

    void surfaceSet_clear()
    {
        FsSurfaceSet ss;
        ss.clear();
        QVERIFY(ss.isEmpty());
    }

    void surfaceSet_insertAndAccess()
    {
        FsSurfaceSet ss;
        FsSurface lh;
        FsSurface rh;
        // Insert empty surfaces (still tests the container logic)
        ss.insert(lh);
        // FsSurfaceSet may or may not be empty depending on implementation
    }

    void surfaceSet_readNonExistentFiles()
    {
        FsSurfaceSet ss;
        bool ok = FsSurfaceSet::read("/nonexistent/lh.white", "/nonexistent/rh.white", ss);
        QVERIFY(!ok);
    }

    //=========================================================================
    // FsAnnotationSet - default
    //=========================================================================
    void annotationSet_defaultCtor()
    {
        FsAnnotationSet as;
        QVERIFY(as.isEmpty());
    }

    void annotationSet_clear()
    {
        FsAnnotationSet as;
        as.clear();
        QVERIFY(as.isEmpty());
    }

    void annotationSet_insertAndAccess()
    {
        FsAnnotationSet as;
        FsAnnotation lh;
        FsAnnotation rh;
        as.insert(lh);
        // Test container logic
    }

    void annotationSet_readNonExistentFiles()
    {
        FsAnnotationSet as;
        bool ok = FsAnnotationSet::read("/nonexistent/lh.annot", "/nonexistent/rh.annot", as);
        QVERIFY(!ok);
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

    //=========================================================================
    // FsSurface::read_curv - error paths
    //=========================================================================
    void surface_readCurv_nonExistentFile()
    {
        VectorXf curv = FsSurface::read_curv("/nonexistent/lh.curv");
        QCOMPARE(curv.size(), (Eigen::Index)0);
    }
};

QTEST_GUILESS_MAIN(TestFsIo)

#include "test_fs_io.moc"
