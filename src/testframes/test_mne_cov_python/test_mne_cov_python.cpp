//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_cov_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates MNECovMatrix reading, projection, regularisation and whitening against mne-python.
 *
 * MNECovMatrix is the noise model of the dipole fit and the MNE-C style
 * inverse code; whatever it gets wrong shows up in the whitened data. The
 * expected values are squared norms of whitened data, x^T C^+ x, from
 * mne-python / numpy on the same files:
 *
 *   ev = read_evokeds('sample_audvis-ave.fif', 0, proj=False).pick('meg', exclude='bads')
 *   P  = make_projector(ev.info['projs'], ev.ch_names)[0]
 *   x  = P @ ev.data[:, 200]
 *   C  = P @ cov[ev.ch_names] @ P.T
 *   x @ pinv(C, rcond=1e-10) @ x                                    # rank 302
 *   C[t, t] += reg * mean(diag(C)[t]) for t in (mag, grad); x @ solve(C, x)
 *   sum(x ** 2 / diag(C))                                           # diagonal only
 *   sum(y ** 2 / diag(adhoc)), y = ev.data[:, 200]                  # grad 5e-13, mag 20e-15
 *
 * The precomputed-eigen and diagonal files are written by FiffCov and read
 * back through MNECovMatrix::read, so the reader's handling of
 * FIFF_MNE_COV_EIGENVALUES/EIGENVECTORS and FIFF_MNE_COV_DIAG is exercised too.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_cov_matrix.h>
#include <mne/mne_proj_op.h>

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace
{

constexpr double kMahalanobisPinv = 1462.0502702959539;
constexpr double kMahalanobisReg010 = 756.3966212888006;
constexpr double kMahalanobisReg005 = 930.9044770727442;
constexpr double kMahalanobisDiagonalOnly = 639.001463278667;
constexpr double kMahalanobisAdHoc = 36555.704660402494;

} // namespace

//=============================================================================================================
/**
 * Cross validates MNECovMatrix against mne-python.
 */
class TestMneCovPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readsHeader();
    void whitensProjected_data();
    void whitensProjected();
    void readsPrecomputedEigen();
    void readsDiagonal();
    void rejectsBadInput();

private:
    std::unique_ptr<MNECovMatrix> projectedNoise() const;
    double whitenedNorm2(const MNECovMatrix& cov, const VectorXf& data) const;

    QString m_covPath;
    QTemporaryDir m_dir;
    FiffEvoked m_evoked;
    QStringList m_names;
    VectorXf m_raw;
    VectorXf m_projected;
    std::unique_ptr<MNEProjOp> m_proj;
};

//=============================================================================================================

void TestMneCovPython::initTestCase()
{
    const QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample";
    m_covPath = base + "/sample_audvis-cov.fif";
    QFile aveFile(base + "/sample_audvis-ave.fif");
    QVERIFY2(QFile::exists(m_covPath) && aveFile.exists(), "test data missing");
    QVERIFY(m_dir.isValid());

    FiffEvoked evoked(aveFile, 0, QPair<float, float>(-1.0f, -1.0f), false);
    for (const QString& name : evoked.info.ch_names) {
        if (name.startsWith("MEG") && !evoked.info.bads.contains(name))
            m_names << name;
    }
    QCOMPARE(m_names.size(), 305);
    m_evoked = evoked.pick_channels(m_names);
    QVERIFY(std::abs(m_evoked.times[200] - 0.1331968087722586) < 1e-6);
    m_raw = m_evoked.data.col(200).cast<float>();

    QVERIFY(MNEProjOp::makeProjection({aveFile.fileName()}, m_evoked.info.chs, static_cast<int>(m_names.size()), m_proj));
    QVERIFY(m_proj);
    QCOMPARE(m_proj->assign_channels(m_names, static_cast<int>(m_names.size())), 0);
    QCOMPARE(m_proj->make_proj(), 0);
    QCOMPARE(m_proj->nvec, 3);
    m_projected = m_raw;
    QCOMPARE(m_proj->project_vector(m_projected, true), 0);
    QVERIFY(std::abs(m_projected.cast<double>().norm() - 8.34902414341237e-11) < 1e-6 * 8.34902414341237e-11);
}

