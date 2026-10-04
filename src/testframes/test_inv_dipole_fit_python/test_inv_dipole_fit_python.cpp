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
#include <inv/dipole_fit/inv_guess_data.h>

#include <fwd/fwd_eeg_sphere_model.h>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_meas_data.h>
#include <mne/mne_meas_data_set.h>

#include <memory>
#include <vector>

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
using namespace MNELIB;
using namespace FWDLIB;
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

//=============================================================================================================
/**
 * Writes a raw file whose MEG channels hold @p data (one column per sample).
 */
bool writeSyntheticRaw(const QString& sourceFile, const QString& outFile, const QStringList& chNames, const MatrixXd& data)
{
    QFile file(sourceFile);
    FiffRawData source(file);
    const RowVectorXi sel = FiffInfoBase::pick_channels(source.info.ch_names, chNames);
    if (sel.size() != data.rows())
        return false;
    FiffInfo info = source.info.pick_info(sel);
    info.bads.clear();

    QFile out(outFile);
    RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(out, info, cals);
    if (!stream)
        return false;
    constexpr int kBuffer = 100;
    for (int first = 0; first < data.cols(); first += kBuffer)
        stream->write_raw_buffer(data.middleCols(first, std::min<int>(kBuffer, static_cast<int>(data.cols()) - first)), cals);
    stream->finish_writing_raw();
    return true;
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
    void bemForwardMatches_data();
    void bemForwardMatches();
    void compensatedFieldMatches_data();
    void compensatedFieldMatches();
    void readsNoiseCov_data();
    void readsNoiseCov();
    void fitMatches_data();
    void fitMatches();
    void rawFitMatches_data();
    void rawFitMatches();
    void surfaceGuessesFitMatches();
    void sphereGuessGrid_data();
    void sphereGuessGrid();
    void guessesFromFile();
    void printsFields();
    void commandLine();
    void commandLineRejects_data();
    void commandLineRejects();
    void selectsNoiseCov_data();
    void selectsNoiseCov();
    void rejectsMissingInput();

private:
    QString m_sampleAve;
    QString m_synthAve;
    std::unique_ptr<InvDipoleFitData> m_fitData;
    InvEcdSet m_rawFit;
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
    m_fitData.reset(InvDipoleFitData::setup_dipole_fit_data(
        QString(), m_sampleAve, QString(), &r0, nullptr, false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, projnames, true, false));
    QVERIFY(m_fitData);

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
    m_synthAve = m_dir.filePath("synthetic-ave.fif");
    QVERIFY(writeSyntheticEvoked(m_sampleAve, m_synthAve, m_chNames, data));

    InvDipoleFitSettings settings;
    settings.measname = m_synthAve;
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

    // A raw recording holding each true field for kBlock samples, longer than
    // the fitter's 10 s segment so the fit has to reload data part way.
    const QString sampleRaw = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    constexpr int kBlock = 1500;
    MatrixXd rawData(fwdData->nmeg, 3 * kBlock);
    for (int k = 0; k < 3; ++k)
        rawData.middleCols(k * kBlock, kBlock) = m_fields.col(k).cast<double>().replicate(1, kBlock);
    const QString synthRaw = m_dir.filePath("synthetic_raw.fif");
    QVERIFY(writeSyntheticRaw(sampleRaw, synthRaw, m_chNames, rawData));

    InvDipoleFitSettings rawSettings;
    rawSettings.measname = synthRaw;
    rawSettings.is_raw = true;
    rawSettings.filter.filter_on = false;
    rawSettings.include_meg = true;
    rawSettings.include_eeg = false;
    rawSettings.guess_mindist = 0.0f;
    rawSettings.tmin = 2.0f;
    rawSettings.tmax = 13.0f;
    rawSettings.tstep = 5.0f;
    rawSettings.dipname = m_dir.filePath("raw-fit.dat");
    rawSettings.checkIntegrity();
    InvDipoleFit rawFit(&rawSettings);
    m_rawFit = rawFit.calculateFit();
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

void TestInvDipoleFitPython::bemForwardMatches_data()
{
    QTest::addColumn<int>("dipole");
    QTest::addColumn<double>("megNorm");
    QTest::addColumn<double>("eegNorm");

    // mne-python on the three-layer sample-1280-1280-1280 BEM solution:
    //   fwd = mne.make_forward_dipole(mne.Dipole(t, pos, amp, ori, gof), bem, ev.info, trans)
    //   P = make_projector(ev.info['projs'], fwd['sol']['row_names'])[0]
    //   np.linalg.norm((P @ fwd['sol']['data'][:, k] * amp[k])[meg or eeg])
    QTest::newRow("dipole 1") << 0 << 2.1842782649138175e-12 << 1.0707594849960882e-05;
    QTest::newRow("dipole 2") << 1 << 1.7820900035529597e-11 << 1.6207656369250373e-05;
    QTest::newRow("dipole 3") << 2 << 6.618196100881725e-12 << 9.877867103888988e-06;
}

void TestInvDipoleFitPython::bemForwardMatches()
{
    QFETCH(int, dipole);
    QFETCH(double, megNorm);
    QFETCH(double, eegNorm);

    const QString data = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
    QStringList projnames{m_sampleAve};
    FwdEegSphereModel::UPtr eegModel = FwdEegSphereModel::setup_eeg_sphere_model(QString(), QString(), 0.09f);
    QVERIFY(eegModel);
    // MEG and EEG through the BEM, with only the diagonal of the noise covariance.
    std::unique_ptr<InvDipoleFitData> fitData(InvDipoleFitData::setup_dipole_fit_data(
        data + "MEG/sample/all-trans.fif", m_sampleAve, data + "subjects/sample/bem/sample-1280-1280-1280-bem.fif",
        nullptr, eegModel.release(), true, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, true, projnames, true, true));
    QVERIFY(fitData);
    QCOMPARE(fitData->nmeg, 305);
    QCOMPARE(fitData->neeg, 59);
    QVERIFY(fitData->funcs == fitData->bem_funcs.get());
    QVERIFY(fitData->noise->cov_diag.size() == 364);

    MatrixXf g(fitData->nmeg + fitData->neeg, 3);
    QCOMPARE(InvDipoleFitData::compute_dipole_field(*fitData, truthPos(dipole), false, g), 0);
    const VectorXd field = (g * truthMoment(dipole)).cast<double>();
    const double meg = field.head(fitData->nmeg).norm();
    const double eeg = field.tail(fitData->neeg).norm();
    QVERIFY2(std::abs(meg - megNorm) < 5e-3 * megNorm, qPrintable(QStringLiteral("MEG %1 vs %2").arg(meg, 0, 'g', 10).arg(megNorm, 0, 'g', 10)));
    QVERIFY2(std::abs(eeg - eegNorm) < 5e-3 * eegNorm, qPrintable(QStringLiteral("EEG %1 vs %2").arg(eeg, 0, 'g', 10).arg(eegNorm, 0, 'g', 10)));

    // EEG needs a layered model; a homogeneous BEM is refused.
    QTest::ignoreMessage(QtCriticalMsg, "Cannot use a homogeneous model in EEG calculations.");
    QVERIFY(!InvDipoleFitData::setup_dipole_fit_data(
        data + "MEG/sample/all-trans.fif", m_sampleAve, data + "subjects/sample/bem/sample-5120-bem.fif",
        nullptr, FwdEegSphereModel::setup_eeg_sphere_model(QString(), QString(), 0.09f).release(), false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, projnames, false, true));
}

//=============================================================================================================

void TestInvDipoleFitPython::compensatedFieldMatches_data()
{
    QTest::addColumn<int>("grade");
    QTest::addColumn<VectorXd>("expected");

    // mne-python make_forward_dipole on the CTF fixture (7 MEG + 29 reference channels),
    // sphere r0 (0, 0, 40) mm, dipole at (30, 20, 70) mm with 30 nAm along (1, 0, 0.2);
    // grade 0 after raw.apply_gradient_compensation(0).
    VectorXd grade3(7), grade0(7);
    grade3 << 3.802754463322344e-14, 4.391023139760363e-14, -3.361109406796458e-14, -3.158846539008664e-14,
        3.0834929702905355e-14, -6.115886208135634e-14, -5.516961095963779e-14;
    grade0 << 2.115518000778138e-15, 1.1527947663125814e-14, -2.053172522664681e-14, -9.44117743983952e-16,
        3.760080630854645e-14, -8.011838019683636e-15, -2.3102320483303626e-14;
    QTest::newRow("third-order gradiometer") << 3 << grade3;
    QTest::newRow("uncompensated") << 0 << grade0;
}

void TestInvDipoleFitPython::compensatedFieldMatches()
{
    QFETCH(int, grade);
    QFETCH(VectorXd, expected);

    // The reference channels must be read and their coils built to compensate the field.
    const QString meas = QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade%1_raw.fif").arg(grade);
    Vector3f r0(0.0f, 0.0f, 0.04f);
    std::unique_ptr<InvDipoleFitData> fitData(InvDipoleFitData::setup_dipole_fit_data(
        QString(), meas, QString(), &r0, nullptr, false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, QStringList(), true, false));
    QVERIFY(fitData);
    QCOMPARE(fitData->nmeg, 7);
    MatrixXf g(7, 3);
    const Vector3f q = 30e-9f * Vector3f(1.0f, 0.0f, 0.2f).normalized();
    QCOMPARE(InvDipoleFitData::compute_dipole_field(*fitData, Vector3f(0.03f, 0.02f, 0.07f), false, g), 0);
    const VectorXd field = (g * q).cast<double>();
    const double err = (field - expected).norm() / expected.norm();
    QVERIFY2(err < 1e-3, qPrintable(QStringLiteral("relative error %1").arg(err)));
}

//=============================================================================================================

void TestInvDipoleFitPython::readsNoiseCov_data()
{
    QTest::addColumn<bool>("diagonal");
    QTest::addColumn<float>("reg");
    // Diagonal covariances are never regularized; a zero regularization leaves a full one as read.
    QTest::newRow("diagonal only") << true << 0.1f;
    QTest::newRow("full, unregularized") << false << 0.0f;
}

void TestInvDipoleFitPython::readsNoiseCov()
{
    QFETCH(bool, diagonal);
    QFETCH(float, reg);

    const QString cov = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif";
    QStringList projnames{m_sampleAve};
    Vector3f r0(0.0f, 0.0f, 0.04f);
    std::unique_ptr<InvDipoleFitData> fitData(InvDipoleFitData::setup_dipole_fit_data(
        QString(), m_sampleAve, QString(), &r0, nullptr, false, QString(), cov,
        5e-13f, 20e-15f, 0.2e-6f, reg, reg, reg, diagonal, projnames, true, false));
    QVERIFY(fitData);
    const MNECovMatrix& noise = *fitData->noise;
    QCOMPARE(noise.ncov, 305);
    VectorXd diag(noise.ncov);
    if (diagonal) {
        QCOMPARE(noise.cov.size(), 0);
        diag = noise.cov_diag;
    } else {
        QCOMPARE(noise.cov.size(), 305 * 306 / 2);
        QCOMPARE(noise.lambda.size(), 305);
        for (int k = 0, p = 0; k < noise.ncov; p += k + 2, ++k)
            diag[k] = noise.cov[p];
    }
    // mne-python: np.diag(P @ C @ P.T) for the 305 good MEG channels of sample_audvis-cov.fif,
    // with P = make_projector(ev.info['projs'], ch_names)[0]; checked by its sum, a
    // position-weighted sum and channels 0, 150 and 304.
    const double sum = diag.sum();
    const double weighted = diag.dot(VectorXd::LinSpaced(305, 1.0, 305.0));
    QVERIFY2(std::abs(sum - 3.3566836756795968e-21) < 1e-5 * sum, qPrintable(QString::number(sum, 'g', 12)));
    QVERIFY2(std::abs(weighted - 5.147342638653126e-19) < 1e-5 * weighted, qPrintable(QString::number(weighted, 'g', 12)));
    QVERIFY(std::abs(diag[0] - 2.272355891906954e-23) < 1e-5 * 2.272355891906954e-23);
    QVERIFY(std::abs(diag[150] - 2.132513211616573e-23) < 1e-5 * 2.132513211616573e-23);
    QVERIFY(std::abs(diag[304] - 2.8726580451278425e-26) < 1e-5 * 2.8726580451278425e-26);
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

void TestInvDipoleFitPython::rawFitMatches_data()
{
    QTest::addColumn<int>("sample");
    QTest::addColumn<int>("dipole");
    // At 300.3 Hz the blocks start at 0, 4.99 and 9.99 s.
    QTest::newRow("2 s") << 0 << 0;
    QTest::newRow("7 s") << 1 << 1;
    QTest::newRow("12 s, second segment") << 2 << 2;
}

void TestInvDipoleFitPython::rawFitMatches()
{
    QFETCH(int, sample);
    QFETCH(int, dipole);
    QCOMPARE(m_rawFit.size(), 3);
    // Same data and model as the evoked fit, so the same fits come out.
    const InvEcd& dip = m_rawFit[sample];
    const InvEcd& ref = m_fit[dipole];
    QVERIFY(dip.valid);
    QVERIFY2((dip.rd - ref.rd).norm() < 1e-4f, qPrintable(QStringLiteral("%1 mm off").arg(1e3 * (dip.rd - ref.rd).norm())));
    QVERIFY((dip.Q - ref.Q).norm() < 1e-3f * ref.Q.norm());
    QVERIFY(std::abs(dip.good - ref.good) < 1e-5f);
    QVERIFY((dip.rd - truthPos(dipole)).norm() < 1e-3f);
}

//=============================================================================================================

void TestInvDipoleFitPython::surfaceGuessesFitMatches()
{
    // Guesses inside the inner skull (MRI coordinates, moved to head with the
    // MRI transform) instead of the default 80 mm sphere.
    const QString data = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
    InvDipoleFitSettings settings;
    settings.measname = m_synthAve;
    settings.mriname = data + "MEG/sample/all-trans.fif";
    settings.guess_surfname = data + "subjects/sample/bem/sample-5120-bem.fif";
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.guess_mindist = 0.0f;
    settings.tmin = 0.0f;
    settings.dipname = m_dir.filePath("surf-fit.dat");
    settings.checkIntegrity();
    InvDipoleFit fit(&settings);
    const InvEcdSet set = fit.calculateFit();
    QCOMPARE(set.size(), 4);
    for (int k = 0; k < 3; ++k) {
        QVERIFY(set[k].valid);
        QVERIFY2((set[k].rd - truthPos(k)).norm() < 1e-3f, qPrintable(QStringLiteral("dipole %1: %2 mm off").arg(k + 1).arg(1e3 * (set[k].rd - truthPos(k)).norm())));
        // Other start points: the simplex ends within 0.2 mm of the sphere-guess fit.
        QVERIFY((set[k].rd - m_fit[k].rd).norm() < 5e-4f);
        QVERIFY(set[k].good > 0.9999f);
    }

    // A guess surface that cannot be read must stop the fit, not run it without guesses.
    settings.guess_surfname = data + "subjects/sample/bem/sample-inner_skull-5120.surf";
    InvDipoleFit badFit(&settings);
    QTest::ignoreMessage(QtCriticalMsg, "Could not create the initial guesses.");
    QCOMPARE(badFit.calculateFit().size(), 0);
}

//=============================================================================================================

void TestInvDipoleFitPython::sphereGuessGrid_data()
{
    QTest::addColumn<double>("radius");
    QTest::addColumn<double>("grid");
    QTest::addColumn<int>("nguess");
    QTest::addColumn<double>("rrSum");

    // mne-python _make_volume_source_space(surf, grid, exclude=0.02, mindist=0) with surf
    // the 642-vertex sphere of icos.fif (id 9003) scaled to the radius around (0, 0, 40) mm,
    // MNE-C's guess boundary. (mne-python's own exact-sphere guesses give 2081 / 892 / 628.)
    QTest::newRow("80 mm, 10 mm grid") << 0.08 << 0.010 << 2073 << 82.9;
    QTest::newRow("60 mm, 10 mm grid") << 0.06 << 0.010 << 865 << 34.58;
    QTest::newRow("80 mm, 15 mm grid") << 0.08 << 0.015 << 627 << 24.84;
}

void TestInvDipoleFitPython::sphereGuessGrid()
{
    QFETCH(double, radius);
    QFETCH(double, grid);
    QFETCH(int, nguess);
    QFETCH(double, rrSum);

    InvGuessData guess(QString(), QString(), 0.0f, 0.02f, static_cast<float>(grid), m_fitData.get(), static_cast<float>(radius));
    QCOMPARE(guess.nguess, nguess);
    QVERIFY(std::abs(guess.rr.cast<double>().sum() - rrSum) < 1e-3);
}

//=============================================================================================================

void TestInvDipoleFitPython::printsFields()
{
    // Sample 0 of the synthetic data is exactly the field of truth dipole 1.
    std::unique_ptr<MNEMeasData> data(MNEMeasData::mne_read_meas_data(m_synthAve, 1, nullptr, nullptr, m_fitData->ch_names,
                                                                      m_fitData->nmeg + m_fitData->neeg));
    QVERIFY(data);
    QString text;
    QTextStream out(&text);
    QVERIFY(InvDipoleFitData::print_fields(truthPos(0), truthMoment(0), 0.0f, 0.0f, *m_fitData, *data, out));
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    QCOMPARE(lines.size(), 305);
    double worst = 0.0;
    for (int k = 0; k < lines.size(); ++k) {
        const QStringList cols = lines[k].split('\t');
        QCOMPARE(cols.size(), 3);
        QCOMPARE(cols[0], m_chNames[k]);
        QVERIFY(std::abs(cols[1].toDouble() - 1e15 * m_fields(k, 0)) <= 1e-3 * std::abs(1e15 * m_fields(k, 0)) + 1e-3);
        worst = std::max(worst, std::abs(cols[2].toDouble() - cols[1].toDouble()));
    }
    QVERIFY2(worst < 1e-2 * 1e15 * m_fields.col(0).cwiseAbs().maxCoeff(), qPrintable(QString::number(worst)));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot pick time"));
    QVERIFY(!InvDipoleFitData::print_fields(truthPos(0), truthMoment(0), 10.0f, 0.0f, *m_fitData, *data, out));
}

//=============================================================================================================

void TestInvDipoleFitPython::guessesFromFile()
{
    // A volume source space as --guess file, moved to head coordinates with the MRI transform.
    const QString data = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
    const QString guessFile = QStringLiteral(MNE_VOL_SRC_DATA_DIR "/sample-vol25-src.fif");
    QVERIFY(QFile::exists(guessFile));
    QStringList projnames{m_sampleAve};
    Vector3f r0(0.0f, 0.0f, 0.04f);
    std::unique_ptr<InvDipoleFitData> fitData(InvDipoleFitData::setup_dipole_fit_data(
        data + "MEG/sample/all-trans.fif", m_sampleAve, QString(), &r0, nullptr, false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, projnames, true, false));
    QVERIFY(fitData);
    fitData->funcs = fitData->sphere_funcs.get();

    InvGuessData guess(guessFile, QString(), 0.0f, 0.0f, 0.01f, fitData.get());
    // mne-python: apply_trans(mri_head_t, src[0]['rr'][src[0]['vertno']])
    QCOMPARE(guess.nguess, 87);
    QVERIFY2(std::abs(guess.rr.cast<double>().sum() - 6.483795735946459) < 1e-5, qPrintable(QString::number(guess.rr.cast<double>().sum(), 'g', 12)));
    QVERIFY((guess.rr.row(0).transpose() - Vector3f(-0.02775475f, -0.04865797f, 0.0368692f)).norm() < 1e-6f);
    QCOMPARE(static_cast<int>(guess.guess_fwd.size()), 87);
    // The guess forward solution is the field of the sphere model at that location.
    MatrixXf g(fitData->nmeg, 3);
    QCOMPARE(InvDipoleFitData::compute_dipole_field(*fitData, Vector3f(guess.rr.row(5).transpose()), false, g), 0);
    QVERIFY(guess.guess_fwd[5]);
    QVERIFY(guess.guess_fwd[5]->sing.size() == 3 && guess.guess_fwd[5]->sing[0] > 0.0f);

    // A file that is not a source space gives no guesses.
    QTest::ignoreMessage(QtCriticalMsg, "No source spaces available here");
    InvGuessData none(m_sampleAve, QString(), 0.0f, 0.0f, 0.01f, fitData.get());
    QCOMPARE(none.nguess, 0);

    // Fields need the noise covariance that whitens them.
    fitData->noise.reset();
    QTest::ignoreMessage(QtCriticalMsg, "Noise covariance missing in compute_guess_fields");
    QVERIFY(!guess.compute_guess_fields(fitData.get()));
    QTest::ignoreMessage(QtCriticalMsg, "Data missing in compute_guess_fields");
    QVERIFY(!guess.compute_guess_fields(nullptr));
}

//=============================================================================================================

void TestInvDipoleFitPython::commandLine()
{
    QByteArrayList args{"mne_dipole_fit", "--meas", "a-ave.fif", "--meg", "--eeg", "--dip", "out.dip",
                        "--grid", "15", "--guessrad", "70", "--mindist", "5", "--exclude", "25",
                        "--origin", "1:2:45", "--tmin", "10", "--tmax", "250", "--tstep", "5", "--integ", "2",
                        "--gradnoise", "8", "--magnoise", "30", "--eegnoise", "0.5", "--reg", "0.2",
                        "--filtersize", "3000", "--lowpass", "30", "--set", "2", "--noproj", "--verbose"};
    std::vector<char*> argv;
    for (QByteArray& a : args)
        argv.push_back(a.data());
    int argc = static_cast<int>(argv.size());
    InvDipoleFitSettings s(&argc, argv.data());

    QCOMPARE(argc, 1);
    QCOMPARE(s.measname, QString("a-ave.fif"));
    QVERIFY(s.include_meg && s.include_eeg && !s.is_raw && s.verbose && s.omit_data_proj);
    QCOMPARE(s.dipname, QString("out.dip"));
    QCOMPARE(s.guess_grid, 0.015f);
    QCOMPARE(s.guess_rad, 0.07f);
    QCOMPARE(s.guess_mindist, 0.005f);
    QCOMPARE(s.guess_exclude, 0.025f);
    QVERIFY((s.r0 - Vector3f(0.001f, 0.002f, 0.045f)).norm() < 1e-7f);
    QCOMPARE(s.tmin, 0.01f);
    QCOMPARE(s.tmax, 0.25f);
    QCOMPARE(s.tstep, 0.005f);
    QCOMPARE(s.integ, 0.002f);
    QCOMPARE(s.grad_std, 8e-13f);
    QCOMPARE(s.mag_std, 30e-15f);
    QCOMPARE(s.eeg_std, 0.5e-6f);
    QCOMPARE(s.grad_reg, 0.2f);
    QCOMPARE(s.mag_reg, 0.2f);
    QCOMPARE(s.eeg_reg, 0.2f);
    QCOMPARE(s.filter.size, 4096);
    QCOMPARE(s.filter.lowpass, 30.0f);
    QCOMPARE(s.setno, 2);
    QVERIFY(s.projnames.isEmpty());

    // Invalid values stop parsing and leave the default in place.
    QByteArrayList bad{"mne_dipole_fit", "--grid", "-3"};
    std::vector<char*> badArgv;
    for (QByteArray& a : bad)
        badArgv.push_back(a.data());
    int badArgc = static_cast<int>(badArgv.size());
    QTest::ignoreMessage(QtCriticalMsg, "Grid spacing should be positive");
    InvDipoleFitSettings rejected(&badArgc, badArgv.data());
    QCOMPARE(rejected.guess_grid, 0.010f);

    // The remaining options, in MNE-C's units (mm, fT/cm, fT, uV, ms).
    QByteArrayList more{"mne_dipole_fit", "--raw", "r_raw.fif", "--eeg", "--bdip", "out.bdip", "--guess", "g.fif",
                        "--gsurf", "s.fif", "--guesssurf", "inner.fif", "--mri", "t.fif", "--bem", "b.fif", "--accurate",
                        "--eegrad", "95", "--eegmodels", "m.txt", "--eegmodel", "Three", "--eegscalp", "--proj", "p.fif",
                        "--bad", "bad.txt", "--noise", "n-cov.fif", "--diagnoise", "--eegreg", "0.3", "--magreg", "0.4",
                        "--gradreg", "0.5", "--bmin", "-100", "--bmax", "0", "--filteroff", "--lowpassw", "7",
                        "--highpass", "0.5", "--magdip", "--gui", "--mindist", "-2", "--exclude", "-1"};
    std::vector<char*> moreArgv;
    for (QByteArray& a : more)
        moreArgv.push_back(a.data());
    int moreArgc = static_cast<int>(moreArgv.size());
    InvDipoleFitSettings m(&moreArgc, moreArgv.data());
    QCOMPARE(moreArgc, 1);
    QVERIFY(m.is_raw && m.include_eeg && !m.include_meg && m.accurate && m.scale_eeg_pos && m.diagnoise && m.fit_mag_dipoles && m.gui);
    QCOMPARE(m.measname, QString("r_raw.fif"));
    QCOMPARE(m.bdipname, QString("out.bdip"));
    QCOMPARE(m.guessname, QString("g.fif"));
    QCOMPARE(m.guess_surfname, QString("inner.fif"));
    QCOMPARE(m.mriname, QString("t.fif"));
    QCOMPARE(m.bemname, QString("b.fif"));
    QCOMPARE(m.eeg_sphere_rad, 0.095f);
    QCOMPARE(m.eeg_model_file, QString("m.txt"));
    QCOMPARE(m.eeg_model_name, QString("Three"));
    // checkIntegrity puts the measurement file first unless --noproj.
    QCOMPARE(m.projnames, QStringList({"r_raw.fif", "p.fif"}));
    QCOMPARE(m.badname, QString("bad.txt"));
    QCOMPARE(m.noisename, QString("n-cov.fif"));
    QCOMPARE(m.eeg_reg, 0.3f);
    QCOMPARE(m.mag_reg, 0.4f);
    QCOMPARE(m.grad_reg, 0.5f);
    QCOMPARE(m.bmin, -0.1f);
    QCOMPARE(m.bmax, 0.0f);
    QVERIFY(m.do_baseline);
    QVERIFY(!m.filter.filter_on);
    QCOMPARE(m.filter.lowpass_width, 7.0f);
    QCOMPARE(m.filter.highpass, 0.5f);
    // Negative guess distances are clamped to zero rather than rejected.
    QCOMPARE(m.guess_mindist, 0.0f);
    QCOMPARE(m.guess_exclude, 0.0f);
}

//=============================================================================================================

void TestInvDipoleFitPython::commandLineRejects_data()
{
    QTest::addColumn<QByteArrayList>("args");
    QTest::addColumn<QString>("message");

    auto row = [](const char* name, QByteArrayList args, const char* message) {
        args.prepend("mne_dipole_fit");
        QTest::newRow(name) << args << QString(message);
    };
    for (const char* opt : {"--guess", "--gsurf", "--guesssurf", "--guessrad", "--mindist", "--exclude", "--grid", "--mri", "--bem",
                            "--origin", "--eegrad", "--eegmodels", "--eegmodel", "--meas", "--raw", "--proj", "--bad", "--noise",
                            "--gradnoise", "--magnoise", "--eegnoise", "--eegreg", "--magreg", "--gradreg", "--reg", "--tstep",
                            "--integ", "--tmin", "--tmax", "--bmin", "--bmax", "--set", "--lowpass", "--lowpassw", "--highpass",
                            "--filtersize", "--dip", "--bdip"})
        row(opt, {opt}, (QByteArray(opt) + ": argument required.").constData());
    row("guessrad not a number", {"--guessrad", "x"}, "Could not interpret the radius.");
    row("guessrad zero", {"--guessrad", "0"}, "Radius should be positive");
    row("mindist not a number", {"--mindist", "x"}, "Could not interpret the distance.");
    row("exclude not a number", {"--exclude", "x"}, "Could not interpret the distance.");
    row("grid not a number", {"--grid", "x"}, "Could not interpret the distance.");
    row("origin malformed", {"--origin", "1:2"}, "Could not interpret the origin.");
    row("eegrad zero", {"--eegrad", "0"}, "Radius must be positive");
    row("gradnoise negative", {"--gradnoise", "-1"}, "Value should be positive");
    row("magnoise negative", {"--magnoise", "-1"}, "Value should be positive");
    row("eegnoise negative", {"--eegnoise", "-1"}, "Value should be positive");
    row("eegreg above one", {"--eegreg", "1.5"}, "Regularization value should be positive and smaller than one.");
    row("magreg negative", {"--magreg", "-0.1"}, "Regularization value should be positive and smaller than one.");
    row("gradreg above one", {"--gradreg", "2"}, "Regularization value should be positive and smaller than one.");
    row("reg above one", {"--reg", "2"}, "Regularization value should be positive and smaller than one.");
    row("tstep negative", {"--tstep", "-1"}, "Time step should be positive");
    row("integ zero", {"--integ", "0"}, "Integration time should be positive.");
    row("set zero", {"--set", "0"}, "Data set number must be > 0");
    row("lowpass zero", {"--lowpass", "0"}, "Lowpass corner must be positive");
    row("lowpassw zero", {"--lowpassw", "0"}, "Lowpass width must be positive");
    row("highpass negative", {"--highpass", "-1"}, "Highpass corner must be positive");
    row("filtersize too small", {"--filtersize", "512"}, "Filtersize should be at least 1024.");
    row("unrecognized", {"--meg", "--bogus", "x"}, "Unrecognized arguments : --bogus x");
}

void TestInvDipoleFitPython::commandLineRejects()
{
    QFETCH(QByteArrayList, args);
    QFETCH(QString, message);
    std::vector<char*> argv;
    for (QByteArray& a : args)
        argv.push_back(a.data());
    int argc = static_cast<int>(argv.size());
    QTest::ignoreMessage(QtCriticalMsg, message.toUtf8().constData());
    // Parsing stops at the error, before checkIntegrity would also complain about the missing --meas.
    InvDipoleFitSettings s(&argc, argv.data());
}

//=============================================================================================================

void TestInvDipoleFitPython::selectsNoiseCov_data()
{
    QTest::addColumn<bool>("dense");
    QTest::addColumn<int>("nUnselected");
    QTest::addColumn<int>("nave");
    QTest::addColumn<bool>("ok");
    // MNE-C (dipole_fit_setup.c): unselected channels get weight 30, the covariance scales
    // by noise nave / data nave, and fewer than 20 remaining channels of a kind is an error.
    QTest::newRow("diag, all selected") << false << 0 << 4 << true;
    QTest::newRow("diag, 3 unselected") << false << 3 << 4 << true;
    QTest::newRow("dense, 3 unselected") << true << 3 << 2 << true;
    QTest::newRow("dense, too few left") << true << 10 << 1 << false;
}

void TestInvDipoleFitPython::selectsNoiseCov()
{
    QFETCH(bool, dense);
    QFETCH(int, nUnselected);
    QFETCH(int, nave);
    QFETCH(bool, ok);

    constexpr int kN = 25;
    QStringList names;
    MNEMeasData meas;
    for (int k = 0; k < kN; ++k) {
        names << QStringLiteral("MEG%1").arg(k, 4, 10, QChar('0'));
        FiffChInfo ch;
        ch.ch_name = names.last();
        meas.chs << ch;
    }
    meas.nchan = kN;
    auto* set = new MNEMeasDataSet;
    set->nave = nave;
    meas.sets << set;
    meas.nset = 1;
    meas.current = set;

    VectorXd diag = VectorXd::LinSpaced(kN, 1.0, 2.0);
    InvDipoleFitData fit;
    fit.nave = 1;
    if (dense) {
        MatrixXd full = 0.1 * MatrixXd::Ones(kN, kN);
        full.diagonal() += diag;
        VectorXd packed(kN * (kN + 1) / 2);
        for (int j = 0; j < kN; ++j)
            for (int k = 0; k <= j; ++k)
                packed[MNECovMatrix::lt_packed_index(j, k)] = full(j, k);
        fit.noise_orig = MNECovMatrix::create_dense(FIFFV_MNE_NOISE_COV, kN, names, packed);
    } else {
        fit.noise_orig = MNECovMatrix::create_diag(FIFFV_MNE_NOISE_COV, kN, names, diag);
    }
    fit.noise_orig->ch_class = VectorXi::Constant(kN, MNE_COV_CH_MEG_GRAD);

    std::vector<int> sels(kN, 1);
    for (int k = 0; k < nUnselected; ++k)
        sels[kN - 1 - k] = 0;
    if (!ok)
        QTest::ignoreMessage(QtCriticalMsg, "Too few MEG channels remaining");
    QCOMPARE(InvDipoleFitData::select_dipole_fit_noise_cov(&fit, &meas, -1, sels.data()) == 0, ok);
    if (!ok)
        return;
    QCOMPARE(fit.nave, nave);

    VectorXd w = VectorXd::Ones(kN);
    w.tail(nUnselected).setConstant(30.0);
    const double ratio = 1.0 / nave;
    if (dense) {
        for (int j = 0; j < kN; ++j)
            for (int k = 0; k <= j; ++k) {
                const double expected = ratio * w[j] * w[k] * ((j == k ? diag[j] : 0.0) + 0.1);
                // condition() rebuilds the matrix from its float eigen decomposition, as MNE-C does.
                QVERIFY(std::abs(fit.noise->cov[MNECovMatrix::lt_packed_index(j, k)] - expected) < 1e-5 * expected);
            }
        QCOMPARE(fit.noise->lambda.size(), kN);
        QVERIFY(fit.noise->lambda.minCoeff() > 0.0);
    } else {
        const VectorXd expected = ratio * w.cwiseProduct(w).cwiseProduct(diag);
        QVERIFY((fit.noise->cov_diag - expected).cwiseAbs().maxCoeff() < 1e-12);
        QVERIFY((fit.noise->inv_lambda - expected.cwiseSqrt().cwiseInverse()).cwiseAbs().maxCoeff() < 1e-9);
    }
    // The original stays untouched for the next selection.
    QCOMPARE(fit.noise_orig->ncov, kN);
    QVERIFY(dense || (fit.noise_orig->cov_diag - diag).cwiseAbs().maxCoeff() == 0.0);

    // Without a selection, nave alone rescales the original.
    QCOMPARE(InvDipoleFitData::select_dipole_fit_noise_cov(&fit, &meas, 2 * nave, nullptr), 0);
    QCOMPARE(fit.nave, 2 * nave);
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
