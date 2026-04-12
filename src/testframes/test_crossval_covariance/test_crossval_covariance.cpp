//=============================================================================================================
/**
 * @file     test_crossval_covariance.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Exemplary cross-validation test: MNE-CPP Ledoit-Wolf covariance
 *           vs. scikit-learn reference via MNE-Python.
 *
 *           This test demonstrates the canonical pattern for comparing a
 *           C++ algorithm output against a Python reference implementation.
 *           It can serve as a template for future cross-validation tests.
 *
 *  Pattern overview:
 *  -----------------------------------------------------------------
 *    1. Both sides load the same raw FIFF file (sample data).
 *    2. C++  computes the result using the library under test.
 *    3. Python computes the reference via numpy.savetxt → temp file.
 *    4. C++  reads the file-based result with PythonTestHelper::readMatrix.
 *    5. Element-wise comparison within a numeric tolerance.
 *  -----------------------------------------------------------------
 *
 *  When Python / MNE-Python / scikit-learn are not available the test
 *  gracefully QSKIPs rather than failing CI.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <sts/sts_cov_estimators.h>
#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <utils/python_test_helper.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestCrossvalCovariance
 *
 * @brief Cross-validates MNE-CPP Ledoit-Wolf covariance against scikit-learn.
 *
 * This test is designed as a *template* — copy it and swap out:
 *   - The C++ algorithm call
 *   - The Python reference snippet
 *   - The comparison tolerance
 * to create new cross-validation tests for any algorithm.
 */
class TestCrossvalCovariance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    //--- Cross-validation tests (require Python + mne + sklearn) ---
    void testLedoitWolfCovarianceVsPython();
    void testLedoitWolfShrinkageVsPython();

    //--- Standalone smoke test (always runs) ---
    void testLedoitWolfSmokeTest();

    void cleanupTestCase();

private:
    /** @brief MNE-CPP side: load raw data and compute Ledoit-Wolf covariance. */
    void computeCppCovariance();

    /** @brief Python side: compute reference Ledoit-Wolf covariance via scikit-learn. */
    void computePythonCovariance();

    // Shared test state
    QString          m_sDataPath;
    QString          m_sRawFile;
    QTemporaryDir    m_tempDir;

    PythonTestHelper m_pyHelper;
    bool             m_bPythonAvailable = false;

    // C++ results
    MatrixXd m_cppCov;
    double   m_cppAlpha = -1.0;

    // Python results
    MatrixXd m_pyCov;
    double   m_pyAlpha  = -1.0;

    // Number of MEG channels used (for sanity checks)
    int      m_nChannels = 0;
};

//=============================================================================================================

void TestCrossvalCovariance::initTestCase()
{
    // --- Paths ---
    m_sDataPath = PythonTestHelper::testDataPath();
    m_sRawFile  = m_sDataPath + "MEG/sample/sample_audvis_trunc_raw.fif";

    QVERIFY2(QFile::exists(m_sRawFile),
             qPrintable(QString("Test data not found: %1").arg(m_sRawFile)));
    QVERIFY(m_tempDir.isValid());

    // --- Python availability ---
    // Report status explicitly so test output always shows what was found.
    bool hasPython = m_pyHelper.isPythonAvailable();
    bool hasMne    = hasPython && m_pyHelper.hasPackage("mne");
    bool hasSklearn = hasPython && m_pyHelper.hasPackage("sklearn");
    m_bPythonAvailable = hasMne && hasSklearn;

    qDebug() << "Python detection: python3=" << hasPython
             << " mne=" << hasMne
             << " sklearn=" << hasSklearn
             << " MNE_REQUIRE_PYTHON=" << PythonTestHelper::isPythonRequired();

    // --- C++ covariance (always computed) ---
    computeCppCovariance();

    // --- Python covariance (if available) ---
    if (m_bPythonAvailable) {
        computePythonCovariance();
    }
}

//=============================================================================================================

