//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_eloreta_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates inverse operator construction and the eLORETA kernel against mne-python.
 *
 * makeInverse_matchesPython builds 70-source (lh.V1) inverse operators with
 * MNE-CPP and compares them with mne-python's own on the same inputs:
 *
 *   inv = make_inverse_operator(info, restrict_forward_to_label(fwd, v1), cov,
 *                               loose=..., depth=..., fixed=...)
 *   p = prepare_inverse_operator(inv, nave, 1/9, 'MNE')
 *   np.abs(p['sing']).sum(), per-source Frobenius norm sum of _assemble_kernel(p, None, 'MNE', None)[0]
 *   np.sum(1 / prepare_inverse_operator(inv, nave, 1/9, 'dSPM')['noisenorm'])
 *
 * The remaining slots build the operators once (loose and fixed) and
 * assemble the eLORETA kernel. Their reference values are mne-python run on
 * the very inverse operators MNE-CPP writes, so both sides start from the
 * same input:
 *
 *   inv = mne.minimum_norm.read_inverse_operator(<written by this test>)
 *   p = prepare_inverse_operator(inv, nave, 1/9, 'eLORETA',
 *                                method_params=dict(eps=1e-6, max_iter=100,
 *                                                   force_equal=fe))
 *   K = _assemble_kernel(p, None, 'eLORETA', None)[0]
 *   np.abs(K).sum(), np.abs(p['sing']).sum(), p['reginv'].sum()
 *
 * The same free operator feeds mne.minimum_norm.estimate_snr (sample evoked,
 * and a ramp of source 10's z column) and apply_inverse_raw on samples
 * 1000-1099 of sample_audvis_trunc_raw.fif. mne-python reads the data with
 * the spaces dropped from the channel names, as MNE-CPP reads all names.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_convenience.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>
#include <fs/fs_label.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestInvEloretaPython
 *
 * @brief Checks the eLORETA kernel against mne-python.
 */
class TestInvEloretaPython : public QObject
{
    Q_OBJECT

private:
    static QString data(const QString& file);

    MNEInverseOperator m_loose;
    MNEInverseOperator m_fixed;
    MNEForwardSolution m_freeFwd;
    FiffEvoked m_evoked;
    FiffInfo m_info;
    FiffCov m_cov;
    FsLabel m_v1;
    int m_nave = 0;

private slots:
    void initTestCase();
    void makeInverse_matchesPython_data();
    void makeInverse_matchesPython();
    void estimateSnr_matchesPython_data();
    void estimateSnr_matchesPython();
    void applyInverseRaw_matchesPython_data();
    void applyInverseRaw_matchesPython();
    void kernel_matchesPython_data();
    void kernel_matchesPython();
    void noiseNorm_matchesPython_data();
    void noiseNorm_matchesPython();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestInvEloretaPython::data(const QString& file)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + file;
}


//=============================================================================================================

void TestInvEloretaPython::initTestCase()
{
    const QString fwdPath = data("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    if (!QFile::exists(fwdPath)) {
        QSKIP("Reference forward solution not found");
    }

    FsLabel v1;
    QVERIFY(FsLabel::read(data("subjects/sample/label/lh.V1.label"), v1));
    m_v1 = v1;

    QFile covFile(data("MEG/sample/sample_audvis-cov.fif"));
    const FiffCov cov(covFile);
    m_cov = cov;
    QFile aveFile(data("MEG/sample/sample_audvis-ave.fif"));
    const FiffEvoked evoked(aveFile, 0);
    m_nave = evoked.nave;
    m_info = evoked.info;
    m_evoked = evoked;

    QFile freeFile(fwdPath);
    const MNEForwardSolution freeFwd = MNEForwardSolution(freeFile).pick_regions({v1});
    m_freeFwd = freeFwd;
    m_loose = MNEInverseOperator::make_inverse_operator(evoked.info, freeFwd, cov, 0.2f, 0.8f, false, true);

    QFile fixedFile(fwdPath);
    const MNEForwardSolution fixedFwd = MNEForwardSolution(fixedFile, true).pick_regions({v1});
    QVERIFY(fixedFwd.isFixedOrient());
    m_fixed = MNEInverseOperator::make_inverse_operator(evoked.info, fixedFwd, cov, 0.0f, 0.8f, true, true);

    QCOMPARE(m_loose.nsource, 70);
    QCOMPARE(m_fixed.nsource, 70);
    // One depth weight per fixed-orientation source, three per free one.
    QCOMPARE(static_cast<int>(m_fixed.source_cov->data.rows()), 70);
    QCOMPARE(static_cast<int>(m_loose.source_cov->data.rows()), 210);
}

//=============================================================================================================

void TestInvEloretaPython::makeInverse_matchesPython_data()
{
    QTest::addColumn<bool>("surfOri");
    QTest::addColumn<float>("loose");
    QTest::addColumn<float>("depth");
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<int>("rows");
    QTest::addColumn<double>("singSum");
    QTest::addColumn<double>("kernelNormSum");
    QTest::addColumn<double>("dspmNormSum");
    QTest::addColumn<double>("nnZSum");

    // The last column is the z sum of the source normals, inv['source_nn'][2::3, 2]
    // for free orientations (the tangential vectors are not unique) and all rows
    // for fixed ones; surface-oriented operators use the patch normals.
    QTest::newRow("loose 0.2") << true << 0.2f << 0.8f << false << 210
                               << 70.71887643595808 << 12172144.218271181 << 8.033782202042283e-08 << -2.3680535720497407;
    QTest::newRow("free") << false << 1.0f << 0.8f << false << 210
                          << 71.9772089624631 << 10247045.448053062 << 6.85092011400161e-08 << 70.0;
    QTest::newRow("free, no depth") << false << 1.0f << 0.0f << false << 210
                                    << 68.06095753009956 << 7684027.38083116 << 4.63638351561198e-08 << 70.0;
    QTest::newRow("fixed from free") << true << 0.0f << 0.8f << true << 70
                                     << 67.61900529555693 << 17118113.357613612 << 1.1110428823216728e-07 << -2.3680535720497407;
}

//=============================================================================================================

void TestInvEloretaPython::makeInverse_matchesPython()
{
    QFETCH(bool, surfOri);
    QFETCH(float, loose);
    QFETCH(float, depth);
    QFETCH(bool, fixed);
    QFETCH(int, rows);
    QFETCH(double, singSum);
    QFETCH(double, kernelNormSum);
    QFETCH(double, dspmNormSum);
    QFETCH(double, nnZSum);

    QFile fwdFile(data("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"));
    const MNEForwardSolution fwd = MNEForwardSolution(fwdFile, false, surfOri).pick_regions({m_v1});
    const MNEInverseOperator inv = MNEInverseOperator::make_inverse_operator(m_info, fwd, m_cov, loose, depth, fixed, true);
    QCOMPARE(inv.nsource, 70);
    QCOMPARE(static_cast<int>(inv.source_nn.rows()), rows);
    double nnZ = 0.0;
    for (int i = rows / 70 - 1; i < rows; i += rows / 70)
        nnZ += inv.source_nn(i, 2);
    QVERIFY2(std::fabs(nnZ - nnZSum) <= 1e-5 * 70, qPrintable(QString("normal z sum %1").arg(nnZ, 0, 'g', 17)));

    InvMinimumNorm mn(inv, 1.0f / 9.0f, "MNE");
    mn.doInverseSetup(m_nave, false);
    const MatrixXd& K = mn.getKernel();
    QCOMPARE(static_cast<int>(K.rows()), rows);
    // The forward and covariance are float32 on disk; 1e-5 is that precision
    // carried through the SVD, a wrong prior or orientation moves these by percent.
    const double gotSing = mn.getPreparedInverseOperator().sing.cwiseAbs().sum();
    QVERIFY2(std::fabs(gotSing - singSum) <= 1e-5 * singSum, qPrintable(QString("sing sum %1").arg(gotSing, 0, 'g', 17)));
    // Sum over sources of the Frobenius norm of their kernel rows, which does not
    // depend on the (non-unique) tangential basis of loose operators.
    const int nOri = rows / 70;
    double gotK = 0.0;
    for (int s = 0; s < 70; ++s)
        gotK += K.middleRows(s * nOri, nOri).norm();
    QVERIFY2(std::fabs(gotK - kernelNormSum) <= 1e-5 * kernelNormSum, qPrintable(QString("kernel norm sum %1").arg(gotK, 0, 'g', 17)));

    const MNEInverseOperator dspm = inv.prepare_inverse_operator(m_nave, 1.0f / 9.0f, true);
    double normSum = 0.0;
    for (int i = 0; i < 70; ++i)
        normSum += 1.0 / dspm.noisenorm.coeff(i, i);
    QVERIFY2(std::fabs(normSum - dspmNormSum) <= 1e-5 * dspmNormSum, qPrintable(QString("sum 1/noisenorm %1").arg(normSum, 0, 'g', 17)));
}

//=============================================================================================================

void TestInvEloretaPython::estimateSnr_matchesPython_data()
{
    QTest::addColumn<double>("amplitude");
    QTest::addColumn<double>("snrSum");
    QTest::addColumn<double>("snr100");
    QTest::addColumn<double>("snr400");
    QTest::addColumn<double>("estSum");
    QTest::addColumn<double>("est100");
    QTest::addColumn<double>("est400");
    QTest::addColumn<int>("zeros");

    // V1 alone cannot explain the auditory response: snr_est never passes the
    // chi^2 test and stops at 1/sqrt(10 * 0.99^1000).
    QTest::newRow("sample evoked") << 0.0 << 7857.236725834051 << 18.832819845852956 << 18.52793729689679
                                   << 20262.11598655369 << 48.12854153575698 << 48.12854153575698 << 0;
    // data = G[:, 32] * 3e-9 * (1..421): the first three samples have snr <= 1. The
    // closest chi^2 decision is 1e-5 relative away from the threshold.
    QTest::newRow("source ramp") << 3e-9 << 22241.730459096696 << 25.288635457990637 << 100.40339424410143
                                 << 2178.910634716406 << 2.123883171237634 << 10.498679616516537 << 3;
}

//=============================================================================================================

void TestInvEloretaPython::estimateSnr_matchesPython()
{
    QFETCH(double, amplitude);
    QFETCH(double, snrSum);
    QFETCH(double, snr100);
    QFETCH(double, snr400);
    QFETCH(double, estSum);
    QFETCH(double, est100);
    QFETCH(double, est400);
    QFETCH(int, zeros);

    FiffEvoked evoked = m_evoked;
    if (amplitude > 0.0) {
        const VectorXd g = m_freeFwd.sol->data.col(32);
        const RowVectorXd ramp = RowVectorXd::LinSpaced(evoked.data.cols(), 1.0, static_cast<double>(evoked.data.cols())) * amplitude;
        evoked.data.setZero();
        for (int i = 0; i < g.size(); ++i)
            evoked.data.row(evoked.info.ch_names.indexOf(m_freeFwd.sol->row_names[i])) = g(i) * ramp;
    } else {
        QTest::ignoreMessage(QtWarningMsg, "[estimateSnr] SNR estimation did not converge.");
    }

    const auto [snr, est] = estimateSnr(evoked, m_loose);
    QCOMPARE(static_cast<int>(snr.size()), 421);
    QCOMPARE(static_cast<int>(est.size()), 421);
    QVERIFY2(std::fabs(snr.sum() - snrSum) <= 1e-6 * snrSum, qPrintable(QString("snr sum %1").arg(snr.sum(), 0, 'g', 17)));
    QVERIFY(std::fabs(snr(100) - snr100) <= 1e-6 * snr100);
    QVERIFY(std::fabs(snr(400) - snr400) <= 1e-6 * snr400);
    // snr_est is 1/sqrt(10 * 0.99^k), so an off-by-one iteration moves a value by 0.5%.
    QVERIFY2(std::fabs(est.sum() - estSum) <= 1e-9 * estSum, qPrintable(QString("est sum %1").arg(est.sum(), 0, 'g', 17)));
    QVERIFY(std::fabs(est(100) - est100) <= 1e-9 * est100);
    QVERIFY(std::fabs(est(400) - est400) <= 1e-9 * est400);
    QCOMPARE(static_cast<int>((est.array() == 0.0).count()), zeros);
}

//=============================================================================================================

void TestInvEloretaPython::applyInverseRaw_matchesPython_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("value10_5");
    QTest::addColumn<double>("value60_50");

    // apply_inverse_raw(raw, inv, 1/9, method, start=1000, stop=1100)
    QTest::newRow("dSPM") << "dSPM" << 5.802343644914849 << 0.000676038579355509 << 0.0005588898740929932;
    QTest::newRow("MNE") << "MNE" << 4.381609183047944e-08 << 4.339177901056041e-12 << 3.5389690616879216e-12;
}

