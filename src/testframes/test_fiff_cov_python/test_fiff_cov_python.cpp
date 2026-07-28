//=============================================================================================================
/**
 * @file     test_fiff_cov_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     February, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MNE-CPP AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

#include <fiff/fiff_cov.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_constants.h>

#include <cmath>

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
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCovPython
 *
 * @brief The TestFiffCovPython class checks the covariance reader against mne-python.
 */
class TestFiffCovPython: public QObject
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
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffCovPython::covPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif";
}

//=============================================================================================================

void TestFiffCovPython::initTestCase()
{
    if(!QFile::exists(covPath())) {
        QSKIP("Covariance test data not found");
    }

    QFile covFile(covPath());
    m_cov = FiffCov(covFile);

    QVERIFY2(m_cov.dim > 0, "covariance did not load");
}

//=============================================================================================================

void TestFiffCovPython::scalars_matchPython_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<int>("expected");

    QTest::newRow("kind")      << "kind"      << 1;      // FIFFV_MNE_NOISE_COV
    QTest::newRow("dim")       << "dim"       << 366;
    QTest::newRow("nfree")     << "nfree"     << 15972;
    QTest::newRow("diag")      << "diag"      << 0;      // stored full, not diagonal
    QTest::newRow("n_names")   << "n_names"   << 366;
    QTest::newRow("n_bads")    << "n_bads"    << 2;
    QTest::newRow("n_projs")   << "n_projs"   << 4;
    QTest::newRow("rows")      << "rows"      << 366;
    QTest::newRow("cols")      << "cols"      << 366;
}

//=============================================================================================================

void TestFiffCovPython::scalars_matchPython()
{
    QFETCH(QString, field);
    QFETCH(int, expected);

    int actual = -1;
    if(field == "kind")          actual = static_cast<int>(m_cov.kind);
    else if(field == "dim")      actual = static_cast<int>(m_cov.dim);
    else if(field == "nfree")    actual = static_cast<int>(m_cov.nfree);
    else if(field == "diag")     actual = m_cov.diag ? 1 : 0;
    else if(field == "n_names")  actual = m_cov.names.size();
    else if(field == "n_bads")   actual = m_cov.bads.size();
    else if(field == "n_projs")  actual = m_cov.projs.size();
    else if(field == "rows")     actual = static_cast<int>(m_cov.data.rows());
    else if(field == "cols")     actual = static_cast<int>(m_cov.data.cols());
    else QFAIL("unknown field");

    QCOMPARE(actual, expected);
}

//=============================================================================================================

void TestFiffCovPython::channelNames_matchPython()
{
    // The names order is what maps a covariance entry onto a channel. If it
    // slips, every row and column refers to the wrong sensor while the matrix
    // still looks perfectly well formed.
    QCOMPARE(m_cov.names.size(), 366);

    // MNE-CPP strips spaces out of channel names as it reads them, so the file's
    // "MEG 0113" becomes "MEG0113". mne-python keeps the name as stored. This is
    // a naming convention rather than a defect: the stripping happens on every
    // read path, in FiffTag for FIFF_CH_INFO and in split_name_list for the name
    // list tags, so the two agree wherever MNE-CPP matches one against the other.
    // It does mean a name from an MNE-CPP structure cannot be compared directly
    // against one from a file or from mne-python without normalising first.
    QCOMPARE(m_cov.names.first(), QString("MEG0113"));
    QCOMPARE(m_cov.names.last(),  QString("EEG060"));

    // Same names as mne-python once the spaces are put back.
    auto respace = [](QString n) {
        static const QRegularExpression re("^([A-Za-z]+)(\\d+)$");
        const QRegularExpressionMatch m = re.match(n);
        return m.hasMatch() ? m.captured(1) + " " + m.captured(2) : n;
    };
    QCOMPARE(respace(m_cov.names.first()), QString("MEG 0113"));
    QCOMPARE(respace(m_cov.names.last()),  QString("EEG 060"));

    // Bad channels are carried on the covariance too, and are stripped the same way.
    QCOMPARE(m_cov.bads.size(), 2);
    QVERIFY(m_cov.bads.contains("MEG2443"));
    QVERIFY(m_cov.bads.contains("EEG053"));

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
    const double sum   = d.sum();
    const double d00   = d(0,0);

    const double expTrace = 1.155697251997e-09;
    const double expSum   = 2.229105787152e-13;
    const double expD00   = 2.272355891907e-23;

    QVERIFY2(std::fabs(trace - expTrace) / std::fabs(expTrace) < 1.0e-9,
             qPrintable(QString("trace is %1, mne-python says %2")
                        .arg(trace, 0, 'e', 12).arg(expTrace, 0, 'e', 12)));

    // Summing every entry pins all 366x366 values rather than a chosen few.
    QVERIFY2(std::fabs(sum - expSum) / std::fabs(expSum) < 1.0e-9,
             qPrintable(QString("sum is %1, mne-python says %2")
                        .arg(sum, 0, 'e', 12).arg(expSum, 0, 'e', 12)));

    QVERIFY2(std::fabs(d00 - expD00) / std::fabs(expD00) < 1.0e-9,
             qPrintable(QString("data(0,0) is %1, mne-python says %2")
                        .arg(d00, 0, 'e', 12).arg(expD00, 0, 'e', 12)));
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
}

//=============================================================================================================

void TestFiffCovPython::missingKind_isReported()
{
    // Asking for a kind the file does not contain has to fail rather than
    // return whatever covariance happens to be there first.
    QFile covFile(covPath());
    FiffStream::SPtr stream(new FiffStream(&covFile));
    QVERIFY(stream->open());

    FiffCov cov;
    const bool ok = stream->read_cov(stream->dirtree(), FIFFV_MNE_FMRI_PRIOR_COV, cov);
    QVERIFY2(!ok, "reading an absent covariance kind reported success");

    stream->close();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCovPython)
#include "test_fiff_cov_python.moc"
