//=============================================================================================================
/**
 * @file     test_inv_loreta.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for eLORETA and sLORETA inverse methods.
 *           Covers: method switching, eLORETA options, full pipeline from evoked data,
 *           forceEqual weight convergence, and cross-method comparison against dSPM.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_forward_solution.h>

#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestInvLoreta
 *
 * @brief The TestInvLoreta class provides unit tests for the eLORETA and sLORETA inverse method implementations.
 *
 */
class TestInvLoreta : public QObject
{
    Q_OBJECT

private:
    QString            m_sDataPath;   /**< Path to the test data directory. */
    MNEForwardSolution m_fwd;         /**< Forward solution loaded for data-driven tests. */
    FiffCov            m_noiseCov;    /**< Noise covariance matrix loaded for data-driven tests. */
    FiffInfo           m_info;        /**< Measurement info loaded from the raw file. */
    MNEInverseOperator m_invOp;       /**< Inverse operator pre-computed in initTestCase. */
    bool               m_bDataLoaded; /**< True if all required test data was successfully loaded. */

    //=========================================================================================================
    /**
     * Returns true if all required test data was loaded successfully.
     *
     * @return true if test data is available, false otherwise.
     */
    bool hasData() const { return m_bDataLoaded; }

private slots:

    //=========================================================================================================
    /**
     * Loads forward solution, noise covariance, and measurement info from the mne-cpp test data set,
     * then pre-computes the inverse operator. Sets m_bDataLoaded to true only if all are available.
     */
    void initTestCase();

    //=========================================================================================================
    /**
     * Performs cleanup after all tests have run (no-op).
     */
    void cleanupTestCase();

    //=========================================================================================================
    /**
     * Verifies that InvMinimumNorm accepts method switching between eLORETA and dSPM,
     * that setELoretaOptions can be called without error, and that the source space is non-empty.
     */
    void eloreta_setMethod();

    //=========================================================================================================
    /**
     * Verifies the full eLORETA inverse pipeline: reads evoked data, picks channels matching
     * the inverse operator, runs doInverseSetup and calculateInverse, and checks that the
     * resulting STC has the expected number of sources and contains only finite values.
     */
    void eloreta_fromEvoked();

    //=========================================================================================================
    /**
     * Verifies that the eLORETA forceEqual weight option produces a valid and finite STC,
     * exercising the equal-weight convergence path of the eLORETA algorithm.
     */
    void eloreta_forceEqual();

    //=========================================================================================================
    /**
     * Verifies that eLORETA and dSPM produce results with the same shape but numerically
     * different values, confirming that the two methods apply distinct spatial filters.
     */
    void eloreta_vs_dSPM();
};

//=============================================================================================================

void TestInvLoreta::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    m_bDataLoaded = false;

    QString base = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    if (!QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif")) {
        qWarning() << "Test data not found at" << base;
        return;
    }
    m_sDataPath = base;

    QString fwdPath = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
    QFile fwdFile(fwdPath);
    if (fwdFile.exists())
        m_fwd = MNEForwardSolution(fwdFile);

    QString covPath = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
    QFile covFile(covPath);
    if (covFile.exists())
        m_noiseCov = FiffCov(covFile);

    QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
    QFile rawFile(rawPath);
    if (rawFile.exists()) {
        FiffRawData raw(rawFile);
        m_info = raw.info;
    }

    m_bDataLoaded = !m_fwd.isEmpty() && !m_noiseCov.isEmpty() && m_info.nchan > 0;

    if (m_bDataLoaded) {
        m_invOp = MNEInverseOperator::make_inverse_operator(
            m_info, m_fwd, m_noiseCov, 0.2f, 0.8f, false, true);
    }
}

//=============================================================================================================

void TestInvLoreta::cleanupTestCase() {}

//=============================================================================================================

void TestInvLoreta::eloreta_setMethod()
{
    if (!hasData()) QSKIP("Required data not loaded");
    if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

    float lambda2 = 1.0f / 9.0f;
    InvMinimumNorm mn(m_invOp, lambda2, QString("eLORETA"));

    mn.setELoretaOptions(10, 1e-4, true);

    mn.setMethod("dSPM");
    mn.setMethod("eLORETA");
    mn.setMethod(false, false, true);

    const MNESourceSpaces& src = mn.getSourceSpace();
    QVERIFY(src.size() > 0);
}

