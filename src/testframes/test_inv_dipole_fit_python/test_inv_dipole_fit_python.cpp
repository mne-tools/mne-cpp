//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_dipole_fit_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates the sphere-model dipole fit against mne-python.
 *
 * The existing dipole-fit test compares MNE-CPP against a file MNE-CPP once
 * wrote, so it cannot tell a correct fit from a consistently wrong one. Here
 * the expected locations, moments and goodness of fit come from mne-python's
 * fit_dipole on the same synthetic data:
 *
 *   ev = mne.read_evokeds('sample_audvis-ave.fif', 0, baseline=None)
 *            .pick('meg', exclude='bads')                       # 305 channels
 *   sphere = mne.make_sphere_model(r0=(0, 0, 0.04), head_radius=None)
 *   sensors = dict(meg=_prep_meg_channels(ev.info, accuracy='normal'))
 *   F = _compute_forwards_meeg(pos, sensors=sensors,
 *           fwd_data=_prep_field_computation(sensors=sensors, bem=sphere))
 *   G[:, k] = F['meg'].T[:, 3k:3k+3] @ (amp[k] * ori[k])        # truth below
 *   data = np.c_[G, G[:, 0] + G[:, 1], G[:, 0] + G[:, 1]]
 *   cov = mne.make_ad_hoc_cov(ev.info, std=dict(grad=5e-13, mag=20e-15))
 *   mne.fit_dipole(mne.EvokedArray(data, ev.info, tmin=0, nave=1), cov,
 *                  sphere, min_dist=0.)
 *
 * The three single-dipole samples must be recovered at the truth; the
 * superposition of two dipoles yields a compromise location that only a
 * correct forward model, projector, whitener and optimiser reproduce. The
 * field MNE-CPP computes for each true dipole is checked against the norm of
 * mne-python's projected field before it is used to build the data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/dipole_fit/inv_dipole_fit.h>
#include <inv/dipole_fit/inv_dipole_fit_data.h>
#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_ecd_set.h>

#include <fiff/fiff_evoked_set.h>

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

using namespace INVLIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace
{

//=============================================================================================================
/**
 * Truth dipoles in head coordinates (m) and their moments (A m).
 */
Vector3f truthPos(int k)
{
    static const float pos[3][3] = {{0.03f, 0.02f, 0.07f}, {-0.04f, 0.01f, 0.06f}, {0.0f, 0.05f, 0.05f}};
    return Vector3f(pos[k][0], pos[k][1], pos[k][2]);
}

Vector3f truthMoment(int k)
{
    static const double ori[3][3] = {{1.0, 0.0, 0.2}, {0.0, 1.0, -0.3}, {-0.7, 0.2, 0.7}};
    static const double amp[3] = {30e-9, 50e-9, 40e-9};
    Vector3d o(ori[k][0], ori[k][1], ori[k][2]);
    return (amp[k] * o.normalized()).cast<float>();
}

//=============================================================================================================
/**
 * Writes the five-sample synthetic evoked file and returns false on failure.
 */
bool writeSyntheticEvoked(const QString& sourceFile, const QString& outFile, const QStringList& chNames, const MatrixXd& data)
{
    QFile file(sourceFile);
    FiffEvokedSet source(file);
    if (source.evoked.isEmpty())
        return false;

    FiffEvoked evoked = source.evoked[0].pick_channels(chNames);
    evoked.info.bads.clear();
    if (evoked.info.nchan != data.rows())
        return false;

    const float sfreq = static_cast<float>(evoked.info.sfreq);
    evoked.data = data;
    evoked.first = 0;
    evoked.last = static_cast<fiff_int_t>(data.cols()) - 1;
    evoked.times = RowVectorXf::LinSpaced(data.cols(), 0.0f, static_cast<float>(data.cols() - 1) / sfreq);
    evoked.nave = 1;
    evoked.comment = QStringLiteral("synthetic");

    FiffEvokedSet out;
    out.info = evoked.info;
    out.evoked.append(evoked);
    return out.save(outFile);
}

} // namespace

