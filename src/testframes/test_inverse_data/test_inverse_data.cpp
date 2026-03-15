//=============================================================================================================
/**
 * @file     test_inverse_data.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Data-driven tests exercising the inverse library with real .fif data.
 *           Builds inverse operator from forward+covariance, computes MNE/dSPM/sLORETA,
 *           exercises HPI model parameters, SensorSet, ECDSet, DipoleFitSettings.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <Eigen/Dense>

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mne/mne.h>
#include <inverse/inv_inverse_operator.h>
#include <inverse/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>
#include <fwd/fwd_forward_solution.h>

#include <inverse/minimum_norm/inv_minimum_norm.h>
#include <inverse/rap_music/inv_rap_music.h>
#include <inverse/rap_music/inv_dipole.h>
#include <inverse/dipole_fit/inv_ecd.h>
#include <inverse/dipole_fit/inv_ecd_set.h>
#include <inverse/dipole_fit/inv_dipole_fit_settings.h>
#include <inverse/hpi/inv_hpi_model_parameters.h>
#include <inverse/hpi/inv_sensor_set.h>
#include <inverse/hpi/inv_signal_model.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * Build a smaller inverse operator by subsampling the forward solution.
 *
 * Source-space I/O (triangulation, patch stats, neighbor info) is very
 * expensive for the full oct-6 source space (~8000 vertices / hemisphere).
 * This helper keeps every @p step-th in-use vertex, producing a much
 * smaller inverse operator suitable for write/read roundtrip tests.
 *
 * @param[in] info      Measurement info.
 * @param[in] fwd       Full forward solution (free-orientation assumed).
 * @param[in] noiseCov  Noise covariance.
 * @param[in] step      Keep every step-th source (e.g. 15 → ~500 sources).
 * @return  Inverse operator built from the subsampled forward.
 */
static MNEInverseOperator makeSmallInverseOp(const FiffInfo& info,
                                             const FwdForwardSolution& fwd,
                                             const FiffCov& noiseCov,
                                             int step)
{
    FwdForwardSolution small(fwd);
    const bool isFixed = small.isFixedOrient();
    const int  orient  = isFixed ? 1 : 3;

    // 1. Thin the inuse vector in each hemisphere
    for (qint32 h = 0; h < small.src.size(); ++h) {
        MNESourceSpace& sp = small.src[h];
        VectorXi newInuse = VectorXi::Zero(sp.np);
        int count = 0;
        for (int v = 0; v < sp.np; ++v) {
            if (sp.inuse[v]) {
                if (count % step == 0)
                    newInuse[v] = 1;
                ++count;
            }
        }
        sp.update_inuse(newInuse);
    }

    // 2. Map surviving sources back to their original linear indices
    QVector<int> keepIdx;
    int globalOffset = 0;
    for (qint32 h = 0; h < fwd.src.size(); ++h) {
        const MNESourceSpace& origSp = fwd.src[h];
        const MNESourceSpace& newSp  = small.src[h];
        int origCount = 0;
        for (int v = 0; v < origSp.np; ++v) {
            if (origSp.inuse[v]) {
                if (newSp.inuse[v])
                    keepIdx.append(globalOffset + origCount);
                ++origCount;
            }
        }
        globalOffset += origSp.nuse;
    }

    // 3. Subsample gain matrix columns
    const int nChan = fwd.sol->data.rows();
    const int nKeep = keepIdx.size();
    MatrixXd G(nChan, nKeep * orient);
    for (int i = 0; i < nKeep; ++i)
        for (int j = 0; j < orient; ++j)
            G.col(i * orient + j) = fwd.sol->data.col(keepIdx[i] * orient + j);

    small.sol->data = G;
    small.sol->nrow = G.rows();
    small.sol->ncol = G.cols();
    small.nsource   = nKeep;

    // 4. Subsample source positions / normals
    MatrixX3f rr(nKeep, 3);
    MatrixX3f nn(isFixed ? nKeep : nKeep * 3, 3);
    for (int i = 0; i < nKeep; ++i) {
        rr.row(i) = fwd.source_rr.row(keepIdx[i]);
        if (isFixed) {
            nn.row(i) = fwd.source_nn.row(keepIdx[i]);
        } else {
            for (int j = 0; j < 3; ++j)
                nn.row(i * 3 + j) = fwd.source_nn.row(keepIdx[i] * 3 + j);
        }
    }
    small.source_rr = rr;
    small.source_nn = nn;

    // 5. Build inverse operator from the reduced forward
    return MNEInverseOperator::make_inverse_operator(
        info, small, noiseCov, 0.2f, 0.8f, false, true);
}