void TestCrossvalCovariance::computeCppCovariance()
{
    // 1. Load raw FIFF file
    QFile file(m_sRawFile);
    FiffRawData raw(file);

    // 2. Pick MEG channels only
    RowVectorXi picks = raw.info.pick_types(true  /*meg*/,
                                             false /*eeg*/,
                                             false /*stim*/);
    m_nChannels = picks.size();
    QVERIFY2(m_nChannels > 0, "No MEG channels found in raw file");

    // 3. Read ~1 second of data (enough for stable covariance, fast in CI)
    fiff_int_t from = raw.first_samp;
    fiff_int_t to   = qMin(raw.first_samp + static_cast<fiff_int_t>(raw.info.sfreq),
                           raw.last_samp);

    MatrixXd data, times;
    raw.read_raw_segment(data, times, from, to, picks);

    QCOMPARE(data.rows(), static_cast<int>(m_nChannels));
    QVERIFY(data.cols() > 0);

    // 4. Zero-mean the data (required by Ledoit-Wolf)
    data.colwise() -= data.rowwise().mean();

    // 5. Compute Ledoit-Wolf covariance
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);
    m_cppCov   = cov;
    m_cppAlpha = alpha;

    QCOMPARE(m_cppCov.rows(), m_nChannels);
    QCOMPARE(m_cppCov.cols(), m_nChannels);
    QVERIFY(m_cppAlpha >= 0.0 && m_cppAlpha <= 1.0);

    qDebug() << "C++ Ledoit-Wolf: channels =" << m_nChannels
             << ", samples =" << data.cols()
             << ", shrinkage =" << m_cppAlpha;
}

//=============================================================================================================

void TestCrossvalCovariance::computePythonCovariance()
{
    QString covFilePath = m_tempDir.filePath("py_cov.txt");

    //
    // Python snippet: use the same data reading approach as C++ side
    // and compute Ledoit-Wolf via sklearn.covariance.ledoit_wolf().
    //
    // Key: we pick MEG channels, read ~1 second from first_samp, zero-mean,
    // and call ledoit_wolf(data.T) to get (covariance, shrinkage).
    //
    // The covariance matrix is saved to a file via np.savetxt for
    // full double-precision round-trip. The shrinkage scalar is printed
    // to stdout for evalDouble-style capture.
    //
    QString pyCode = QString(
        "import mne\n"
        "import numpy as np\n"
        "from sklearn.covariance import ledoit_wolf\n"
        "\n"
        "raw = mne.io.read_raw_fif('%1', preload=True, verbose=False)\n"
        "picks = mne.pick_types(raw.info, meg=True, eeg=False, stim=False)\n"
        "sfreq = raw.info['sfreq']\n"
        "first = raw.first_samp\n"
        "last  = min(first + int(sfreq), raw.last_samp)\n"
        "\n"
        "# Extract the same segment as C++: first_samp .. first_samp + sfreq\n"
        "data = raw.get_data(picks=picks, start=0, stop=last - first + 1)\n"
        "\n"
        "# Zero-mean (row-wise, matching C++ convention)\n"
        "data = data - data.mean(axis=1, keepdims=True)\n"
        "\n"
        "# Ledoit-Wolf (sklearn expects samples x features)\n"
        "cov, shrinkage = ledoit_wolf(data.T)\n"
        "\n"
        "# Save covariance matrix to file (full precision)\n"
        "np.savetxt('%2', cov, fmt='%%.17e')\n"
        "\n"
        "# Print shrinkage to stdout\n"
        "print(f'{shrinkage:.17e}')\n"
    ).arg(m_sRawFile, covFilePath);

    // Run Python and capture both the file and stdout
    PythonRunnerResult result = m_pyHelper.eval(pyCode, 120000);
    QVERIFY2(result.success,
             qPrintable(QString("Python failed (exit %1): %2")
                       .arg(result.exitCode).arg(result.stdErr)));

    // Parse shrinkage from stdout
    bool alphaOk = false;
    m_pyAlpha = result.stdOut.trimmed().toDouble(&alphaOk);
    QVERIFY2(alphaOk,
             qPrintable(QString("Could not parse shrinkage: '%1'").arg(result.stdOut.trimmed())));

    // Read covariance matrix from file
    bool covOk = false;
    m_pyCov = PythonTestHelper::readMatrix(covFilePath, &covOk);
    QVERIFY2(covOk, "Failed to read Python covariance matrix from file");

    QCOMPARE(m_pyCov.rows(), m_nChannels);
    QCOMPARE(m_pyCov.cols(), m_nChannels);

    qDebug() << "Python Ledoit-Wolf: shrinkage =" << m_pyAlpha;
}

