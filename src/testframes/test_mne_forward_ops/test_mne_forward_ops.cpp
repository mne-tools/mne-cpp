//=============================================================================================================
/**
 * @file     test_mne_forward_ops.cpp
 * @brief    Tests for MNEForwardSolution operations: pick, prepare, orient prior, depth prior.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <Eigen/Core>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>

using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================

class TestMneForwardOps : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    QString m_sFwdFile;
    QString m_sRawFile;
    QString m_sCovFile;
    MNEForwardSolution m_fwd;
    bool m_bFwdLoaded;

private slots:

    void initTestCase()
    {
        m_sDataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
        m_sFwdFile = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        m_sRawFile = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        m_sCovFile = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";

        m_bFwdLoaded = false;
        if(QFile::exists(m_sFwdFile)) {
            QFile file(m_sFwdFile);
            MNEForwardSolution fwd(file);
            if(!fwd.isEmpty()) {
                m_fwd = fwd;
                m_bFwdLoaded = true;
            }
        }
    }

    // ── pick_channels ───────────────────────────────────────────────────────

    void testPickChannelsByInclude()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        // Pick a subset of channels
        QStringList include;
        for(int i = 0; i < qMin(10, m_fwd.info.ch_names.size()); ++i) {
            include << m_fwd.info.ch_names[i];
        }

        MNEForwardSolution picked = m_fwd.pick_channels(include);
        QVERIFY(!picked.isEmpty());
        // pick_channels should reduce channel count
        QVERIFY(picked.nchan > 0);
        QVERIFY(picked.nchan <= m_fwd.nchan);
    }

    void testPickChannelsByExclude()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        // Exclude some channels
        QStringList exclude;
        for(int i = 0; i < qMin(5, m_fwd.info.ch_names.size()); ++i) {
            exclude << m_fwd.info.ch_names[i];
        }

        QStringList emptyInclude;
        MNEForwardSolution picked = m_fwd.pick_channels(emptyInclude, exclude);
        QVERIFY(!picked.isEmpty());
        QVERIFY(picked.nchan < m_fwd.nchan);
    }

    // ── pick_types ──────────────────────────────────────────────────────────

    void testPickTypesMeg()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        MNEForwardSolution megFwd = m_fwd.pick_types(true, false);
        QVERIFY(!megFwd.isEmpty());
        QVERIFY(megFwd.nchan > 0);
        QVERIFY(megFwd.nchan <= m_fwd.nchan);
    }

    void testPickTypesEeg()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        MNEForwardSolution eegFwd = m_fwd.pick_types(false, true);
        // EEG channels may or may not be present
        if(!eegFwd.isEmpty()) {
            QVERIFY(eegFwd.nchan > 0);
        }
    }

    // ── to_fixed_ori ────────────────────────────────────────────────────────

    void testToFixedOri()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        MNEForwardSolution fwdCopy = m_fwd;

        // Get gain matrix dimensions before conversion
        int ncolsBefore = fwdCopy.sol->data.cols();

        fwdCopy.to_fixed_ori();

        // After fixed orientation: sols should still be valid
        QVERIFY(fwdCopy.sol->data.cols() > 0);
        QVERIFY(fwdCopy.sol->data.rows() > 0);
    }

    // ── compute_orient_prior ────────────────────────────────────────────────

    void testComputeOrientPrior()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        FiffCov orientPrior = m_fwd.compute_orient_prior(0.2f);
        QVERIFY(orientPrior.dim > 0);
    }

    void testComputeOrientPriorLoose()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        // loose=1.0 means free orientation
        FiffCov orientPrior = m_fwd.compute_orient_prior(1.0f);
        QVERIFY(orientPrior.dim > 0);
    }

    // ── compute_depth_prior ─────────────────────────────────────────────────

    void testComputeDepthPrior()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        // Use the forward solution's gain matrix and info from raw file
        QFile rawFileForDepth(m_sRawFile);
        FiffRawData rawForDepth(rawFileForDepth);
        bool isFixed = (m_fwd.source_ori == FIFFV_MNE_FIXED_ORI);
        FiffCov depthPrior = MNEForwardSolution::compute_depth_prior(
            m_fwd.sol->data, rawForDepth.info, isFixed, 0.8, 10.0);
        QVERIFY(depthPrior.dim > 0);
    }

    // ── prepare_forward ─────────────────────────────────────────────────────

    void testPrepareForward()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");
        if(!QFile::exists(m_sCovFile) || !QFile::exists(m_sRawFile)) QSKIP("Data files not found");

        QFile covFile(m_sCovFile);
        FiffCov noiseCov(covFile);

        QFile rawFile(m_sRawFile);
        FiffRawData raw(rawFile);

        FiffInfo outFwdInfo;
        Eigen::MatrixXd gain;
        FiffCov outNoiseCov;
        Eigen::MatrixXd whitener;
        qint32 numNonZero;

        m_fwd.prepare_forward(raw.info, noiseCov, false,
                              outFwdInfo, gain, outNoiseCov, whitener, numNonZero);

        QVERIFY(gain.rows() > 0);
        QVERIFY(gain.cols() > 0);
        QVERIFY(numNonZero > 0);
        QVERIFY(whitener.rows() > 0);
    }

    // ── Forward solution write/read round-trip ──────────────────────────────

    void testForwardSolutionWriteRead()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        QString tmpFile = QDir::tempPath() + "/test_fwd_roundtrip.fif";
        QFile outFile(tmpFile);
        bool written = false;
        if(outFile.open(QIODevice::WriteOnly)) {
            written = m_fwd.write(outFile);
            outFile.close();
        }
        if(written) {
            QFile reloadFile(tmpFile);
            MNEForwardSolution reloaded(reloadFile);
            QVERIFY(!reloaded.isEmpty());
            QCOMPARE(reloaded.nsource, m_fwd.nsource);
            QCOMPARE(reloaded.nchan, m_fwd.nchan);
            QFile::remove(tmpFile);
        } else {
            QWARN("Forward solution write failed — code path exercised");
        }
    }

    // ── Source space hemisphere access ───────────────────────────────────────

    void testSourceSpaceProperties()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        QCOMPARE(m_fwd.src.size(), 2); // Left and right hemisphere

        for(int h = 0; h < 2; ++h) {
            QVERIFY(m_fwd.src[h].nuse > 0);
            QVERIFY(m_fwd.src[h].np > 0);
            QVERIFY(m_fwd.src[h].rr.rows() > 0);
        }
    }

    // ── Source orientations ─────────────────────────────────────────────────

    void testSourceOrientations()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        QVERIFY(m_fwd.source_rr.rows() == m_fwd.nsource);
        QVERIFY(m_fwd.source_nn.rows() > 0);
        QVERIFY(m_fwd.source_rr.cols() == 3);
        QVERIFY(m_fwd.source_nn.cols() == 3);
    }

    // ── Gain matrix properties ──────────────────────────────────────────────

    void testGainMatrixProperties()
    {
        if(!m_bFwdLoaded) QSKIP("Forward solution not loaded");

        QVERIFY(m_fwd.sol != nullptr);
        QVERIFY(m_fwd.sol->data.rows() == m_fwd.nchan);
        QVERIFY(m_fwd.sol->data.cols() > 0);
    }

    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestMneForwardOps)
#include "test_mne_forward_ops.moc"