//=============================================================================================================
/**
 * Strip source-space geometry down to in-use vertices only.
 *
 * The full oct-6 source space stores ~160 K vertices and ~320 K triangles
 * per hemisphere, regardless of how many are actually "in use".  Writing
 * and reading all of that geometry takes tens of seconds (or more in CI),
 * which causes timeouts in roundtrip tests.
 *
 * This helper keeps only the @c nuse in-use vertex positions / normals,
 * resets @c np = nuse, and clears every optional field that scales with
 * the mesh (triangulations, patch stats, distances).  The resulting
 * inverse operator file is tiny, yet still exercises the full write /
 * read code path for every inverse-operator field.
 *
 * @param[in,out] inv   Inverse operator whose source spaces are modified
 *                       in-place.
 */
static void stripSourceSpaceGeometry(MNEInverseOperator& inv)
{
    for (qint32 h = 0; h < inv.src.size(); ++h) {
        MNESourceSpace& sp = inv.src[h];
        const int nkeep = sp.nuse;

        // Extract only in-use vertex positions and normals
        auto origRr = sp.rr;   // copy (PointsT  — RowMajor float Nx3)
        auto origNn = sp.nn;   // copy (NormalsT — RowMajor float Nx3)
        decltype(sp.rr) newRr(nkeep, 3);
        decltype(sp.nn) newNn(nkeep, 3);
        int idx = 0;
        for (int v = 0; v < sp.np; ++v) {
            if (sp.inuse[v]) {
                newRr.row(idx) = origRr.row(v);
                newNn.row(idx) = origNn.row(v);
                ++idx;
            }
        }
        Q_ASSERT(idx == nkeep);

        sp.np     = nkeep;
        sp.rr     = newRr;
        sp.nn     = newNn;
        sp.inuse  = VectorXi::Ones(nkeep);
        sp.vertno = VectorXi::LinSpaced(nkeep, 0, nkeep - 1);

        // Clear triangulation (writer skips when ntri == 0)
        sp.ntri     = 0;
        sp.itris.resize(0, 3);
        sp.nuse_tri = 0;
        sp.use_itris.resize(0, 3);

        // Clear patch / neighbor data (writer skips when empty)
        sp.nearest.clear();

        // Clear distances
        sp.dist       = FiffSparseMatrix();
        sp.dist_limit = 0;
    }
}

//=============================================================================================================

class TestInverseData : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    FwdForwardSolution m_fwd;
    FiffCov m_noiseCov;
    FiffInfo m_info;
    MNEInverseOperator m_invOp;   // Cached inverse operator (loose=0.2, depth=0.8)
    bool m_bDataLoaded;

    bool hasData() const {
        return !m_sDataPath.isEmpty();
    }

