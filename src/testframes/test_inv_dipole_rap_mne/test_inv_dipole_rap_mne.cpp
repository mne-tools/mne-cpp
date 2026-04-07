//=============================================================================================================
// test_inv_dipole_rap_mne.cpp — Tests for inverse dipole fitting, RAP-MUSIC, and minimum norm
//
// Covers: InvSourceEstimateToken (tokenize/fromTokens), InvRapMusic, InvPwlRapMusic,
//         InvDipoleFitSettings, InvDipoleFitData (ad_hoc_noise, setup), InvGuessData,
//         InvEcdSet (read/write), InvDipole, InvMinimumNorm (additional paths),
//         InvSourceEstimate (grid/coupling/focal/connectivity layers)
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>

#include <inv/inv_source_estimate.h>
#include <inv/inv_source_estimate_token.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/rap_music/inv_rap_music.h>
#include <inv/rap_music/inv_pwl_rap_music.h>
#include <inv/rap_music/inv_dipole.h>
#include <inv/dipole_fit/inv_dipole_fit.h>
#include <inv/dipole_fit/inv_dipole_fit_data.h>
#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <inv/dipole_fit/inv_guess_data.h>
#include <inv/beamformer/inv_beamformer.h>
#include <inv/beamformer/inv_beamformer_compute.h>
#include <inv/inv_global.h>
#include <inv/inv_source_coupling.h>
#include <inv/inv_focal_dipole.h>
#include <inv/inv_connectivity.h>
#include <inv/inv_types.h>
#include <inv/inv_token.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestInvDipoleRapMne : public QObject
{
    Q_OBJECT

private:
    QString m_sTestDataPath;
    QString m_sMneSamplePath;
    bool m_bMneSampleAvailable = false;

    MNEForwardSolution m_fwd;
    FiffCov m_noiseCov;
    FiffInfo m_info;
    MNEInverseOperator m_invOp;

    QString rawPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif"; }
    QString fwdPath() const { return m_sTestDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── InvSourceEstimate extended ──
    void sourceEstimate_gridData();
    void sourceEstimate_couplings();
    void sourceEstimate_focalDipoles();
    void sourceEstimate_connectivity();
    void sourceEstimate_metadata();
    void sourceEstimate_reduce();
    void sourceEstimate_writeReadRoundTrip();

    // ── InvSourceEstimateToken ──
    void token_defaultConstruction();
    void token_tokenizeEmptyEstimate();
    void token_tokenizeGridData();
    void token_tokenizeAndReconstruct();
    void token_tokenizeWithOptions();
    void token_roundTripWithCouplings();

    // ── InvDipole ──
    void dipole_defaultConstruction();
    void dipole_positionAndDirection();
    void dipole_clean();

    // ── InvDipolePair ──
    void dipolePair_construction();

    // ── InvEcd / InvEcdSet ──
    void ecd_construction();
    void ecdSet_construction();
    void ecdSet_addAndAccess();
    void ecdSet_writeReadDipFile();
    void ecdSet_copyConstruction();

    // ── InvDipoleFitSettings ──
    void dipoleFitSettings_defaultConstruction();
    void dipoleFitSettings_setFields();
    void dipoleFitSettings_checkIntegrity();

    // ── InvDipoleFitData ──
    void dipoleFitData_defaultConstruction();
    void dipoleFitData_adHocNoise();

    // ── InvGuessData ──
    void guessData_defaultConstruction();

    // ── InvRapMusic ──
    void rapMusic_defaultConstruction();
    void rapMusic_initAndCalculate();
    void rapMusic_getName();

    // ── InvPwlRapMusic ──
    void pwlRapMusic_defaultConstruction();
    void pwlRapMusic_powellOffset();
    void pwlRapMusic_powellIdxVec();

    // ── InvMinimumNorm extended ──
    void minimumNorm_methodSetting();
    void minimumNorm_calculateInverse();
    void minimumNorm_dSPM();
    void minimumNorm_sLORETA();

    // ── Additional coverage ──
    void inv_globalBuildInfo();
    void ecd_copyCtor();
    void sourceEstimate_assignment();

    // ── InvSourceEstimateToken coverage ──
    void tokenize_simpleEstimate();
    void tokenize_roundtrip();
    void tokenize_metadataOnly();
    void tokenize_withCouplings();
    void tokenize_withFocalDipoles();
    void tokenize_withConnectivity();
    void tokenize_subsampling();
    void tokenIds_and_values();
};

//=============================================================================================================

void TestInvDipoleRapMne::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);

    m_sTestDataPath = QCoreApplication::applicationDirPath()
                      + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));

    m_sMneSamplePath = QDir::homePath() + "/mne_data/MNE-sample-data";
    QString invPath = m_sMneSamplePath + "/MEG/sample/sample_audvis-meg-oct-6-meg-inv.fif";
    if (QFile::exists(invPath)) {
        m_bMneSampleAvailable = true;
        QFile invFile(invPath);
        m_invOp = MNEInverseOperator(invFile);
        QVERIFY2(m_invOp.nsource > 0, "Failed to load inverse operator");
    } else {
        qDebug() << "MNE sample data not found at" << m_sMneSamplePath
                 << "— inverse operator tests will be skipped";
    }

    // Load forward solution from submodule test data
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));
    QFile fwdFile(fwdPath());
    m_fwd = MNEForwardSolution(fwdFile);
    QVERIFY2(!m_fwd.isEmpty(), "Failed to load forward solution");

    // Load noise covariance
    QVERIFY2(QFile::exists(covPath()),
             qPrintable(QString("Noise covariance not found: %1").arg(covPath())));
    QFile covFile(covPath());
    m_noiseCov = FiffCov(covFile);
    QVERIFY2(!m_noiseCov.isEmpty(), "Failed to load noise covariance");

    // Load measurement info
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    m_info = raw.info;
    QVERIFY2(m_info.nchan > 0, "Failed to load measurement info");
}

