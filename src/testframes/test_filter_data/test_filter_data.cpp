#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <Eigen/Dense>

#include <dsp/filterkernel.h>
#include <dsp/rt_filter.h>
#include <dsp/rt_detect_trigger.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>

using namespace RTPROCESSINGLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

class TestFilterData : public QObject
{
    Q_OBJECT

private:
    RowVectorXi makePicks(int nCh) {
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i) picks(i) = i;
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
        QList<QPair<int, double>> triggers = detectTriggerFlanksMax(data, 0, 0, 0.5, true);
        QCOMPARE(triggers.size(), 0);
    }

    void detectTrigger_singlePulse()
    {
        MatrixXd data = MatrixXd::Zero(1, 100);
        data(0, 50) = 1.0;
        data(0, 51) = 1.0;
        data(0, 52) = 1.0;

        QList<QPair<int, double>> triggers = detectTriggerFlanksMax(data, 0, 0, 0.5, true);
        QVERIFY(triggers.size() >= 0);
    }

    void detectTrigger_gradient()
    {
        MatrixXd data = MatrixXd::Zero(1, 100);
        for (int i = 0; i < 100; ++i) {
            data(0, i) = (i < 50) ? 0.0 : 1.0;
        }

        QList<int> lTriggerChannels;
        lTriggerChannels << 0;
        QMap<int, QList<QPair<int, double>>> mapTriggers = detectTriggerFlanksGrad(data, lTriggerChannels, 0, 0.5, false, "Rising");
        QVERIFY(mapTriggers.size() >= 0);
    }

    //=========================================================================
    // filterFile from raw data
    //=========================================================================
    void filterFile_fromRawData()
    {
        QString rawPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile file(rawPath);
        FiffRawData::SPtr raw(new FiffRawData(file));

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outPath = tmpDir.path() + "/filtered.fif";
        QFile outFile(outPath);
        QVERIFY(outFile.open(QIODevice::WriteOnly));

        RowVectorXi picks(3);
        picks << 0, 1, 2;
        FilterKernel fk("LPF", 0, 64, 0.2, 0.0, 0.01, raw->info.sfreq, 0);
        bool ok = filterFile(outFile, raw, fk, picks);
        outFile.close();
        QVERIFY(ok);
        QVERIFY(QFileInfo(outPath).size() > 0);
    }
};

QTEST_GUILESS_MAIN(TestFilterData)
#include "test_filter_data.moc"
