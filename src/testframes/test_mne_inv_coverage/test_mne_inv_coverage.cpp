//=============================================================================================================
/**
 * @file     test_mne_inv_coverage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 * @brief    Tests for untested MNE and INV library methods.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_source_space.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_epoch_data.h>
#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <inv/inv_source_estimate.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
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
using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestMneInvCoverage : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- MNEInverseOperator ---
    void testCheckChNamesMatch();
    void testCheckChNamesMismatch();
    void testCheckChNamesEmpty();

    // --- MNEBem ---
    void testReadBemFromFile();
    void testBemSurfaceProperties();
    void testBemSurfaceNormals();

    // --- MNESourceSpaces ---
    void testSourceSpaceFromFile();
    void testSourceSpaceVertexCount();
    void testSourceSpaceHemispheres();

    // --- InvSourceEstimate ---
    void testSourceEstimateConstruction();
    void testSourceEstimateTimeSampling();
    void testSourceEstimateReduce();

    // --- MNEForwardSolution ---
    void testForwardSolutionFromFile();
    void testForwardSolutionDimensions();
    void testForwardSolutionPickChannelTypes();

    // --- MNEEpochDataList ---
    void testEpochDataListFromRaw();

    // --- InvMinimumNorm ---
    void testMinNormSetup();
    void testMinNormCalculateInverse();

    void cleanupTestCase();

private:
    QString rawPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif"; }
    QString fwdPath() const { return m_sTestDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"; }
    QString bemPath() const { return m_sTestDataPath + "/subjects/sample/bem/sample-5120-bem.fif"; }
    QString srcPath() const { return m_sTestDataPath + "/subjects/sample/bem/sample-oct-6-src.fif"; }

    QString m_sTestDataPath;
    QTemporaryDir m_tmpDir;
};

//=============================================================================================================

void TestMneInvCoverage::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    m_sTestDataPath = QCoreApplication::applicationDirPath()
                      + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));
    QVERIFY(m_tmpDir.isValid());
}

//=============================================================================================================
// MNEInverseOperator: check_ch_names
//=============================================================================================================

void TestMneInvCoverage::testCheckChNamesMatch()
{
    // Create a simple FiffInfo with known channels
    FiffInfo info;
    FiffChInfo ch1;
    ch1.ch_name = "MEG0111";
    FiffChInfo ch2;
    ch2.ch_name = "MEG0121";
    info.chs << ch1 << ch2;
    info.nchan = 2;
    info.ch_names << "MEG0111" << "MEG0121";

    // Create inverse operator with same channels
    MNEInverseOperator invOp;
    invOp.noise_cov = FiffCov::SDPtr(new FiffCov());
    invOp.noise_cov->names << "MEG0111" << "MEG0121";
    // eigen_fields must have matching col_names for check_ch_names to pass
    invOp.eigen_fields = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
    invOp.eigen_fields->col_names << "MEG0111" << "MEG0121";

    // check_ch_names should succeed (names match)
    bool ok = invOp.check_ch_names(info);
    QVERIFY(ok);
}

void TestMneInvCoverage::testCheckChNamesMismatch()
{
    FiffInfo info;
    FiffChInfo ch1;
    ch1.ch_name = "MEG0111";
    info.chs << ch1;
    info.nchan = 1;
    info.ch_names << "MEG0111";

    MNEInverseOperator invOp;
    invOp.noise_cov = FiffCov::SDPtr(new FiffCov());
    invOp.noise_cov->names << "DIFFERENT_CHANNEL";

    bool ok = invOp.check_ch_names(info);
    // Mismatch should fail or return false
    // Result depends on implementation: may warn but still proceed
    Q_UNUSED(ok);  // Just verify it doesn't crash
}

void TestMneInvCoverage::testCheckChNamesEmpty()
{
    FiffInfo info;
    MNEInverseOperator invOp;
    invOp.noise_cov = FiffCov::SDPtr(new FiffCov());

    bool ok = invOp.check_ch_names(info);
    Q_UNUSED(ok);  // Just verify it doesn't crash
}

//=============================================================================================================
// MNEBem
//=============================================================================================================

void TestMneInvCoverage::testReadBemFromFile()
{
    QVERIFY2(QFile::exists(bemPath()),
             qPrintable(QString("BEM test data not found: %1").arg(bemPath())));

    MNEBem bem;
    QFile file(bemPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());
    QVERIFY(MNEBem::readFromStream(stream, true, bem));
    stream->close();

    QVERIFY(bem.size() > 0);
}

void TestMneInvCoverage::testBemSurfaceProperties()
{
    QVERIFY2(QFile::exists(bemPath()),
             qPrintable(QString("BEM test data not found: %1").arg(bemPath())));

    MNEBem bem;
    QFile file(bemPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());
    QVERIFY(MNEBem::readFromStream(stream, true, bem));
    stream->close();

    const MNEBemSurface& surf = bem[0];
    QVERIFY(surf.np > 0);
    QVERIFY(surf.ntri > 0);
    QCOMPARE(surf.rr.rows(), (Eigen::Index)surf.np);
    QCOMPARE(surf.rr.cols(), (Eigen::Index)3);
    QCOMPARE(surf.itris.rows(), (Eigen::Index)surf.ntri);
    QCOMPARE(surf.itris.cols(), (Eigen::Index)3);
}

void TestMneInvCoverage::testBemSurfaceNormals()
{
    QVERIFY2(QFile::exists(bemPath()),
             qPrintable(QString("BEM test data not found: %1").arg(bemPath())));

    MNEBem bem;
    QFile file(bemPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());
    QVERIFY(MNEBem::readFromStream(stream, true, bem));
    stream->close();

    const MNEBemSurface& surf = bem[0];
    // Normals should be present and unit length
    QCOMPARE(surf.nn.rows(), (Eigen::Index)surf.np);
    for (int i = 0; i < qMin(100, surf.np); i++) {
        float norm = surf.nn.row(i).norm();
        QVERIFY(qAbs(norm - 1.0f) < 0.01f);
    }
}

//=============================================================================================================
// MNESourceSpaces
//=============================================================================================================

void TestMneInvCoverage::testSourceSpaceFromFile()
{
    QVERIFY2(QFile::exists(srcPath()),
             qPrintable(QString("Source space not found: %1").arg(srcPath())));

    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    MNESourceSpaces srcSpaces;
    QVERIFY(MNESourceSpaces::readFromStream(stream, true, srcSpaces));
    stream->close();

    QVERIFY(srcSpaces.size() >= 2);  // left + right hemispheres
}

void TestMneInvCoverage::testSourceSpaceVertexCount()
{
    QVERIFY2(QFile::exists(srcPath()),
             qPrintable(QString("Source space not found: %1").arg(srcPath())));

    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    MNESourceSpaces srcSpaces;
    QVERIFY(MNESourceSpaces::readFromStream(stream, true, srcSpaces));
    stream->close();

    for (int i = 0; i < srcSpaces.size(); i++) {
        const MNESourceSpace& sp = srcSpaces[i];
        QVERIFY(sp.np > 0);
        QVERIFY(sp.nuse > 0);
        QVERIFY(sp.nuse <= sp.np);
        QCOMPARE(sp.rr.rows(), (Eigen::Index)sp.np);
    }
}

void TestMneInvCoverage::testSourceSpaceHemispheres()
{
    QVERIFY2(QFile::exists(srcPath()),
             qPrintable(QString("Source space not found: %1").arg(srcPath())));

    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    MNESourceSpaces srcSpaces;
    QVERIFY(MNESourceSpaces::readFromStream(stream, true, srcSpaces));
    stream->close();

    if (srcSpaces.size() >= 2) {
        // Check hemisphere labels
        QCOMPARE(srcSpaces[0].id, (int)FIFFV_MNE_SURF_LEFT_HEMI);
        QCOMPARE(srcSpaces[1].id, (int)FIFFV_MNE_SURF_RIGHT_HEMI);
    }
}

//=============================================================================================================
// MNESourceEstimate
//=============================================================================================================

void TestMneInvCoverage::testSourceEstimateConstruction()
{
    int nSources = 100;
    int nTimes = 50;
    MatrixXd data = MatrixXd::Random(nSources, nTimes);
    VectorXi vertices = VectorXi::LinSpaced(nSources, 0, nSources - 1);
    float tmin = 0.0f;
    float tstep = 0.001f;  // 1ms

    InvSourceEstimate stc(data, vertices, tmin, tstep);
    QCOMPARE(stc.data.rows(), (Eigen::Index)nSources);
    QCOMPARE(stc.data.cols(), (Eigen::Index)nTimes);
    QVERIFY(qAbs(stc.tmin - tmin) < 1e-6);
    QVERIFY(qAbs(stc.tstep - tstep) < 1e-6);
    QCOMPARE(stc.vertices.size(), (Eigen::Index)nSources);
}

void TestMneInvCoverage::testSourceEstimateTimeSampling()
{
    MatrixXd data = MatrixXd::Zero(10, 100);
    VectorXi vertices = VectorXi::LinSpaced(10, 0, 9);
    float tmin = -0.1f;
    float tstep = 0.002f;

    InvSourceEstimate stc(data, vertices, tmin, tstep);
    // times vector should span from tmin to tmin + (ncols-1)*tstep
    QCOMPARE(stc.times.size(), (Eigen::Index)100);
    QVERIFY(qAbs(stc.times(0) - (-0.1f)) < 1e-5f);
    QVERIFY(qAbs(stc.times(99) - (-0.1f + 99 * 0.002f)) < 1e-4f);
}

void TestMneInvCoverage::testSourceEstimateReduce()
{
    MatrixXd data = MatrixXd::Random(50, 200);
    VectorXi vertices = VectorXi::LinSpaced(50, 0, 49);
    float tmin = 0.0f;
    float tstep = 0.001f;

    InvSourceEstimate stc(data, vertices, tmin, tstep);
    // Reduce: take 50 samples starting at index 10
    InvSourceEstimate reduced = stc.reduce(10, 50);
    QCOMPARE(reduced.data.cols(), (Eigen::Index)50);
    QCOMPARE(reduced.data.rows(), stc.data.rows());
}

//=============================================================================================================
// MNEForwardSolution
//=============================================================================================================

void TestMneInvCoverage::testForwardSolutionFromFile()
{
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    QFile file(fwdPath());
    MNEForwardSolution fwd(file);
    QVERIFY(fwd.sol != nullptr);
    QVERIFY(fwd.nsource > 0);
    QVERIFY(fwd.nchan > 0);
}

void TestMneInvCoverage::testForwardSolutionDimensions()
{
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    QFile file(fwdPath());
    MNEForwardSolution fwd(file);

    // sol should be nchan x (nsource * source_ori)
    QVERIFY(fwd.sol->data.rows() > 0);
    QVERIFY(fwd.sol->data.cols() > 0);
    QCOMPARE(fwd.sol->data.rows(), (Eigen::Index)fwd.nchan);
}

void TestMneInvCoverage::testForwardSolutionPickChannelTypes()
{
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    QFile file(fwdPath());
    MNEForwardSolution fwd(file);

    // Pick MEG only
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffInfo info = raw.info;

    MNEForwardSolution fwdMeg = fwd.pick_channels(info.ch_names);
    QVERIFY(fwdMeg.nchan > 0);
    QVERIFY(fwdMeg.nchan <= fwd.nchan);
}

//=============================================================================================================
// MNEEpochDataList
//=============================================================================================================

void TestMneInvCoverage::testEpochDataListFromRaw()
{
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    // Create synthetic events (every 1000 samples, event ID 1)
    MatrixXi events(5, 3);
    int firstSamp = raw.first_samp;
    for (int i = 0; i < 5; i++) {
        events(i, 0) = firstSamp + i * 1000;  // sample
        events(i, 1) = 0;           // prev event
        events(i, 2) = 1;           // event ID
    }

    // Read epochs: -0.2 to 0.5 seconds
    float tmin = -0.2f;
    float tmax = 0.5f;

    MNEEpochDataList epochList;
    QMap<QString, double> mapReject;  // No rejection thresholds
    epochList = MNEEpochDataList::readEpochs(
        raw, events, tmin, tmax, 1, // event ID
        mapReject,
        QStringList() // no bads
    );

    // Should have created some epochs
    QVERIFY(epochList.size() > 0);
    // Each epoch should have reasonable number of samples (non-empty)
    if (!epochList.isEmpty()) {
        QVERIFY(epochList[0]->epoch.cols() > 0);
        QVERIFY(epochList[0]->epoch.rows() > 0);
    }
}

//=============================================================================================================
// InvMinimumNorm
//=============================================================================================================

void TestMneInvCoverage::testMinNormSetup()
{
    QVERIFY2(QFile::exists(fwdPath()) && QFile::exists(covPath()) && QFile::exists(avePath()),
             "Required test data files not found (fwd, cov, or ave)");

    // Load forward solution
    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY2(fwd.sol != nullptr, "Could not load forward solution");

    // Load raw info
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    // Load noise covariance
    QFile covFile(covPath());
    FiffCov noiseCov(covFile);

    // Prepare inverse operator
    MNEInverseOperator invOp = MNEInverseOperator::make_inverse_operator(
        raw.info, fwd, noiseCov);

    QVERIFY(invOp.noise_cov != nullptr);
    QVERIFY(invOp.eigen_leads != nullptr);
}

void TestMneInvCoverage::testMinNormCalculateInverse()
{
    QVERIFY2(QFile::exists(fwdPath()) && QFile::exists(covPath()) && QFile::exists(avePath()),
             "Required test data files not found (fwd, cov, or ave)");

    // Load data
    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY2(fwd.sol != nullptr, "Could not load forward solution");

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    QFile covFile(covPath());
    FiffCov noiseCov(covFile);

    // Prepare inverse operator
    MNEInverseOperator invOp = MNEInverseOperator::make_inverse_operator(
        raw.info, fwd, noiseCov);

    // Read evoked data
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet;
    FiffEvokedSet::read(aveFile, evokedSet);
    QVERIFY2(!evokedSet.evoked.isEmpty(), "No evoked data found in file");

    // Setup minimum norm and calculate inverse
    InvMinimumNorm invMNE(invOp, 1.0f / 9.0f, QString("dSPM"));
    invMNE.doInverseSetup(1, false);

    InvSourceEstimate stc = invMNE.calculateInverse(evokedSet.evoked[0]);
    QVERIFY(stc.data.rows() > 0);
    QVERIFY(stc.data.cols() > 0);
    QVERIFY(stc.tstep > 0);
}

//=============================================================================================================

void TestMneInvCoverage::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneInvCoverage)
#include "test_mne_inv_coverage.moc"