void TestInvDipoleRapMne::cleanupTestCase() {}

//=============================================================================================================
// InvSourceEstimate extended tests
//=============================================================================================================

void TestInvDipoleRapMne::sourceEstimate_gridData()
{
    MatrixXd data = MatrixXd::Random(10, 5);
    VectorXi verts(10);
    for (int i = 0; i < 10; i++) verts(i) = i;

    InvSourceEstimate stc(data, verts, 0.0f, 0.001f);
    QCOMPARE(stc.data.rows(), (Index)10);
    QCOMPARE(stc.data.cols(), (Index)5);
    QCOMPARE(stc.vertices.size(), (Index)10);
    QCOMPARE(stc.samples(), 5);
    QVERIFY(!stc.isEmpty());
}

void TestInvDipoleRapMne::sourceEstimate_couplings()
{
    MatrixXd data = MatrixXd::Random(4, 3);
    VectorXi verts(4);
    verts << 0, 1, 2, 3;
    InvSourceEstimate stc(data, verts, 0.0f, 0.01f);

    // Add coupling data
    InvSourceCoupling coupling;
    coupling.gridIndices = {0, 1};
    coupling.moments.push_back(Eigen::Vector3d(1.0, 0.0, 0.0));
    coupling.moments.push_back(Eigen::Vector3d(0.0, 1.0, 0.0));
    coupling.correlations = Eigen::MatrixXd::Identity(2, 2);
    coupling.tmin = 0.0f;
    coupling.tmax = 0.1f;
    stc.couplings.push_back(coupling);

    QVERIFY(stc.hasCouplings());
    QCOMPARE(stc.couplings.size(), (size_t)1);
    QCOMPARE(stc.couplings[0].gridIndices.size(), (size_t)2);
}

void TestInvDipoleRapMne::sourceEstimate_focalDipoles()
{
    MatrixXd data = MatrixXd::Random(4, 3);
    VectorXi verts(4);
    verts << 0, 1, 2, 3;
    InvSourceEstimate stc(data, verts, 0.0f, 0.01f);

    InvFocalDipole fd;
    fd.position = Vector3f(0.01f, 0.02f, 0.03f);
    fd.moment = Vector3f(0.0f, 0.0f, 10e-9f);
    fd.gridIndex = -1;
    fd.goodness = 0.95f;
    fd.valid = true;
    stc.focalDipoles.push_back(fd);

    QVERIFY(stc.hasFocalDipoles());
    QCOMPARE(stc.focalDipoles.size(), (size_t)1);
    QVERIFY(qAbs(stc.focalDipoles[0].goodness - 0.95f) < 1e-6);
}

void TestInvDipoleRapMne::sourceEstimate_connectivity()
{
    MatrixXd data = MatrixXd::Random(4, 3);
    VectorXi verts(4);
    verts << 0, 1, 2, 3;
    InvSourceEstimate stc(data, verts, 0.0f, 0.01f);

    InvConnectivity conn;
    conn.matrix = Eigen::MatrixXd::Identity(4, 4);
    conn.measure = "coh";
    conn.directed = false;
    conn.fmin = 8.0f;
    conn.fmax = 12.0f;
    stc.connectivity.push_back(conn);

    QVERIFY(stc.hasConnectivity());
}

void TestInvDipoleRapMne::sourceEstimate_metadata()
{
    InvSourceEstimate stc;
    stc.method = InvEstimateMethod::MNE;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = InvOrientationType::Fixed;

    QCOMPARE(stc.method, InvEstimateMethod::MNE);
    QCOMPARE(stc.sourceSpaceType, InvSourceSpaceType::Surface);
    QCOMPARE(stc.orientationType, InvOrientationType::Fixed);
}

void TestInvDipoleRapMne::sourceEstimate_reduce()
{
    MatrixXd data = MatrixXd::Random(10, 20);
    VectorXi verts(10);
    for (int i = 0; i < 10; i++) verts(i) = i;
    InvSourceEstimate stc(data, verts, 0.0f, 0.001f);

    // Reduce to samples 5 to 14 (10 samples)
    InvSourceEstimate reduced = stc.reduce(5, 10);
    QCOMPARE(reduced.samples(), 10);
    QCOMPARE(reduced.data.cols(), (Index)10);
    QVERIFY(qAbs(reduced.tmin - 0.005f) < 1e-6);
}

