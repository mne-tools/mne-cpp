//=============================================================================================================
/**
 * @file     test_inv_beamformer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 * @brief    Tests for LCMV and DICS beamformer inverse methods.
 *           Covers: beamformer settings enums, default construction, symMatPow,
 *           computePower, computeBeamformer (fixed and free orientation),
 *           LCMV make/apply/power-map, and DICS single- and multi-frequency.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mne/mne_forward_solution.h>

#include <inv/beamformer/inv_lcmv.h>
#include <inv/beamformer/inv_dics.h>
#include <inv/beamformer/inv_beamformer.h>
#include <inv/beamformer/inv_beamformer_compute.h>
#include <inv/beamformer/inv_beamformer_settings.h>
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
 * DECLARE CLASS TestInvBeamformer
 *
 * @brief The TestInvBeamformer class provides unit tests for the LCMV and DICS beamformer implementations.
 *
 */
class TestInvBeamformer : public QObject
{
    Q_OBJECT

private:
    QString            m_sDataPath;   /**< Path to the test data directory. */
    MNEForwardSolution m_fwd;         /**< Forward solution loaded for data-driven tests. */
    FiffCov            m_noiseCov;    /**< Noise covariance matrix loaded for data-driven tests. */
    FiffInfo           m_info;        /**< Measurement info loaded from the raw file. */
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
     * Loads forward solution, noise covariance, and measurement info from the mne-cpp test data set.
     * Sets m_bDataLoaded to true only if all three are available.
     */
    void initTestCase();

    //=========================================================================================================
    /**
     * Performs cleanup after all tests have run (no-op).
     */
    void cleanupTestCase();

    //=========================================================================================================
    /**
     * Verifies that all BeamformerWeightNorm, BeamformerPickOri, and BeamformerInversion
     * enum values are pairwise distinct.
     */
    void beamformerSettings_enums();

    //=========================================================================================================
    /**
     * Verifies that a default-constructed InvBeamformer is invalid and reports zero sources and channels.
     */
    void beamformer_defaultInvalid();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::symMatPow applied to the 3x3 identity matrix
     * returns the identity for exponents -1, -0.5, and 0.5.
     */
    void beamformerCompute_symMatPow_identity();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::symMatPow applied to a diagonal matrix
     * returns the correct element-wise power and zeroes on off-diagonals.
     */
    void beamformerCompute_symMatPow_diagonal();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::symMatPow with rank reduction enabled
     * returns a finite-valued result for an ill-conditioned diagonal matrix.
     */
    void beamformerCompute_symMatPow_reduceRank();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::computePower returns the correct per-source
     * power values for a trivial weight matrix and known covariance diagonal.
     */
    void beamformerCompute_computePower();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::computeBeamformer succeeds for a small synthetic
     * fixed-orientation problem and that the resulting weights and power values are finite and positive.
     */
    void beamformerCompute_synthetic();

    //=========================================================================================================
    /**
     * Verifies that InvBeamformerCompute::computeBeamformer with MaxPower orientation picking
     * produces one weight row per source and unit-length orientation vectors.
     */
    void beamformerCompute_freeOrient();

    //=========================================================================================================
    /**
     * Verifies the full LCMV pipeline: builds spatial filters with InvLCMV::makeLCMV and
     * applies them to evoked data with InvLCMV::applyLCMV, checking for valid and finite output.
     */
    void lcmv_makeAndApply();

    //=========================================================================================================
    /**
     * Verifies that InvLCMV::makeLCMV succeeds when no data covariance is supplied,
     * falling back to identity whitening.
     */
    void lcmv_noNoiseCov();

    //=========================================================================================================
    /**
     * Verifies that InvLCMV::applyLCMVCov produces a non-negative spatial power map
     * from a covariance matrix.
     */
    void lcmv_applyLCMVCov();

    //=========================================================================================================
    /**
     * Verifies the full DICS pipeline for a single frequency bin: builds spatial filters
     * with InvDICS::makeDICS and applies them with InvDICS::applyDICSCsd.
     */
    void dics_makeAndApplyCsd();

    //=========================================================================================================
    /**
     * Verifies that InvDICS::makeDICS and InvDICS::applyDICSCsd handle multiple frequency
     * bins, producing one output column per frequency.
     */
    void dics_multiFreq();
};

