#include <QtTest/QtTest>
#include <QBuffer>
#include <Eigen/Dense>
#include <Eigen/SparseCore>

#include <mne/mne_source_spaces.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fwd/fwd_forward_solution.h>
#include <inverse/mne_inverse_operator.h>
#include <inverse/mne_source_estimate.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_cluster_info.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <fs/fs_label.h>

using namespace MNELIB;
using namespace FWDLIB;
using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

class TestMneDataTypes : public QObject
{
    Q_OBJECT

private:
    QString dataPath;

    void initTestCase()
    {
        dataPath = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    }

private slots:
    //=========================================================================
    // MNESourceSpaces
    //=========================================================================
    void sourceSpace_defaultCtor()
    {
        MNESourceSpaces ss;
        QVERIFY(ss.isEmpty());
        QCOMPARE(ss.size(), 0);
    }

    void sourceSpace_copyCtor()
    {
        MNESourceSpaces ss;
        MNESourceSpaces copy(ss);
        QVERIFY(copy.isEmpty());
    }

    void sourceSpace_clear()
    {
        MNESourceSpaces ss;
        ss.clear();
        QVERIFY(ss.isEmpty());
    }

    void sourceSpace_equality()
    {
        MNESourceSpaces ss1, ss2;
        QVERIFY(ss1 == ss2);
    }

    void sourceSpace_getVertno()
    {
        MNESourceSpaces ss;
        QList<VectorXi> vertno = ss.get_vertno();
        QCOMPARE(vertno.size(), 0);
    }

    void sourceSpace_readFromFile()
    {
        QString srcPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
        if (!QFile::exists(srcPath)) {
            QSKIP("Source space file not found");
        }
        QFile file(srcPath);
        FiffStream::SPtr stream(new FiffStream(&file));
        if (!stream->open()) {
            QSKIP("Failed to open source space file");
        }

        MNESourceSpaces ss;
        bool ok = MNESourceSpaces::readFromStream(stream, true, ss);
        stream->close();
        if (ok) {
            QVERIFY(!ss.isEmpty());
            QCOMPARE(ss.size(), 2);
            QList<VectorXi> vertno = ss.get_vertno();
            QCOMPARE(vertno.size(), 2);
            QVERIFY(vertno[0].size() > 0);
        }
    }

    //=========================================================================
    // MNEBem
    //=========================================================================
    void bem_defaultCtor()
    {
        MNEBem bem;
        QVERIFY(bem.isEmpty());
        QCOMPARE(bem.size(), 0);
    }

    void bem_copyCtor()
    {
        MNEBem bem;
        MNEBem copy(bem);
        QVERIFY(copy.isEmpty());
    }

