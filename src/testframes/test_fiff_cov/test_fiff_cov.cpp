//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     test_fiff_cov.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2017
 * @brief    Test for I/O of a FiffCov
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>

#include <iostream>
#include <utils/ioutils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

namespace
{

QString sampleDataPath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample";
}

MatrixXi deriveStimEvents(const FiffRawData& raw)
{
    int stimIdx = -1;
    for (int channelIndex = 0; channelIndex < raw.info.nchan; ++channelIndex) {
        if (raw.info.chs[channelIndex].kind == FIFFV_STIM_CH) {
            stimIdx = channelIndex;
            if (raw.info.ch_names.value(channelIndex).remove(QLatin1Char(' ')) == QLatin1String("STI014")) {
                break;
            }
        }
    }

    if (stimIdx < 0) {
        return MatrixXi();
    }

    RowVectorXi picks(1);
    picks << stimIdx;

    MatrixXd stimData;
    MatrixXd stimTimes;
    if (!raw.read_raw_segment(stimData, stimTimes, raw.first_samp, raw.last_samp, picks) || stimData.rows() != 1) {
        return MatrixXi();
    }

    QVector<Vector3i> detectedEvents;
    int previousValue = 0;
    for (int sampleOffset = 0; sampleOffset < stimData.cols(); ++sampleOffset) {
        const int currentValue = qRound(stimData(0, sampleOffset));
        if (currentValue != previousValue && currentValue != 0) {
            detectedEvents.append(Vector3i(raw.first_samp + sampleOffset,
                                           previousValue,
                                           currentValue));
        }
        previousValue = currentValue;
    }

    MatrixXi events(detectedEvents.size(), 3);
    for (int row = 0; row < detectedEvents.size(); ++row) {
        events(row, 0) = detectedEvents.at(row)(0);
        events(row, 1) = detectedEvents.at(row)(1);
        events(row, 2) = detectedEvents.at(row)(2);
    }

    return events;
}

QList<int> uniqueEventCodes(const MatrixXi& events, int maxCodes = -1)
{
    QList<int> codes;
    for (int row = 0; row < events.rows(); ++row) {
        const int code = events(row, 2);
        if (code == 0 || codes.contains(code)) {
            continue;
        }

        codes.append(code);
        if (maxCodes > 0 && codes.size() >= maxCodes) {
            break;
        }
    }

    return codes;
}

}

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCov
 *
 * @brief The TestFiffCov class provides covariance reading verification tests
 *
 */
class TestFiffCov : public QObject
{
    Q_OBJECT

public:
    TestFiffCov();

private slots:
    void initTestCase();
    void compareData();
    void compareKind();
    void compareDiag();
    void compareDim();
    void compareNfree();
    void computeFromEpochs_sampleRaw();
    void computeFromEpochs_matchesPython_data();
    void computeFromEpochs_matchesPython();
    void saveRoundTrip_computedCovariance();
    void cleanupTestCase();

private:
    double dEpsilon;

    FiffCov covLoaded;
    FiffCov covResult;
};

//=============================================================================================================

TestFiffCov::TestFiffCov()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestFiffCov::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    qDebug() << "Epsilon" << dEpsilon;
    //Read the results produced with MNE-CPP
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif");
    covLoaded = FiffCov(t_fileIn);

    //Read the result data produced with mne_matlab
    MatrixXd data;
    IOUtils::read_eigen_matrix(data, QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_data_sample_audvis-cov.dat");
    covResult.data = data;

    covResult.kind = 1;
    covResult.diag = 0;
    covResult.dim = 366;
    covResult.nfree = 15972;
}

//=============================================================================================================

void TestFiffCov::compareData()
{
    //Make the values a little bit bigger
    MatrixXd mDataDiff = covResult.data * 1000000 - covLoaded.data * 1000000;

    //    qDebug()<<"abs(covResult.data.sum()) "<<covResult.data.normalized().sum();
    //    qDebug()<<"abs(covLoaded.data.sum()) "<<covLoaded.data.normalized().sum();
    //    qDebug()<<"abs(mDataDiff.sum()) "<<abs(mDataDiff.sum());
    //    qDebug()<<"epsilon "<<epsilon;

    QVERIFY(std::abs(mDataDiff.sum()) < dEpsilon);
}

//=============================================================================================================

void TestFiffCov::compareKind()
{
    QVERIFY(covResult.kind == covLoaded.kind);
}

//=============================================================================================================

void TestFiffCov::compareDiag()
{
    QVERIFY(covResult.diag == covLoaded.diag);
}

//=============================================================================================================

void TestFiffCov::compareDim()
{
    QVERIFY(covResult.dim == covLoaded.dim);
}

//=============================================================================================================

void TestFiffCov::compareNfree()
{
    QVERIFY(covResult.nfree == covLoaded.nfree);
}

//=============================================================================================================

