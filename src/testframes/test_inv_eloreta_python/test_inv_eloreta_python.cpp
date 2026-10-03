//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_eloreta_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates the eLORETA kernel against mne-python on identical inverse operators.
 *
 * The test builds a 70-source (lh.V1) inverse operator with MNE-CPP, once
 * with loose orientations and once with fixed orientations, and assembles
 * the eLORETA kernel. The reference values are mne-python run on the very
 * inverse operators MNE-CPP writes, so both sides start from the same input:
 *
 *   inv = mne.minimum_norm.read_inverse_operator(<written by this test>)
 *   p = prepare_inverse_operator(inv, nave, 1/9, 'eLORETA',
 *                                method_params=dict(eps=1e-6, max_iter=100,
 *                                                   force_equal=fe))
 *   K = _assemble_kernel(p, None, 'eLORETA', None)[0]
 *   np.abs(K).sum(), np.abs(p['sing']).sum(), p['reginv'].sum()
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/minimum_norm/inv_minimum_norm.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
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
    int m_nave = 0;

private slots:
    void initTestCase();
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

    QFile covFile(data("MEG/sample/sample_audvis-cov.fif"));
    const FiffCov cov(covFile);
    QFile aveFile(data("MEG/sample/sample_audvis-ave.fif"));
    const FiffEvoked evoked(aveFile, 0);
    m_nave = evoked.nave;

    QFile freeFile(fwdPath);
    const MNEForwardSolution freeFwd = MNEForwardSolution(freeFile).pick_regions({v1});
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
