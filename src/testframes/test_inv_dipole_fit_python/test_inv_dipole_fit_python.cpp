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

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>

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
    void fitMatches_data();
    void fitMatches();
    void rawFitMatches_data();
    void rawFitMatches();
    void surfaceGuessesFitMatches();
    void sphereGuessGrid_data();
    void sphereGuessGrid();
    void commandLine();
    void commandLineRejects_data();
    void commandLineRejects();
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