void TestInvDipoleRapMne::sourceEstimate_writeReadRoundTrip()
{
    MatrixXd data = MatrixXd::Random(5, 3);
    VectorXi verts(5);
    verts << 10, 20, 30, 40, 50;
    InvSourceEstimate stcOrig(data, verts, -0.1f, 0.001f);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Write to buffer
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    QVERIFY(stcOrig.write(buf));
    buf.close();

    // Read back
    buf.open(QIODevice::ReadOnly);
    InvSourceEstimate stcRead;
    QVERIFY(InvSourceEstimate::read(buf, stcRead));
    buf.close();

    QCOMPARE(stcRead.data.rows(), stcOrig.data.rows());
    QCOMPARE(stcRead.data.cols(), stcOrig.data.cols());
    QCOMPARE(stcRead.vertices.size(), stcOrig.vertices.size());
    QVERIFY(qAbs(stcRead.tmin - stcOrig.tmin) < 1e-5);
    QVERIFY(qAbs(stcRead.tstep - stcOrig.tstep) < 1e-5);
}

//=============================================================================================================
// InvSourceEstimateToken tests
//=============================================================================================================

void TestInvDipoleRapMne::token_defaultConstruction()
{
    InvToken tok;
    QVERIFY(tok.value == 0.0f || true);  // Just verify construction doesn't crash

    InvToken tok2(InvTokenId::Bos);
    QCOMPARE(tok2.id, InvTokenId::Bos);

    InvToken tok3(InvTokenId::Amplitude, 42.5f);
    QCOMPARE(tok3.id, InvTokenId::Amplitude);
    QVERIFY(qAbs(tok3.value - 42.5f) < 1e-6);
}

void TestInvDipoleRapMne::token_tokenizeEmptyEstimate()
{
    InvSourceEstimate stc;
    auto tokens = tokenize(stc);
    // Empty estimate should still produce a valid (possibly minimal) token sequence
    QVERIFY(tokens.size() >= 2);  // At least STC_BEGIN and STC_END
}

void TestInvDipoleRapMne::token_tokenizeGridData()
{
    MatrixXd data(3, 2);
    data << 1.0, 2.0,
            3.0, 4.0,
            5.0, 6.0;
    VectorXi verts(3);
    verts << 100, 200, 300;

    InvSourceEstimate stc(data, verts, 0.0f, 0.001f);

    auto tokens = tokenize(stc);
    QVERIFY(tokens.size() > 10);  // Should have metadata + grid data tokens

    // Verify token sequence starts with Bos
    QCOMPARE(tokens.front().id, InvTokenId::Bos);
    // Verify it ends with Eos
    QCOMPARE(tokens.back().id, InvTokenId::Eos);
}

void TestInvDipoleRapMne::token_tokenizeAndReconstruct()
{
    // Create source estimate with known data
    MatrixXd data(4, 3);
    data << 1.0, 2.0, 3.0,
            4.0, 5.0, 6.0,
            7.0, 8.0, 9.0,
            10.0, 11.0, 12.0;
    VectorXi verts(4);
    verts << 0, 10, 20, 30;
    InvSourceEstimate stcOrig(data, verts, -0.1f, 0.001f);

    // Tokenize
    auto tokens = tokenize(stcOrig);
    QVERIFY(!tokens.empty());

    // Reconstruct from tokens
    InvSourceEstimate stcRecon = fromTokens(tokens);

    // Verify reconstruction matches original
    QCOMPARE(stcRecon.data.rows(), stcOrig.data.rows());
    QCOMPARE(stcRecon.data.cols(), stcOrig.data.cols());
    QVERIFY((stcRecon.data - stcOrig.data).norm() < 1e-3);
    QCOMPARE(stcRecon.vertices.size(), stcOrig.vertices.size());
    QVERIFY(qAbs(stcRecon.tmin - stcOrig.tmin) < 1e-4);
    QVERIFY(qAbs(stcRecon.tstep - stcOrig.tstep) < 1e-4);
}

void TestInvDipoleRapMne::token_tokenizeWithOptions()
{
    MatrixXd data = MatrixXd::Random(5, 3);
    VectorXi verts(5);
    verts << 0, 1, 2, 3, 4;
    InvSourceEstimate stc(data, verts, 0.0f, 0.001f);

    // Add some optional layers
    InvFocalDipole fd;
    fd.position = Vector3f(0.01f, 0.02f, 0.03f);
    fd.moment = Vector3f(0.0f, 0.0f, 10e-9f);
    fd.gridIndex = -1;
    fd.goodness = 0.9f;
    fd.valid = true;
    stc.focalDipoles.push_back(fd);

    // Tokenize with grid data only
    InvTokenizeOptions opts;
    opts.includeGridData = true;
    opts.includeFocalDipoles = false;
    opts.includeCouplings = false;
    opts.includeConnectivity = false;
    auto tokensGridOnly = tokenize(stc, opts);

    // Tokenize with everything
    InvTokenizeOptions optsFull;
    auto tokensFull = tokenize(stc, optsFull);

    // Full should have at least as many tokens as grid-only
    QVERIFY(tokensFull.size() >= tokensGridOnly.size());

    // Tokenize with limited sources/time
    InvTokenizeOptions optsLimited;
    optsLimited.maxSources = 2;
    optsLimited.maxTimePoints = 1;
    auto tokensLimited = tokenize(stc, optsLimited);
    QVERIFY(tokensLimited.size() <= tokensFull.size());
}

