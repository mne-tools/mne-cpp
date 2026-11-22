// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <memory>
#include <mne/base/string_utils.h>
#include <QTemporaryDir>
#include <Eigen/Dense>

#include <mne/dsp/filterkernel.h>
#include <mne/dsp/rt/rt_filter.h>
#include <mne/dsp/rt/rt_detect_trigger.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_info.h>

using namespace mne;
using namespace Eigen;

class TestFilterData : public QObject
{
    Q_OBJECT

private:
    RowVectorXi makePicks(int nCh)
    {
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i)
            picks(i) = i;
        return picks;
    }

private slots:
    //=========================================================================
    // filterData - various filter types with explicit picks
    //=========================================================================
    void filterData_lpfMultiChannel()
    {
        int nCh = 4, N = 512;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        MatrixXd filtered = filterData(data, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0, picks);
        QCOMPARE(filtered.rows(), nCh);
        QCOMPARE(filtered.cols(), N);
    }

    void filterData_hpfMultiChannel()
    {
        int nCh = 3, N = 512;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        MatrixXd filtered = filterData(data, 1, 0.0, 5.0, 2.0, 1000.0, 128, 0, picks);
        QCOMPARE(filtered.rows(), nCh);
    }

    void filterData_bpfMultiChannel()
    {
        int nCh = 3, N = 512;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        MatrixXd filtered = filterData(data, 2, 20.0, 10.0, 5.0, 1000.0, 128, 0, picks);
        QCOMPARE(filtered.rows(), nCh);
    }

    void filterData_withKernelObj()
    {
        int nCh = 3, N = 512;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        FilterKernel fk("LPF", 0, 128, 0.2, 0.0, 0.01, 1000.0, 0);
        MatrixXd filtered = filterData(data, fk, picks);
        QCOMPARE(filtered.rows(), nCh);

        // Default picks must mean every channel; Eigen's integer LinSpaced(n, 0, n) overruns only for n <= 2
        const MatrixXd twoChannels = data.topRows(2);
        const MatrixXd allChannels = filterData(twoChannels, fk);
        QVERIFY((allChannels - filtered.topRows(2)).cwiseAbs().maxCoeff() < 1e-12);
    }

    void filterData_keepOverhead()
    {
        int nCh = 2, N = 256;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        FilterKernel fk("LPF", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        MatrixXd filtered = filterData(data, fk, picks, true, true);
        QCOMPARE(filtered.rows(), nCh);
        QVERIFY(filtered.cols() >= N);
    }

    //=========================================================================
    // filterDataBlock
    //=========================================================================
    void filterDataBlock_multiChannel()
    {
        int nCh = 3, N = 256;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);
        MatrixXd filtered = filterDataBlock(data, picks, fk, false);
        QCOMPARE(filtered.rows(), nCh);
    }

    void filterDataBlock_threaded()
    {
        int nCh = 4, N = 256;
        MatrixXd data = MatrixXd::Random(nCh, N);
        RowVectorXi picks = makePicks(nCh);

        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);
        MatrixXd filtered = filterDataBlock(data, picks, fk, true);
        QCOMPARE(filtered.rows(), nCh);
    }

    //=========================================================================
    // FilterOverlapAdd - continuous streaming filter
    //=========================================================================
    void filterOverlapAdd_paramsCtor()
    {
        FilterOverlapAdd filter;
        int nCh = 3;
        RowVectorXi picks = makePicks(nCh);

        MatrixXd block = MatrixXd::Random(nCh, 256);
        MatrixXd result = filter.calculate(block, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0, picks);
        QCOMPARE(result.rows(), nCh);
    }

    void filterOverlapAdd_multipleBlocks()
    {
        FilterOverlapAdd filter;
        int nCh = 3;
        RowVectorXi picks = makePicks(nCh);
        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);

        for (int b = 0; b < 5; ++b) {
            MatrixXd block = MatrixXd::Random(nCh, 256);
            MatrixXd result = filter.calculate(block, fk, picks);
            QCOMPARE(result.rows(), nCh);
        }

        filter.reset();
    }

    //=========================================================================
    // DetectTrigger
    //=========================================================================
    void detectTrigger_flatSignal()
    {
        MatrixXd data = MatrixXd::Zero(1, 1000);
        QVERIFY(detect_trigger_flanks_max(data, 0, 0, 0.5, true).empty());
    }

    void detectTrigger_singlePulse()
    {
        MatrixXd data = MatrixXd::Zero(1, 100);
        data(0, 50) = 1.0;
        data(0, 51) = 1.0;
        data(0, 52) = 1.0;

        QVERIFY(detect_trigger_flanks_max(data, 0, 0, 0.5, true) == (std::vector<TriggerFlank>{{50, 1.0}}));
    }

    void detectTrigger_gradient()
    {
        MatrixXd data = MatrixXd::Zero(1, 100);
        for (int i = 0; i < 100; ++i) {
            data(0, i) = (i < 50) ? 0.0 : 1.0;
        }

        const auto triggers = detect_trigger_flanks_grad(data, std::vector<int>{0}, 0, 0.5, FlankDirection::Rising);
        QVERIFY(triggers.at(0) == (std::vector<TriggerFlank>{{50, 1.0}}));
    }

    //=========================================================================
    // filterFile from raw data
    //=========================================================================
    void filterFile_fromRawData()
    {
        QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        std::shared_ptr<FiffRawData> raw(new FiffRawData(rawPath.toStdString()));

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outPath = tmpDir.path() + "/filtered.fif";

        RowVectorXi picks(3);
        picks << 0, 1, 2;
        FilterKernel fk("LPF", 0, 64, 0.2, 0.0, 0.01, raw->info.sfreq, 0);
        bool ok = filterFile(outPath.toStdU16String(), raw, fk, picks);
        QVERIFY(ok);

        // Same samples, and the channels not picked are copied unchanged at their own times
        FiffRawData filtered(outPath.toStdString());
        QCOMPARE(filtered.first_samp, raw->first_samp);
        QCOMPARE(filtered.last_samp, raw->last_samp);
        MatrixXd a, b, t;
        QVERIFY(raw->read_raw_segment(a, t, raw->first_samp, raw->last_samp));
        QVERIFY(filtered.read_raw_segment(b, t, raw->first_samp, raw->last_samp));
        const int stim = static_cast<int>(mne::index_of(raw->info.ch_names, "STI 014"));
        QVERIFY(stim >= 0);
        QCOMPARE(b.row(stim), a.row(stim));
        QVERIFY((b.row(3) - a.row(3)).cwiseAbs().maxCoeff() <= 1e-6 * a.row(3).cwiseAbs().maxCoeff());
        // Block-wise overlap-add gives the picked channels the same values as filtering the whole recording at once
        const MatrixXd whole = filterData(a.topRows(3), fk, picks, false, false);
        QCOMPARE(whole.cols(), a.cols());
        QVERIFY2((b.topRows(3) - whole).cwiseAbs().maxCoeff() <= 1e-4 * whole.cwiseAbs().maxCoeff(),
                 qPrintable(QString::number((b.topRows(3) - whole).cwiseAbs().maxCoeff() / whole.cwiseAbs().maxCoeff())));

        QVERIFY(!filterFile(tmpDir.filePath(QStringLiteral("no/such/dir/out.fif")).toStdU16String(), raw, fk, picks));
    }
};

QTEST_GUILESS_MAIN(TestFilterData)
#include "test_filter_data.moc"
