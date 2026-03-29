//=============================================================================================================
/**
 * @file     test_fiff_cov.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Gabriel B Motta, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Test for I/O of a FiffCov
 *
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

namespace {

QString sampleDataPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample";
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
class TestFiffCov: public QObject
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
    MatrixXd mDataDiff = covResult.data*1000000 - covLoaded.data*1000000;

//    qDebug()<<"abs(covResult.data.sum()) "<<covResult.data.normalized().sum();
//    qDebug()<<"abs(covLoaded.data.sum()) "<<covLoaded.data.normalized().sum();
//    qDebug()<<"abs(mDataDiff.sum()) "<<abs(mDataDiff.sum());
//    qDebug()<<"epsilon "<<epsilon;

    QVERIFY( std::abs(mDataDiff.sum()) < dEpsilon );
}

//=============================================================================================================

void TestFiffCov::compareKind()
{
    QVERIFY( covResult.kind == covLoaded.kind );
}

//=============================================================================================================

void TestFiffCov::compareDiag()
{
    QVERIFY( covResult.diag == covLoaded.diag );
}

//=============================================================================================================

void TestFiffCov::compareDim()
{
    QVERIFY( covResult.dim == covLoaded.dim );
}

//=============================================================================================================

void TestFiffCov::compareNfree()
{
    QVERIFY( covResult.nfree == covLoaded.nfree );
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