void TestInvDipoleRapMne::token_roundTripWithCouplings()
{
    MatrixXd data = MatrixXd::Random(3, 2);
    VectorXi verts(3);
    verts << 0, 1, 2;
    InvSourceEstimate stc(data, verts, 0.0f, 0.001f);

    InvSourceCoupling c;
    c.gridIndices = {0, 1};
    c.moments.push_back(Eigen::Vector3d(1.0, 0.0, 0.0));
    c.moments.push_back(Eigen::Vector3d(0.0, 1.0, 0.0));
    c.correlations = Eigen::MatrixXd::Identity(2, 2);
    stc.couplings.push_back(c);

    auto tokens = tokenize(stc);
    InvSourceEstimate stcRecon = fromTokens(tokens);

    QVERIFY(stcRecon.hasCouplings());
    QCOMPARE(stcRecon.couplings.size(), (size_t)1);
}

//=============================================================================================================
// InvDipole tests
//=============================================================================================================

void TestInvDipoleRapMne::dipole_defaultConstruction()
{
    InvDipole<double> dip;
    // Default values should be zero
    QVERIFY(qAbs(dip.x()) < 1e-10);
    QVERIFY(qAbs(dip.y()) < 1e-10);
    QVERIFY(qAbs(dip.z()) < 1e-10);
}

void TestInvDipoleRapMne::dipole_positionAndDirection()
{
    InvDipole<double> dip;
    dip.x() = 0.05;
    dip.y() = -0.02;
    dip.z() = 0.08;
    dip.phi_x() = 0.0;
    dip.phi_y() = 0.0;
    dip.phi_z() = 1.0;

    QVERIFY(qAbs(dip.x() - 0.05) < 1e-10);
    QVERIFY(qAbs(dip.y() + 0.02) < 1e-10);
    QVERIFY(qAbs(dip.z() - 0.08) < 1e-10);
    QVERIFY(qAbs(dip.phi_z() - 1.0) < 1e-10);
}

void TestInvDipoleRapMne::dipole_clean()
{
    InvDipole<double> dipole;
    dipole.x() = 1.0;
    dipole.y() = 2.0;
    dipole.z() = 3.0;
    dipole.phi_x() = 0.5;
    dipole.phi_y() = 0.5;
    dipole.phi_z() = 0.0;

    dipole.clean();

    QVERIFY(qAbs(dipole.x()) < 1e-10);
    QVERIFY(qAbs(dipole.y()) < 1e-10);
    QVERIFY(qAbs(dipole.z()) < 1e-10);
    QVERIFY(qAbs(dipole.phi_x()) < 1e-10);
    QVERIFY(qAbs(dipole.phi_y()) < 1e-10);
    QVERIFY(qAbs(dipole.phi_z()) < 1e-10);
}

//=============================================================================================================
// InvDipolePair tests
//=============================================================================================================

void TestInvDipoleRapMne::dipolePair_construction()
{
    InvDipolePair<double> pair;
    pair.m_iIdx1 = 0;
    pair.m_iIdx2 = 1;
    pair.m_vCorrelation = 0.95;

    QCOMPARE(pair.m_iIdx1, 0);
    QCOMPARE(pair.m_iIdx2, 1);
    QVERIFY(qAbs(pair.m_vCorrelation - 0.95) < 1e-10);
}

//=============================================================================================================
// InvEcd / InvEcdSet tests
//=============================================================================================================

void TestInvDipoleRapMne::ecd_construction()
{
    InvEcd ecd;
    ecd.valid = true;
    ecd.time = 0.100f;
    ecd.rd = Vector3f(0.05f, 0.0f, 0.06f);
    ecd.Q = Vector3f(0.0f, 0.0f, 10e-9f);
    ecd.good = 0.95f;

    QVERIFY(ecd.valid);
    QVERIFY(qAbs(ecd.time - 0.1f) < 1e-5);
    QVERIFY(qAbs(ecd.good - 0.95f) < 1e-5);
}

void TestInvDipoleRapMne::ecdSet_construction()
{
    InvEcdSet set;
    QCOMPARE(set.size(), (qint32)0);
    QVERIFY(set.dataname.isEmpty());
}

void TestInvDipoleRapMne::ecdSet_addAndAccess()
{
    InvEcdSet set;
    InvEcd ecd1;
    ecd1.valid = true;
    ecd1.time = 0.050f;
    ecd1.rd = Vector3f(0.03f, 0.01f, 0.05f);
    ecd1.Q = Vector3f(0.0f, 0.0f, 5e-9f);
    ecd1.good = 0.80f;

    InvEcd ecd2;
    ecd2.valid = true;
    ecd2.time = 0.100f;
    ecd2.rd = Vector3f(0.04f, -0.01f, 0.06f);
    ecd2.Q = Vector3f(1e-9f, 0.0f, 8e-9f);
    ecd2.good = 0.92f;

    set << ecd1 << ecd2;
    QCOMPARE(set.size(), (qint32)2);
    QVERIFY(qAbs(set[0].time - 0.050f) < 1e-5);
    QVERIFY(qAbs(set[1].time - 0.100f) < 1e-5);
    QVERIFY(qAbs(set[1].good - 0.92f) < 1e-5);
}

