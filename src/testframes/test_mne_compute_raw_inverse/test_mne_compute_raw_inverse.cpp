//=============================================================================================================
/**
 * @file     test_mne_compute_raw_inverse.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
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
 * @brief    Integration test for the mne_compute_raw_inverse application.
 *           Runs the application executable and verifies its STC output
 *           against mne-python reference data.
 *
 *           Coverage: When built with -DWITH_CODE_COV=ON the application
 *           binary is instrumented by gcov. Running it via QProcess from
 *           this test generates .gcda profiling data that is picked up by
 *           the coverage toolchain, giving line-level coverage for the
 *           application's main.cpp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_sourceestimate.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QProcess>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneComputeRawInverse
 *
 * @brief Integration test that exercises the mne_compute_raw_inverse CLI
 *        application by running it as a subprocess with various flag
 *        combinations. Each test writes STC output to a temporary directory
 *        and verifies the result.
 *
 * Test methods:
 *  1. testDSPM          – full source space dSPM (the standard use case)
 *  2. testSLORETA        – full source space sLORETA
 *  3. testMNE            – full source space MNE
 *  4. testWithBaseline   – dSPM with explicit baseline correction
 *  5. testHelp           – --help flag returns exit code 0
 *  6. testMissingArgs    – missing --in/--inv returns non-zero exit code
 *  7. testMatchesMnePython – compare dSPM output against mne-python STCs
 */
class TestMneComputeRawInverse : public QObject
{
    Q_OBJECT

public:
    TestMneComputeRawInverse();

private slots:
    void initTestCase();
    void testDSPM();
    void testSLORETA();
    void testMNE();
    void testWithBaseline();
    void testHelp();
    void testMissingArgs();
    void testMatchesMnePython();
    void cleanupTestCase();

private:
    /** Locate MNE sample data directory. */
    QString findDataPath();
    /** Locate the mne_compute_raw_inverse executable. */
    QString findExecutable();
    /** Locate python with mne installed. */
    QString findPython();
    /** Locate the generate_reference_stc.py helper. */
    QString findGenerateScript();

    /** Run the application with the given arguments. Returns exit code. */
    int runApp(const QStringList &args, QString &stdoutStr, int timeoutMs = 300000);

    /** Read an STC file and return a MNESourceEstimate. */
    static bool readStc(const QString &path, MNESourceEstimate &stc);

    /**
     * Discover the LH/RH STC file pair produced by an application run.
     *
     * The application names output files as
     *   <outBase>-<sanitized_comment>-lh.stc
     *   <outBase>-<sanitized_comment>-rh.stc
     * where <sanitized_comment> depends on the evoked set's comment field.
     *
     * This helper scans the directory for files matching <prefix>*-lh.stc
     * and <prefix>*-rh.stc and returns the first match.  Because each test
     * writes to its own QTemporaryDir with --set 0, there will be exactly
     * one pair.
     *
     * @param[in]  outBase  The --out value passed to the app (dir + prefix).
     * @param[out] lhPath   Receives the discovered LH path.
     * @param[out] rhPath   Receives the discovered RH path.
     * @return true if both files were found.
     */
    static bool findStcPair(const QString &outBase, QString &lhPath, QString &rhPath);

    QString m_sDataPath;       /**< Path to MNE-sample-data. */
    QString m_sEvokedFile;     /**< sample_audvis-ave.fif */
    QString m_sInvFile;        /**< Inverse operator file. */
    QString m_sExecutable;     /**< Path to mne_compute_raw_inverse binary. */
    bool m_bDataAvailable;     /**< Whether sample data was found. */
    bool m_bExeAvailable;      /**< Whether the executable was found. */
};

//=============================================================================================================

TestMneComputeRawInverse::TestMneComputeRawInverse()
    : m_bDataAvailable(false)
    , m_bExeAvailable(false)
{
}

//=============================================================================================================

QString TestMneComputeRawInverse::findDataPath()
{
    QStringList candidates;

    // 1. mne-cpp-test-data (CI / submodule)
    candidates << QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";

    // 2. MNE sample data (standard mne-python location)
    candidates << QDir::homePath() + "/mne_data/MNE-sample-data";

    // 3. Local resources
    candidates << QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data";

    // 4. Environment variable
    QString envPath = qEnvironmentVariable("MNE_DATA");
    if (!envPath.isEmpty()) {
        candidates.prepend(envPath + "/MNE-sample-data");
        candidates.prepend(envPath);
    }

    for (const QString &path : candidates) {
        QString evokedFile = path + "/MEG/sample/sample_audvis-ave.fif";
        QString invFile = path + "/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
        if (QFile::exists(evokedFile) && QFile::exists(invFile)) {
            return path;
        }
    }
    return QString();
}

