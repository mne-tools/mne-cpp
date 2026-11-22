//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_cov_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Cross validates the noise covariance reader against mne-python.
 *
 * A noise covariance that is read slightly wrong does not announce itself. It
 * produces an inverse operator that still runs and still returns a source
 * estimate, only a wrong one. That makes the reader worth checking against an
 * independent implementation rather than against MNE-CPP's own output.
 *
 * Reference values below come from mne.read_cov on the same file:
 *
 *   kind      1 (FIFFV_MNE_NOISE_COV)
 *   dim       366
 *   nfree     15972
 *   diag      False
 *   names     366, first "MEG 0113", last "EEG 060"
 *   bads      2, "MEG 2443" and "EEG 053"
 *   projs     4
 *   trace     1.155697251997e-09
 *   sum       2.229105787152e-13
 *   data[0,0] 2.272355891907e-23
 *   symmetry  exact, max |C - C^T| = 0
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_cov.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCovPython
 *
 * @brief The TestFiffCovPython class checks the covariance reader against mne-python.
 */
class TestFiffCovPython : public QObject
{
    Q_OBJECT

public:
    TestFiffCovPython() = default;

private:
    static QString covPath();

    FiffCov m_cov;

private slots:
    void initTestCase();
    void scalars_matchPython_data();
    void scalars_matchPython();
    void channelNames_matchPython();
    void data_matchesPython();
    void data_isSymmetric();
    void missingKind_isReported();
    void regularize_matchesPython_data();
    void regularize_matchesPython();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffCovPython::covPath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif";
}

//=============================================================================================================

void TestFiffCovPython::initTestCase()
{
    if (!QFile::exists(covPath())) {
        QSKIP("Covariance test data not found");
    }

    m_cov = FiffCov(covPath().toStdU16String());

    QVERIFY2(m_cov.dim > 0, "covariance did not load");
}

//=============================================================================================================

void TestFiffCovPython::scalars_matchPython_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<int>("expected");

    QTest::newRow("kind") << "kind" << 1; // FIFFV_MNE_NOISE_COV
    QTest::newRow("dim") << "dim" << 366;
    QTest::newRow("nfree") << "nfree" << 15972;
    QTest::newRow("diag") << "diag" << int{0}; // stored full, not diagonal
    QTest::newRow("n_names") << "n_names" << 366;
    QTest::newRow("n_bads") << "n_bads" << 2;
    QTest::newRow("n_projs") << "n_projs" << 4;
    QTest::newRow("rows") << "rows" << 366;
    QTest::newRow("cols") << "cols" << 366;
}

//=============================================================================================================

void TestFiffCovPython::scalars_matchPython()
{
    QFETCH(QString, field);
    QFETCH(int, expected);

    int actual = -1;
    if (field == "kind")
        actual = static_cast<int>(m_cov.kind);
    else if (field == "dim")
        actual = static_cast<int>(m_cov.dim);
    else if (field == "nfree")
        actual = static_cast<int>(m_cov.nfree);
    else if (field == "diag")
        actual = m_cov.diag ? 1 : 0;
    else if (field == "n_names")
        actual = static_cast<int>(m_cov.names.size());
    else if (field == "n_bads")
        actual = static_cast<int>(m_cov.bads.size());
    else if (field == "n_projs")
        actual = static_cast<int>(m_cov.projs.size());
    else if (field == "rows")
        actual = static_cast<int>(m_cov.data.rows());
    else if (field == "cols")
        actual = static_cast<int>(m_cov.data.cols());
    else
        QFAIL("unknown field");

    QCOMPARE(actual, expected);
}

//=============================================================================================================