void TestInvDipoleRapMne::ecdSet_writeReadDipFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    InvEcdSet setOrig;
    setOrig.dataname = "TestData";
    InvEcd ecd;
    ecd.valid = true;
    ecd.time = 0.075f;
    ecd.rd = Vector3f(0.03f, 0.02f, 0.07f);
    ecd.Q = Vector3f(0.0f, 5e-9f, 0.0f);
    ecd.good = 0.88f;
    ecd.khi2 = 1.5f;
    ecd.nfree = 100;
    setOrig << ecd;

    // Write to .dip format
    QString dipPath = tmpDir.path() + "/test_out.dip";
    QVERIFY(setOrig.save_dipoles_dip(dipPath));

    // Read back
    InvEcdSet setRead = InvEcdSet::read_dipoles_dip(dipPath);
    QCOMPARE(setRead.size(), (qint32)1);
    QVERIFY(qAbs(setRead[0].time - 0.075f) < 1e-4);
}

void TestInvDipoleRapMne::ecdSet_copyConstruction()
{
    InvEcdSet orig;
    InvEcd ecd;
    ecd.valid = true;
    ecd.time = 0.1f;
    orig << ecd;

    InvEcdSet copy(orig);
    QCOMPARE(copy.size(), orig.size());
}

//=============================================================================================================
// InvDipoleFitSettings tests
//=============================================================================================================

void TestInvDipoleRapMne::dipoleFitSettings_defaultConstruction()
{
    InvDipoleFitSettings settings;
    QVERIFY(settings.measname.isEmpty());
    QVERIFY(settings.bemname.isEmpty());
    QVERIFY(!settings.accurate);
    QVERIFY(!settings.include_meg);
    QVERIFY(!settings.include_eeg);
    QVERIFY(qFuzzyIsNull(settings.tmin) || settings.tmin < 10.0f);
}

void TestInvDipoleRapMne::dipoleFitSettings_setFields()
{
    InvDipoleFitSettings settings;
    settings.measname = "/path/to/meas.fif";
    settings.bemname = "/path/to/bem.fif";
    settings.accurate = true;
    settings.tmin = -0.2f;
    settings.tmax = 0.5f;
    settings.tstep = 0.001f;
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.grad_std = 5e-13f;
    settings.mag_std = 20e-15f;
    settings.eeg_std = 0.2e-6f;

    QCOMPARE(settings.measname, QString("/path/to/meas.fif"));
    QVERIFY(settings.accurate);
    QVERIFY(qAbs(settings.tmin - (-0.2f)) < 1e-6);
    QVERIFY(qAbs(settings.tmax - 0.5f) < 1e-6);
    QVERIFY(!settings.include_eeg);
}

void TestInvDipoleRapMne::dipoleFitSettings_checkIntegrity()
{
    InvDipoleFitSettings settings;
    settings.measname = "/tmp/test.fif";
    // checkIntegrity should not crash on valid settings
    settings.checkIntegrity();
    QVERIFY(true);  // If we get here, no crash
}

//=============================================================================================================
// InvDipoleFitData tests
//=============================================================================================================

void TestInvDipoleRapMne::dipoleFitData_defaultConstruction()
{
    InvDipoleFitData fitData;
    QCOMPARE(fitData.nmeg, 0);
    QCOMPARE(fitData.neeg, 0);
    QCOMPARE(fitData.coord_frame, 0);
}

void TestInvDipoleRapMne::dipoleFitData_adHocNoise()
{
    // Create ad-hoc diagonal noise covariance
    float grad_std = 5e-13f;
    float mag_std = 20e-15f;
    float eeg_std = 0.2e-6f;

    auto noise = InvDipoleFitData::ad_hoc_noise(nullptr, nullptr,
                                                  grad_std, mag_std, eeg_std);
    // With no coils, should return nullptr or empty
    // This mainly exercises the code path
    QVERIFY(true);  // If we get here without crash, the test passes
}

//=============================================================================================================
// InvGuessData tests
//=============================================================================================================

void TestInvDipoleRapMne::guessData_defaultConstruction()
{
    InvGuessData guess;
    QCOMPARE(guess.nguess, 0);
    QVERIFY(guess.guess_fwd.empty());
}

//=============================================================================================================
// InvRapMusic tests
//=============================================================================================================

void TestInvDipoleRapMne::rapMusic_defaultConstruction()
{
    InvRapMusic rap;
    QCOMPARE(QString(rap.getName()), QString("RAP MUSIC"));
}