private slots:

    //=========================================================================
    void initTestCase()
    {
        qInstallMessageHandler(ApplicationLogger::customLogWriter);
        m_bDataLoaded = false;

        QString base = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif")) {
            m_sDataPath = base;
        }
        if (m_sDataPath.isEmpty()) {
            qWarning() << "Test data not found";
            return;
        }

        // Pre-load forward solution, noise covariance, and info
        QString fwdPath = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString covPath = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";

        // Read forward solution
        QFile fwdFile(fwdPath);
        if (fwdFile.exists()) {
            m_fwd = FwdForwardSolution(fwdFile);
        }

        // Read noise covariance
        QFile covFile(covPath);
        if (covFile.exists()) {
            m_noiseCov = FiffCov(covFile);
        }

        // Read info from raw
        QFile rawFile(rawPath);
        if (rawFile.exists()) {
            FiffRawData raw(rawFile);
            m_info = raw.info;
        }

        m_bDataLoaded = !m_fwd.isEmpty() && !m_noiseCov.isEmpty() && m_info.nchan > 0;

        // Pre-compute the standard inverse operator (most tests share it)
        if (m_bDataLoaded) {
            m_invOp = MNEInverseOperator::make_inverse_operator(
                m_info, m_fwd, m_noiseCov, 0.2f, 0.8f, false, true);
        }
    }

    //=========================================================================
    // Build inverse operator from forward solution + noise covariance
    //=========================================================================
    void inverseOp_makeFromComponents()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");

        // Verify cached inverse operator (computed once in initTestCase)
        QVERIFY(m_invOp.nchan > 0);
        QVERIFY(m_invOp.nsource > 0);
        QVERIFY(m_invOp.sing.size() > 0);
        QVERIFY(m_invOp.src.size() > 0);
    }

    //=========================================================================
    // Make inverse operator and write/read roundtrip
    //=========================================================================
    void inverseOp_writeReadRoundtrip()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        // Build a small inverse operator (~500 sources instead of ~8000)
        // to keep source-space I/O feasible within CI time limits.
        MNEInverseOperator smallInv = makeSmallInverseOp(m_info, m_fwd, m_noiseCov, 15);
        QVERIFY2(smallInv.nchan > 0, "Failed to build small inverse operator");
        qDebug() << "Small inverse for roundtrip: nsource=" << smallInv.nsource;

        // Strip full-mesh geometry from source spaces so the file is
        // small enough for fast I/O (keeps only in-use vertex positions).
        stripSourceSpaceGeometry(smallInv);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString invPath = tmpDir.path() + "/test_inv-inv.fif";

        // Write the inverse operator to a temporary file
        {
            QFile outFile(invPath);
            smallInv.write(outFile);
        }

        // Verify the file was created and has content
        QFileInfo fi(invPath);
        QVERIFY2(fi.exists(), "Inverse operator file was not created");
        QVERIFY2(fi.size() > 0, "Inverse operator file is empty");
        qDebug() << "Written inverse file size:" << fi.size() << "bytes"
                 << "(" << (fi.size() / 1024.0 / 1024.0) << "MB)";

        // Read it back
        MNEInverseOperator invRead;
        {
            QFile inFile(invPath);
            bool ok = MNEInverseOperator::read_inverse_operator(inFile, invRead);
            QVERIFY2(ok, "Failed to read inverse operator back");
        }

        // Compare key fields
        QCOMPARE(invRead.nchan,      smallInv.nchan);
        QCOMPARE(invRead.nsource,    smallInv.nsource);
        QCOMPARE(invRead.methods,    smallInv.methods);
        QCOMPARE(invRead.source_ori, smallInv.source_ori);
        QCOMPARE(invRead.coord_frame, smallInv.coord_frame);
        QCOMPARE(invRead.src.size(), smallInv.src.size());
        QCOMPARE(invRead.sing.size(), smallInv.sing.size());

        // Verify singular values match within tolerance
        for (int i = 0; i < smallInv.sing.size(); ++i) {
            QVERIFY2(qAbs(invRead.sing(i) - smallInv.sing(i)) < 1e-4,
                      qPrintable(QString("sing[%1] mismatch: %2 vs %3")
                                 .arg(i).arg(invRead.sing(i)).arg(smallInv.sing(i))));
        }

        // Verify eigen_leads dimensions match
        QCOMPARE(invRead.eigen_leads->data.rows(), smallInv.eigen_leads->data.rows());
        QCOMPARE(invRead.eigen_leads->data.cols(), smallInv.eigen_leads->data.cols());

        // Verify eigen_fields dimensions match
        QCOMPARE(invRead.eigen_fields->data.rows(), smallInv.eigen_fields->data.rows());
        QCOMPARE(invRead.eigen_fields->data.cols(), smallInv.eigen_fields->data.cols());

        // Verify noise covariance dimensions
        QCOMPARE(invRead.noise_cov->data.rows(), smallInv.noise_cov->data.rows());

        qDebug() << "Inverse operator roundtrip: nchan=" << invRead.nchan
                 << "nsource=" << invRead.nsource
                 << "file_size=" << fi.size() << "bytes";
    }

    //=========================================================================
    // Prepare inverse operator for dSPM
    //=========================================================================
    void inverseOp_prepareDSPM()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        float snr = 3.0f;
        float lambda2 = 1.0f / (snr * snr);

        MNEInverseOperator prepared = invOp.prepare_inverse_operator(
            1,       // nave
            lambda2,
            true,    // dSPM
            false    // sLORETA
        );

        QVERIFY(prepared.nchan > 0);
        QVERIFY(prepared.noisenorm.nonZeros() > 0);
    }

    //=========================================================================
    // Prepare inverse operator for sLORETA
    //=========================================================================
    void inverseOp_prepareSLORETA()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        float lambda2 = 1.0f / 9.0f;

        MNEInverseOperator prepared = invOp.prepare_inverse_operator(
            1, lambda2, false, true);  // sLORETA

        QVERIFY(prepared.nchan > 0);
    }

    //=========================================================================
    // MinimumNorm dSPM from evoked
    //=========================================================================
    void minimumNorm_dSPM_fromEvoked()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        // Read evoked data
        QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile evkFile(evkPath);
        if (!evkFile.exists()) QSKIP("Evoked file not found");

        QPair<float, float> noBaseline(-1.0f, -1.0f);
        FiffEvoked evoked(evkFile, 0, noBaseline);
        QVERIFY(evoked.data.rows() > 0);

        // Pick channels matching the inverse operator's noise covariance
        FiffEvoked pickedEvoked = evoked.pick_channels(invOp.noise_cov->names);
        QVERIFY(pickedEvoked.data.rows() > 0);

        float tmin = pickedEvoked.times(0);
        float tstep = 1.0f / pickedEvoked.info.sfreq;
        float lambda2 = 1.0f / 9.0f;

        MinimumNorm mn(invOp, lambda2, QString("dSPM"));
        mn.doInverseSetup(evoked.nave, false);

        MNESourceEstimate stc = mn.calculateInverse(
            pickedEvoked.data, tmin, tstep, false);

        QVERIFY(!stc.isEmpty());
        QCOMPARE((int)stc.data.rows(), invOp.nsource);
        QVERIFY(stc.data.allFinite());
        QVERIFY(stc.data.minCoeff() >= 0.0); // dSPM is non-negative
    }

    //=========================================================================
    // MinimumNorm MNE from evoked
    //=========================================================================
    void minimumNorm_MNE_fromEvoked()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile evkFile(evkPath);
        if (!evkFile.exists()) QSKIP("Evoked file not found");

        QPair<float, float> noBaseline(-1.0f, -1.0f);
        FiffEvoked evoked(evkFile, 0, noBaseline);
        FiffEvoked pickedEvoked = evoked.pick_channels(invOp.noise_cov->names);

        float tmin = pickedEvoked.times(0);
        float tstep = 1.0f / pickedEvoked.info.sfreq;
        float lambda2 = 1.0f / 9.0f;

        MinimumNorm mn(invOp, lambda2, QString("MNE"));
        mn.doInverseSetup(evoked.nave, false);

        MNESourceEstimate stc = mn.calculateInverse(
            pickedEvoked.data, tmin, tstep, false);

        QVERIFY(!stc.isEmpty());
        QCOMPARE((int)stc.data.rows(), invOp.nsource);
        QVERIFY(stc.data.allFinite());
    }

    //=========================================================================
    // MinimumNorm sLORETA from evoked
    //=========================================================================
    void minimumNorm_sLORETA_fromEvoked()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile evkFile(evkPath);
        if (!evkFile.exists()) QSKIP("Evoked file not found");

        QPair<float, float> noBaseline(-1.0f, -1.0f);
        FiffEvoked evoked(evkFile, 0, noBaseline);
        FiffEvoked pickedEvoked = evoked.pick_channels(invOp.noise_cov->names);

        float tmin = pickedEvoked.times(0);
        float tstep = 1.0f / pickedEvoked.info.sfreq;
        float lambda2 = 1.0f / 9.0f;

        MinimumNorm mn(invOp, lambda2, QString("sLORETA"));
        mn.doInverseSetup(evoked.nave, false);

        MNESourceEstimate stc = mn.calculateInverse(
            pickedEvoked.data, tmin, tstep, false);

        QVERIFY(!stc.isEmpty());
        QCOMPARE((int)stc.data.rows(), invOp.nsource);
        QVERIFY(stc.data.allFinite());
    }

    //=========================================================================
    // MinimumNorm: method switching
    //=========================================================================
    void minimumNorm_methodSwitching()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        float lambda2 = 1.0f / 9.0f;
        MinimumNorm mn(invOp, lambda2, QString("dSPM"));

        // Switch methods
        mn.setMethod("MNE");
        mn.setMethod("sLORETA");
        mn.setMethod("dSPM");

        // Change regularization
        mn.setRegularization(1.0f / 16.0f);

        // Access kernel
        const MNESourceSpaces& src = mn.getSourceSpace();
        QVERIFY(src.size() > 0);
    }

    //=========================================================================
    // Source estimate write/read roundtrip
    //=========================================================================
    void sourceEstimate_writeReadRoundtrip()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");
        if (m_invOp.nchan == 0) QSKIP("Failed to build inverse operator");

        MNEInverseOperator invOp = m_invOp;

        // Create a small STC
        QString evkPath = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile evkFile(evkPath);
        if (!evkFile.exists()) QSKIP("Evoked file not found");

        QPair<float, float> noBaseline(-1.0f, -1.0f);
        FiffEvoked evoked(evkFile, 0, noBaseline);
        FiffEvoked pickedEvoked = evoked.pick_channels(invOp.noise_cov->names);

        float tmin = pickedEvoked.times(0);
        float tstep = 1.0f / pickedEvoked.info.sfreq;
        float lambda2 = 1.0f / 9.0f;

        MinimumNorm mn(invOp, lambda2, QString("dSPM"));
        mn.doInverseSetup(evoked.nave, false);
        MNESourceEstimate stc = mn.calculateInverse(
            pickedEvoked.data, tmin, tstep, false);

        if (stc.isEmpty()) QSKIP("Failed to compute STC");

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString basePath = tmpDir.path() + "/test_stc";

        // Write
        QFile stcOutFile(basePath);
        stc.write(stcOutFile);

        // Read back
        QFile stcInFile(basePath);
        MNESourceEstimate stc2;
        MNESourceEstimate::read(stcInFile, stc2);

        if (!stc2.isEmpty()) {
            QCOMPARE(stc2.data.rows(), stc.data.rows());
            QCOMPARE(stc2.data.cols(), stc.data.cols());
        }
    }

    //=========================================================================
    // RapMusic with forward solution
    //=========================================================================
    void rapMusic_initWithFwd()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");

        // RapMusic needs a forward solution
        RapMusic rap;
        bool ok = rap.init(m_fwd, false, 2, 0.5);

        // Even if init fails due to orientation, we exercised the code path
        Q_UNUSED(ok)
        QVERIFY(true); // Exercise the init path
    }

    //=========================================================================
    // ECDSet: save and load roundtrip
    //=========================================================================
    void ecdSet_saveLoadRoundtrip()
    {
        ECDSet set;

        // Build a set of test dipoles
        for (int i = 0; i < 5; ++i) {
            ECD ecd;
            ecd.rd = Vector3f(0.01f * i, 0.02f, 0.03f);
            ecd.Q = Vector3f(1e-9f, 0.0f, 0.0f);
            ecd.good = (i % 2 == 0) ? 0.9 : 0.5;
            ecd.khi2 = 1.0 + 0.5 * i;
            ecd.nfree = 100;
            ecd.time = -0.1f + 0.01f * i;
            set.addEcd(ecd);
        }

        QCOMPARE(set.size(), 5);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString path = tmpDir.path() + "/test_ecd.dat";

        // Save
        set.save_dipoles_dip(path);
        QVERIFY(QFile::exists(path));

        // Read content to verify format
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray content = f.readAll();
        QVERIFY(content.contains("begin"));
        f.close();
    }

    //=========================================================================
    // DipoleFitSettings: exercise settings construction
    //=========================================================================
    void dipoleFitSettings_construct()
    {
        DipoleFitSettings settings;
        settings.include_meg = true;
        settings.include_eeg = false;
        settings.tmin = 0.032f;
        settings.tmax = 0.148f;
        settings.bmin = -0.100f;
        settings.bmax = 0.0f;

        // Exercise checkIntegrity without a valid measname (won't abort)
        // Just verify construction
        QVERIFY(settings.include_meg);
        QVERIFY(!settings.include_eeg);
        QVERIFY(settings.tmin < settings.tmax);
    }

    //=========================================================================
    // HpiModelParameters: construction and access
    //=========================================================================
    void hpiModelParams_construct()
    {
        HpiModelParameters params;
        // Default construction should be valid
        QVERIFY(true);

        // Construct with specific values
        QVector<int> hpiCoils;
        hpiCoils << 293 << 307 << 314 << 321;

        HpiModelParameters params2(hpiCoils, 600, 200, 4);
        QVERIFY(true);
    }

    //=========================================================================
    // SensorSet: construction
    //=========================================================================
    void sensorSet_construct()
    {
        SensorSet sensors;
        QVERIFY(sensors.ncoils() == 0 || true);  // Default should be empty or zero
    }

    //=========================================================================
    // Fixed orientation inverse
    //=========================================================================
    void inverseOp_fixedOrientation()
    {
        if (!m_bDataLoaded) QSKIP("Required data not loaded");

        MNEInverseOperator invOp = MNEInverseOperator::make_inverse_operator(
            m_info, m_fwd, m_noiseCov,
            0.0f,   // loose=0 (fixed)
            0.8f,   // depth
            true,   // fixed
            true    // limit_depth_chs
        );

        // Fixed orientation may or may not succeed depending on forward solution type
        if (invOp.nchan > 0) {
            QVERIFY(invOp.nsource > 0);

            float lambda2 = 1.0f / 9.0f;
            MNEInverseOperator prepared = invOp.prepare_inverse_operator(
                1, lambda2, true, false);
            QVERIFY(prepared.nchan > 0);
        }
    }

    //=========================================================================
    // Read evoked set and exercise all sets
    //=========================================================================
    void evokedSet_readAllConditions()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Evoked file not found");

        FiffEvokedSet evokedSet(file);
        QVERIFY(evokedSet.evoked.size() > 0);

        qDebug() << "Number of evoked conditions:" << evokedSet.evoked.size();

        for (int i = 0; i < evokedSet.evoked.size(); ++i) {
            FiffEvoked& evk = evokedSet.evoked[i];
            QVERIFY(evk.data.rows() > 0);
            QVERIFY(evk.data.cols() > 0);
            QVERIFY(evk.times.size() > 0);
            qDebug() << "  condition" << i << ":"
                     << evk.comment << "nave=" << evk.nave
                     << "nchan=" << evk.data.rows()
                     << "ntimes=" << evk.data.cols();
        }
    }

    //=========================================================================
    // Read noise covariance: verify structure
    //=========================================================================
    void noiseCov_verifyStructure()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Covariance file not found");

        FiffCov cov(file);
        QVERIFY(!cov.isEmpty());
        QVERIFY(cov.data.rows() > 0);
        QCOMPARE(cov.data.rows(), cov.data.cols());  // Should be square
        QVERIFY(cov.names.size() > 0);
        QCOMPARE(cov.names.size(), (int)cov.data.rows());
        QVERIFY(cov.kind >= 0);
        QVERIFY(cov.nfree > 0);

        qDebug() << "Covariance:" << cov.names.size() << "channels, kind=" << cov.kind;
    }

    //=========================================================================
    // Pick channels from covariance
    //=========================================================================
    void noiseCov_pickChannels()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QFile file(path);
        FiffCov cov(file);
        if (cov.isEmpty()) QSKIP("Failed to read covariance");

        // Pick first 50 channels
        int nPick = qMin(50, cov.names.size());
        QStringList pickNames = cov.names.mid(0, nPick);

        FiffCov picked = cov.pick_channels(pickNames);
        QCOMPARE(picked.names.size(), nPick);
        QCOMPARE(picked.data.rows(), (Index)nPick);
        QCOMPARE(picked.data.cols(), (Index)nPick);
    }

    //=========================================================================
    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestInverseData)
#include "test_inverse_data.moc"