//=============================================================================================================

void TestInvEloretaPython::applyInverseRaw_matchesPython()
{
    QFETCH(QString, method);
    QFETCH(double, absSum);
    QFETCH(double, value10_5);
    QFETCH(double, value60_50);

    QFile rawFile(data("MEG/sample/sample_audvis_trunc_raw.fif"));
    const FiffRawData raw(rawFile);
    const int from = raw.first_samp + 1000;

    const InvSourceEstimate stc = applyInverseRaw(raw, m_loose, 1.0f / 9.0f, method, from, from + 99);
    QCOMPARE(static_cast<int>(stc.data.rows()), 70);
    QCOMPARE(static_cast<int>(stc.data.cols()), 100);
    QCOMPARE(stc.tmin, static_cast<float>(from) / raw.info.sfreq);
    QVERIFY2(std::fabs(stc.data.cwiseAbs().sum() - absSum) <= 1e-5 * absSum,
             qPrintable(QString("abs sum %1").arg(stc.data.cwiseAbs().sum(), 0, 'g', 17)));
    QVERIFY(std::fabs(stc.data(10, 5) - value10_5) <= 1e-5 * value10_5);
    QVERIFY(std::fabs(stc.data(60, 50) - value60_50) <= 1e-5 * value60_50);
}

//=============================================================================================================