//=============================================================================================================

std::unique_ptr<MNECovMatrix> TestMneCovPython::projectedNoise() const
{
    auto cov = MNECovMatrix::read(m_covPath, FIFFV_MNE_NOISE_COV);
    if (!cov)
        return nullptr;
    auto picked = cov->pick_chs_omit(m_names, static_cast<int>(m_names.size()), false, m_evoked.info.chs);
    if (!picked || m_proj->apply_cov(picked.get()) != 0 || picked->classify_channels(m_evoked.info.chs, static_cast<int>(m_names.size())) != 0)
        return nullptr;
    return picked;
}

double TestMneCovPython::whitenedNorm2(const MNECovMatrix& cov, const VectorXf& data) const
{
    VectorXf in = data;
    VectorXf out(data.size());
    if (cov.whiten_vector(in, out, static_cast<int>(data.size())) != 0)
        return -1.0;
    return out.cast<double>().squaredNorm();
}

//=============================================================================================================

void TestMneCovPython::readsHeader()
{
    auto cov = MNECovMatrix::read(m_covPath, FIFFV_MNE_NOISE_COV);
    QVERIFY(cov);
    QCOMPARE(cov->ncov, 366);
    QCOMPARE(cov->nfree, 15972);
    // Name lists are read without spaces (see test_fiff_cov_python).
    QCOMPARE(cov->bads, QStringList({"MEG2443", "EEG053"}));
    QVERIFY(!cov->is_diag());
    QVERIFY(cov->proj);
    QCOMPARE(cov->proj->nitems, 4);

    // np.tril(cov.data).sum(): the packed lower triangle.
    QVERIFY(std::abs(cov->cov.sum() - 5.77960081287843e-10) < 1e-9 * 5.77960081287843e-10);
    double trace = 0.0;
    for (int k = 0; k < cov->ncov; ++k)
        trace += cov->cov[MNECovMatrix::lt_packed_index(k, k)];
    QVERIFY(std::abs(trace - 1.1556972519969712e-09) < 1e-9 * 1.1556972519969712e-09);
}

//=============================================================================================================

void TestMneCovPython::whitensProjected_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<double>("reg");
    QTest::addColumn<double>("expected");

    QTest::newRow("full rank 302") << "eigen" << 0.0 << kMahalanobisPinv;
    QTest::newRow("regularised 0.1") << "eigen" << 0.1 << kMahalanobisReg010;
    QTest::newRow("regularised 0.05") << "eigen" << 0.05 << kMahalanobisReg005;
    QTest::newRow("diagonal only") << "diag" << 0.0 << kMahalanobisDiagonalOnly;
}

void TestMneCovPython::whitensProjected()
{
    QFETCH(QString, mode);
    QFETCH(double, reg);
    QFETCH(double, expected);

    auto cov = projectedNoise();
    QVERIFY(cov);
    if (reg > 0.0)
        cov->regularize(Vector3f(static_cast<float>(reg), static_cast<float>(reg), 0.0f));
    if (mode == "diag")
        cov->revert_to_diag();
    QCOMPARE(cov->decompose_eigen(), 0);
    if (mode == "eigen" && reg == 0.0)
        QCOMPARE(cov->nzero, 3);

    const double norm2 = whitenedNorm2(*cov, m_projected);
    QVERIFY2(std::abs(norm2 - expected) < 1e-4 * expected,
             qPrintable(QStringLiteral("x^T C^+ x = %1, mne-python %2").arg(norm2, 0, 'g', 12).arg(expected, 0, 'g', 12)));
}

//=============================================================================================================