//=============================================================================================================
/**
 * Cross validates the sphere-model dipole fit against mne-python.
 */
class TestInvDipoleFitPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void forwardFieldMatches_data();
    void forwardFieldMatches();
    void fitMatches_data();
    void fitMatches();
    void rejectsMissingInput();

private:
    QString m_sampleAve;
    QTemporaryDir m_dir;
    QStringList m_chNames;
    MatrixXf m_fields; /**< 305 x 3 projected fields of the true dipoles. */
    InvEcdSet m_fit;
};

//=============================================================================================================

void TestInvDipoleFitPython::initTestCase()
{
    m_sampleAve = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
    QVERIFY2(QFile::exists(m_sampleAve), "test data missing");
    QVERIFY(m_dir.isValid());

    // Fields of the true dipoles, from the very forward model the fit uses.
    QStringList projnames{m_sampleAve};
    Vector3f r0(0.0f, 0.0f, 0.04f);
    std::unique_ptr<InvDipoleFitData> fwdData(InvDipoleFitData::setup_dipole_fit_data(
        QString(), m_sampleAve, QString(), &r0, nullptr, false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, projnames, true, false));
    QVERIFY(fwdData);
    QCOMPARE(fwdData->nmeg, 305);
    QCOMPARE(fwdData->neeg, 0);
    fwdData->funcs = fwdData->sphere_funcs.get();
    m_chNames = fwdData->ch_names.mid(0, fwdData->nmeg);

    m_fields.resize(fwdData->nmeg, 3);
    for (int k = 0; k < 3; ++k) {
        MatrixXf g(fwdData->nmeg, 3);
        QCOMPARE(InvDipoleFitData::compute_dipole_field(*fwdData, truthPos(k), false, g), 0);
        m_fields.col(k) = g * truthMoment(k);
    }

    MatrixXd data(fwdData->nmeg, 5);
    data.leftCols(3) = m_fields.cast<double>();
    data.col(3) = (m_fields.col(0) + m_fields.col(1)).cast<double>();
    data.col(4) = data.col(3);
    const QString synth = m_dir.filePath("synthetic-ave.fif");
    QVERIFY(writeSyntheticEvoked(m_sampleAve, synth, m_chNames, data));

    InvDipoleFitSettings settings;
    settings.measname = synth;
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.guess_mindist = 0.0f;
    settings.tmin = 0.0f;
    settings.dipname = m_dir.filePath("fit.dat");
    settings.checkIntegrity();

    InvDipoleFit fit(&settings);
    m_fit = fit.calculateFit();
    // The time loop stops before the last sample, which only pads the data.
    QCOMPARE(m_fit.size(), 4);
}

//=============================================================================================================

void TestInvDipoleFitPython::forwardFieldMatches_data()
{
    QTest::addColumn<int>("dipole");
    QTest::addColumn<double>("pythonNorm");

    // np.linalg.norm(make_projector(projs, ch_names)[0] @ G[:, k])
    QTest::newRow("dipole 1") << 0 << 7.861945130117361e-12;
    QTest::newRow("dipole 2") << 1 << 1.8537999803835052e-11;
    QTest::newRow("dipole 3") << 2 << 1.1855243720505123e-11;
}

void TestInvDipoleFitPython::forwardFieldMatches()
{
    QFETCH(int, dipole);
    QFETCH(double, pythonNorm);

    const double norm = m_fields.col(dipole).cast<double>().norm();
    QVERIFY2(std::abs(norm - pythonNorm) < 1e-4 * pythonNorm,
             qPrintable(QStringLiteral("field norm %1 vs mne-python %2").arg(norm, 0, 'g', 12).arg(pythonNorm, 0, 'g', 12)));
}

//=============================================================================================================