void TestInvEloretaPython::kernel_matchesPython_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("forceEqual");
    QTest::addColumn<int>("rows");
    QTest::addColumn<double>("kernelAbsSum");
    QTest::addColumn<double>("singSum");
    QTest::addColumn<double>("reginvSum");

    // MNE rows check the shared preparation; their sing/reginv are not compared (zero marks "skip").
    QTest::newRow("MNE loose") << "MNE" << false << false << 210 << 111383781.99468082 << 0.0 << 0.0;
    QTest::newRow("MNE fixed") << "MNE" << true << false << 70 << 106988626.67247605 << 0.0 << 0.0;
    QTest::newRow("eLORETA loose independent") << "eLORETA" << false << false << 210 << 96341413.3468054 << 68.61269042471012 << 43.58717124957271;
    QTest::newRow("eLORETA loose force equal") << "eLORETA" << false << true << 210 << 95231145.14647257 << 68.64767456445117 << 43.293457695846556;
    QTest::newRow("eLORETA fixed") << "eLORETA" << true << false << 70 << 96407507.6436803 << 64.79162873700938 << 35.47167322985986;
}

//=============================================================================================================

void TestInvEloretaPython::kernel_matchesPython()
{
    QFETCH(QString, method);
    QFETCH(bool, fixed);
    QFETCH(bool, forceEqual);
    QFETCH(int, rows);
    QFETCH(double, kernelAbsSum);
    QFETCH(double, singSum);
    QFETCH(double, reginvSum);

    InvMinimumNorm mn(fixed ? m_fixed : m_loose, 1.0f / 9.0f, method);
    mn.setELoretaOptions(100, 1e-6, forceEqual);
    mn.doInverseSetup(m_nave, false);

    const MatrixXd& K = mn.getKernel();
    QCOMPARE(static_cast<int>(K.rows()), rows);
    QCOMPARE(static_cast<int>(K.cols()), 364);

    // Both sides iterate to eps = 1e-6 on the weights from the same float32
    // operator, so the kernels agree to roughly 1e-6 relative; 1e-4 leaves a
    // margin for the different eigensolvers while a wrong update rule moves
    // the sums by percent.
    const double gotK = K.cwiseAbs().sum();
    QVERIFY2(std::fabs(gotK - kernelAbsSum) <= 1e-4 * kernelAbsSum, qPrintable(QString("|K| sum %1, mne-python %2").arg(gotK, 0, 'g', 17).arg(kernelAbsSum, 0, 'g', 17)));

    if (singSum == 0.0) {
        return;
    }
    MNEInverseOperator& inv = mn.getPreparedInverseOperator();
    const double gotSing = inv.sing.cwiseAbs().sum();
    QVERIFY2(std::fabs(gotSing - singSum) <= 1e-4 * singSum, qPrintable(QString("sing sum %1, mne-python %2").arg(gotSing, 0, 'g', 17).arg(singSum, 0, 'g', 17)));
    const double gotReginv = inv.reginv.sum();
    QVERIFY2(std::fabs(gotReginv - reginvSum) <= 1e-4 * reginvSum, qPrintable(QString("reginv sum %1, mne-python %2").arg(gotReginv, 0, 'g', 17).arg(reginvSum, 0, 'g', 17)));
}