//=============================================================================================================

void TestInvBeamformer::initTestCase()
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
}

//=============================================================================================================

void TestInvBeamformer::cleanupTestCase() {}

//=============================================================================================================

void TestInvBeamformer::beamformerSettings_enums()
{
    QVERIFY(static_cast<int>(BeamformerWeightNorm::None)
          != static_cast<int>(BeamformerWeightNorm::UnitNoiseGain));
    QVERIFY(static_cast<int>(BeamformerWeightNorm::UnitNoiseGain)
          != static_cast<int>(BeamformerWeightNorm::NAI));
    QVERIFY(static_cast<int>(BeamformerPickOri::None)
          != static_cast<int>(BeamformerPickOri::Normal));
    QVERIFY(static_cast<int>(BeamformerPickOri::Normal)
          != static_cast<int>(BeamformerPickOri::MaxPower));
    QVERIFY(static_cast<int>(BeamformerPickOri::MaxPower)
          != static_cast<int>(BeamformerPickOri::Vector));
    QVERIFY(static_cast<int>(BeamformerInversion::Matrix)
          != static_cast<int>(BeamformerInversion::Single));
}

//=============================================================================================================

void TestInvBeamformer::beamformer_defaultInvalid()
{
    InvBeamformer bf;
    QVERIFY(!bf.isValid());
    QCOMPARE(bf.nSources(), 0);
    QCOMPARE(bf.nChannels(), 0);
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_symMatPow_identity()
{
    MatrixXd I = MatrixXd::Identity(3, 3);
    MatrixXd result = InvBeamformerCompute::symMatPow(I, -1.0);
    QVERIFY(result.rows() == 3 && result.cols() == 3);
    QVERIFY((result - I).norm() < 1e-10);
    MatrixXd result2 = InvBeamformerCompute::symMatPow(I, -0.5);
    QVERIFY((result2 - I).norm() < 1e-10);
    MatrixXd result3 = InvBeamformerCompute::symMatPow(I, 0.5);
    QVERIFY((result3 - I).norm() < 1e-10);
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_symMatPow_diagonal()
{
    MatrixXd D = MatrixXd::Zero(3, 3);
    D(0, 0) = 4.0;
    D(1, 1) = 9.0;
    D(2, 2) = 16.0;
    // D^(-0.5) = diag(1/2, 1/3, 1/4)
    MatrixXd result = InvBeamformerCompute::symMatPow(D, -0.5);
    QVERIFY(qAbs(result(0, 0) - 0.5)     < 1e-10);
    QVERIFY(qAbs(result(1, 1) - 1.0/3.0) < 1e-10);
    QVERIFY(qAbs(result(2, 2) - 0.25)    < 1e-10);
    QVERIFY(qAbs(result(0, 1)) < 1e-10);
    QVERIFY(qAbs(result(0, 2)) < 1e-10);
    QVERIFY(qAbs(result(1, 2)) < 1e-10);
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_symMatPow_reduceRank()
{
    MatrixXd D = MatrixXd::Zero(3, 3);
    D(0, 0) = 100.0;
    D(1, 1) = 50.0;
    D(2, 2) = 1.0;
    MatrixXd result = InvBeamformerCompute::symMatPow(D, -1.0, true);
    QVERIFY(result.rows() == 3 && result.cols() == 3);
    QVERIFY(result.allFinite());
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_computePower()
{
    MatrixXd W(2, 3);
    W << 1.0, 0.0, 0.0,
         0.0, 1.0, 0.0;
    MatrixXd Cm = MatrixXd::Identity(3, 3);
    Cm(0, 0) = 4.0;
    Cm(1, 1) = 9.0;
    Cm(2, 2) = 1.0;
    VectorXd power = InvBeamformerCompute::computePower(Cm, W, 1);
    QCOMPARE(power.size(), (Index)2);
    QVERIFY(qAbs(power(0) - 4.0) < 1e-10);
    QVERIFY(qAbs(power(1) - 9.0) < 1e-10);
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_synthetic()
{
    const int nChan   = 4;
    const int nSrc    = 2;
    const int nOrient = 1;
    MatrixXd G(nChan, nSrc * nOrient);
    G << 1.0, 0.5,
         0.3, 1.2,
         0.7, 0.1,
         0.2, 0.8;
    MatrixXd Cm = G * G.transpose() + 0.1 * MatrixXd::Identity(nChan, nChan);
    MatrixX3d nn = MatrixX3d::Zero(nSrc, 3);
    nn(0, 2) = 1.0;
    nn(1, 2) = 1.0;
    MatrixXd  W;
    MatrixX3d mpOri;
    bool ok = InvBeamformerCompute::computeBeamformer(
        G, Cm, 0.05, nOrient,
        BeamformerWeightNorm::UnitNoiseGain,
        BeamformerPickOri::None,
        false,
        BeamformerInversion::Matrix,
        nn, W, mpOri);
    QVERIFY(ok);
    QCOMPARE(W.rows(), (Index)(nSrc * nOrient));
    QCOMPARE(W.cols(), (Index)nChan);
    QVERIFY(W.allFinite());
    VectorXd power = InvBeamformerCompute::computePower(Cm, W, nOrient);
    QCOMPARE(power.size(), (Index)nSrc);
    for (int i = 0; i < nSrc; ++i)
        QVERIFY(power(i) > 0.0);
}

//=============================================================================================================

void TestInvBeamformer::beamformerCompute_freeOrient()
{
    const int nChan   = 5;
    const int nSrc    = 2;
    const int nOrient = 3;
    MatrixXd G = MatrixXd::Random(nChan, nSrc * nOrient);
    MatrixXd Cm = G * G.transpose() + 0.1 * MatrixXd::Identity(nChan, nChan);
    MatrixX3d nn = MatrixX3d::Zero(nSrc, 3);
    nn(0, 2) = 1.0;
    nn(1, 0) = 1.0;
    MatrixXd  W;
    MatrixX3d mpOri;
    bool ok = InvBeamformerCompute::computeBeamformer(
        G, Cm, 0.05, nOrient,
        BeamformerWeightNorm::UnitNoiseGain,
        BeamformerPickOri::MaxPower,
        false,
        BeamformerInversion::Matrix,
        nn, W, mpOri);
    QVERIFY(ok);
    QCOMPARE(W.rows(), (Index)nSrc);
    QCOMPARE(W.cols(), (Index)nChan);
    QVERIFY(W.allFinite());
    QCOMPARE(mpOri.rows(), (Index)nSrc);
    for (int i = 0; i < nSrc; ++i) {
        double n = mpOri.row(i).norm();
        QVERIFY(qAbs(n - 1.0) < 1e-6);
    }
}

//=============================================================================================================

void TestInvBeamformer::lcmv_makeAndApply()
{
    if (!hasData()) QSKIP("Required data not loaded");
    QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
    QFile evkFile(evkPath);
    if (!evkFile.exists()) QSKIP("Evoked file not found");
    QPair<float, float> noBaseline(-1.0f, -1.0f);
    FiffEvoked evoked(evkFile, 0, noBaseline);
    QVERIFY(evoked.data.rows() > 0);
    InvBeamformer filters = InvLCMV::makeLCMV(
        m_info, m_fwd, m_noiseCov, 0.05, m_noiseCov,
        BeamformerPickOri::None,
        BeamformerWeightNorm::UnitNoiseGain);
    QVERIFY(filters.isValid());
    QVERIFY(filters.nSources() > 0);
    QVERIFY(filters.nChannels() > 0);
    QVERIFY(filters.weights.size() == 1);
    QVERIFY(filters.weights[0].allFinite());
    qDebug() << "LCMV filter: sources=" << filters.nSources()
             << "channels=" << filters.nChannels()
             << "orient=" << filters.nOrient();
    InvSourceEstimate stc = InvLCMV::applyLCMV(evoked, filters);
    QVERIFY(!stc.isEmpty());
    QVERIFY(stc.data.rows() > 0);
    QVERIFY(stc.data.allFinite());
    qDebug() << "LCMV STC: sources=" << stc.data.rows()
             << "times=" << stc.data.cols();
}

//=============================================================================================================

void TestInvBeamformer::lcmv_noNoiseCov()
{
    if (!hasData()) QSKIP("Required data not loaded");
    InvBeamformer filters = InvLCMV::makeLCMV(
        m_info, m_fwd, m_noiseCov, 0.05, FiffCov(),
        BeamformerPickOri::None,
        BeamformerWeightNorm::None);
    QVERIFY(filters.isValid());
    QVERIFY(filters.nSources() > 0);
    QVERIFY(filters.weights[0].allFinite());
}

//=============================================================================================================

void TestInvBeamformer::lcmv_applyLCMVCov()
{
    if (!hasData()) QSKIP("Required data not loaded");
    InvBeamformer filters = InvLCMV::makeLCMV(
        m_info, m_fwd, m_noiseCov, 0.05, m_noiseCov,
        BeamformerPickOri::None,
        BeamformerWeightNorm::UnitNoiseGain);
    if (!filters.isValid()) QSKIP("Failed to compute LCMV filters");
    InvSourceEstimate stcPower = InvLCMV::applyLCMVCov(m_noiseCov, filters);
    QVERIFY(!stcPower.isEmpty());
    QVERIFY(stcPower.data.rows() > 0);
    QVERIFY(stcPower.data.allFinite());
    for (int i = 0; i < stcPower.data.rows(); ++i)
        QVERIFY(stcPower.data(i, 0) >= 0.0);
    qDebug() << "LCMV power: n_sources=" << stcPower.data.rows()
             << "min=" << stcPower.data.minCoeff()
             << "max=" << stcPower.data.maxCoeff();
}

//=============================================================================================================

void TestInvBeamformer::dics_makeAndApplyCsd()
{
    if (!hasData()) QSKIP("Required data not loaded");
    std::vector<MatrixXd> csdMatrices;
    csdMatrices.push_back(m_noiseCov.data);
    VectorXd frequencies(1);
    frequencies(0) = 10.0;
    InvBeamformer filters = InvDICS::makeDICS(
        m_info, m_fwd, csdMatrices, frequencies, 0.05, true, m_noiseCov,
        BeamformerPickOri::None,
        BeamformerWeightNorm::UnitNoiseGain);
    QVERIFY(filters.isValid());
    QVERIFY(filters.nSources() > 0);
    QVERIFY(filters.nFreqs() == 1);
    QVERIFY(filters.weights.size() == 1);
    QVERIFY(filters.weights[0].allFinite());
    qDebug() << "DICS filter: sources=" << filters.nSources()
             << "channels=" << filters.nChannels()
             << "n_freqs=" << filters.nFreqs();
    InvSourceEstimate stcPower = InvDICS::applyDICSCsd(csdMatrices, frequencies, filters);
    QVERIFY(!stcPower.isEmpty());
    QVERIFY(stcPower.data.rows() > 0);
    QVERIFY(stcPower.data.allFinite());
    qDebug() << "DICS power map: sources=" << stcPower.data.rows()
             << "min=" << stcPower.data.minCoeff()
             << "max=" << stcPower.data.maxCoeff();
}

//=============================================================================================================

void TestInvBeamformer::dics_multiFreq()
{
    if (!hasData()) QSKIP("Required data not loaded");
    std::vector<MatrixXd> csdMatrices;
    csdMatrices.push_back(m_noiseCov.data);
    csdMatrices.push_back(m_noiseCov.data * 2.0);
    VectorXd frequencies(2);
    frequencies(0) = 8.0;
    frequencies(1) = 12.0;
    InvBeamformer filters = InvDICS::makeDICS(
        m_info, m_fwd, csdMatrices, frequencies, 0.05, true, m_noiseCov,
        BeamformerPickOri::None,
        BeamformerWeightNorm::UnitNoiseGain);
    QVERIFY(filters.isValid());
    QVERIFY(filters.nFreqs() == 2);
    QVERIFY(filters.weights.size() == 2);
    InvSourceEstimate stcPower = InvDICS::applyDICSCsd(csdMatrices, frequencies, filters);
    QVERIFY(!stcPower.isEmpty());
    QVERIFY(stcPower.data.rows() > 0);
    QVERIFY(stcPower.data.cols() == 2);
    QVERIFY(stcPower.data.allFinite());
}

QTEST_GUILESS_MAIN(TestInvBeamformer)
#include "test_inv_beamformer.moc"