void TestFiffCov::computeFromEpochs_sampleRaw()
{
    QFile rawFile(sampleDataPath() + "/sample_audvis_trunc_raw.fif");
    if (!rawFile.exists()) {
        QSKIP("Sample raw file not found");
    }

    FiffRawData raw(rawFile);
    const MatrixXi events = deriveStimEvents(raw);
    QVERIFY(events.rows() > 0);

    const QList<int> codes = uniqueEventCodes(events, 4);
    QVERIFY(!codes.isEmpty());

    const FiffCov cov = FiffCov::compute_from_epochs(raw,
                                                     events,
                                                     codes,
                                                     -0.2f,
                                                     0.0f,
                                                     -0.2f,
                                                     0.0f,
                                                     true,
                                                     true);

    QVERIFY(!cov.isEmpty());
    QCOMPARE(cov.dim, raw.info.nchan);
    QCOMPARE(cov.data.rows(), static_cast<Index>(raw.info.nchan));
    QCOMPARE(cov.data.cols(), static_cast<Index>(raw.info.nchan));
    QVERIFY(cov.nfree > 0);
}

//=============================================================================================================

void TestFiffCov::computeFromEpochs_matchesPython_data()
{
    // mne.Epochs(raw, find_events(raw), event_id=codes, tmin, tmax, baseline, proj=False, picks="all",
    // reject=None) with events shifted by round(delay * sfreq); C = sum_e X X^T / (N - 1), minus
    // N/(N - 1) m m^T with the grand mean m if the mean is removed. C[MEG0113], C[EEG001], C[MEG0113,EEG001].
    QTest::addColumn<QList<int>>("codes");
    QTest::addColumn<float>("tmin");
    QTest::addColumn<float>("tmax");
    QTest::addColumn<bool>("baseline");
    QTest::addColumn<float>("bmax");
    QTest::addColumn<bool>("removeMean");
    QTest::addColumn<float>("delay");
    QTest::addColumn<int>("nfree");
    QTest::addColumn<Vector3d>("ref");
    QTest::newRow("baseline whole window") << QList<int>{1, 2} << -0.2f << 0.0f << true << 0.0f << true << 0.0f << 731
                                           << Vector3d(2.652924332488991e-30, 7.054628857691069e-17, 5.769123724874705e-24);
    QTest::newRow("pre-stimulus baseline, mean kept") << QList<int>{3} << -0.1f << 0.1f << true << 0.0f << false << 0.0f << 365
                                                      << Vector3d(1.4593490861834027e-30, 6.011250385749755e-18, 2.5599899614022574e-25);
    QTest::newRow("no baseline, 50 ms delay") << QList<int>{1, 2, 3, 4} << -0.2f << 0.0f << false << 0.0f << true << 0.05f << 1402
                                              << Vector3d(2.7808598052155747e-30, 1.5451764897172425e-16, 7.910586499046573e-24);
}

void TestFiffCov::computeFromEpochs_matchesPython()
{
    QFETCH(QList<int>, codes);
    QFETCH(float, tmin);
    QFETCH(float, tmax);
    QFETCH(bool, baseline);
    QFETCH(float, bmax);
    QFETCH(bool, removeMean);
    QFETCH(float, delay);
    QFETCH(int, nfree);
    QFETCH(Vector3d, ref);

    QFile rawFile(sampleDataPath() + "/sample_audvis_trunc_raw.fif");
    if (!rawFile.exists()) {
        QSKIP("Sample raw file not found");
    }
    FiffRawData raw(rawFile);
    const MatrixXi events = deriveStimEvents(raw);

    const FiffCov cov = FiffCov::compute_from_epochs(raw, events, codes, tmin, tmax, tmin, bmax, baseline, removeMean, 0, delay);
    QCOMPARE(cov.nfree, nfree);
    const int i = cov.names.indexOf("MEG0113");
    const int e = cov.names.indexOf("EEG001");
    const Vector3d got(cov.data(i, i), cov.data(e, e), cov.data(i, e));
    for (int k = 0; k < 3; ++k)
        QVERIFY2(std::abs(got[k] - ref[k]) <= 1e-6 * std::abs(ref[k]), qPrintable(QString("entry %1: %2 vs %3").arg(k).arg(got[k], 0, 'g', 17).arg(ref[k], 0, 'g', 17)));
}

//=============================================================================================================

void TestFiffCov::saveRoundTrip_computedCovariance()
{
    QFile rawFile(sampleDataPath() + "/sample_audvis_trunc_raw.fif");
    if (!rawFile.exists()) {
        QSKIP("Sample raw file not found");
    }

    FiffRawData raw(rawFile);
    const MatrixXi events = deriveStimEvents(raw);
    QVERIFY(events.rows() > 0);

    const QList<int> codes = uniqueEventCodes(events, 4);
    QVERIFY(!codes.isEmpty());

    const FiffCov cov = FiffCov::compute_from_epochs(raw,
                                                     events,
                                                     codes,
                                                     -0.2f,
                                                     0.0f,
                                                     -0.2f,
                                                     0.0f,
                                                     true,
                                                     true);
    QVERIFY(!cov.isEmpty());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString outPath = tempDir.path() + "/computed-cov.fif";
    QVERIFY(cov.save(outPath));
    QVERIFY(QFile::exists(outPath));

    QFile savedFile(outPath);
    FiffCov roundTrip(savedFile);
    QVERIFY(!roundTrip.isEmpty());
    QCOMPARE(roundTrip.dim, cov.dim);
    QCOMPARE(roundTrip.names, cov.names);
    QVERIFY(roundTrip.data.isApprox(cov.data, 1e-9));
}

//=============================================================================================================

void TestFiffCov::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCov)
#include "test_fiff_cov.moc"