void TestInvDipoleFitPython::fitMatches_data()
{
    QTest::addColumn<int>("sample");
    QTest::addColumn<Vector3d>("posMm");
    QTest::addColumn<Vector3d>("qNAm");
    QTest::addColumn<double>("gofPercent");
    QTest::addColumn<double>("posTolMm");
    QTest::addColumn<double>("gofTolPercent");
    QTest::addColumn<double>("qRelTol");

    QTest::newRow("dipole 1") << 0
                              << Vector3d(30.00945623584166, 19.97976658598626, 69.98608396579458)
                              << Vector3d(14.972583573738602, -9.63296316190587, -8.565797930810295)
                              << 99.99996959113513 << 1.0 << 0.01 << 0.02;
    QTest::newRow("dipole 2") << 1
                              << Vector3d(-40.04982335185104, 9.973564340711066, 60.01200905231966)
                              << Vector3d(3.614398707811369, 46.89498330567209, -16.138015060874316)
                              << 99.99989203262074 << 1.0 << 0.01 << 0.02;
    QTest::newRow("dipole 3") << 2
                              << Vector3d(0.013850544237364313, 50.038161447483645, 50.01443183488143)
                              << Vector3d(-27.691677758850794, -5.0170025269134575, 25.10627975480261)
                              << 99.99993293714775 << 1.0 << 0.01 << 0.02;
    QTest::newRow("dipoles 1+2") << 3
                                 << Vector3d(-32.30062022182863, 4.3154411809000095, 59.22459604438498)
                                 << Vector3d(4.550472894389616, 74.13165002776464, -8.995126771310593)
                                 << 90.35942573873831 << 2.0 << 0.2 << 0.06;
    // The two-dipole sample has a shallow residual: like MNE-C (fit_dipoles.c)
    // the simplex stops at ftol = 1e-2 of the residual, mne-python's COBYLA at
    // rhoend = 5e-5. mne-python's fixed-position fit at MNE-CPP's optimum
    // (-33.49, 4.48, 58.82) mm gives 90.23 % against 90.36 % at its own, and
    // a moment of (4.30, 71.35, -9.35) nAm, within 0.05 nAm of MNE-CPP's.
}

void TestInvDipoleFitPython::fitMatches()
{
    QFETCH(int, sample);
    QFETCH(Vector3d, posMm);
    QFETCH(Vector3d, qNAm);
    QFETCH(double, gofPercent);
    QFETCH(double, posTolMm);
    QFETCH(double, gofTolPercent);
    QFETCH(double, qRelTol);

    const InvEcd& dip = m_fit[sample];
    QVERIFY(dip.valid);

    const Vector3d pos = 1e3 * dip.rd.cast<double>();
    const Vector3d q = 1e9 * dip.Q.cast<double>();
    QVERIFY2((pos - posMm).norm() < posTolMm,
             qPrintable(QStringLiteral("position (%1, %2, %3) mm").arg(pos[0]).arg(pos[1]).arg(pos[2])));
    QVERIFY2((q - qNAm).norm() < qRelTol * qNAm.norm(),
             qPrintable(QStringLiteral("moment (%1, %2, %3) nAm").arg(q[0]).arg(q[1]).arg(q[2])));
    QVERIFY2(std::abs(100.0 * dip.good - gofPercent) < gofTolPercent,
             qPrintable(QStringLiteral("goodness of fit %1 %").arg(100.0 * dip.good)));

    // MNE-C convention (fit_dipoles.c): nchan - 3 - ncomp - nproj = 305 - 3 - 2 - 3.
    // mne-python reports rank - ncomp = 300 for the same fit.
    QCOMPARE(dip.nfree, 297);
}

//=============================================================================================================

void TestInvDipoleFitPython::rejectsMissingInput()
{
    InvDipoleFitSettings settings;
    settings.measname = m_dir.filePath("does-not-exist-ave.fif");
    settings.include_meg = true;
    settings.dipname = m_dir.filePath("none.dat");
    settings.checkIntegrity();
    InvDipoleFit fit(&settings);
    QCOMPARE(fit.calculateFit().size(), 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvDipoleFitPython)
#include "test_inv_dipole_fit_python.moc"