void TestMneCovPython::readsPrecomputedEigen()
{
    QFile covFile(m_covPath);
    FiffCov cov(covFile);
    FiffCov prepared = cov.pick_channels(m_names).prepare_noise_cov(m_evoked.info, m_names);
    QCOMPARE(static_cast<int>(prepared.eig.size()), 305);
    prepared.projs.clear();
    prepared.bads.clear();
    const QString path = m_dir.filePath("eig-cov.fif");
    QVERIFY(prepared.save(path));

    auto read = MNECovMatrix::read(path, prepared.kind);
    QVERIFY(read);
    QCOMPARE(static_cast<int>(read->lambda.size()), 305);
    QCOMPARE(static_cast<int>(read->eigen.rows()), 305);
    QCOMPARE(read->nzero, 3);
    QCOMPARE(read->decompose_eigen(), 0);

    const double norm2 = whitenedNorm2(*read, m_projected);
    QVERIFY2(std::abs(norm2 - kMahalanobisPinv) < 1e-4 * kMahalanobisPinv,
             qPrintable(QStringLiteral("x^T C^+ x = %1").arg(norm2, 0, 'g', 12)));

    // The three null-space components drop out, so unprojected data whitens the same.
    const double rawNorm2 = whitenedNorm2(*read, m_raw);
    QVERIFY2(std::abs(rawNorm2 - 1462.0502794767388) < 1e-4 * 1462.0502794767388,
             qPrintable(QStringLiteral("unprojected x^T C^+ x = %1").arg(rawNorm2, 0, 'g', 12)));
}

//=============================================================================================================

void TestMneCovPython::readsDiagonal()
{
    FiffCov adHoc;
    adHoc.kind = FIFFV_MNE_NOISE_COV;
    adHoc.diag = true;
    adHoc.dim = static_cast<int>(m_names.size());
    adHoc.names = m_names;
    adHoc.nfree = 1;
    adHoc.data.resize(adHoc.dim, 1);
    for (int k = 0; k < adHoc.dim; ++k) {
        const double sd = m_evoked.info.chs[k].unit == FIFF_UNIT_T ? 20e-15 : 5e-13;
        adHoc.data(k, 0) = sd * sd;
    }
    const QString path = m_dir.filePath("diag-cov.fif");
    QVERIFY(adHoc.save(path));

    auto read = MNECovMatrix::read(path, FIFFV_MNE_NOISE_COV);
    QVERIFY(read);
    QVERIFY(read->is_diag());
    QCOMPARE(read->ncov, 305);
    QCOMPARE(read->decompose_eigen(), 0);

    const double norm2 = whitenedNorm2(*read, m_raw);
    QVERIFY2(std::abs(norm2 - kMahalanobisAdHoc) < 1e-4 * kMahalanobisAdHoc,
             qPrintable(QStringLiteral("x^T C^-1 x = %1").arg(norm2, 0, 'g', 12)));

    auto dup = read->dup();
    QVERIFY(dup->is_diag());
    QCOMPARE(dup->decompose_eigen(), 0);
    QVERIFY(std::abs(whitenedNorm2(*dup, m_raw) - norm2) < 1e-9 * norm2);
}

//=============================================================================================================

void TestMneCovPython::rejectsBadInput()
{
    auto cov = MNECovMatrix::read(m_covPath, FIFFV_MNE_NOISE_COV);
    QVERIFY(cov);
    QVERIFY(!cov->pick_chs_omit(QStringList({"NOT A CHANNEL"}), 1, false, {}));
    QVERIFY(!cov->pick_chs_omit(QStringList(), 0, false, {}));

    VectorXf data = VectorXf::Zero(10);
    VectorXf out(10);
    QVERIFY(cov->whiten_vector(data, out, 10) != 0);
    QVERIFY(cov->add_inv() != 0);
    QVERIFY(cov->condition(1e-6f, -1) != 0);

    QVERIFY(!MNECovMatrix::read(m_covPath, 99));
    QVERIFY(!MNECovMatrix::read(m_dir.filePath("missing-cov.fif"), FIFFV_MNE_NOISE_COV));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneCovPython)
#include "test_mne_cov_python.moc"