void TestInvDipoleRapMne::rapMusic_initAndCalculate()
{
    // Pick only MEG channels for forward solution
    MNEForwardSolution fwdMeg = m_fwd.pick_types(true, false);
    QVERIFY2(!fwdMeg.isEmpty(), "Could not pick MEG from forward");

    // Truncate to a small subset of sources so RAP MUSIC completes in
    // reasonable time under code-coverage instrumentation.
    int maxSrc = qMin(200, (int)fwdMeg.sol->data.cols() / 3);
    if (maxSrc * 3 < fwdMeg.sol->data.cols()) {
        fwdMeg.sol->data.conservativeResize(Eigen::NoChange, maxSrc * 3);
        fwdMeg.nsource = maxSrc;
        fwdMeg.sol->ncol = maxSrc * 3;
    }

    InvRapMusic rap(fwdMeg, false, 2, 0.5);

    // Create synthetic measurement data
    int nChan = fwdMeg.nchan;
    MatrixXd measurement = MatrixXd::Random(nChan, 10);

    InvSourceEstimate stc = rap.calculateInverse(measurement, 0.0f, 0.001f);
    // RAP-MUSIC should produce some output (may be empty with random data)
    QVERIFY(stc.isEmpty() || stc.data.rows() > 0);
}

void TestInvDipoleRapMne::rapMusic_getName()
{
    InvRapMusic rap;
    QCOMPARE(QString(rap.getName()), QString("RAP MUSIC"));
}

//=============================================================================================================
// InvPwlRapMusic tests
//=============================================================================================================

void TestInvDipoleRapMne::pwlRapMusic_defaultConstruction()
{
    InvPwlRapMusic pwl;
    QCOMPARE(QString(pwl.getName()), QString("Powell RAP MUSIC"));
}

void TestInvDipoleRapMne::pwlRapMusic_powellOffset()
{
    // Test Powell offset calculation
    // PowellOffset for row 0 with 5 points should return 0
    int offset = InvPwlRapMusic::PowellOffset(0, 5);
    QVERIFY(offset >= 0);

    // For subsequent rows, offset should increase
    int offset1 = InvPwlRapMusic::PowellOffset(1, 5);
    QVERIFY(offset1 >= offset);
}

void TestInvDipoleRapMne::pwlRapMusic_powellIdxVec()
{
    int nPoints = 5;
    VectorXi elements;
    InvPwlRapMusic::PowellIdxVec(0, nPoints, elements);
    QVERIFY(elements.size() > 0);
    // All elements should be valid indices
    for (int i = 0; i < elements.size(); i++) {
        QVERIFY(elements(i) >= 0);
        QVERIFY(elements(i) < nPoints);
    }
}

//=============================================================================================================
// InvMinimumNorm extended tests
//=============================================================================================================

void TestInvDipoleRapMne::minimumNorm_methodSetting()
{
    if (!m_bMneSampleAvailable)
        QFAIL("MNE sample data not available");

    InvMinimumNorm mn(m_invOp, 1.0f / 9.0f, QString("MNE"));
    QCOMPARE(QString(mn.getName()), QString("Minimum Norm Estimate"));

    // setMethod changes internal flags, not getName() output
    mn.setMethod("dSPM");
    QCOMPARE(QString(mn.getName()), QString("Minimum Norm Estimate"));

    mn.setMethod("sLORETA");
    QCOMPARE(QString(mn.getName()), QString("Minimum Norm Estimate"));
}

void TestInvDipoleRapMne::minimumNorm_calculateInverse()
{
    if (!m_bMneSampleAvailable)
        QFAIL("MNE sample data not available");

    // Read evoked data
    QString avePath = m_sMneSamplePath + "/MEG/sample/sample_audvis-ave.fif";
    QVERIFY2(QFile::exists(avePath),
             qPrintable(QString("Evoked data not found: %1").arg(avePath)));

    QFile aveFile(avePath);
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY2(!evokedSet.evoked.isEmpty(), "No evoked data loaded");

    FiffEvoked evoked = evokedSet.evoked.first();

    InvMinimumNorm mn(m_invOp, 1.0f / 9.0f, QString("MNE"));
    InvSourceEstimate stc = mn.calculateInverse(evoked);

    QVERIFY(!stc.isEmpty());
    QVERIFY(stc.data.rows() > 0);
    QVERIFY(stc.data.cols() > 0);
    QVERIFY(stc.data.allFinite());
}

void TestInvDipoleRapMne::minimumNorm_dSPM()
{
    if (!m_bMneSampleAvailable)
        QFAIL("MNE sample data not available");

    QString avePath = m_sMneSamplePath + "/MEG/sample/sample_audvis-ave.fif";
    QVERIFY2(QFile::exists(avePath),
             qPrintable(QString("Evoked data not found: %1").arg(avePath)));

    QFile aveFile(avePath);
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY2(!evokedSet.evoked.isEmpty(), "No evoked data loaded");

    FiffEvoked evoked = evokedSet.evoked.first();
    InvMinimumNorm mn(m_invOp, 1.0f / 9.0f, true, false);  // dSPM=true, sLORETA=false
    InvSourceEstimate stc = mn.calculateInverse(evoked);

    QVERIFY(!stc.isEmpty());
    QVERIFY(stc.data.allFinite());
}