//=============================================================================================================

QString TestMneComputeRawInverse::findExecutable()
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();

    // The test binary is in out/<config>/tests/, the app in out/<config>/apps/
#ifdef Q_OS_WIN
    QString exeName = "mne_compute_raw_inverse.exe";
#else
    QString exeName = "mne_compute_raw_inverse";
#endif

    // Relative to test binary
    candidates << appDir + "/../apps/" + exeName;
    candidates << appDir + "/" + exeName;    // Windows: tests and apps share same dir

    // Absolute fallbacks
    candidates << appDir + "/../../Release/apps/" + exeName;
    candidates << QDir::homePath() + "/Programming/mne-cpp/out/Release/apps/" + exeName;

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

//=============================================================================================================

QString TestMneComputeRawInverse::findPython()
{
    QStringList candidates;
    candidates << "python3" << "python" << "python3.14" << "python3.13"
               << "python3.12" << "python3.11" << "python3.10";

    for (const QString &py : candidates) {
        QProcess proc;
        proc.start(py, QStringList() << "-c" << "import mne; print(mne.__version__)");
        proc.waitForFinished(10000);
        if (proc.exitCode() == 0) {
            return py;
        }
    }
    return QString();
}

//=============================================================================================================

QString TestMneComputeRawInverse::findGenerateScript()
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();

    // Relative to test executable (several levels up into source tree)
    candidates << appDir + "/../../../src/testframes/test_compute_raw_inverse/generate_reference_stc.py";
    candidates << appDir + "/../../../../src/testframes/test_compute_raw_inverse/generate_reference_stc.py";

    // Relative to this source file at compile time
    candidates << QString::fromUtf8(__FILE__).replace(
        "test_mne_compute_raw_inverse/test_mne_compute_raw_inverse.cpp",
        "test_compute_raw_inverse/generate_reference_stc.py");

    // Absolute fallback
    candidates << QDir::homePath() + "/Programming/mne-cpp/src/testframes/test_compute_raw_inverse/generate_reference_stc.py";

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

//=============================================================================================================

int TestMneComputeRawInverse::runApp(const QStringList &args, QString &stdoutStr, int timeoutMs)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    // Propagate environment so gcov .gcda files are written properly
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    proc.setProcessEnvironment(env);

    proc.start(m_sExecutable, args);
    bool finished = proc.waitForFinished(timeoutMs);
    stdoutStr = QString::fromUtf8(proc.readAll());

    if (!finished) {
        printf("  WARNING: Process timed out after %d ms\n", timeoutMs);
        proc.kill();
        proc.waitForFinished(5000);
        return -1;
    }
    return proc.exitCode();
}

//=============================================================================================================

bool TestMneComputeRawInverse::readStc(const QString &path, MNESourceEstimate &stc)
{
    QFile f(path);
    return MNESourceEstimate::read(f, stc);
}

//=============================================================================================================

void TestMneComputeRawInverse::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> mne_compute_raw_inverse Integration Test Init >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    m_sDataPath = findDataPath();
    if (m_sDataPath.isEmpty()) {
        QSKIP("MNE sample data not found. Set MNE_DATA or download sample data.");
        return;
    }

    m_sEvokedFile = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    m_sInvFile = m_sDataPath + "/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    m_bDataAvailable = true;

    m_sExecutable = findExecutable();
    if (m_sExecutable.isEmpty()) {
        QSKIP("mne_compute_raw_inverse executable not found. Build the application first.");
        return;
    }
    m_bExeAvailable = true;

    printf("  Data path  : %s\n", m_sDataPath.toUtf8().constData());
    printf("  Executable : %s\n", m_sExecutable.toUtf8().constData());
}

//=============================================================================================================

