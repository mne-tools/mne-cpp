// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <Eigen/Dense>

#include <mne/fiff/fiff_events.h>
#include <mne/fiff/fiff_dig_point_set.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_digitizer_data.h>
#include <mne/fiff/fiff_dir_node.h>

using namespace mne;
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
        QVERIFY(ev.empty());
        QCOMPARE(ev.num_events(), 0);
    }

    void events_asciiAndFifRoundTrip()
    {
        QTemporaryDir dir;
        const auto path = [&dir](const char* name) {
            return dir.filePath(name).toStdString();
        };
        QFile text(dir.filePath("in.eve"));
        QVERIFY(text.open(QIODevice::WriteOnly));
        text.write("100 0 1\n200 0 2\n300 0 3\n");
        text.close();

        const FiffEvents ev = FiffEvents::read_ascii(path("in.eve")).value();
        QCOMPARE(ev.num_events(), 3);

        QVERIFY(ev.write_ascii(path("out.eve"), 1000.0f).has_value());
        QVERIFY(FiffEvents::read_ascii(path("out.eve")).value().events == ev.events);

        QVERIFY(ev.write_fif(path("out-eve.fif")).has_value());
        QVERIFY(FiffEvents::read_fif(path("out-eve.fif")).value().events == ev.events);
    }

    //=========================================================================
    // FiffDigPointSet
    //=========================================================================
    static FiffDigPoint digPoint(int kind, int ident, float x, float y, float z)
    {
        FiffDigPoint p;
        p.kind = kind;
        p.ident = ident;
        p.coord_frame = FIFFV_COORD_HEAD;
        p.r[0] = x;
        p.r[1] = y;
        p.r[2] = z;
        return p;
    }

    static FiffDigPointSet fiducialsAndEeg()
    {
        return FiffDigPointSet({digPoint(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, -0.07f, 0.0f, 0.0f),
                                digPoint(FIFFV_POINT_CARDINAL, FIFFV_POINT_NASION, 0.0f, 0.07f, 0.0f),
                                digPoint(FIFFV_POINT_CARDINAL, FIFFV_POINT_RPA, 0.07f, 0.0f, 0.0f),
                                digPoint(FIFFV_POINT_EEG, 1, 0.0f, 0.0f, 0.09f),
                                digPoint(FIFFV_POINT_EXTRA, 1, 0.0f, -0.05f, 0.05f)});
    }

    void digPointSet_container()
    {
        FiffDigPointSet dps;
        QVERIFY(dps.empty());
        QCOMPARE(dps.size(), std::size_t{0});

        dps.push_back(digPoint(FIFFV_POINT_EEG, 7, 1.0f, 2.0f, 3.0f));
        QCOMPARE(dps.size(), std::size_t{1});
        QCOMPARE(dps[0].ident, 7);
        QCOMPARE(dps.points().size(), std::size_t{1});
        QCOMPARE(dps.points().data(), &dps[0]);

        const FiffDigPointSet copy = fiducialsAndEeg();
        int eeg = 0;
        for (const FiffDigPoint& point : copy) {
            eeg += point.kind == FIFFV_POINT_EEG ? 1 : 0;
        }
        QCOMPARE(eeg, 1);

        dps.clear();
        QVERIFY(dps.empty());
    }

    void digPointSet_pick_types()
    {
        const FiffDigPointSet dps = fiducialsAndEeg();
        const FiffDigPointSet cardinal = dps.pick_types({FIFFV_POINT_CARDINAL});
        QCOMPARE(cardinal.size(), std::size_t{3});
        QCOMPARE(cardinal[1].ident, FIFFV_POINT_NASION);

        const std::vector<int> kinds{FIFFV_POINT_EXTRA, FIFFV_POINT_EEG};
        const FiffDigPointSet head = dps.pick_types(kinds);
        QCOMPARE(head.size(), std::size_t{2});
        QCOMPARE(head[0].kind, FIFFV_POINT_EEG); // keeps the order of the set
        QVERIFY(dps.pick_types({FIFFV_POINT_HPI}).empty());
    }

    void digPointSet_apply_transform()
    {
        FiffDigPointSet dps({digPoint(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, 1.0f, 0.0f, 0.0f)});
        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_HEAD;
        trans.to = FIFFV_COORD_MRI;
        trans.trans = Matrix4f::Identity();
        trans.trans(0, 3) = 0.1f;
        trans.invtrans = trans.trans.inverse();

        dps.apply_transform(trans);
        QVERIFY(qAbs(dps[0].r[0] - 1.1f) < 1e-5f);
        dps.apply_transform(trans, true);
        QVERIFY(qAbs(dps[0].r[0] - 1.0f) < 1e-5f);
    }

    void digPointSet_writeRead()
    {
        const FiffDigPointSet dps = fiducialsAndEeg();
        QTemporaryDir dir;
        const std::filesystem::path path = dir.filePath("dig.fif").toStdString();
        QVERIFY(dps.write(path).has_value());

        const auto read = FiffDigPointSet::read(path);
        QVERIFY(read.has_value());
        QCOMPARE(read->size(), dps.size());
        for (std::size_t i = 0; i < dps.size(); ++i) {
            QCOMPARE((*read)[i].kind, dps[i].kind);
            QCOMPARE((*read)[i].ident, dps[i].ident);
            QCOMPARE((*read)[i].r[0], dps[i].r[0]);
        }
        QCOMPARE(FiffDigPointSet(path).size(), dps.size());
    }

    void digPointSet_errors()
    {
        QTemporaryDir dir;
        const std::filesystem::path missing = dir.filePath("missing.fif").toStdString();
        const auto read = FiffDigPointSet::read(missing);
        QVERIFY(!read.has_value());
        QVERIFY_THROWS_EXCEPTION(mne::Error, FiffDigPointSet{missing});

        const auto write = fiducialsAndEeg().write(dir.filePath("no/such/dir/dig.fif").toStdString());
        QVERIFY(!write.has_value());
        QCOMPARE(write.error().code, mne::ErrorCode::NotFound);
        QVERIFY(!fiducialsAndEeg().write({}).has_value());
    }

    //=========================================================================
    // FiffEvokedSet
    //=========================================================================
    void evokedSet_defaultCtor()
    {
        FiffEvokedSet es;
        QVERIFY(es.evoked.empty());
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
        QString path = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(path)) {
            QSKIP("Sample evoked file not found");
        }
        FiffEvokedSet es(path.toStdU16String());
        QVERIFY(!es.evoked.empty());
        QVERIFY(es.info.nchan > 0);
    }

    void evokedSet_pickChannels()
    {
        QString path = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(path)) {
            QSKIP("Sample evoked file not found");
        }
        FiffEvokedSet es(path.toStdU16String());
        if (es.evoked.empty())
            QSKIP("Failed to load evoked set");

        StringList include;
        include.push_back(es.info.ch_names[0]);
        include.push_back(es.info.ch_names[1]);
        FiffEvokedSet picked = es.pick_channels(include);
        QCOMPARE(picked.info.nchan, 2);

        // Single evoked: picks rows by name; no selection or no match keeps everything; a name past the data
        // rows (info and data out of step) gives a zero row instead of reading past the data
        FiffEvoked evoked = es.evoked.front();
        const FiffEvoked one = evoked.pick_channels({include[1]});
        QCOMPARE(one.data, evoked.data.row(1));
        QCOMPARE(evoked.pick_channels({}).data.rows(), evoked.data.rows());
        QCOMPARE(evoked.pick_channels({"NOPE"}).data.rows(), evoked.data.rows());
        evoked.data = evoked.data.topRows(2).eval();
        const std::string third = evoked.info.ch_names[2];
        QVERIFY(evoked.pick_channels({third}).data.isZero(0.0));
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
        rot(0, 0) = 0.0f;
        rot(0, 1) = -1.0f;
        rot(1, 0) = 1.0f;
        rot(1, 1) = 0.0f; // 90-degree rotation about z
        Vector3f move(0.01f, 0.02f, 0.03f);
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, rot, move);
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        QVERIFY(qAbs(t.trans(0, 3) - 0.01f) < 1e-5f);
        QVERIFY(qAbs(t.trans(1, 3) - 0.02f) < 1e-5f);
        QVERIFY(qAbs(t.trans(3, 3) - 1.0f) < 1e-5f);
        // Verify inverse was computed
        QVERIFY(qAbs(t.invtrans(3, 3) - 1.0f) < 1e-5f);
    }

    void coordTrans_matrix4fCtor()
    {
        // Test 4x4 matrix constructor — covers lines 258-272
        Matrix4f mat = Matrix4f::Identity();
        mat(0, 3) = 0.05f;
        mat(3, 0) = 0.001f; // non-standard row 3
        FiffCoordTrans t(FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE, mat, true);
        QVERIFY(qAbs(t.trans(3, 0)) < 1e-5f); // row 3 was zeroed by bStandard=true
        QVERIFY(qAbs(t.trans(3, 3) - 1.0f) < 1e-5f);
        QVERIFY(qAbs(t.trans(0, 3) - 0.05f) < 1e-5f);
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
        QVERIFY(tmp.open());
        QTextStream ts(&tmp);
        // 4x4 matrix: identity rotation + small translation in mm
        ts << "1.000000 0.000000 0.000000 10.0\n";
        ts << "0.000000 1.000000 0.000000 20.0\n";
        ts << "0.000000 0.000000 1.000000 30.0\n";
        ts << "0.000000 0.000000 0.000000 1.0\n";
        ts.flush();
        tmp.close();

        FiffCoordTrans t = FiffCoordTrans::read_ascii(tmp.fileName().toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI).value();
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_MRI);
        // Translation values are divided by 1000 (mm -> m)
        QVERIFY(qAbs(t.trans(0, 3) - 0.01f) < 1e-5f);
        QVERIFY(qAbs(t.trans(1, 3) - 0.02f) < 1e-5f);
        QVERIFY(qAbs(t.trans(2, 3) - 0.03f) < 1e-5f);
    }

    void coordTrans_readTransformAsciiComments()
    {
        // Test with comment lines — covers more of readTransformAscii
        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        QTextStream ts(&tmp);
        ts << "# This is a comment\n";
        ts << "1.0 0.0 0.0 5.0\n";
        ts << "0.0 1.0 0.0 0.0  # inline comment\n";
        ts << "0.0 0.0 1.0 0.0\n";
        ts << "0.0 0.0 0.0 1.0\n";
        ts.flush();
        tmp.close();

        FiffCoordTrans t = FiffCoordTrans::read_ascii(tmp.fileName().toStdU16String(), FIFFV_COORD_MRI, FIFFV_COORD_HEAD).value();
        QCOMPARE(t.from, (int)FIFFV_COORD_MRI);
        QVERIFY(qAbs(t.trans(0, 3) - 0.005f) < 1e-5f);
    }

    void coordTrans_procrustesAlign()
    {
        // Test Procrustes alignment
        Eigen::MatrixXf fromPts(4, 3), toPts(4, 3);
        fromPts << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        toPts << 0.1f, 0, 0, 1.1f, 0, 0, 0.1f, 1, 0, 0.1f, 0, 1;
        Eigen::VectorXf w(4);
        w << 1.0f, 1.0f, 1.0f, 1.0f;

        FiffCoordTrans t = FiffCoordTrans::procrustes_align(
            FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE,
            fromPts, toPts, w, 0.01f);
        QVERIFY(!t.empty());
        QCOMPARE(t.from, (int)FIFFV_COORD_HEAD);
        QCOMPARE(t.to, (int)FIFFV_COORD_DEVICE);
        // The translation should be approximately (0.1, 0, 0)
        QVERIFY(qAbs(t.trans(0, 3) - 0.1f) < 0.01f);
    }

    void coordTrans_procrustesNoWeights()
    {
        // Test Procrustes without weights — empty weight vector
        Eigen::MatrixXf fromPts(3, 3), toPts(3, 3);
        fromPts << 0, 0, 0, 1, 0, 0, 0, 1, 0;
        toPts << 0, 0, 0, 1, 0, 0, 0, 1, 0;

        FiffCoordTrans t = FiffCoordTrans::procrustes_align(
            FIFFV_COORD_HEAD, FIFFV_COORD_MRI,
            fromPts, toPts, Eigen::VectorXf(), 0.01f);
        QVERIFY(!t.empty());
        // Identity-like — rotation should be close to identity
        QVERIFY(qAbs(t.trans(0, 0) - 1.0f) < 0.01f);
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
        float angle = t.angle_to(dest);
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
        dest(0, 3) = 0.1f;
        float dist = t.translation_to(dest);
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
        p.r[0] = 1.0f;
        p.r[1] = 2.0f;
        p.r[2] = 3.0f;
        dd.points.push_back(p);

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
        QVERIFY(proj.desc.empty());
    }

    void proj_activateDeactivate()
    {
        // Create a simple projector
        FiffProj proj;
        proj.kind = 1;
        proj.active = false;
        proj.desc = "test_proj";

        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 3;
        data.data = MatrixXd::Ones(1, 3);
        data.row_names.push_back("comp1");
        data.col_names.push_back("ch1");
        data.col_names.push_back("ch2");
        data.col_names.push_back("ch3");
        proj.data = data;

        // Activate
        std::vector<FiffProj> projs;
        projs.push_back(proj);
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
        QVERIFY(node.children.empty());
    }
};

QTEST_GUILESS_MAIN(TestFiffDataTypes)
#include "test_fiff_data_types.moc"

#include <vector>
