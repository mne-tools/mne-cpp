/**
 * @file test_mne_extended.cpp
 * @brief Comprehensive tests for MNELIB classes: MNENamedMatrix, MNEEpochData,
 *        MNEEpochDataList, MNEClusterInfo, MNESourceSpaces, MNEForwardSolution.
 */
#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include <mne/mne.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_cluster_info.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_named_matrix.h>
#include <fiff/fiff.h>

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

class TestMneExtended : public QObject
{
    Q_OBJECT

private:
    QString dataPath() const {
        return QCoreApplication::applicationDirPath() +
               "/../resources/data/mne-cpp-test-data/";
    }

private slots:

    //=========================================================================
    // MNENamedMatrix
    //=========================================================================
    void mneNamedMatrix_defaultCtor()
    {
        MNENamedMatrix m;
        QCOMPARE(m.nrow, 0);
        QCOMPARE(m.ncol, 0);
        QVERIFY(m.rowlist.isEmpty());
        QVERIFY(m.collist.isEmpty());
        QCOMPARE(m.data.rows(), 0);
        QCOMPARE(m.data.cols(), 0);
    }

    void mneNamedMatrix_build()
    {
        QStringList rows = {"r0", "r1"};
        QStringList cols = {"c0", "c1", "c2"};
        MatrixXf data(2, 3);
        data << 1, 2, 3, 4, 5, 6;

        auto mat = MNENamedMatrix::build(2, 3, rows, cols, data);
        QVERIFY(mat != nullptr);
        QCOMPARE(mat->nrow, 2);
        QCOMPARE(mat->ncol, 3);
        QCOMPARE(mat->rowlist.size(), 2);
        QCOMPARE(mat->collist.size(), 3);
        QVERIFY(mat->data.isApprox(data));
    }

    void mneNamedMatrix_copyConstruct()
    {
        QStringList rows = {"r0", "r1"};
        QStringList cols = {"c0", "c1"};
        MatrixXf data(2, 2);
        data << 1, 2, 3, 4;

        auto orig = MNENamedMatrix::build(2, 2, rows, cols, data);
        MNENamedMatrix copy(*orig);

        QCOMPARE(copy.nrow, orig->nrow);
        QCOMPARE(copy.ncol, orig->ncol);
        QCOMPARE(copy.rowlist, orig->rowlist);
        QCOMPARE(copy.collist, orig->collist);
        QVERIFY(copy.data.isApprox(orig->data));
    }