bool TestMneComputeRawInverse::findStcPair(const QString &outBase, QString &lhPath, QString &rhPath)
{
    QFileInfo fi(outBase);
    QString dir = fi.absolutePath();
    QString prefix = fi.fileName();          // e.g. "dspm_out"

    QDir d(dir);
    QStringList lhFiles = d.entryList(QStringList() << (prefix + "*-lh.stc"), QDir::Files);
    QStringList rhFiles = d.entryList(QStringList() << (prefix + "*-rh.stc"), QDir::Files);

    if (lhFiles.isEmpty() || rhFiles.isEmpty())
        return false;

    lhPath = dir + "/" + lhFiles.first();
    rhPath = dir + "/" + rhFiles.first();
    return true;
}

//=============================================================================================================

void TestMneComputeRawInverse::testDSPM()
{
    if (!m_bDataAvailable || !m_bExeAvailable) QSKIP("Prerequisites missing");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test dSPM (application) >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temp dir");

    QString outBase = tmpDir.path() + "/dspm_out";
    QStringList args;
    args << "--in" << m_sEvokedFile
         << "--inv" << m_sInvFile
         << "--snr" << "3"
         << "--spm"
         << "--set" << "0"
         << "--out" << outBase;

    QString output;
    int exitCode = runApp(args, output);

    printf("  Exit code: %d\n", exitCode);
    QCOMPARE(exitCode, 0);

    // Discover output files (the comment tag depends on the evoked data)
    QString lhPath, rhPath;
    QVERIFY2(findStcPair(outBase, lhPath, rhPath),
             qPrintable("STC pair not found for prefix: " + outBase));

    // Read and validate
    MNESourceEstimate stcLh, stcRh;
    QVERIFY2(readStc(lhPath, stcLh), "Failed to read LH STC");
    QVERIFY2(readStc(rhPath, stcRh), "Failed to read RH STC");

    printf("  LH: %d sources, %d times\n", (int)stcLh.data.rows(), (int)stcLh.data.cols());
    printf("  RH: %d sources, %d times\n", (int)stcRh.data.rows(), (int)stcRh.data.cols());

    // Expected sizes for oct-6 MEG+EEG inverse
    QCOMPARE((int)stcLh.data.rows(), 3732);
    QCOMPARE((int)stcRh.data.rows(), 3766);
    QVERIFY2(stcLh.data.cols() > 400, "Too few time points (LH)");
    QVERIFY2(stcRh.data.cols() > 400, "Too few time points (RH)");

    // dSPM values should be non-negative and finite
    QVERIFY2(stcLh.data.allFinite(), "LH contains NaN/Inf");
    QVERIFY2(stcRh.data.allFinite(), "RH contains NaN/Inf");
    QVERIFY2(stcLh.data.minCoeff() >= 0.0, "LH has negative dSPM values");
    QVERIFY2(stcRh.data.minCoeff() >= 0.0, "RH has negative dSPM values");

    // Peak should be reasonable
    double maxLh = stcLh.data.maxCoeff();
    double maxRh = stcRh.data.maxCoeff();
    printf("  LH max: %.4f, RH max: %.4f\n", maxLh, maxRh);
    QVERIFY2(maxLh > 1.0 && maxLh < 1000.0, "LH peak out of range");
    QVERIFY2(maxRh > 1.0 && maxRh < 1000.0, "RH peak out of range");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test dSPM Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testSLORETA()
{
    if (!m_bDataAvailable || !m_bExeAvailable) QSKIP("Prerequisites missing");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test sLORETA (application) >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temp dir");

    QString outBase = tmpDir.path() + "/sloreta_out";
    QStringList args;
    args << "--in" << m_sEvokedFile
         << "--inv" << m_sInvFile
         << "--snr" << "3"
         << "--sloreta"
         << "--set" << "0"
         << "--out" << outBase;

    QString output;
    int exitCode = runApp(args, output);

    printf("  Exit code: %d\n", exitCode);
    QCOMPARE(exitCode, 0);

    // Discover and verify output files
    QString lhPath, rhPath;
    QVERIFY2(findStcPair(outBase, lhPath, rhPath),
             qPrintable("STC pair not found for prefix: " + outBase));

    MNESourceEstimate stcLh, stcRh;
    QVERIFY2(readStc(lhPath, stcLh), "Failed to read LH STC");
    QVERIFY2(readStc(rhPath, stcRh), "Failed to read RH STC");

    printf("  LH: %d sources, %d times\n", (int)stcLh.data.rows(), (int)stcLh.data.cols());
    printf("  RH: %d sources, %d times\n", (int)stcRh.data.rows(), (int)stcRh.data.cols());

    QCOMPARE((int)stcLh.data.rows(), 3732);
    QCOMPARE((int)stcRh.data.rows(), 3766);
    QVERIFY2(stcLh.data.allFinite(), "LH contains NaN/Inf");
    QVERIFY2(stcRh.data.allFinite(), "RH contains NaN/Inf");
    QVERIFY2(stcLh.data.minCoeff() >= 0.0, "LH has negative sLORETA values");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test sLORETA Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testMNE()
{
    if (!m_bDataAvailable || !m_bExeAvailable) QSKIP("Prerequisites missing");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test MNE (application) >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temp dir");

    QString outBase = tmpDir.path() + "/mne_out";
    QStringList args;
    args << "--in" << m_sEvokedFile
         << "--inv" << m_sInvFile
         << "--snr" << "3"
         << "--set" << "0"
         << "--out" << outBase;

    // Neither --spm nor --sloreta → MNE method
    QString output;
    int exitCode = runApp(args, output);

    printf("  Exit code: %d\n", exitCode);
    QCOMPARE(exitCode, 0);

    QString lhPath, rhPath;
    QVERIFY2(findStcPair(outBase, lhPath, rhPath),
             qPrintable("STC pair not found for prefix: " + outBase));

    MNESourceEstimate stcLh, stcRh;
    QVERIFY2(readStc(lhPath, stcLh), "Failed to read LH STC");
    QVERIFY2(readStc(rhPath, stcRh), "Failed to read RH STC");

    printf("  LH: %d sources, %d times\n", (int)stcLh.data.rows(), (int)stcLh.data.cols());
    printf("  RH: %d sources, %d times\n", (int)stcRh.data.rows(), (int)stcRh.data.cols());

    QCOMPARE((int)stcLh.data.rows(), 3732);
    QCOMPARE((int)stcRh.data.rows(), 3766);
    QVERIFY2(stcLh.data.allFinite(), "LH contains NaN/Inf");
    QVERIFY2(stcRh.data.allFinite(), "RH contains NaN/Inf");

    // MNE values can be negative (current estimate, not noise-normalized)
    double maxVal = std::max(stcLh.data.maxCoeff(), stcRh.data.maxCoeff());
    printf("  Combined max: %.6e\n", maxVal);
    QVERIFY2(maxVal > 0.0, "MNE has no positive signal");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test MNE Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testWithBaseline()
{
    if (!m_bDataAvailable || !m_bExeAvailable) QSKIP("Prerequisites missing");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test dSPM with Baseline (application) >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temp dir");

    // Run without baseline
    QString outNobl = tmpDir.path() + "/nobl";
    {
        QStringList args;
        args << "--in" << m_sEvokedFile
             << "--inv" << m_sInvFile
             << "--snr" << "3"
             << "--spm"
             << "--set" << "0"
             << "--out" << outNobl;
        QString output;
        int exitCode = runApp(args, output);
        QCOMPARE(exitCode, 0);
    }

    // Run with baseline
    QString outBl = tmpDir.path() + "/withbl";
    {
        QStringList args;
        args << "--in" << m_sEvokedFile
             << "--inv" << m_sInvFile
             << "--snr" << "3"
             << "--spm"
             << "--set" << "0"
             << "--bmin" << "-200"
             << "--bmax" << "0"
             << "--out" << outBl;
        QString output;
        int exitCode = runApp(args, output);
        QCOMPARE(exitCode, 0);
    }

    // Discover & read both LH outputs and compare
    QString noblLh, noblRh;
    QVERIFY2(findStcPair(outNobl, noblLh, noblRh),
             qPrintable("STC pair not found for: " + outNobl));
    QString blLh, blRh;
    QVERIFY2(findStcPair(outBl, blLh, blRh),
             qPrintable("STC pair not found for: " + outBl));

    MNESourceEstimate stcNobl, stcBl;
    QVERIFY2(readStc(noblLh, stcNobl), "Failed to read no-baseline LH");
    QVERIFY2(readStc(blLh, stcBl), "Failed to read baseline LH");

    QCOMPARE((int)stcNobl.data.rows(), (int)stcBl.data.rows());
    QCOMPARE((int)stcNobl.data.cols(), (int)stcBl.data.cols());

    // Baseline correction should change the values
    double diff = (stcNobl.data - stcBl.data).norm();
    double norm = stcNobl.data.norm();
    double relDiff = diff / norm;
    printf("  Relative difference (no-baseline vs baseline): %.6e\n", relDiff);
    QVERIFY2(relDiff > 1e-3, "Baseline correction had no effect on dSPM output");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Baseline Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testHelp()
{
    if (!m_bExeAvailable) QSKIP("Executable not found");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test --help >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QString output;
    int exitCode = runApp(QStringList() << "--help", output);

    printf("  Exit code: %d\n", exitCode);
    QCOMPARE(exitCode, 0);

    // Check that help text contains expected keywords
    QVERIFY2(output.contains("--in"), "Help missing --in option");
    QVERIFY2(output.contains("--inv"), "Help missing --inv option");
    QVERIFY2(output.contains("--snr"), "Help missing --snr option");
    QVERIFY2(output.contains("--spm"), "Help missing --spm option");
    QVERIFY2(output.contains("--sloreta"), "Help missing --sloreta option");
    QVERIFY2(output.contains("inverse", Qt::CaseInsensitive), "Help missing 'inverse' keyword");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test --help Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testMissingArgs()
{
    if (!m_bExeAvailable) QSKIP("Executable not found");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test Missing Arguments >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // No arguments at all → should fail
    {
        QString output;
        int exitCode = runApp(QStringList(), output);
        printf("  No args:        exit code = %d\n", exitCode);
        QVERIFY2(exitCode != 0, "Should fail with no arguments");
    }

    // Only --in, missing --inv → should fail
    {
        QString output;
        int exitCode = runApp(QStringList() << "--in" << "dummy.fif", output);
        printf("  Missing --inv:  exit code = %d\n", exitCode);
        QVERIFY2(exitCode != 0, "Should fail without --inv");
    }

    // Only --inv, missing --in → should fail
    {
        QString output;
        int exitCode = runApp(QStringList() << "--inv" << "dummy.fif", output);
        printf("  Missing --in:   exit code = %d\n", exitCode);
        QVERIFY2(exitCode != 0, "Should fail without --in");
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Missing Args Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::testMatchesMnePython()
{
    if (!m_bDataAvailable || !m_bExeAvailable) QSKIP("Prerequisites missing");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test Matches MNE-Python >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // --- 1. Find Python with MNE ---
    QString python = findPython();
    if (python.isEmpty()) {
        QSKIP("Python with MNE not found. Install mne-python to run this test.");
        return;
    }

    QString script = findGenerateScript();
    if (script.isEmpty()) {
        QSKIP("generate_reference_stc.py not found.");
        return;
    }
    printf("  Python: %s\n", python.toUtf8().constData());
    printf("  Script: %s\n", script.toUtf8().constData());

    // --- 2. Create temp directory ---
    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temp dir");

    QString cppDir = tmpDir.path() + "/cpp";
    QString pyDir = tmpDir.path() + "/python";
    QDir().mkpath(cppDir);
    QDir().mkpath(pyDir);

    // --- 3. Run mne_compute_raw_inverse (dSPM, SNR=3, set 0, no baseline) ---
    QString outBase = cppDir + "/result";
    {
        QStringList args;
        args << "--in" << m_sEvokedFile
             << "--inv" << m_sInvFile
             << "--snr" << "3"
             << "--spm"
             << "--set" << "0"
             << "--out" << outBase;
        QString output;
        int exitCode = runApp(args, output);
        QCOMPARE(exitCode, 0);
    }

    QString cppLhPath, cppRhPath;
    QVERIFY2(findStcPair(outBase, cppLhPath, cppRhPath),
             qPrintable("C++ STC pair not found for prefix: " + outBase));

    // --- 4. Generate mne-python reference ---
    printf("  Generating mne-python reference STCs...\n");
    {
        QProcess proc;
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start(python, QStringList() << script << pyDir << m_sDataPath);
        bool finished = proc.waitForFinished(120000);
        QString pyOut = QString::fromUtf8(proc.readAll());
        for (const QString &line : pyOut.split('\n')) {
            printf("    %s\n", line.toUtf8().constData());
        }
        if (!finished || proc.exitCode() != 0) {
            QSKIP("mne-python reference generation failed");
            return;
        }
    }

    // --- 5. Read both sets of STCs ---
    MNESourceEstimate cppLh, cppRh, pyLh, pyRh;
    QVERIFY2(readStc(cppLhPath, cppLh), "Failed to read C++ LH");
    QVERIFY2(readStc(cppRhPath, cppRh), "Failed to read C++ RH");
    QVERIFY2(readStc(pyDir + "/ref-lh.stc", pyLh), "Failed to read Python LH");
    QVERIFY2(readStc(pyDir + "/ref-rh.stc", pyRh), "Failed to read Python RH");

    // --- 6. Compare LH ---
    printf("\n  === Left Hemisphere ===\n");
    printf("  C++:    %d vertices, %d times\n", (int)cppLh.data.rows(), (int)cppLh.data.cols());
    printf("  Python: %d vertices, %d times\n", (int)pyLh.data.rows(), (int)pyLh.data.cols());

    QCOMPARE((int)cppLh.data.rows(), (int)pyLh.data.rows());
    QCOMPARE((int)cppLh.data.cols(), (int)pyLh.data.cols());
    QVERIFY2(cppLh.vertices == pyLh.vertices, "LH vertex indices mismatch");

    VectorXd cppFlatLh = Map<const VectorXd>(cppLh.data.data(), cppLh.data.size());
    VectorXd pyFlatLh = Map<const VectorXd>(pyLh.data.data(), pyLh.data.size());
    VectorXd centCppLh = cppFlatLh.array() - cppFlatLh.mean();
    VectorXd centPyLh = pyFlatLh.array() - pyFlatLh.mean();
    double corrLh = centCppLh.dot(centPyLh) / (centCppLh.norm() * centPyLh.norm());
    double relErrLh = (cppLh.data - pyLh.data).cwiseAbs().maxCoeff() / pyLh.data.cwiseAbs().maxCoeff();

    printf("  Correlation: %.10f\n", corrLh);
    printf("  Rel error:   %.2e\n", relErrLh);

    QVERIFY2(corrLh > 0.999, qPrintable(QString("LH correlation too low: %1").arg(corrLh)));
    QVERIFY2(relErrLh < 0.01, qPrintable(QString("LH relative error too high: %1").arg(relErrLh)));

    // --- 7. Compare RH ---
    printf("\n  === Right Hemisphere ===\n");
    printf("  C++:    %d vertices, %d times\n", (int)cppRh.data.rows(), (int)cppRh.data.cols());
    printf("  Python: %d vertices, %d times\n", (int)pyRh.data.rows(), (int)pyRh.data.cols());

    QCOMPARE((int)cppRh.data.rows(), (int)pyRh.data.rows());
    QCOMPARE((int)cppRh.data.cols(), (int)pyRh.data.cols());
    QVERIFY2(cppRh.vertices == pyRh.vertices, "RH vertex indices mismatch");

    VectorXd cppFlatRh = Map<const VectorXd>(cppRh.data.data(), cppRh.data.size());
    VectorXd pyFlatRh = Map<const VectorXd>(pyRh.data.data(), pyRh.data.size());
    VectorXd centCppRh = cppFlatRh.array() - cppFlatRh.mean();
    VectorXd centPyRh = pyFlatRh.array() - pyFlatRh.mean();
    double corrRh = centCppRh.dot(centPyRh) / (centCppRh.norm() * centPyRh.norm());
    double relErrRh = (cppRh.data - pyRh.data).cwiseAbs().maxCoeff() / pyRh.data.cwiseAbs().maxCoeff();

    printf("  Correlation: %.10f\n", corrRh);
    printf("  Rel error:   %.2e\n", relErrRh);

    QVERIFY2(corrRh > 0.999, qPrintable(QString("RH correlation too low: %1").arg(corrRh)));
    QVERIFY2(relErrRh < 0.01, qPrintable(QString("RH relative error too high: %1").arg(relErrRh)));

    printf("\n  === Summary ===\n");
    printf("  LH: corr=%.10f, relErr=%.2e\n", corrLh, relErrLh);
    printf("  RH: corr=%.10f, relErr=%.2e\n", corrRh, relErrRh);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Matches MNE-Python Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneComputeRawInverse::cleanupTestCase()
{
    printf("<<<<<<<<<<<<<<<<<<<<<<<<< mne_compute_raw_inverse Integration Test Cleanup <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneComputeRawInverse)
#include "test_mne_compute_raw_inverse.moc"