void TestFiffCovPython::channelNames_matchPython()
{
    // The names order is what maps a covariance entry onto a channel. If it
    // slips, every row and column refers to the wrong sensor while the matrix
    // still looks perfectly well formed.
    QCOMPARE(m_cov.names.size(), 366);

    // Names as stored in the file, as mne-python reads them.
    QCOMPARE(m_cov.names.front(), QString("MEG 0113"));
    QCOMPARE(m_cov.names.back(), QString("EEG 060"));
    QCOMPARE(m_cov.bads.size(), 2);
    QVERIFY(std::ranges::contains(m_cov.bads, "MEG 2443"));
    QVERIFY(std::ranges::contains(m_cov.bads, "EEG 053"));

    // The name list has to line up with the matrix it describes.
    QCOMPARE(m_cov.names.size(), static_cast<int>(m_cov.data.rows()));
}

//=============================================================================================================

void TestFiffCovPython::data_matchesPython()
{
    const MatrixXd& d = m_cov.data;

    // Covariance entries here are of order 1e-23, so comparisons are relative.
    // An absolute tolerance would pass on an all zero matrix.
    const double trace = d.trace();
    const double sum = d.sum();
    const double d00 = d(0, 0);

    const double expTrace = 1.155697251997e-09;
    const double expSum = 2.229105787152e-13;
    const double expD00 = 2.272355891907e-23;

    QVERIFY2(std::fabs(trace - expTrace) / std::fabs(expTrace) < 1.0e-9,
             qPrintable(QString("trace is %1, mne-python says %2")
                            .arg(trace, 0, 'e', 12)
                            .arg(expTrace, 0, 'e', 12)));

    // Summing every entry pins all 366x366 values rather than a chosen few.
    QVERIFY2(std::fabs(sum - expSum) / std::fabs(expSum) < 1.0e-9,
             qPrintable(QString("sum is %1, mne-python says %2")
                            .arg(sum, 0, 'e', 12)
                            .arg(expSum, 0, 'e', 12)));

    QVERIFY2(std::fabs(d00 - expD00) / std::fabs(expD00) < 1.0e-9,
             qPrintable(QString("data(0,0) is %1, mne-python says %2")
                            .arg(d00, 0, 'e', 12)
                            .arg(expD00, 0, 'e', 12)));
}

//=============================================================================================================

void TestFiffCovPython::data_isSymmetric()
{
    // A covariance matrix is symmetric by definition. The file stores only the
    // lower triangle, so this checks that the reader mirrored it correctly
    // rather than leaving half the matrix at zero.
    const MatrixXd& d = m_cov.data;
    QCOMPARE(d.rows(), d.cols());

    const double asym = (d - d.transpose()).cwiseAbs().maxCoeff();
    QVERIFY2(asym == 0.0,
             qPrintable(QString("covariance is not symmetric, max |C - C^T| = %1")
                            .arg(asym, 0, 'e', 3)));

    // The diagonal holds variances, which cannot be negative.
    QVERIFY2(d.diagonal().minCoeff() > 0.0, "covariance has a non positive variance on its diagonal");

    // The grand average weights each covariance by its degrees of freedom; mismatched sizes give none
    FiffCov twice = m_cov;
    twice.data *= 2.0;
    twice.nfree = 3 * m_cov.nfree;
    const FiffCov grand = FiffCov::compute_grand_average({m_cov, twice});
    QCOMPARE(grand.nfree, 4 * m_cov.nfree);
    QVERIFY(grand.data.isApprox(1.75 * d));
    QCOMPARE(FiffCov::compute_grand_average({}).dim, -1);
    FiffCov smaller = m_cov.pick_channels(StringList(m_cov.names.begin() + 0, m_cov.names.begin() + 0 + 3));
    QCOMPARE(FiffCov::compute_grand_average({m_cov, smaller}).dim, -1);
    QCOMPARE(m_cov.save({}).error().code, mne::ErrorCode::Io);
}

//=============================================================================================================

void TestFiffCovPython::missingKind_isReported()
{
    // Asking for a kind the file does not contain has to fail rather than
    // return whatever covariance happens to be there first.
    FiffStream stream(covPath().toStdU16String());
    QVERIFY(stream.open());

    FiffCov cov;
    const bool ok = stream.read_cov(stream.dirtree(), FIFFV_MNE_FMRI_PRIOR_COV, cov);
    QVERIFY2(!ok, "reading an absent covariance kind reported success");

    stream.close();
}