//=============================================================================================================

void TestCrossvalCovariance::testLedoitWolfCovarianceVsPython()
{
    GUARD_PYTHON(m_bPythonAvailable,
                 "Python/MNE-Python/scikit-learn not available");

    //
    // Compare covariance matrices element-wise.
    //
    // Tolerance notes:
    //   - Both implementations follow the Ledoit & Wolf (2004) analytic formula.
    //   - Numerical differences arise from floating-point summation order and
    //     potential differences in how the sample covariance is normalized
    //     (1/n vs 1/(n-1)). sklearn uses 1/(n-1) by default (assume_centered=False).
    //   - We use a relative tolerance on the Frobenius norm.
    //
    double frobCpp = m_cppCov.norm();
    double frobDiff = (m_cppCov - m_pyCov).norm();
    double relDiff = frobDiff / frobCpp;

    qDebug() << "Covariance Frobenius norm (C++):" << frobCpp
             << "  diff:" << frobDiff
             << "  relative:" << relDiff;

    // Relative tolerance: 1% — generous enough for minor implementation
    // differences but tight enough to catch real bugs.
    const double kRelTol = 0.01;
    QVERIFY2(relDiff < kRelTol,
             qPrintable(QString("Covariance matrices differ by %1 relative "
                                "(tolerance: %2)").arg(relDiff).arg(kRelTol)));
}

//=============================================================================================================

void TestCrossvalCovariance::testLedoitWolfShrinkageVsPython()
{
    GUARD_PYTHON(m_bPythonAvailable,
                 "Python/MNE-Python/scikit-learn not available");

    //
    // Compare shrinkage coefficients.
    //
    double absDiff = qAbs(m_cppAlpha - m_pyAlpha);

    qDebug() << "Shrinkage C++:" << m_cppAlpha
             << " Python:" << m_pyAlpha
             << " |diff|:" << absDiff;

    // Absolute tolerance: 0.01 — both should agree on the shrinkage
    // coefficient within 1 percentage point.
    const double kAbsTol = 0.01;
    QVERIFY2(absDiff < kAbsTol,
             qPrintable(QString("Shrinkage coefficients differ by %1 "
                                "(C++=%2, Python=%3, tolerance=%4)")
                       .arg(absDiff).arg(m_cppAlpha).arg(m_pyAlpha).arg(kAbsTol)));
}

//=============================================================================================================

void TestCrossvalCovariance::testLedoitWolfSmokeTest()
{
    //
    // This test always runs (no Python dependency).
    // Validates basic properties of the C++ computation.
    //
    QVERIFY(m_cppCov.rows() == m_nChannels);
    QVERIFY(m_cppCov.cols() == m_nChannels);

    // Symmetric
    double asymmetry = (m_cppCov - m_cppCov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-12,
             qPrintable(QString("Covariance not symmetric: asym=%1").arg(asymmetry)));

    // Positive diagonal
    for (int i = 0; i < m_nChannels; ++i) {
        QVERIFY2(m_cppCov(i, i) > 0.0,
                 qPrintable(QString("Negative diagonal at index %1: %2")
                           .arg(i).arg(m_cppCov(i, i))));
    }

    // Shrinkage in [0, 1]
    QVERIFY(m_cppAlpha >= 0.0);
    QVERIFY(m_cppAlpha <= 1.0);
}

//=============================================================================================================

void TestCrossvalCovariance::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestCrossvalCovariance)
#include "test_crossval_covariance.moc"