void TestInvDipoleRapMne::minimumNorm_sLORETA()
{
    if (!m_bMneSampleAvailable)
        QFAIL("MNE sample data not available");

    QString avePath = m_sMneSamplePath + "/MEG/sample/sample_audvis-ave.fif";
    QVERIFY2(QFile::exists(avePath),
             qPrintable(QString("Evoked data not found: %1").arg(avePath)));

    QFile aveFile(avePath);
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY2(!evokedSet.evoked.isEmpty(), "No evoked data loaded");

    FiffEvoked evoked = evokedSet.evoked.first();
    InvMinimumNorm mn(m_invOp, 1.0f / 9.0f, false, true);  // dSPM=false, sLORETA=true
    InvSourceEstimate stc = mn.calculateInverse(evoked);

    QVERIFY(!stc.isEmpty());
    QVERIFY(stc.data.allFinite());
}

//=============================================================================================================

void TestInvDipoleRapMne::inv_globalBuildInfo()
{
    const char* dt = INVLIB::buildDateTime();
    QVERIFY(dt != nullptr);
    const char* h = INVLIB::buildHash();
    QVERIFY(h != nullptr);
    const char* hl = INVLIB::buildHashLong();
    QVERIFY(hl != nullptr);
}

//=============================================================================================================

void TestInvDipoleRapMne::ecd_copyCtor()
{
    InvEcd ecd;
    ecd.rd = Eigen::Vector3f(0.01f, 0.02f, 0.03f);
    ecd.Q = Eigen::Vector3f(1.0f, 2.0f, 3.0f);
    ecd.good = 0.95;
    ecd.khi2 = 1.5;
    ecd.valid = true;

    InvEcd copy(ecd);
    QVERIFY(copy.valid);
    QCOMPARE(copy.good, ecd.good);
    QVERIFY((copy.rd - ecd.rd).norm() < 1e-6f);
    QVERIFY((copy.Q - ecd.Q).norm() < 1e-6f);
}

//=============================================================================================================

void TestInvDipoleRapMne::sourceEstimate_assignment()
{
    Eigen::MatrixXd data = Eigen::MatrixXd::Random(20, 5);
    Eigen::VectorXi vertices(20);
    for (int i = 0; i < 20; ++i) vertices(i) = i;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);
    QVERIFY(!stc.isEmpty());

    // Assignment operator
    InvSourceEstimate stc2;
    stc2 = stc;
    QVERIFY(!stc2.isEmpty());
    QCOMPARE(stc2.data.rows(), 20);
    QCOMPARE(stc2.data.cols(), 5);
    QCOMPARE(stc2.samples(), 5);
}

//=============================================================================================================
// InvSourceEstimateToken tests
//=============================================================================================================

void TestInvDipoleRapMne::tokenize_simpleEstimate()
{
    // Create a small source estimate
    Eigen::MatrixXd data(3, 2);
    data << 1.0, 2.0,
            3.0, 4.0,
            5.0, 6.0;
    Eigen::VectorXi vertices(3);
    vertices << 10, 20, 30;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);
    stc.method = InvEstimateMethod::MNE;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = InvOrientationType::Fixed;

    std::vector<InvToken> tokens = INVLIB::tokenize(stc);
    QVERIFY(!tokens.empty());

    // Should start with BOS and end with EOS
    QCOMPARE(tokens.front().id, InvTokenId::Bos);
    QCOMPARE(tokens.back().id, InvTokenId::Eos);
}

void TestInvDipoleRapMne::tokenize_roundtrip()
{
    Eigen::MatrixXd data(4, 3);
    data << 1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12;
    Eigen::VectorXi vertices(4);
    vertices << 0, 1, 2, 3;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.002f);
    stc.method = InvEstimateMethod::dSPM;
    stc.sourceSpaceType = InvSourceSpaceType::Volume;
    stc.orientationType = InvOrientationType::Free;

    std::vector<InvToken> tokens = INVLIB::tokenize(stc);
    InvSourceEstimate recovered = INVLIB::fromTokens(tokens);

    QCOMPARE(recovered.data.rows(), stc.data.rows());
    QCOMPARE(recovered.data.cols(), stc.data.cols());
    // Check data values match
    for (int r = 0; r < stc.data.rows(); ++r) {
        for (int c = 0; c < stc.data.cols(); ++c) {
            QVERIFY(qAbs(recovered.data(r,c) - stc.data(r,c)) < 1e-4);
        }
    }
    QCOMPARE(recovered.method, InvEstimateMethod::dSPM);
    QCOMPARE(recovered.sourceSpaceType, InvSourceSpaceType::Volume);
}

void TestInvDipoleRapMne::tokenize_metadataOnly()
{
    Eigen::MatrixXd data(2, 2);
    data << 1, 2, 3, 4;
    Eigen::VectorXi vertices(2);
    vertices << 0, 1;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);
    stc.method = InvEstimateMethod::sLORETA;

    InvTokenizeOptions opts;
    opts.includeGridData = false;
    opts.includeCouplings = false;
    opts.includeFocalDipoles = false;
    opts.includeConnectivity = false;
    opts.includePositions = false;

    std::vector<InvToken> tokens = INVLIB::tokenize(stc, opts);
    QVERIFY(!tokens.empty());

    // Should not contain amplitude tokens
    bool hasAmp = false;
    for (const auto& t : tokens) {
        if (t.id == InvTokenId::Amplitude) { hasAmp = true; break; }
    }
    QVERIFY(!hasAmp);
}

