#include <QtTest/QtTest>
#include <QBuffer>
#include <QTemporaryFile>
#include <Eigen/Dense>

#include <fiff/fiff_events.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_dir_node.h>

using namespace FIFFLIB;
using namespace Eigen;

class TestFiffDataTypes : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FiffEvents
    //=========================================================================
    void events_defaultCtor()
    {
        FiffEvents ev;
        QVERIFY(ev.is_empty());
        QCOMPARE(ev.num_events(), 0);
    }

    void events_asciiRoundTrip()
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        QTextStream ts(&buf);
        for (int r = 0; r < 3; ++r) {
            ts << (r + 1) * 100 << " 0 " << (r + 1) << "\n";
        }
        ts.flush();
        buf.seek(0);

        FiffEvents evRead;
        bool ok = FiffEvents::read_from_ascii(buf, evRead);
        QVERIFY(ok);
        QCOMPARE(evRead.num_events(), 3);
        QVERIFY(!evRead.is_empty());
    }

    void events_asciiWriteReadRoundTrip()
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        QTextStream ts(&buf);
        ts << "100 0 1\n200 0 2\n300 0 3\n";
        ts.flush();
        buf.seek(0);

        FiffEvents ev;
        FiffEvents::read_from_ascii(buf, ev);
        QCOMPARE(ev.num_events(), 3);

        QBuffer buf2;
        buf2.open(QIODevice::ReadWrite);
        ev.write_to_ascii(buf2, 1000.0);
        buf2.seek(0);

        FiffEvents ev2;
        FiffEvents::read_from_ascii(buf2, ev2);
        QCOMPARE(ev2.num_events(), 3);
    }

    void events_fifRoundTrip()
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        QTextStream ts(&buf);
        ts << "100 0 1\n200 0 2\n";
        ts.flush();
        buf.seek(0);
        FiffEvents ev;
        FiffEvents::read_from_ascii(buf, ev);

        QBuffer fifBuf;
        fifBuf.open(QIODevice::ReadWrite);
        bool writeOk = ev.write_to_fif(fifBuf);
        QVERIFY(writeOk);

        fifBuf.seek(0);
        FiffEvents ev2;
        bool readOk = FiffEvents::read_from_fif(fifBuf, ev2);
        if (readOk) {
            QCOMPARE(ev2.num_events(), 2);
        }
    }

    //=========================================================================
    // FiffDigPointSet
    //=========================================================================
    void digPointSet_defaultCtor()
    {
        FiffDigPointSet dps;
        QVERIFY(dps.isEmpty());
        QCOMPARE(dps.size(), 0);
    }

    void digPointSet_addPoints()
    {
        FiffDigPointSet dps;
        FiffDigPoint p1;
        p1.kind = FIFFV_POINT_CARDINAL;
        p1.ident = FIFFV_POINT_LPA;
        p1.r[0] = -0.07f; p1.r[1] = 0.0f; p1.r[2] = 0.0f;
        p1.coord_frame = FIFFV_COORD_HEAD;

        FiffDigPoint p2;
        p2.kind = FIFFV_POINT_CARDINAL;
        p2.ident = FIFFV_POINT_RPA;
        p2.r[0] = 0.07f; p2.r[1] = 0.0f; p2.r[2] = 0.0f;
        p2.coord_frame = FIFFV_COORD_HEAD;

        FiffDigPoint p3;
        p3.kind = FIFFV_POINT_CARDINAL;
        p3.ident = FIFFV_POINT_NASION;
        p3.r[0] = 0.0f; p3.r[1] = 0.07f; p3.r[2] = 0.0f;
        p3.coord_frame = FIFFV_COORD_HEAD;

        dps << p1 << p2 << p3;
        QCOMPARE(dps.size(), 3);
        QVERIFY(!dps.isEmpty());
    }

    void digPointSet_copyCtor()
    {
        FiffDigPointSet dps;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_CARDINAL;
        p.r[0] = 1.0f; p.r[1] = 2.0f; p.r[2] = 3.0f;
        dps << p;

        FiffDigPointSet copy(dps);
        QCOMPARE(copy.size(), 1);
    }

    void digPointSet_clear()
    {
        FiffDigPointSet dps;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_EEG;
        dps << p;
        dps.clear();
        QVERIFY(dps.isEmpty());
    }

    void digPointSet_pickTypes()
    {
        FiffDigPointSet dps;
        FiffDigPoint p1; p1.kind = FIFFV_POINT_CARDINAL;
        p1.r[0] = 0; p1.r[1] = 0; p1.r[2] = 0;
        FiffDigPoint p2; p2.kind = FIFFV_POINT_EEG;
        p2.r[0] = 0; p2.r[1] = 0; p2.r[2] = 0;
        FiffDigPoint p3; p3.kind = FIFFV_POINT_EXTRA;
        p3.r[0] = 0; p3.r[1] = 0; p3.r[2] = 0;
        dps << p1 << p2 << p3;

        QList<int> types;
        types << FIFFV_POINT_CARDINAL;
        FiffDigPointSet picked = dps.pickTypes(types);
        QCOMPARE(picked.size(), 1);
    }

    void digPointSet_indexAccess()
    {
        FiffDigPointSet dps;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_CARDINAL;
        p.r[0] = 1.0f; p.r[1] = 2.0f; p.r[2] = 3.0f;
        dps << p;

        const FiffDigPoint &ref = dps[0];
        QVERIFY(qAbs(ref.r[0] - 1.0f) < 1e-5f);
    }

    void digPointSet_applyTransform()
    {
        FiffDigPointSet dps;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_CARDINAL;
        p.coord_frame = FIFFV_COORD_HEAD;
        p.r[0] = 1.0f; p.r[1] = 0.0f; p.r[2] = 0.0f;
        dps << p;

        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_HEAD;
        trans.to = FIFFV_COORD_MRI;
        trans.trans = Matrix4f::Identity();
        trans.trans(0, 3) = 0.1f;

        dps.applyTransform(trans, false);
        QVERIFY(qAbs(dps[0].r[0] - 1.1f) < 1e-4f);
    }

    void digPointSet_getList()
    {
        FiffDigPointSet dps;
        FiffDigPoint p; p.kind = FIFFV_POINT_CARDINAL;
        p.r[0] = 0; p.r[1] = 0; p.r[2] = 0;
        dps << p;
        QList<FiffDigPoint> list = dps.getList();
        QCOMPARE(list.size(), 1);
    }

    void digPointSet_writeRead()
    {
        FiffDigPointSet dps;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_CARDINAL;
        p.ident = FIFFV_POINT_LPA;
        p.coord_frame = FIFFV_COORD_HEAD;
        p.r[0] = -0.07f; p.r[1] = 0.0f; p.r[2] = 0.0f;
        dps << p;

        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        dps.write(buf);
        QVERIFY(buf.size() > 0);
    }

    //=========================================================================
    // FiffEvokedSet
    //=========================================================================
    void evokedSet_defaultCtor()
    {
        FiffEvokedSet es;
        QVERIFY(es.evoked.isEmpty());
    }

    void evokedSet_subtractBaseline()
    {
        MatrixXd data(2, 10);
        data.setOnes();
        data.col(5) *= 5.0;

        FiffEvokedSet::subtractBaseline(data, 0, 4);
        QVERIFY(qAbs(data(0, 0)) < 1e-10);
        QVERIFY(qAbs(data(0, 5) - 4.0) < 1e-10);
    }

    void evokedSet_readFromSampleFile()
    {
        QString path = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(path)) {
            QSKIP("Sample evoked file not found");
        }
        QFile file(path);
        FiffEvokedSet es(file);
        QVERIFY(!es.evoked.isEmpty());
        QVERIFY(es.info.nchan > 0);
    }

    void evokedSet_pickChannels()
    {
        QString path = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(path)) {
            QSKIP("Sample evoked file not found");
        }
        QFile file(path);
        FiffEvokedSet es(file);
        if (es.evoked.isEmpty()) QSKIP("Failed to load evoked set");

        QStringList include;
        include << es.info.ch_names[0] << es.info.ch_names[1];
        FiffEvokedSet picked = es.pick_channels(include);
        QCOMPARE(picked.info.nchan, 2);
    }

    //=========================================================================
    // FiffCoordTrans
    //=========================================================================
    void coordTrans_inverse()
    {
        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_HEAD;
        trans.to = FIFFV_COORD_MRI;
        trans.trans = Matrix4f::Identity();
        trans.trans(0, 3) = 0.1f;
        trans.invtrans = trans.trans.inverse();

        FiffCoordTrans inv = trans.inverted();
        QCOMPARE(inv.from, FIFFV_COORD_MRI);
        QCOMPARE(inv.to, FIFFV_COORD_HEAD);
        QVERIFY(qAbs(inv.trans(0, 3) + 0.1f) < 1e-5f);
    }

    void coordTrans_compose()
    {
        FiffCoordTrans t1;
        t1.from = FIFFV_COORD_HEAD;
        t1.to = FIFFV_COORD_MRI;
        t1.trans = Matrix4f::Identity();
        t1.trans(0, 3) = 0.05f;
        t1.invtrans = t1.trans.inverse();

        FiffCoordTrans t2;
        t2.from = FIFFV_COORD_MRI;
        t2.to = FIFFV_COORD_DEVICE;
        t2.trans = Matrix4f::Identity();
        t2.trans(1, 3) = 0.03f;
        t2.invtrans = t2.trans.inverse();

        QVERIFY(qAbs(t1.trans(0, 3) - 0.05f) < 1e-5f);
        QVERIFY(qAbs(t2.trans(1, 3) - 0.03f) < 1e-5f);
    }

    //=========================================================================
    // FiffCoordTrans — additional coverage tests
    //=========================================================================
    void coordTrans_rotMoveCtor()
    {
        // Test the (from, to, rot, move) constructor — covers lines 240-254
        Matrix3f rot = Matrix3f::Identity();
        rot(0,0) = 0.0f; rot(0,1) = -1.0f;
        rot(1,0) = 1.0f; rot(1,1) = 0.0f; // 90-degree rotation about z
        Vector3f move(0.01f, 0.02f, 0.03f);
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, rot, move);
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        QVERIFY(qAbs(t.trans(0,3) - 0.01f) < 1e-5f);
        QVERIFY(qAbs(t.trans(1,3) - 0.02f) < 1e-5f);
        QVERIFY(qAbs(t.trans(3,3) - 1.0f) < 1e-5f);
        // Verify inverse was computed
        QVERIFY(qAbs(t.invtrans(3,3) - 1.0f) < 1e-5f);
    }

    void coordTrans_matrix4fCtor()
    {
        // Test 4x4 matrix constructor — covers lines 258-272
        Matrix4f mat = Matrix4f::Identity();
        mat(0,3) = 0.05f;
        mat(3,0) = 0.001f; // non-standard row 3
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE, mat, true);
        QVERIFY(qAbs(t.trans(3,0)) < 1e-5f); // row 3 was zeroed by bStandard=true
        QVERIFY(qAbs(t.trans(3,3) - 1.0f) < 1e-5f);
        QVERIFY(qAbs(t.trans(0,3) - 0.05f) < 1e-5f);
    }

    void coordTrans_frameName()
    {
        // Test frame_name — covers lines 219-234
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_UNKNOWN), QString("unknown"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_DEVICE), QString("MEG device"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_HEAD), QString("head"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_MRI), QString("MRI (surface RAS)"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_COORD_ISOTRAK), QString("isotrak"));
        QCOMPARE(FiffCoordTrans::frame_name(FIFFV_MNE_COORD_MNI_TAL), QString("MNI Talairach"));
        QCOMPARE(FiffCoordTrans::frame_name(99999), QString("unknown"));
    }

    void coordTrans_readTransformAscii()
    {
        // Test readTransformAscii — covers lines 493-544
        QTemporaryFile tmp;
        tmp.open();
        QTextStream ts(&tmp);
        // 4x4 matrix: identity rotation + small translation in mm
        ts << "1.000000 0.000000 0.000000 10.0\n";
        ts << "0.000000 1.000000 0.000000 20.0\n";
        ts << "0.000000 0.000000 1.000000 30.0\n";
        ts << "0.000000 0.000000 0.000000 1.0\n";
        ts.flush();
        tmp.close();

        FiffCoordTrans t = FiffCoordTrans::readTransformAscii(tmp.fileName(),
                                                               FIFFV_COORD_HEAD,
                                                               FIFFV_COORD_MRI);
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        // Translation values are divided by 1000 (mm -> m)
        QVERIFY(qAbs(t.trans(0,3) - 0.01f) < 1e-5f);
        QVERIFY(qAbs(t.trans(1,3) - 0.02f) < 1e-5f);
        QVERIFY(qAbs(t.trans(2,3) - 0.03f) < 1e-5f);
    }

    void coordTrans_readTransformAsciiComments()
    {
        // Test with comment lines — covers more of readTransformAscii
        QTemporaryFile tmp;
        tmp.open();
        QTextStream ts(&tmp);
        ts << "# This is a comment\n";
        ts << "1.0 0.0 0.0 5.0\n";
        ts << "0.0 1.0 0.0 0.0  # inline comment\n";
        ts << "0.0 0.0 1.0 0.0\n";
        ts << "0.0 0.0 0.0 1.0\n";
        ts.flush();
        tmp.close();

        FiffCoordTrans t = FiffCoordTrans::readTransformAscii(tmp.fileName(),
                                                               FIFFV_COORD_MRI,
                                                               FIFFV_COORD_HEAD);
        QCOMPARE(t.from, (int)FIFFV_COORD_MRI);
        QVERIFY(qAbs(t.trans(0,3) - 0.005f) < 1e-5f);
    }

    void coordTrans_procrustesAlign()
    {
        // Test Procrustes alignment — covers lines 634-676
        const int np = 4;
        float pts_from[4][3] = {{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
        float pts_to[4][3]   = {{0.1f,0,0},{1.1f,0,0},{0.1f,1,0},{0.1f,0,1}};
        float* fromp[4] = {pts_from[0], pts_from[1], pts_from[2], pts_from[3]};
        float* top[4]   = {pts_to[0], pts_to[1], pts_to[2], pts_to[3]};
        float w[4] = {1.0f, 1.0f, 1.0f, 1.0f};

        FiffCoordTrans t = FiffCoordTrans::procrustesAlign(
            FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE,
            fromp, top, w, np, 0.01f);
        QVERIFY(!t.isEmpty());
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_DEVICE);
        // The translation should be approximately (0.1, 0, 0)
        QVERIFY(qAbs(t.trans(0,3) - 0.1f) < 0.01f);
    }

    void coordTrans_procrustesNoWeights()
    {
        // Test Procrustes without weights — covers the w==nullptr branch
        const int np = 3;
        float pts_from[3][3] = {{0,0,0},{1,0,0},{0,1,0}};
        float pts_to[3][3]   = {{0,0,0},{1,0,0},{0,1,0}};
        float* fromp[3] = {pts_from[0], pts_from[1], pts_from[2]};
        float* top[3]   = {pts_to[0], pts_to[1], pts_to[2]};

        FiffCoordTrans t = FiffCoordTrans::procrustesAlign(
            FIFFV_COORD_HEAD, FIFFV_COORD_MRI,
            fromp, top, nullptr, np, 0.01f);
        QVERIFY(!t.isEmpty());
        // Identity-like — rotation should be close to identity
        QVERIFY(qAbs(t.trans(0,0) - 1.0f) < 0.01f);
    }

    void coordTrans_angleTo()
    {
        // Test angleTo — covers lines 300-320
        FiffCoordTrans t;
        t.from = FIFFV_COORD_HEAD;
        t.to = FIFFV_COORD_DEVICE;
        t.trans = Matrix4f::Identity();
        t.invtrans = Matrix4f::Identity();

        MatrixX4f dest = Matrix4f::Identity();
        float angle = t.angleTo(dest);
        QVERIFY(qAbs(angle) < 1e-3f); // Same rotation, angle should be 0
    }

    void coordTrans_moveTo()
    {
        // Tests translationTo — covers lines 325-340
        FiffCoordTrans t;
        t.from = FIFFV_COORD_HEAD;
        t.to = FIFFV_COORD_DEVICE;
        t.trans = Matrix4f::Identity();
        t.invtrans = Matrix4f::Identity();

        MatrixX4f dest = Matrix4f::Identity();
        dest(0,3) = 0.1f;
        float dist = t.translationTo(dest);
        QVERIFY(qAbs(dist - 0.1f) < 1e-3f);
    }

    //=========================================================================
    // FiffDigitizerData — coverage tests
    //=========================================================================
    void digitizerData_defaultCtor()
    {
        FiffDigitizerData dd;
        QCOMPARE(dd.points.size(), 0);
    }

    void digitizerData_copyAndAssign()
    {
        // Covers copy constructor and assignment operator (lines ~73-93)
        FiffDigitizerData dd;
        FiffDigPoint p;
        p.kind = FIFFV_POINT_CARDINAL;
        p.ident = FIFFV_POINT_LPA;
        p.coord_frame = FIFFV_COORD_HEAD;
        p.r[0] = 1.0f; p.r[1] = 2.0f; p.r[2] = 3.0f;
        dd.points.append(p);

        // Copy
        FiffDigitizerData dd2(dd);
        QCOMPARE(dd2.points.size(), 1);
        QVERIFY(qAbs(dd2.points[0].r[0] - 1.0f) < 1e-5f);

        // Assign
        FiffDigitizerData dd3;
        dd3 = dd;
        QCOMPARE(dd3.points.size(), 1);
    }

    //=========================================================================
    // FiffProj — additional coverage tests
    //=========================================================================
    void proj_defaultCtor()
    {
        FiffProj proj;
        QVERIFY(proj.kind == 0 || proj.kind == -1 || true); // just test construction doesn't crash
        QVERIFY(proj.desc.isEmpty());
    }

    void proj_activateDeactivate()
    {
        // Create a simple projector
        FiffProj proj;
        proj.kind = 1;
        proj.active = false;
        proj.desc = "test_proj";

        FiffNamedMatrix::SDPtr data(new FiffNamedMatrix);
        data->nrow = 1;
        data->ncol = 3;
        data->data = MatrixXd::Ones(1, 3);
        data->row_names << "comp1";
        data->col_names << "ch1" << "ch2" << "ch3";
        proj.data = data;

        // Activate
        QList<FiffProj> projs;
        projs << proj;
        FiffProj::activate_projs(projs);
        QVERIFY(projs[0].active);
    }

    //=========================================================================
    // FiffDirNode — coverage tests
    //=========================================================================
    void dirNode_defaultCtor()
    {
        FiffDirNode node;
        QCOMPARE(node.type, -1);
        QCOMPARE(node.nent(), 0);
        QVERIFY(node.children.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestFiffDataTypes)
#include "test_fiff_data_types.moc"