//=============================================================================================================

void TestFiffCovPython::regularize_matchesPython_data()
{
    // mne.cov.regularize(cov, read_info(sample_audvis_trunc_raw.fif), mag=0.1, grad=0.05, eeg=0.2,
    // proj=..., rank="full"): trace, sum |C|, C[MEG0111,MEG0111], C[EEG001,EEG001], C[MEG0112,MEG0112].
    QTest::addColumn<bool>("proj");
    QTest::addColumn<double>("trace");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("grad11");
    QTest::addColumn<double>("eeg1");
    QTest::addColumn<double>("mag12");
    QTest::newRow("with SSP") << true << 1.386792121597564e-09 << 2.0671792043092587e-08 << 6.036611780302426e-26 << 4.648008748962221e-11 << 6.1608397326172324e-24;
    QTest::newRow("without SSP") << false << 1.3867921217721483e-09 << 2.0640104718027955e-08 << 6.03548239156651e-26 << 4.648008832222862e-11 << 6.1608397326172324e-24;
}

void TestFiffCovPython::regularize_matchesPython()
{
    QFETCH(bool, proj);
    QFETCH(double, trace);
    QFETCH(double, absSum);
    QFETCH(double, grad11);
    QFETCH(double, eeg1);
    QFETCH(double, mag12);

    const FiffRawData raw((QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
    // The covariance names channels as stored ("MEG 0111"); MNE-CPP's info strips the space.
    FiffInfo info = raw.info;
    const FiffCov& cov = m_cov;

    const FiffCov reg = cov.regularize(info, 0.1, 0.05, 0.2, proj);
    auto at = [&](const char* a, const char* b) {
        return reg.data(mne::index_of(reg.names, a), mne::index_of(reg.names, b));
    };
    auto close = [](double got, double want) {
        return std::abs(got - want) <= 1e-9 * std::abs(want);
    };
    QVERIFY2(close(reg.data.trace(), trace), qPrintable(QString("trace %1").arg(reg.data.trace(), 0, 'g', 17)));
    QVERIFY2(close(reg.data.cwiseAbs().sum(), absSum), qPrintable(QString("sum %1").arg(reg.data.cwiseAbs().sum(), 0, 'g', 17)));
    QVERIFY2(close(at("MEG 0111", "MEG 0111"), grad11), qPrintable(QString("grad %1").arg(at("MEG 0111", "MEG 0111"), 0, 'g', 17)));
    QVERIFY2(close(at("EEG 001", "EEG 001"), eeg1), qPrintable(QString("eeg %1").arg(at("EEG 001", "EEG 001"), 0, 'g', 17)));
    QVERIFY2(close(at("MEG 0112", "MEG 0112"), mag12), qPrintable(QString("mag %1").arg(at("MEG 0112", "MEG 0112"), 0, 'g', 17)));

    // A zero amount leaves that channel type alone; excluding every EEG channel leaves EEG untouched too
    const FiffCov noEeg = cov.regularize(info, 0.1, 0.05, 0.0, proj);
    QCOMPARE(noEeg.data(mne::index_of(noEeg.names, "EEG 001"), mne::index_of(noEeg.names, "EEG 001")), cov.data(mne::index_of(cov.names, "EEG 001"), mne::index_of(cov.names, "EEG 001")));
    StringList eegNames;
    for (const FiffChInfo& ch : info.chs) {
        if (ch.kind == FIFFV_EEG_CH)
            eegNames.push_back(ch.ch_name);
    }
    const FiffCov excluded = cov.regularize(info, 0.1, 0.05, 0.2, proj, eegNames);
    QCOMPARE(excluded.data(mne::index_of(excluded.names, "EEG 001"), mne::index_of(excluded.names, "EEG 001")), cov.data(mne::index_of(cov.names, "EEG 001"), mne::index_of(cov.names, "EEG 001")));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCovPython)
#include "test_fiff_cov_python.moc"
