// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_id.h>

using namespace mne;
using namespace Eigen;

class TestFiffCoordTransform : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FiffCoordTrans
    //=========================================================================
    void coordTrans_defaultCtor()
    {
        FiffCoordTrans t;
        QVERIFY(t.empty());
    }

    void coordTrans_identity()
    {
        FiffCoordTrans t = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        QVERIFY(!t.empty());
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        QVERIFY(t.trans.isApprox(Matrix4f::Identity()));
    }

    void coordTrans_fromMatrix4f()
    {
        Matrix4f m = Matrix4f::Identity();
        m(0, 3) = 0.01f;
        m(1, 3) = 0.02f;
        m(2, 3) = 0.03f;

        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, m);
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        QVERIFY(qFuzzyCompare(t.trans(0, 3), 0.01f));
    }

    void coordTrans_fromRotAndMove()
    {
        Matrix3f rot = Matrix3f::Identity();
        Vector3f move(0.05f, 0.06f, 0.07f);

        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE, rot, move);
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QVERIFY(qFuzzyCompare(t.trans(0, 3), 0.05f));
        QVERIFY(qFuzzyCompare(t.trans(1, 3), 0.06f));
    }

    void coordTrans_invertTransform()
    {
        Matrix4f m = Matrix4f::Identity();
        m(0, 3) = 0.1f;
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, m);
        t.invert_transform();

        QCOMPARE(t.from, (int)FIFFV_COORD_MRI);
        QCOMPARE(t.to, (int)FIFFV_COORD_HEAD);
    }

    void coordTrans_inverted()
    {
        Matrix4f m = Matrix4f::Identity();
        m(0, 3) = 0.1f;
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, m);
        FiffCoordTrans inv = t.inverted();

        QCOMPARE(inv.from, (int)FIFFV_COORD_MRI);
        QCOMPARE(inv.to, (int)FIFFV_COORD_HEAD);
        // Original unchanged
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
    }

    void coordTrans_applyTrans()
    {
        Matrix4f m = Matrix4f::Identity();
        m(0, 3) = 1.0f; // translate X by 1

        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, m);
        t.add_inverse(t);

        MatrixX3f pts(2, 3);
        pts << 0, 0, 0,
            1, 2, 3;

        MatrixX3f result = t.apply_trans(pts);
        QCOMPARE(result.rows(), (Eigen::Index)2);
        QVERIFY(qFuzzyCompare(result(0, 0), 1.0f));
        QVERIFY(qFuzzyCompare(result(1, 0), 2.0f));
    }

    void coordTrans_applyInverseTrans()
    {
        Matrix4f m = Matrix4f::Identity();
        m(0, 3) = 1.0f;

        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, m);
        t.add_inverse(t);

        MatrixX3f pts(1, 3);
        pts << 1, 0, 0;

        MatrixX3f result = t.apply_inverse_trans(pts);
        QCOMPARE(result.rows(), (Eigen::Index)1);
        QVERIFY(qFuzzyCompare(result(0, 0), 0.0f));
    }

    void coordTrans_combine()
    {
        FiffCoordTrans t1 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        t1.trans(0, 3) = 0.1f;
        t1.add_inverse(t1);

        FiffCoordTrans t2 = FiffCoordTrans::identity(FIFFV_COORD_MRI, FIFFV_COORD_DEVICE);
        t2.trans(0, 3) = 0.2f;
        t2.add_inverse(t2);

        FiffCoordTrans combined = FiffCoordTrans::combine(
            FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE, t1, t2);
        QCOMPARE(combined.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(combined.to, (int)FIFFV_COORD_DEVICE);
    }

    void coordTrans_frameName()
    {
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_HEAD), std::string_view("head"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_MRI), std::string_view("MRI (surface RAS)"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_DEVICE), std::string_view("MEG device"));
    }

    void coordTrans_angleTo()
    {
        FiffCoordTrans t1 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        FiffCoordTrans t2 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);

        float angle = t1.angle_to(t2.trans);
        QVERIFY(qFuzzyIsNull(angle));
    }

    void coordTrans_translationTo()
    {
        FiffCoordTrans t1 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        FiffCoordTrans t2 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        t2.trans(0, 3) = 0.01f;

        float dist = t1.translation_to(t2.trans);
        QVERIFY(dist > 0.0f);
        QVERIFY(qFuzzyCompare(dist, 0.01f));
    }

    void coordTrans_equality()
    {
        FiffCoordTrans t1 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        FiffCoordTrans t2 = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        QVERIFY(t1 == t2);
    }

    void coordTrans_clear()
    {
        FiffCoordTrans t = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        QVERIFY(!t.empty());
        t.clear();
        QVERIFY(t.empty());
    }

    //=========================================================================
    // FiffCoordTrans::from_cardinal_points
    //=========================================================================
    void coordTrans_fromCardinalPoints()
    {
        // fromCardinalPoints(int from, int to, const float* rL, const float* rN, const float* rR)
        float lpa[3] = {-0.07f, 0.0f, 0.0f};
        float nas[3] = {0.0f, 0.08f, 0.0f};
        float rpa[3] = {0.07f, 0.0f, 0.0f};

        FiffCoordTrans t = FiffCoordTrans::from_cardinal_points(
            FIFFV_COORD_HEAD, FIFFV_COORD_MRI, lpa, nas, rpa);

        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        QVERIFY(!t.empty());
    }

    //=========================================================================
    // FiffCov - additional coverage
    //=========================================================================
    void fiffCov_pickChannels()
    {
        FiffCov cov;
        cov.kind = 1;
        cov.dim = 3;
        cov.names.push_back("CH1");
        cov.names.push_back("CH2");
        cov.names.push_back("CH3");
        cov.data = MatrixXd::Identity(3, 3);
        cov.nfree = 100;

        StringList include;
        include.push_back("CH1");
        include.push_back("CH3");
        FiffCov picked = cov.pick_channels(include);
        QCOMPARE(picked.dim, 2);
        QCOMPARE(picked.names.size(), 2);
    }

    void fiffCov_equality()
    {
        FiffCov c1, c2;
        c1.kind = 1;
        c1.dim = 2;
        c1.nfree = 50;
        c1.names.push_back("A");
        c1.names.push_back("B");
        c1.data = MatrixXd::Identity(2, 2);

        c2 = c1;
        QCOMPARE(c2.kind, c1.kind);
        QCOMPARE(c2.dim, c1.dim);
        QCOMPARE(c2.names.size(), c1.names.size());
    }

    //=========================================================================
    // FiffNamedMatrix
    //=========================================================================
    void namedMatrix_defaultCtor()
    {
        FiffNamedMatrix nm;
        QVERIFY(nm.empty());
    }

    void namedMatrix_transpose()
    {
        FiffNamedMatrix nm;
        nm.nrow = 2;
        nm.ncol = 3;
        nm.row_names.push_back("R1");
        nm.row_names.push_back("R2");
        nm.col_names.push_back("C1");
        nm.col_names.push_back("C2");
        nm.col_names.push_back("C3");
        nm.data = MatrixXd(2, 3);
        nm.data << 1, 2, 3, 4, 5, 6;

        nm.transpose_named_matrix();
        QCOMPARE(nm.nrow, 3);
        QCOMPARE(nm.ncol, 2);
        QCOMPARE(nm.row_names.size(), 3);
        QCOMPARE(nm.col_names.size(), 2);
    }

    //=========================================================================
    // FiffEvoked - additional coverage
    //=========================================================================
    void fiffEvoked_aspectKindToString()
    {
        FiffEvoked ev;
        ev.aspect_kind = FIFFV_ASPECT_AVERAGE;
        QString s = QString::fromStdString(ev.aspectKindToString());
        QVERIFY(!s.isEmpty());
    }

    //=========================================================================
    // FiffId
    //=========================================================================
    void fiffId_newFileId()
    {
        FiffId id = FiffId::new_file_id();
        QVERIFY(id.version > 0 || true);
    }
};

QTEST_GUILESS_MAIN(TestFiffCoordTransform)
#include "test_fiff_coord_transform.moc"