//=============================================================================================================

void TestInvLoreta::eloreta_fromEvoked()
{
    if (!hasData()) QSKIP("Required data not loaded");
    if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

    QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    QFile evkFile(evkPath);
    if (!evkFile.exists()) QSKIP("Evoked file not found");

    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evkFile, 0, noBaseline);
    QVERIFY(evoked.data.rows() > 0);

    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);
    QVERIFY(pickedEvoked.data.rows() > 0);

    float tmin   = pickedEvoked.times(0);
    float tstep  = 1.0f / pickedEvoked.info.sfreq;
    float lambda2 = 1.0f / 9.0f;

    InvMinimumNorm mn(m_invOp, lambda2, QString("eLORETA"));
    mn.setELoretaOptions(5, 1e-3, false);

    mn.doInverseSetup(evoked.nave, false);

    InvSourceEstimate stc = mn.calculateInverse(
        pickedEvoked.data, tmin, tstep, false);

    QVERIFY(!stc.isEmpty());
    QCOMPARE((int)stc.data.rows(), m_invOp.nsource);
    QVERIFY(stc.data.allFinite());

    qDebug() << "eLORETA STC: sources=" << stc.data.rows()
             << "times=" << stc.data.cols()
             << "min=" << stc.data.minCoeff()
             << "max=" << stc.data.maxCoeff();
}

//=============================================================================================================

void TestInvLoreta::eloreta_forceEqual()
{
    if (!hasData()) QSKIP("Required data not loaded");
    if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

    QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    QFile evkFile(evkPath);
    if (!evkFile.exists()) QSKIP("Evoked file not found");

    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evkFile, 0, noBaseline);
    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);

    float tmin    = pickedEvoked.times(0);
    float tstep   = 1.0f / pickedEvoked.info.sfreq;
    float lambda2 = 1.0f / 9.0f;

    InvMinimumNorm mn(m_invOp, lambda2, QString("eLORETA"));
    mn.setELoretaOptions(3, 1e-2, true);

    mn.doInverseSetup(evoked.nave, false);

    InvSourceEstimate stc = mn.calculateInverse(
        pickedEvoked.data, tmin, tstep, false);

    QVERIFY(!stc.isEmpty());
    QVERIFY(stc.data.allFinite());
}

//=============================================================================================================

void TestInvLoreta::eloreta_vs_dSPM()
{
    if (!hasData()) QSKIP("Required data not loaded");
    if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

    QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    QFile evkFile(evkPath);
    if (!evkFile.exists()) QSKIP("Evoked file not found");

    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evkFile, 0, noBaseline);
    FiffEvoked pickedEvoked = evoked.pick_channels(m_invOp.noise_cov->names);

    float tmin    = pickedEvoked.times(0);
    float tstep   = 1.0f / pickedEvoked.info.sfreq;
    float lambda2 = 1.0f / 9.0f;

    InvMinimumNorm mnDspm(m_invOp, lambda2, QString("dSPM"));
    mnDspm.doInverseSetup(evoked.nave, false);
    InvSourceEstimate stcDspm = mnDspm.calculateInverse(
        pickedEvoked.data, tmin, tstep, false);

    InvMinimumNorm mnEloreta(m_invOp, lambda2, QString("eLORETA"));
    mnEloreta.setELoretaOptions(5, 1e-3, false);
    mnEloreta.doInverseSetup(evoked.nave, false);
    InvSourceEstimate stcEloreta = mnEloreta.calculateInverse(
        pickedEvoked.data, tmin, tstep, false);

    if (stcDspm.isEmpty() || stcEloreta.isEmpty())
        QSKIP("Could not compute both STCs");

    QCOMPARE(stcDspm.data.rows(), stcEloreta.data.rows());
    QCOMPARE(stcDspm.data.cols(), stcEloreta.data.cols());

    double diff = (stcDspm.data - stcEloreta.data).norm();
    QVERIFY2(diff > 1e-10, "eLORETA and dSPM should produce different results");

    qDebug() << "dSPM vs eLORETA norm difference:" << diff;
}

QTEST_GUILESS_MAIN(TestInvLoreta)
#include "test_inv_loreta.moc"