//=============================================================================================================

void TestInvEloretaPython::noiseNorm_matchesPython_data()
{
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("sLORETA");
    QTest::addColumn<double>("normSum");

    // np.sum(1 / prepare_inverse_operator(inv, nave, 1/9, method)['noisenorm'])
    QTest::newRow("dSPM loose") << false << false << 6.850920184879696e-08;
    QTest::newRow("sLORETA loose") << false << true << 1.6188218074164552e-07;
    QTest::newRow("dSPM fixed") << true << false << 1.081406763848875e-07;
    QTest::newRow("sLORETA fixed") << true << true << 2.605112453713598e-07;
}

//=============================================================================================================

void TestInvEloretaPython::noiseNorm_matchesPython()
{
    QFETCH(bool, fixed);
    QFETCH(bool, sLORETA);
    QFETCH(double, normSum);

    const MNEInverseOperator prepared = (fixed ? m_fixed : m_loose).prepare_inverse_operator(m_nave, 1.0f / 9.0f, !sLORETA, sLORETA);

    // One factor per source location, also for fixed orientation.
    QCOMPARE(static_cast<int>(prepared.noisenorm.rows()), 70);
    double sum = 0.0;
    for (int i = 0; i < 70; ++i) {
        const double v = prepared.noisenorm.coeff(i, i);
        QVERIFY(v > 0.0);
        sum += 1.0 / v;
    }
    QVERIFY2(std::fabs(sum - normSum) <= 1e-4 * normSum, qPrintable(QString("sum 1/noisenorm %1, mne-python %2").arg(sum, 0, 'g', 17).arg(normSum, 0, 'g', 17)));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvEloretaPython)
#include "test_inv_eloreta_python.moc"
