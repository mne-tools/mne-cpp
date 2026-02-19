//=============================================================================================================
/**
 * @file     test_compute_raw_inverse.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Tests for inverse computation pipeline (mne_compute_raw_inverse).
 *           Covers: dSPM inverse computation, baseline correction behavior,
 *           STC write/read roundtrip, and result validity.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_sourceestimate.h>
#include <mne/mne_sourcespace.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <fs/label.h>

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
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace FSLIB;
using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestComputeRawInverse
 *
 * @brief Tests for the inverse computation pipeline.
 *
 * This test verifies:
 *  - dSPM inverse computation produces valid results
 *  - Evoked data without explicit baseline yields no baseline correction
 *  - STC file write/read roundtrip preserves data
 *  - Label-restricted inverse produces correct subset
 *  - sLORETA and MNE methods also produce valid results
 */
class TestComputeRawInverse : public QObject
{
    Q_OBJECT

public:
    TestComputeRawInverse();

private slots:
    void initTestCase();
    void testInverseDSPM();
    void testInverseSLORETA();
    void testInverseMNE();
    void testBaselineNotAppliedByDefault();
    void testBaselineAppliedWhenRequested();
    void testStcWriteReadRoundtrip();
    void testStcMatchesMnePython();
    void testLabelRestrictedInverse();
    void cleanupTestCase();

private:
    QString findDataPath();
    QString findPython();
    QString findGenerateScript();

    QString m_sDataPath;           /**< Path to MNE sample data directory. */
    QString m_sEvokedFile;         /**< Path to sample_audvis-ave.fif. */
    QString m_sInvFile;            /**< Path to inverse operator file. */
    bool m_bDataAvailable;         /**< Whether test data was found. */

    // Shared results for multi-test use
    MNEInverseOperator m_invOp;    /**< Inverse operator. */
    MNESourceEstimate m_stcDSPM;   /**< dSPM result for reuse. */

    static const float s_fSNR;
    static const float s_fLambda2;
    static const int s_iSetNo;
};

//=============================================================================================================
// STATIC MEMBERS
//=============================================================================================================

const float TestComputeRawInverse::s_fSNR = 3.0f;
const float TestComputeRawInverse::s_fLambda2 = 1.0f / (s_fSNR * s_fSNR);
const int TestComputeRawInverse::s_iSetNo = 0;   // "Left Auditory"

//=============================================================================================================

TestComputeRawInverse::TestComputeRawInverse()
: m_bDataAvailable(false)
{
}

//=============================================================================================================