    void bem_addSurface()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        surf.np = 100;
        surf.ntri = 50;
        surf.rr = MatrixX3f::Random(100, 3);
        surf.itris = MatrixX3i::Zero(50, 3);
        surf.nn = MatrixX3f::Random(100, 3);
        bem << surf;
        QCOMPARE(bem.size(), 1);
        QVERIFY(!bem.isEmpty());
    }

    void bem_clear()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        bem << surf;
        bem.clear();
        QVERIFY(bem.isEmpty());
    }

    void bem_readFromFile()
    {
        QString bemPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(bemPath)) {
            QSKIP("BEM file not found");
        }
        QFile file(bemPath);
        MNEBem bem(file);
        QVERIFY(!bem.isEmpty());
        QVERIFY(bem.size() > 0);
    }

    void bem_transformInvTransform()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        surf.np = 3;
        surf.ntri = 1;
        surf.coord_frame = FIFFV_COORD_MRI;
        surf.rr.resize(3, 3);
        surf.rr << 1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 1.0f;
        surf.nn = MatrixX3f::Zero(3, 3);
        surf.itris.resize(1, 3);
        surf.itris << 0, 1, 2;
        bem << surf;

        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_MRI;
        trans.to = FIFFV_COORD_HEAD;
        trans.trans = Matrix4f::Identity();
        trans.trans(0, 3) = 0.1f;
        trans.invtrans = trans.trans.inverse();

        bem.transform(trans);
        QVERIFY(qAbs(bem[0].rr(0, 0) - 1.1f) < 0.01f);

        bem.invtransform(trans);
        QVERIFY(qAbs(bem[0].rr(0, 0) - 1.0f) < 0.01f);
    }

    void bem_writeToBuffer()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = FIFFV_BEM_SURF_ID_BRAIN;
        surf.np = 3;
        surf.ntri = 1;
        surf.coord_frame = FIFFV_COORD_MRI;
        surf.rr.resize(3, 3);
        surf.rr << 0.01f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 0.01f;
        surf.nn.resize(3, 3);
        surf.nn << 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f;
        surf.itris.resize(1, 3);
        surf.itris << 0, 1, 2;
        bem << surf;

        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        bem.write(buf);
        QVERIFY(buf.size() > 0);
    }

    //=========================================================================
    // FwdForwardSolution with real data
    //=========================================================================
    void fwd_readFromFile()
    {
        QString fwdPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath)) {
            QSKIP("Forward solution file not found");
        }
        QFile file(fwdPath);
        FwdForwardSolution fwd(file);
        if (fwd.isEmpty()) {
            QSKIP("Failed to read forward solution");
        }

        QVERIFY(!fwd.isEmpty());
        QVERIFY(!fwd.isFixedOrient());
        QVERIFY(fwd.sol->data.rows() > 0);

        VectorXi testSel(2);
        testSel << 0, 1;
        VectorXi triplets = fwd.tripletSelection(testSel);
        QCOMPARE(triplets.size(), 6);

        FwdForwardSolution fwdMeg = fwd.pick_types(true, false);
        QVERIFY(fwdMeg.sol->data.rows() > 0);
    }

    void fwd_restrictGainMatrix()
    {
        int nSensors = 10;
        int nSources = 6;
        MatrixXd gain = MatrixXd::Random(nSensors, nSources);

        FiffInfo info;
        info.nchan = nSensors;
        for (int i = 0; i < nSensors; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("CH%1").arg(i);
            ch.kind = FIFFV_MEG_CH;
            ch.unit = FIFF_UNIT_T;
            ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
            ch.range = 1.0;
            ch.cal = 1.0;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }
        info.bads.clear();

        FwdForwardSolution::restrict_gain_matrix(gain, info);
        QCOMPARE(gain.rows(), nSensors);
    }

    //=========================================================================
    // MNEInverseOperator
    //=========================================================================
    void invOp_defaultCtor()
    {
        MNEInverseOperator inv;
        QVERIFY(!inv.isFixedOrient());
    }

    void invOp_copyCtor()
    {
        MNEInverseOperator inv;
        MNEInverseOperator copy(inv);
        QVERIFY(!copy.isFixedOrient());
    }

    void invOp_kernel()
    {
        MNEInverseOperator inv;
        MatrixXd kernel = inv.getKernel();
        QCOMPARE(kernel.rows(), 0);
    }

    //=========================================================================
    // MNESourceEstimate
    //=========================================================================
    void sourceEstimate_defaultCtor()
    {
        MNESourceEstimate stc;
        QVERIFY(stc.isEmpty());
    }

    void sourceEstimate_paramCtor()
    {
        MatrixXd data = MatrixXd::Random(100, 10);
        VectorXi vertices(100);
        for (int i = 0; i < 100; ++i) vertices(i) = i;
        float tmin = 0.0f;
        float tstep = 0.001f;

        MNESourceEstimate stc(data, vertices, tmin, tstep);
        QVERIFY(!stc.isEmpty());
        QCOMPARE(stc.data.rows(), 100);
        QCOMPARE(stc.data.cols(), 10);
    }

    //=========================================================================
    // MNEEpochData
    //=========================================================================
    void epochData_fromEvoked()
    {
        QString avePath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath)) {
            QSKIP("Sample evoked file not found");
        }
        QFile file(avePath);
        FiffEvokedSet es(file);
        if (es.evoked.isEmpty()) {
            QSKIP("Failed to read evoked set");
        }

        FiffEvoked ev = es.evoked[0];
        MNEEpochData::SPtr epoch(new MNEEpochData());
        epoch->epoch = ev.data;
        epoch->tmin = ev.times(0);
        epoch->tmax = ev.times(ev.times.size() - 1);
        epoch->bReject = false;

        QVERIFY(epoch->epoch.rows() > 0);
        QVERIFY(epoch->epoch.cols() > 0);

        epoch->applyBaselineCorrection(QPair<float, float>(epoch->tmin, 0.0f));
    }

    //=========================================================================
    // MNEClusterInfo
    //=========================================================================
    void clusterInfo_basic()
    {
        MNEClusterInfo ci;
        ci.clusterVertnos.append(VectorXi::LinSpaced(5, 0, 4));
        ci.clusterVertnos.append(VectorXi::LinSpaced(3, 10, 12));
        QCOMPARE(ci.clusterVertnos.size(), 2);

        ci.clusterVertnos.clear();
        QCOMPARE(ci.clusterVertnos.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestMneDataTypes)
#include "test_mne_data_types.moc"