    void mneNamedMatrix_pickRowsAndCols()
    {
        QStringList rows = {"r0", "r1", "r2"};
        QStringList cols = {"c0", "c1", "c2", "c3"};
        MatrixXf data(3, 4);
        data << 1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12;

        auto mat = MNENamedMatrix::build(3, 4, rows, cols, data);

        // Pick row "r1" and cols "c1", "c3"
        QStringList pickRows = {"r1"};
        QStringList pickCols = {"c1", "c3"};
        auto sub = mat->pick(pickRows, 1, pickCols, 2);
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->nrow, 1);
        QCOMPARE(sub->ncol, 2);
        QCOMPARE(sub->rowlist.size(), 1);
        QCOMPARE(sub->rowlist[0], QString("r1"));
        // row "r1" = [5,6,7,8], cols c1=6, c3=8
        QCOMPARE(sub->data(0, 0), 6.0f);
        QCOMPARE(sub->data(0, 1), 8.0f);
    }

    void mneNamedMatrix_pickAllRows()
    {
        QStringList rows = {"r0", "r1"};
        QStringList cols = {"c0", "c1"};
        MatrixXf data(2, 2);
        data << 1, 2, 3, 4;

        auto mat = MNENamedMatrix::build(2, 2, rows, cols, data);

        // Empty pick list = keep all
        QStringList emptyPick;
        QStringList pickCols = {"c0"};
        auto sub = mat->pick(emptyPick, 2, pickCols, 1);
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->nrow, 2);
        QCOMPARE(sub->ncol, 1);
    }

    void mneNamedMatrix_pickInvalidName()
    {
        QStringList rows = {"r0"};
        QStringList cols = {"c0"};
        MatrixXf data(1, 1);
        data << 42;

        auto mat = MNENamedMatrix::build(1, 1, rows, cols, data);
        QStringList pickRows = {"nonexistent"};
        auto sub = mat->pick(pickRows, 1, QStringList(), 1);
        // Should return nullptr when name not found
        QVERIFY(sub == nullptr);
    }

    //=========================================================================
    // MNEEpochData
    //=========================================================================
    void epochData_defaultCtor()
    {
        MNEEpochData e;
        QCOMPARE(e.epoch.rows(), 0);
        QCOMPARE(e.epoch.cols(), 0);
        QCOMPARE(e.event, -1);
        QCOMPARE(e.tmin, -1.0f);
        QCOMPARE(e.tmax, -1.0f);
        QCOMPARE(e.bReject, false);
    }

    void epochData_copyCtor()
    {
        MNEEpochData e;
        e.epoch = MatrixXd::Random(3, 100);
        e.event = 1;
        e.tmin = -0.2f;
        e.tmax = 0.5f;
        e.bReject = true;

        MNEEpochData copy(e);
        QVERIFY(copy == e);
        QVERIFY(copy.epoch.isApprox(e.epoch));
        QCOMPARE(copy.bReject, true);
    }

    void epochData_applyBaseline()
    {
        MNEEpochData e;
        // 2 channels, 10 time points
        e.epoch = MatrixXd::Ones(2, 10) * 5.0;
        e.tmin = 0.0f;
        e.tmax = 0.9f;

        // baseline from 0s to 0.4s => samples 0..4
        QPair<float, float> baseline(0.0f, 0.4f);
        e.applyBaselineCorrection(baseline);

        // After baseline correction, mean of baseline segment should be subtracted
        // Original was all 5s, mean is 5, so result should be ~0
        for (int ch = 0; ch < 2; ++ch) {
            for (int s = 0; s < 10; ++s) {
                QVERIFY(qAbs(e.epoch(ch, s)) < 1e-10);
            }
        }
    }

    void epochData_pickChannels()
    {
        MNEEpochData e;
        e.epoch = MatrixXd::Random(5, 20);
        e.event = 2;
        e.tmin = -0.1f;
        e.tmax = 0.3f;

        RowVectorXi sel(2);
        sel << 1, 3;
        e.pick_channels(sel);

        QCOMPARE(e.epoch.rows(), (Eigen::Index)2);
        QCOMPARE(e.epoch.cols(), (Eigen::Index)20);
    }

    void epochData_equality()
    {
        MNEEpochData a, b;
        a.epoch = MatrixXd::Ones(2, 5);
        a.event = 1;
        a.tmin = 0.0f;
        a.tmax = 0.5f;
        a.bReject = false;

        b = a; // shallow copy
        b.epoch = a.epoch; // same data
        QVERIFY(a == b);

        b.event = 2;
        QVERIFY(!(a == b));
    }

    //=========================================================================
    // MNEEpochDataList
    //=========================================================================
    void epochDataList_defaultCtor()
    {
        MNEEpochDataList list;
        QCOMPARE(list.size(), 0);
    }

    void epochDataList_applyBaselineCorrection()
    {
        MNEEpochDataList list;
        for (int i = 0; i < 3; ++i) {
            auto ep = MNEEpochData::SPtr::create();
            ep->epoch = MatrixXd::Ones(2, 10) * (i + 1.0);
            ep->tmin = 0.0f;
            ep->tmax = 0.9f;
            ep->event = 1;
            ep->bReject = false;
            list.append(ep);
        }

        QPair<float, float> baseline(0.0f, 0.4f);
        list.applyBaselineCorrection(baseline);

        // All epochs should now be zero-mean in baseline window
        for (const auto& ep : list) {
            for (int ch = 0; ch < ep->epoch.rows(); ++ch) {
                for (int s = 0; s < ep->epoch.cols(); ++s) {
                    QVERIFY(qAbs(ep->epoch(ch, s)) < 1e-10);
                }
            }
        }
    }

    void epochDataList_dropRejected()
    {
        MNEEpochDataList list;
        for (int i = 0; i < 5; ++i) {
            auto ep = MNEEpochData::SPtr::create();
            ep->epoch = MatrixXd::Random(2, 10);
            ep->event = 1;
            ep->tmin = 0.0f;
            ep->tmax = 0.5f;
            ep->bReject = (i % 2 == 0); // reject even indices
            list.append(ep);
        }

        QCOMPARE(list.size(), 5);
        list.dropRejected();
        QCOMPARE(list.size(), 2); // indices 1 and 3 survive
    }

    void epochDataList_pickChannels()
    {
        MNEEpochDataList list;
        for (int i = 0; i < 3; ++i) {
            auto ep = MNEEpochData::SPtr::create();
            ep->epoch = MatrixXd::Random(5, 10);
            ep->event = 1;
            ep->tmin = 0.0f;
            ep->tmax = 0.5f;
            ep->bReject = false;
            list.append(ep);
        }

        RowVectorXi sel(2);
        sel << 0, 4;
        list.pick_channels(sel);

        for (const auto& ep : list) {
            QCOMPARE(ep->epoch.rows(), (Eigen::Index)2);
        }
    }

    void epochDataList_checkForArtifact()
    {
        // Create a synthetic FiffInfo with one MEG channel
        FiffInfo info;
        FiffChInfo ch;
        ch.ch_name = "MEG0001";
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
        ch.unit = FIFF_UNIT_T;
        info.chs.append(ch);
        info.ch_names.append("MEG0001");
        info.nchan = 1;

        // Data well within threshold (peak-to-peak = 0 since all values the same)
        MatrixXd dataOk = MatrixXd::Ones(1, 100) * 1e-13;
        QMap<QString, double> mapReject;
        mapReject["mag"] = 5e-12;

        bool hasArtifact = MNEEpochDataList::checkForArtifact(dataOk, info, mapReject);
        QCOMPARE(hasArtifact, false);

        // Data exceeding threshold (peak-to-peak = 2e-10 >> 5e-12)
        MatrixXd dataBad = MatrixXd::Zero(1, 100);
        dataBad(0, 0) = -1e-10;
        dataBad(0, 99) = 1e-10;
        hasArtifact = MNEEpochDataList::checkForArtifact(dataBad, info, mapReject);
        QCOMPARE(hasArtifact, true);
    }

    void epochDataList_average()
    {
        // Create synthetic epochs and average
        FiffInfo info;
        FiffChInfo ch;
        ch.ch_name = "MEG0001";
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
        ch.unit = FIFF_UNIT_T;
        info.chs.append(ch);
        info.ch_names.append("MEG0001");
        info.nchan = 1;
        info.sfreq = 100.0;

        MNEEpochDataList list;
        for (int i = 0; i < 4; ++i) {
            auto ep = MNEEpochData::SPtr::create();
            ep->epoch = MatrixXd::Ones(1, 50) * (i + 1.0);
            ep->event = 1;
            ep->tmin = 0.0f;
            ep->tmax = 0.49f;
            ep->bReject = false;
            list.append(ep);
        }

        FiffEvoked avg = list.average(info, 0, 49);
        // Average of 1,2,3,4 = 2.5
        QCOMPARE(avg.data.rows(), (Eigen::Index)1);
        QCOMPARE(avg.data.cols(), (Eigen::Index)50);
        QVERIFY(qAbs(avg.data(0, 0) - 2.5) < 1e-10);
    }

    //=========================================================================
    // MNEClusterInfo
    //=========================================================================
    void clusterInfo_defaultAndEmpty()
    {
        MNEClusterInfo ci;
        QVERIFY(ci.isEmpty());
        QCOMPARE(ci.numClust(), 0);
    }

    void clusterInfo_addAndClear()
    {
        MNEClusterInfo ci;
        ci.clusterLabelNames.append("label1");
        ci.clusterLabelIds.append(1);
        ci.centroidVertno.append(42);
        ci.centroidSource_rr.append(Vector3f(0.1f, 0.2f, 0.3f));
        VectorXi verts(3);
        verts << 10, 20, 30;
        ci.clusterVertnos.append(verts);
        MatrixX3f rr(3, 3);
        rr.setRandom();
        ci.clusterSource_rr.append(rr);
        VectorXd dist(3);
        dist << 0.01, 0.02, 0.03;
        ci.clusterDistances.append(dist);

        QVERIFY(!ci.isEmpty());
        QCOMPARE(ci.numClust(), 1);

        ci.clear();
        QVERIFY(ci.isEmpty());
        QCOMPARE(ci.numClust(), 0);
    }

    void clusterInfo_equality()
    {
        MNEClusterInfo a, b;

        // Add matching data
        a.clusterLabelNames.append("L1");
        a.clusterLabelIds.append(1);
        a.centroidVertno.append(10);
        a.centroidSource_rr.append(Vector3f(1.0f, 2.0f, 3.0f));
        VectorXi verts(2);
        verts << 10, 20;
        a.clusterVertnos.append(verts);
        MatrixX3f rr(2, 3);
        rr << 1, 2, 3, 4, 5, 6;
        a.clusterSource_rr.append(rr);
        VectorXd dist(2);
        dist << 0.1, 0.2;
        a.clusterDistances.append(dist);

        b = a;
        QVERIFY(a == b);
    }

    void clusterInfo_writeToFile()
    {
        MNEClusterInfo ci;
        ci.clusterLabelNames.append("TestLabel");
        ci.clusterLabelIds.append(42);
        ci.centroidVertno.append(7);
        ci.centroidSource_rr.append(Vector3f(0.01f, 0.02f, 0.03f));
        VectorXi verts(2);
        verts << 7, 8;
        ci.clusterVertnos.append(verts);
        MatrixX3f rr(2, 3);
        rr << 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f;
        ci.clusterSource_rr.append(rr);
        VectorXd dist(2);
        dist << 0.001, 0.002;
        ci.clusterDistances.append(dist);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString filePath = tmpDir.path() + "/cluster_info.txt";
        ci.write(filePath);

        QFile f(filePath);
        QVERIFY(f.exists());
        QVERIFY(f.size() > 0);
    }

    //=========================================================================
    // MNESourceSpaces
    //=========================================================================
    void sourceSpace_defaultCtor()
    {
        MNESourceSpaces ss;
        QVERIFY(ss.isEmpty());
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

    //=========================================================================
    // MNEForwardSolution
    //=========================================================================
    void forwardSolution_defaultCtor()
    {
        MNEForwardSolution fwd;
        QVERIFY(fwd.isEmpty());
        // Note: isClustered() crashes on empty forward solution (accesses empty source space)
    }

    void forwardSolution_clear()
    {
        MNEForwardSolution fwd;
        fwd.clear();
        QVERIFY(fwd.isEmpty());
    }

    void forwardSolution_tripletSelection()
    {
        MNEForwardSolution fwd;
        VectorXi sel(3);
        sel << 0, 1, 2;
        VectorXi tri = fwd.tripletSelection(sel);
        // Should produce [0,1,2, 3,4,5, 6,7,8]
        QCOMPARE(tri.size(), (Eigen::Index)9);
        QCOMPARE(tri(0), 0);
        QCOMPARE(tri(1), 1);
        QCOMPARE(tri(2), 2);
        QCOMPARE(tri(3), 3);
        QCOMPARE(tri(4), 4);
        QCOMPARE(tri(5), 5);
        QCOMPARE(tri(6), 6);
        QCOMPARE(tri(7), 7);
        QCOMPARE(tri(8), 8);
    }

    void forwardSolution_readFromFile()
    {
        QString fwdFile = dataPath() + "Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile)) {
            QSKIP("Forward solution file not available");
        }

        QFile file(fwdFile);
        MNEForwardSolution fwd;
        bool ok = MNEForwardSolution::read(file, fwd);
        QVERIFY(ok);
        QVERIFY(!fwd.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestMneExtended)

#include "test_mne_extended.moc"