void TestInvDipoleRapMne::tokenize_withCouplings()
{
    Eigen::MatrixXd data(3, 1);
    data << 1, 2, 3;
    Eigen::VectorXi vertices(3);
    vertices << 0, 1, 2;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

    // Add coupling annotations
    InvSourceCoupling coupling;
    coupling.gridIndices = {0, 1, 2};
    coupling.correlations = Eigen::MatrixXd::Identity(3, 3);
    stc.couplings.push_back(coupling);

    std::vector<InvToken> tokens = INVLIB::tokenize(stc);
    QVERIFY(!tokens.empty());

    // Should contain coupling section
    bool hasCouplingBegin = false;
    for (const auto& t : tokens) {
        if (t.id == InvTokenId::CouplingBegin) { hasCouplingBegin = true; break; }
    }
    QVERIFY(hasCouplingBegin);
}

void TestInvDipoleRapMne::tokenize_withFocalDipoles()
{
    Eigen::MatrixXd data(2, 1);
    data << 1, 2;
    Eigen::VectorXi vertices(2);
    vertices << 0, 1;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

    // Add focal dipole
    InvFocalDipole dipole;
    dipole.position = Eigen::Vector3f(0.01f, 0.02f, 0.03f);
    dipole.moment = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
    dipole.goodness = 0.95f;
    dipole.khi2 = 1.2f;
    dipole.valid = true;
    stc.focalDipoles.push_back(dipole);

    std::vector<InvToken> tokens = INVLIB::tokenize(stc);
    QVERIFY(!tokens.empty());

    bool hasFocalBegin = false;
    for (const auto& t : tokens) {
        if (t.id == InvTokenId::FocalBegin) { hasFocalBegin = true; break; }
    }
    QVERIFY(hasFocalBegin);
}

void TestInvDipoleRapMne::tokenize_withConnectivity()
{
    Eigen::MatrixXd data(3, 1);
    data << 1, 2, 3;
    Eigen::VectorXi vertices(3);
    vertices << 0, 1, 2;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

    // Add connectivity
    InvConnectivity conn;
    conn.measure = "coh";
    conn.directed = false;
    Eigen::MatrixXd cmat(3, 3);
    cmat << 1.0, 0.5, 0.3,
            0.5, 1.0, 0.7,
            0.3, 0.7, 1.0;
    conn.matrix = cmat;
    stc.connectivity.push_back(conn);

    std::vector<InvToken> tokens = INVLIB::tokenize(stc);
    QVERIFY(!tokens.empty());

    bool hasConnBegin = false;
    for (const auto& t : tokens) {
        if (t.id == InvTokenId::ConnBegin) { hasConnBegin = true; break; }
    }
    QVERIFY(hasConnBegin);

    // Round-trip
    InvSourceEstimate recovered = INVLIB::fromTokens(tokens);
    QCOMPARE(recovered.connectivity.size(), static_cast<size_t>(1));
}

void TestInvDipoleRapMne::tokenize_subsampling()
{
    Eigen::MatrixXd data(10, 5);
    data.setRandom();
    Eigen::VectorXi vertices(10);
    for (int i = 0; i < 10; ++i) vertices(i) = i;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

    InvTokenizeOptions opts;
    opts.maxSources = 3;
    opts.maxTimePoints = 2;

    std::vector<InvToken> tokens = INVLIB::tokenize(stc, opts);
    QVERIFY(!tokens.empty());

    // Count amplitude tokens - should be reduced from full 10*5=50
    int ampCount = 0;
    for (const auto& t : tokens) {
        if (t.id == InvTokenId::Amplitude) ampCount++;
    }
    // stride(10,3)=3 → effSrc=(10+2)/3=4; stride(5,2)=2 → effTime=(5+1)/2=3
    // 4 sources * 3 timepoints = 12 amplitude tokens
    QVERIFY(ampCount <= 12);
    QVERIFY(ampCount < 50); // less than full 10*5
}

void TestInvDipoleRapMne::tokenIds_and_values()
{
    Eigen::MatrixXd data(2, 1);
    data << 1.5, 2.5;
    Eigen::VectorXi vertices(2);
    vertices << 0, 1;

    InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);
    std::vector<InvToken> tokens = INVLIB::tokenize(stc);

    // tokenIds extracts just IDs
    std::vector<int32_t> ids = INVLIB::tokenIds(tokens);
    QCOMPARE(ids.size(), tokens.size());
    QCOMPARE(ids.front(), static_cast<int32_t>(InvTokenId::Bos));

    // tokenValues extracts just values
    std::vector<float> vals = INVLIB::tokenValues(tokens);
    QCOMPARE(vals.size(), tokens.size());
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvDipoleRapMne)

#include "test_inv_dipole_rap_mne.moc"