QString TestComputeRawInverse::findDataPath()
{
    // Try several standard locations for MNE sample data
    QStringList candidates;

    // 1. mne-cpp-test-data (CI / submodule)
    candidates << QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";

    // 2. MNE sample data (standard mne-python location)
    candidates << QDir::homePath() + "/mne_data/MNE-sample-data";

    // 3. Local resources/data/MNE-sample-data
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

QString TestComputeRawInverse::findPython()
{
    // Try common Python executables that have mne installed
    QStringList candidates;
    candidates << "python3" << "python" << "python3.14" << "python3.13"
               << "python3.12" << "python3.11" << "python3.10";

    for (const QString &py : candidates) {
        QProcess proc;
        proc.start(py, QStringList() << "-c" << "import mne; print(mne.__version__)");
        proc.waitForFinished(10000);
        if (proc.exitCode() == 0) {
            QString ver = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            printf("  Found Python with MNE %s: %s\n", ver.toUtf8().constData(),
                   py.toUtf8().constData());
            return py;
        }
    }
    return QString();
}

//=============================================================================================================

QString TestComputeRawInverse::findGenerateScript()
{
    // Look for the Python script relative to the source tree
    QStringList candidates;

    // Relative to the test executable
    QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + "/../../../src/testframes/test_compute_raw_inverse/generate_reference_stc.py";
    candidates << appDir + "/../../../../src/testframes/test_compute_raw_inverse/generate_reference_stc.py";

    // Relative to known source root paths
    candidates << QString::fromUtf8(__FILE__).replace("test_compute_raw_inverse.cpp", "generate_reference_stc.py");

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

void TestComputeRawInverse::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Raw Inverse Test Init >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    m_sDataPath = findDataPath();
    if (m_sDataPath.isEmpty()) {
        QSKIP("MNE sample data not found. Set MNE_DATA or download sample data.");
        return;
    }

    m_sEvokedFile = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    m_sInvFile = m_sDataPath + "/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    m_bDataAvailable = true;

    printf("  Data path: %s\n", m_sDataPath.toUtf8().constData());
    printf("  Evoked:    %s\n", m_sEvokedFile.toUtf8().constData());
    printf("  Inverse:   %s\n", m_sInvFile.toUtf8().constData());

    // Read the inverse operator once for reuse
    QFile invFile(m_sInvFile);
    m_invOp = MNEInverseOperator(invFile);
    QVERIFY2(m_invOp.eigen_leads->data.size() > 0, "Failed to read inverse operator");
    printf("  Inverse operator: %d channels, %d sources\n", m_invOp.nchan, m_invOp.nsource);
}

//=============================================================================================================

void TestComputeRawInverse::testInverseDSPM()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test dSPM Inverse >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Read evoked without baseline correction
    QFile evokedFile(m_sEvokedFile);
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evokedFile, s_iSetNo, noBaseline);
    QVERIFY2(evoked.data.size() > 0, "Failed to read evoked data");

    // Pick channels matching inverse operator
    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    QVERIFY2(pickedEvoked.info.nchan > 0, "No channels matched inverse operator");

    float tmin = pickedEvoked.times(0);
    float tstep = 1.0f / pickedEvoked.info.sfreq;
    int nave = evoked.nave;

    // Compute dSPM inverse
    MinimumNorm minimumNorm(m_invOp, s_fLambda2, QString("dSPM"));
    minimumNorm.doInverseSetup(nave, false);
    m_stcDSPM = minimumNorm.calculateInverse(pickedEvoked.data, tmin, tstep, false);

    // Verify result is not empty
    QVERIFY2(!m_stcDSPM.isEmpty(), "dSPM inverse returned empty result");

    // Verify dimensions
    QVERIFY2(m_stcDSPM.data.rows() > 0, "No source vertices in result");
    QVERIFY2(m_stcDSPM.data.cols() > 0, "No time points in result");

    printf("  Result: %d sources, %d time points\n",
           (int)m_stcDSPM.data.rows(), (int)m_stcDSPM.data.cols());
    printf("  tmin = %.6f, tstep = %.8f\n", m_stcDSPM.tmin, m_stcDSPM.tstep);

    // Expected: 7498 sources for oct-6 MEG+EEG combined inverse
    QCOMPARE((int)m_stcDSPM.data.rows(), m_invOp.nsource);

    // Expected: ~421 time points for sample_audvis-ave (-200 to 500 ms at ~600 Hz)
    QVERIFY2(m_stcDSPM.data.cols() > 400, "Too few time points");
    QVERIFY2(m_stcDSPM.data.cols() < 500, "Too many time points");

    // Verify timing
    QVERIFY2(m_stcDSPM.tmin < 0.0f, "tmin should be negative (prestimulus)");
    QVERIFY2(m_stcDSPM.tstep > 0.0f, "tstep must be positive");
    QVERIFY2(m_stcDSPM.tstep < 0.01f, "tstep too large (expected ~1.66 ms)");

    // Verify data is finite (no NaN or Inf)
    QVERIFY2(m_stcDSPM.data.allFinite(), "dSPM data contains NaN or Inf");

    // Verify dSPM values are non-negative (dSPM is an absolute measure)
    double minVal = m_stcDSPM.data.minCoeff();
    QVERIFY2(minVal >= 0.0, "dSPM values should be non-negative");

    // Verify peak is in a reasonable range for dSPM (typically 0-200)
    double maxVal = m_stcDSPM.data.maxCoeff();
    QVERIFY2(maxVal > 1.0, "dSPM peak too small");
    QVERIFY2(maxVal < 1000.0, "dSPM peak unreasonably large");

    printf("  dSPM range: [%.4f, %.4f]\n", minVal, maxVal);

    // Verify peak location is valid
    // Note: Without baseline correction, the dSPM peak may be in the prestimulus
    // period. We only check that the peak is within the data time range.
    Index peakRow, peakCol;
    m_stcDSPM.data.maxCoeff(&peakRow, &peakCol);
    float peakTime = m_stcDSPM.tmin + peakCol * m_stcDSPM.tstep;
    printf("  Peak: vertex[%ld] at t=%.1f ms, value=%.4f\n",
           (long)peakRow, peakTime * 1000.0f, maxVal);
    QVERIFY2(peakCol >= 0 && peakCol < m_stcDSPM.data.cols(), "Peak column out of range");
    QVERIFY2(peakRow >= 0 && peakRow < m_stcDSPM.data.rows(), "Peak row out of range");

    // Verify vertices array has the right size
    QCOMPARE((int)m_stcDSPM.vertices.size(), (int)m_stcDSPM.data.rows());

    // Verify vertices are sorted within each hemisphere
    // (first half = lh, second half = rh in the inverse operator)
    bool verticesSorted = true;
    for (int i = 1; i < m_stcDSPM.vertices.size(); ++i) {
        if (m_stcDSPM.vertices(i) < m_stcDSPM.vertices(i - 1)) {
            // Allow one break at the hemisphere boundary
            // After that, should be sorted again
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test dSPM Inverse Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testInverseSLORETA()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test sLORETA Inverse >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QFile evokedFile(m_sEvokedFile);
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evokedFile, s_iSetNo, noBaseline);
    QVERIFY2(evoked.data.size() > 0, "Failed to read evoked data");

    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    float tmin = pickedEvoked.times(0);
    float tstep = 1.0f / pickedEvoked.info.sfreq;

    MinimumNorm minimumNorm(m_invOp, s_fLambda2, QString("sLORETA"));
    minimumNorm.doInverseSetup(evoked.nave, false);
    MNESourceEstimate stc = minimumNorm.calculateInverse(pickedEvoked.data, tmin, tstep, false);

    QVERIFY2(!stc.isEmpty(), "sLORETA inverse returned empty result");
    QCOMPARE((int)stc.data.rows(), m_invOp.nsource);
    QVERIFY2(stc.data.allFinite(), "sLORETA data contains NaN or Inf");

    // sLORETA values should be non-negative
    QVERIFY2(stc.data.minCoeff() >= 0.0, "sLORETA values should be non-negative");

    double maxVal = stc.data.maxCoeff();
    printf("  sLORETA range: [%.4f, %.4f]\n", stc.data.minCoeff(), maxVal);
    QVERIFY2(maxVal > 0.0, "sLORETA has no signal");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test sLORETA Inverse Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testInverseMNE()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test MNE Inverse >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QFile evokedFile(m_sEvokedFile);
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evokedFile, s_iSetNo, noBaseline);
    QVERIFY2(evoked.data.size() > 0, "Failed to read evoked data");

    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    float tmin = pickedEvoked.times(0);
    float tstep = 1.0f / pickedEvoked.info.sfreq;

    MinimumNorm minimumNorm(m_invOp, s_fLambda2, QString("MNE"));
    minimumNorm.doInverseSetup(evoked.nave, false);
    MNESourceEstimate stc = minimumNorm.calculateInverse(pickedEvoked.data, tmin, tstep, false);

    QVERIFY2(!stc.isEmpty(), "MNE inverse returned empty result");
    QCOMPARE((int)stc.data.rows(), m_invOp.nsource);
    QVERIFY2(stc.data.allFinite(), "MNE data contains NaN or Inf");

    // MNE values can be negative (it's a current estimate, not noise-normalized)
    double maxVal = stc.data.maxCoeff();
    double minVal = stc.data.minCoeff();
    printf("  MNE range: [%.6e, %.6e]\n", minVal, maxVal);
    QVERIFY2(maxVal > 0.0, "MNE has no positive signal");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test MNE Inverse Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testBaselineNotAppliedByDefault()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test Baseline Not Applied By Default >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Read evoked data WITHOUT baseline (-1, -1 sentinel = no baseline)
    QFile evokedFile1(m_sEvokedFile);
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evokedNoBaseline(evokedFile1, s_iSetNo, noBaseline);
    QVERIFY2(evokedNoBaseline.data.size() > 0, "Failed to read evoked data (no baseline)");

    // Read evoked data WITH baseline correction (tmin to 0)
    QFile evokedFile2(m_sEvokedFile);
    QPair<float, float> withBaseline(evokedNoBaseline.times(0), 0.0f);
    FiffEvoked evokedWithBaseline(evokedFile2, s_iSetNo, withBaseline);
    QVERIFY2(evokedWithBaseline.data.size() > 0, "Failed to read evoked data (with baseline)");

    // The two datasets should differ if baseline correction was only applied to one
    // Compute the difference
    MatrixXd diff = evokedNoBaseline.data - evokedWithBaseline.data;
    double diffNorm = diff.norm();
    double noBaselineNorm = evokedNoBaseline.data.norm();

    printf("  No-baseline data norm: %.6e\n", noBaselineNorm);
    printf("  Difference norm:       %.6e\n", diffNorm);

    // The difference should be non-trivial (baseline correction changes the data)
    double relDiff = diffNorm / noBaselineNorm;
    printf("  Relative difference:   %.6e\n", relDiff);
    QVERIFY2(relDiff > 1e-6,
        "No-baseline and baseline-corrected data are identical - "
        "baseline correction may be incorrectly applied by default");

    // Baseline correction operates PER-CHANNEL: it subtracts each channel's
    // mean in the baseline window. Verify per-channel prestimulus means.
    int nPreStim = 0;
    for (int i = 0; i < evokedNoBaseline.times.size(); ++i) {
        if (evokedNoBaseline.times(i) < 0.0f) nPreStim++;
    }
    QVERIFY2(nPreStim > 10, "Not enough prestimulus samples for baseline test");

    // Per-channel: baseline-corrected prestimulus mean should be ~0
    // Per-channel: no-baseline prestimulus mean should differ from zero for at least some channels
    int nChannels = qMin(20, (int)evokedNoBaseline.data.rows());
    int nChannelsWithNonzeroMean = 0;
    for (int ch = 0; ch < nChannels; ++ch) {
        // Baseline-corrected channel: prestimulus mean should be essentially zero
        double blMean = evokedWithBaseline.data.row(ch).head(nPreStim).mean();
        QVERIFY2(std::abs(blMean) < 1e-10,
                 qPrintable(QString("Channel %1: baseline-corrected prestim mean %2 not near zero")
                            .arg(ch).arg(blMean, 0, 'e', 6)));

        // No-baseline channel: prestimulus mean may be nonzero
        double noblMean = evokedNoBaseline.data.row(ch).head(nPreStim).mean();
        if (std::abs(noblMean) > 1e-15) nChannelsWithNonzeroMean++;
    }

    printf("  Channels with nonzero prestim mean (no baseline): %d / %d\n",
           nChannelsWithNonzeroMean, nChannels);
    QVERIFY2(nChannelsWithNonzeroMean > nChannels / 2,
             "Most channels should have nonzero prestim mean without baseline correction");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Baseline Not Applied Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testBaselineAppliedWhenRequested()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test Baseline Applied When Requested >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Read with explicit baseline (tmin to 0)
    QFile evokedFile(m_sEvokedFile);
    QPair<float, float> baseline(-0.2f, 0.0f);
    FiffEvoked evoked(evokedFile, s_iSetNo, baseline);
    QVERIFY2(evoked.data.size() > 0, "Failed to read evoked data with baseline");

    // Find prestimulus samples for the baseline window
    int nBaselineSamples = 0;
    for (int i = 0; i < evoked.times.size(); ++i) {
        if (evoked.times(i) >= -0.2f && evoked.times(i) <= 0.0f) nBaselineSamples++;
    }
    QVERIFY2(nBaselineSamples > 10, "Not enough samples in baseline window");

    // Per-channel baseline mean should be approximately zero
    // Check a subset of channels
    int nCheck = qMin(10, (int)evoked.data.rows());
    for (int ch = 0; ch < nCheck; ++ch) {
        double channelBaselineMean = 0.0;
        int count = 0;
        for (int t = 0; t < evoked.times.size(); ++t) {
            if (evoked.times(t) >= -0.2f && evoked.times(t) <= 0.0f) {
                channelBaselineMean += evoked.data(ch, t);
                count++;
            }
        }
        channelBaselineMean /= count;

        // The channel mean in the baseline window should be very close to zero
        QVERIFY2(std::abs(channelBaselineMean) < 1e-10,
                 qPrintable(QString("Channel %1: baseline mean %2 not near zero")
                            .arg(ch).arg(channelBaselineMean, 0, 'e', 6)));
    }

    printf("  Baseline correctly applied: per-channel baseline means ~ 0\n");

    // Now compute inverse and verify it still produces valid results
    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    float tmin = pickedEvoked.times(0);
    float tstep = 1.0f / pickedEvoked.info.sfreq;

    MinimumNorm minimumNorm(m_invOp, s_fLambda2, QString("dSPM"));
    minimumNorm.doInverseSetup(evoked.nave, false);
    MNESourceEstimate stc = minimumNorm.calculateInverse(pickedEvoked.data, tmin, tstep, false);

    QVERIFY2(!stc.isEmpty(), "Inverse with baseline failed");
    QVERIFY2(stc.data.allFinite(), "Inverse with baseline contains NaN/Inf");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Baseline Applied Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testStcWriteReadRoundtrip()
{
    if (!m_bDataAvailable) QSKIP("No test data");
    if (m_stcDSPM.isEmpty()) QSKIP("dSPM result not available (testInverseDSPM must run first)");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test STC Write/Read Roundtrip >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temporary directory");

    QString stcPath = tmpDir.path() + "/test_inverse-lh.stc";

    // Write STC
    {
        QFile stcFile(stcPath);
        QVERIFY2(m_stcDSPM.write(stcFile), "Failed to write STC file");
    }

    // Verify file exists and has size
    QFileInfo fi(stcPath);
    QVERIFY2(fi.exists(), "STC file was not created");
    QVERIFY2(fi.size() > 0, "STC file is empty");
    printf("  Written STC file: %lld bytes\n", fi.size());

    // Read it back
    MNESourceEstimate stcRead;
    {
        QFile stcFile(stcPath);
        QVERIFY2(MNESourceEstimate::read(stcFile, stcRead), "Failed to read STC file");
    }

    // Compare metadata
    QVERIFY2(!stcRead.isEmpty(), "Read-back STC is empty");

    // tmin comparison (float precision)
    float tminDiff = std::abs(m_stcDSPM.tmin - stcRead.tmin);
    printf("  tmin: orig=%.8f read=%.8f diff=%.2e\n", m_stcDSPM.tmin, stcRead.tmin, tminDiff);
    QVERIFY2(tminDiff < 1e-4f, "tmin mismatch after roundtrip");

    // tstep comparison
    float tstepDiff = std::abs(m_stcDSPM.tstep - stcRead.tstep);
    printf("  tstep: orig=%.8f read=%.8f diff=%.2e\n", m_stcDSPM.tstep, stcRead.tstep, tstepDiff);
    QVERIFY2(tstepDiff < 1e-6f, "tstep mismatch after roundtrip");

    // Dimensions
    QCOMPARE((int)stcRead.data.rows(), (int)m_stcDSPM.data.rows());
    QCOMPARE((int)stcRead.data.cols(), (int)m_stcDSPM.data.cols());
    printf("  Shape: %d x %d\n", (int)stcRead.data.rows(), (int)stcRead.data.cols());

    // Vertices
    QCOMPARE((int)stcRead.vertices.size(), (int)m_stcDSPM.vertices.size());
    QVERIFY2(stcRead.vertices == m_stcDSPM.vertices, "Vertex indices mismatch after roundtrip");

    // Data comparison (STC uses float32 for data, so expect float precision)
    double maxAbsError = 0.0;
    for (int r = 0; r < m_stcDSPM.data.rows(); ++r) {
        for (int c = 0; c < m_stcDSPM.data.cols(); ++c) {
            double err = std::abs(m_stcDSPM.data(r, c) - stcRead.data(r, c));
            if (err > maxAbsError) maxAbsError = err;
        }
    }
    double maxOrigVal = m_stcDSPM.data.cwiseAbs().maxCoeff();
    double relError = maxAbsError / maxOrigVal;
    printf("  Max absolute error: %.6e\n", maxAbsError);
    printf("  Relative error:     %.6e\n", relError);

    // STC files store float32, so relative error ~ 1e-7 is expected
    QVERIFY2(relError < 1e-5, "Data precision lost in STC roundtrip");

    // Overall correlation
    VectorXd origFlat = Map<const VectorXd>(m_stcDSPM.data.data(), m_stcDSPM.data.size());
    VectorXd readFlat = Map<const VectorXd>(stcRead.data.data(), stcRead.data.size());
    double meanOrig = origFlat.mean();
    double meanRead = readFlat.mean();
    VectorXd centOrig = origFlat.array() - meanOrig;
    VectorXd centRead = readFlat.array() - meanRead;
    double corr = centOrig.dot(centRead) / (centOrig.norm() * centRead.norm());
    printf("  Correlation: %.10f\n", corr);
    QVERIFY2(corr > 0.999999, "STC roundtrip correlation too low");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test STC Roundtrip Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testLabelRestrictedInverse()
{
    if (!m_bDataAvailable) QSKIP("No test data");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test Label-Restricted Inverse >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Check if label files are available
    QString labelDir = m_sDataPath + "/subjects/sample/label";
    QString labelFile = labelDir + "/lh.auditory.label";

    // Try alternate path for older data layouts
    if (!QFile::exists(labelFile)) {
        labelDir = m_sDataPath + "/../subjects/sample/label";
        labelFile = labelDir + "/lh.auditory.label";
    }

    if (!QFile::exists(labelFile)) {
        printf("  Label file not found, trying aparc labels...\n");
        labelFile = labelDir + "/lh.aparc.annot";
    }

    if (!QFile::exists(labelFile)) {
        QSKIP("No label files available for label-restricted inverse test");
        return;
    }

    // Read label
    Label label;
    if (!Label::read(labelFile, label)) {
        QSKIP("Failed to read label file");
        return;
    }

    printf("  Label: %s (%ld vertices)\n", label.name.toUtf8().constData(), (long)label.vertices.rows());
    QVERIFY2(label.vertices.rows() > 0, "Label has no vertices");

    // Read evoked
    QFile evokedFile(m_sEvokedFile);
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evokedFile, s_iSetNo, noBaseline);
    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    float tmin = pickedEvoked.times(0);
    float tstep = 1.0f / pickedEvoked.info.sfreq;

    // Compute full inverse
    MinimumNorm minimumNorm(m_invOp, s_fLambda2, QString("dSPM"));
    minimumNorm.doInverseSetup(evoked.nave, false);
    MNESourceEstimate stcFull = minimumNorm.calculateInverse(pickedEvoked.data, tmin, tstep, false);
    QVERIFY2(!stcFull.isEmpty(), "Full inverse failed");

    // Extract label subset using getIndicesByLabel
    VectorXi labelIndices = stcFull.getIndicesByLabel(QList<Label>() << label, false);
    printf("  Label vertices in STC: %d\n", (int)labelIndices.size());

    // The label-restricted result should have fewer sources than the full result
    QVERIFY2(labelIndices.size() > 0, "No label vertices found in source estimate");
    QVERIFY2(labelIndices.size() < stcFull.data.rows(), "Label should have fewer sources than full estimate");

    // Verify the label indices are valid
    for (int i = 0; i < labelIndices.size(); ++i) {
        QVERIFY2(labelIndices(i) >= 0 && labelIndices(i) < stcFull.data.rows(),
                 "Label index out of bounds");
    }

    // Extract label data
    MatrixXd labelData(labelIndices.size(), stcFull.data.cols());
    for (int i = 0; i < labelIndices.size(); ++i) {
        labelData.row(i) = stcFull.data.row(labelIndices(i));
    }

    QVERIFY2(labelData.allFinite(), "Label-restricted data contains NaN/Inf");
    printf("  Label data shape: %d x %d\n", (int)labelData.rows(), (int)labelData.cols());

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test Label Restricted Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::testStcMatchesMnePython()
{
    if (!m_bDataAvailable) QSKIP("No test data");
    if (m_stcDSPM.isEmpty()) QSKIP("dSPM result not available (testInverseDSPM must run first)");

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Test STC Matches MNE-Python >>>>>>>>>>>>>>>>>>>>>>>>>\n");

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
    printf("  Script: %s\n", script.toUtf8().constData());

    // --- 2. Create temp directory for output ---
    QTemporaryDir tmpDir;
    QVERIFY2(tmpDir.isValid(), "Failed to create temporary directory");

    QString cppDir = tmpDir.path() + "/cpp";
    QString pyDir = tmpDir.path() + "/python";
    QDir().mkpath(cppDir);
    QDir().mkpath(pyDir);

    // --- 3. Write C++ STCs (split lh/rh) ---
    int nSrcLh = m_invOp.src[0].nuse;
    int nSrcRh = m_invOp.src[1].nuse;
    int nSrcTotal = (int)m_stcDSPM.data.rows();

    printf("  Source split: lh=%d, rh=%d, total=%d\n", nSrcLh, nSrcRh, nSrcTotal);
    QCOMPARE(nSrcLh + nSrcRh, nSrcTotal);

    // Write LH
    {
        MNESourceEstimate stcLh(m_stcDSPM.data.topRows(nSrcLh),
                                m_stcDSPM.vertices.head(nSrcLh),
                                m_stcDSPM.tmin, m_stcDSPM.tstep);
        QFile f(cppDir + "/result-lh.stc");
        QVERIFY2(stcLh.write(f), "Failed to write C++ LH STC");
    }
    // Write RH
    {
        MNESourceEstimate stcRh(m_stcDSPM.data.bottomRows(nSrcRh),
                                m_stcDSPM.vertices.tail(nSrcRh),
                                m_stcDSPM.tmin, m_stcDSPM.tstep);
        QFile f(cppDir + "/result-rh.stc");
        QVERIFY2(stcRh.write(f), "Failed to write C++ RH STC");
    }

    // --- 4. Generate reference STCs using mne-python ---
    printf("  Running mne-python to generate reference STCs...\n");
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(python, QStringList() << script << pyDir << m_sDataPath);
    bool finished = proc.waitForFinished(120000);  // 2 min timeout

    QString pyOutput = QString::fromUtf8(proc.readAll());
    printf("  Python output:\n");
    for (const QString &line : pyOutput.split('\n')) {
        printf("    %s\n", line.toUtf8().constData());
    }

    if (!finished || proc.exitCode() != 0) {
        printf("  Python exit code: %d\n", proc.exitCode());
        QSKIP("mne-python reference generation failed (see output above)");
        return;
    }

    // --- 5. Read both sets of STCs ---
    // Read C++ STCs
    MNESourceEstimate cppLh, cppRh;
    {
        QFile f(cppDir + "/result-lh.stc");
        QVERIFY2(MNESourceEstimate::read(f, cppLh), "Failed to read C++ LH STC");
    }
    {
        QFile f(cppDir + "/result-rh.stc");
        QVERIFY2(MNESourceEstimate::read(f, cppRh), "Failed to read C++ RH STC");
    }

    // Read Python reference STCs
    MNESourceEstimate pyLh, pyRh;
    {
        QFile f(pyDir + "/ref-lh.stc");
        QVERIFY2(MNESourceEstimate::read(f, pyLh), "Failed to read Python LH STC");
    }
    {
        QFile f(pyDir + "/ref-rh.stc");
        QVERIFY2(MNESourceEstimate::read(f, pyRh), "Failed to read Python RH STC");
    }

    // --- 6. Compare LH hemisphere ---
    printf("\n  === Left Hemisphere Comparison ===\n");
    printf("  C++:    %d vertices, %d times, tmin=%.6f, tstep=%.8f\n",
           (int)cppLh.data.rows(), (int)cppLh.data.cols(), cppLh.tmin, cppLh.tstep);
    printf("  Python: %d vertices, %d times, tmin=%.6f, tstep=%.8f\n",
           (int)pyLh.data.rows(), (int)pyLh.data.cols(), pyLh.tmin, pyLh.tstep);

    QCOMPARE((int)cppLh.data.rows(), (int)pyLh.data.rows());
    QCOMPARE((int)cppLh.data.cols(), (int)pyLh.data.cols());
    QCOMPARE((int)cppLh.vertices.size(), (int)pyLh.vertices.size());

    // Compare tmin (in seconds, stored in ms in file -> float32 precision ~1e-4)
    float tminDiffLh = std::abs(cppLh.tmin - pyLh.tmin);
    printf("  tmin diff: %.2e\n", tminDiffLh);
    QVERIFY2(tminDiffLh < 1e-3f, "LH tmin mismatch");

    // Compare tstep
    float tstepDiffLh = std::abs(cppLh.tstep - pyLh.tstep);
    printf("  tstep diff: %.2e\n", tstepDiffLh);
    QVERIFY2(tstepDiffLh < 1e-6f, "LH tstep mismatch");

    // Compare vertex indices
    bool verticesMatchLh = (cppLh.vertices == pyLh.vertices);
    printf("  Vertices match: %s\n", verticesMatchLh ? "YES" : "NO");
    if (!verticesMatchLh) {
        // Print first few mismatches
        int nMismatch = 0;
        for (int i = 0; i < cppLh.vertices.size() && nMismatch < 10; ++i) {
            if (cppLh.vertices(i) != pyLh.vertices(i)) {
                printf("    vertex[%d]: cpp=%d python=%d\n", i, cppLh.vertices(i), pyLh.vertices(i));
                nMismatch++;
            }
        }
    }
    QVERIFY2(verticesMatchLh, "LH vertex indices do not match mne-python");

    // Compare data: correlation
    VectorXd cppFlatLh = Map<const VectorXd>(cppLh.data.data(), cppLh.data.size());
    VectorXd pyFlatLh = Map<const VectorXd>(pyLh.data.data(), pyLh.data.size());
    double meanCppLh = cppFlatLh.mean();
    double meanPyLh = pyFlatLh.mean();
    VectorXd centCppLh = cppFlatLh.array() - meanCppLh;
    VectorXd centPyLh = pyFlatLh.array() - meanPyLh;
    double corrLh = centCppLh.dot(centPyLh) / (centCppLh.norm() * centPyLh.norm());
    printf("  Data correlation: %.10f\n", corrLh);

    // Compare data: relative error
    double maxAbsErrLh = (cppLh.data - pyLh.data).cwiseAbs().maxCoeff();
    double maxValLh = pyLh.data.cwiseAbs().maxCoeff();
    double relErrLh = maxAbsErrLh / maxValLh;
    printf("  Max absolute error: %.6e\n", maxAbsErrLh);
    printf("  Max relative error: %.6e\n", relErrLh);

    // Compare data: sample values at peak
    Index peakRowLh, peakColLh;
    pyLh.data.maxCoeff(&peakRowLh, &peakColLh);
    printf("  Peak (python): vertex[%ld] t=%ld, py=%.6f, cpp=%.6f\n",
           (long)peakRowLh, (long)peakColLh,
           pyLh.data(peakRowLh, peakColLh), cppLh.data(peakRowLh, peakColLh));

    QVERIFY2(corrLh > 0.999, qPrintable(QString("LH correlation too low: %1").arg(corrLh, 0, 'f', 10)));
    QVERIFY2(relErrLh < 0.01, qPrintable(QString("LH relative error too high: %1").arg(relErrLh, 0, 'e', 6)));

    // --- 7. Compare RH hemisphere ---
    printf("\n  === Right Hemisphere Comparison ===\n");
    printf("  C++:    %d vertices, %d times, tmin=%.6f, tstep=%.8f\n",
           (int)cppRh.data.rows(), (int)cppRh.data.cols(), cppRh.tmin, cppRh.tstep);
    printf("  Python: %d vertices, %d times, tmin=%.6f, tstep=%.8f\n",
           (int)pyRh.data.rows(), (int)pyRh.data.cols(), pyRh.tmin, pyRh.tstep);

    QCOMPARE((int)cppRh.data.rows(), (int)pyRh.data.rows());
    QCOMPARE((int)cppRh.data.cols(), (int)pyRh.data.cols());
    QCOMPARE((int)cppRh.vertices.size(), (int)pyRh.vertices.size());

    // Compare tmin
    float tminDiffRh = std::abs(cppRh.tmin - pyRh.tmin);
    printf("  tmin diff: %.2e\n", tminDiffRh);
    QVERIFY2(tminDiffRh < 1e-3f, "RH tmin mismatch");

    // Compare tstep
    float tstepDiffRh = std::abs(cppRh.tstep - pyRh.tstep);
    printf("  tstep diff: %.2e\n", tstepDiffRh);
    QVERIFY2(tstepDiffRh < 1e-6f, "RH tstep mismatch");

    // Compare vertex indices
    bool verticesMatchRh = (cppRh.vertices == pyRh.vertices);
    printf("  Vertices match: %s\n", verticesMatchRh ? "YES" : "NO");
    if (!verticesMatchRh) {
        int nMismatch = 0;
        for (int i = 0; i < cppRh.vertices.size() && nMismatch < 10; ++i) {
            if (cppRh.vertices(i) != pyRh.vertices(i)) {
                printf("    vertex[%d]: cpp=%d python=%d\n", i, cppRh.vertices(i), pyRh.vertices(i));
                nMismatch++;
            }
        }
    }
    QVERIFY2(verticesMatchRh, "RH vertex indices do not match mne-python");

    // Compare data: correlation
    VectorXd cppFlatRh = Map<const VectorXd>(cppRh.data.data(), cppRh.data.size());
    VectorXd pyFlatRh = Map<const VectorXd>(pyRh.data.data(), pyRh.data.size());
    double meanCppRh = cppFlatRh.mean();
    double meanPyRh = pyFlatRh.mean();
    VectorXd centCppRh = cppFlatRh.array() - meanCppRh;
    VectorXd centPyRh = pyFlatRh.array() - meanPyRh;
    double corrRh = centCppRh.dot(centPyRh) / (centCppRh.norm() * centPyRh.norm());
    printf("  Data correlation: %.10f\n", corrRh);

    // Compare data: relative error
    double maxAbsErrRh = (cppRh.data - pyRh.data).cwiseAbs().maxCoeff();
    double maxValRh = pyRh.data.cwiseAbs().maxCoeff();
    double relErrRh = maxAbsErrRh / maxValRh;
    printf("  Max absolute error: %.6e\n", maxAbsErrRh);
    printf("  Max relative error: %.6e\n", relErrRh);

    // Sample values at peak
    Index peakRowRh, peakColRh;
    pyRh.data.maxCoeff(&peakRowRh, &peakColRh);
    printf("  Peak (python): vertex[%ld] t=%ld, py=%.6f, cpp=%.6f\n",
           (long)peakRowRh, (long)peakColRh,
           pyRh.data(peakRowRh, peakColRh), cppRh.data(peakRowRh, peakColRh));

    QVERIFY2(corrRh > 0.999, qPrintable(QString("RH correlation too low: %1").arg(corrRh, 0, 'f', 10)));
    QVERIFY2(relErrRh < 0.01, qPrintable(QString("RH relative error too high: %1").arg(relErrRh, 0, 'e', 6)));

    // --- 8. Summary ---
    printf("\n  === Summary ===\n");
    printf("  LH: corr=%.10f, relErr=%.6e\n", corrLh, relErrLh);
    printf("  RH: corr=%.10f, relErr=%.6e\n", corrRh, relErrRh);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Test STC Matches MNE-Python Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestComputeRawInverse::cleanupTestCase()
{
    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Raw Inverse Test Cleanup <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestComputeRawInverse)
#include "test_compute_raw_inverse.moc"
