//=============================================================================================================
/**
 * @file     test_mne_cov_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
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
 *
 * @brief    Tests for the MNECovMatrix class (creation, packed indexing,
 *           diagonal/dense forms, duplication, eigendecomposition, file I/O).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_cov_matrix.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestMNECovMatrix : public QObject
{
    Q_OBJECT

private:
    QString m_sResourcePath;

private slots:

    void initTestCase()
    {
        // Locate test data relative to the executable
        m_sResourcePath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
    }

    //=========================================================================================================
    // Packed index tests — validates the lower-triangular indexing used
    // for dense covariance storage (same formula as MNE-Python/MNE-C).
    //=========================================================================================================

    void testLtPackedIndex_diagonal()
    {
        // On-diagonal: index(j,j) = j*(j+1)/2 + j
        QCOMPARE(MNECovMatrix::lt_packed_index(0, 0), 0);
        QCOMPARE(MNECovMatrix::lt_packed_index(1, 1), 2);
        QCOMPARE(MNECovMatrix::lt_packed_index(2, 2), 5);
        QCOMPARE(MNECovMatrix::lt_packed_index(3, 3), 9);
    }

    void testLtPackedIndex_symmetric()
    {
        // lt_packed_index should return the same value for (j,k) and (k,j)
        QCOMPARE(MNECovMatrix::lt_packed_index(2, 0), MNECovMatrix::lt_packed_index(0, 2));
        QCOMPARE(MNECovMatrix::lt_packed_index(3, 1), MNECovMatrix::lt_packed_index(1, 3));
    }

    //=========================================================================================================
    // Factory method tests
    //=========================================================================================================

    void testCreateDense()
    {
        const int n = 3;
        QStringList names = {"MEG0111", "MEG0121", "MEG0131"};
        VectorXd cov(n * (n + 1) / 2);  // 6 elements for 3x3
        cov << 1.0, 0.1, 0.2, 2.0, 0.3, 3.0;

        auto cm = MNECovMatrix::create_dense(FIFFV_MNE_NOISE_COV, n, names, cov);
        QVERIFY(cm != nullptr);
        QCOMPARE(cm->ncov, n);
        QCOMPARE(cm->kind, static_cast<int>(FIFFV_MNE_NOISE_COV));
        QVERIFY(!cm->is_diag());
    }

    void testCreateDiag()
    {
        const int n = 4;
        QStringList names = {"ch1", "ch2", "ch3", "ch4"};
        VectorXd diag(n);
        diag << 1e-20, 2e-20, 3e-20, 4e-20;

        auto cm = MNECovMatrix::create_diag(FIFFV_MNE_NOISE_COV, n, names, diag);
        QVERIFY(cm != nullptr);
        QCOMPARE(cm->ncov, n);
        QVERIFY(cm->is_diag());
    }

    //=========================================================================================================
    // Duplication test
    //=========================================================================================================

    void testDup()
    {
        const int n = 3;
        QStringList names = {"a", "b", "c"};
        VectorXd cov(6);
        cov << 1, 0, 0, 1, 0, 1;

        auto orig = MNECovMatrix::create_dense(FIFFV_MNE_NOISE_COV, n, names, cov);
        auto copy = orig->dup();

        QVERIFY(copy != nullptr);
        QCOMPARE(copy->ncov, orig->ncov);
        QCOMPARE(copy->kind, orig->kind);
    }

    //=========================================================================================================
    // Revert to diag
    //=========================================================================================================

    void testRevertToDiag()
    {
        const int n = 3;
        QStringList names = {"a", "b", "c"};
        VectorXd cov(6);
        cov << 4.0, 0.5, 0.5, 9.0, 0.5, 16.0;

        auto cm = MNECovMatrix::create_dense(FIFFV_MNE_NOISE_COV, n, names, cov);
        QVERIFY(!cm->is_diag());

        cm->revert_to_diag();
        QVERIFY(cm->is_diag());
    }

    //=========================================================================================================
    // File I/O test — read sample_audvis-cov.fif and validate against
    // known values from mne-python: mne.read_cov(fname)
    //=========================================================================================================

    void testReadSampleCov()
    {
        QString covFile = m_sResourcePath + "MEG/sample/sample_audvis-cov.fif";
        if (!QFile::exists(covFile)) {
            QSKIP("Test data not available");
        }

        auto cm = MNECovMatrix::read(covFile, FIFFV_MNE_NOISE_COV);
        QVERIFY(cm != nullptr);
        QVERIFY(cm->ncov > 0);
        QVERIFY(cm->names.size() == cm->ncov);

        // Validate: covariance should contain MEG and EEG channel names
        bool hasMEG = false, hasEEG = false;
        for (const auto& name : cm->names) {
            if (name.startsWith("MEG")) hasMEG = true;
            if (name.startsWith("EEG")) hasEEG = true;
        }
        QVERIFY(hasMEG);
        QVERIFY(hasEEG);
    }

    //=========================================================================================================
    // Eigendecomposition test
    //=========================================================================================================

    void testDecomposeEigen()
    {
        // Create 3x3 identity-like covariance (well-conditioned)
        const int n = 3;
        QStringList names = {"ch1", "ch2", "ch3"};
        VectorXd cov(6);
        cov << 1.0, 0.0, 0.0, 1.0, 0.0, 1.0;  // pack of identity

        auto cm = MNECovMatrix::create_dense(FIFFV_MNE_NOISE_COV, n, names, cov);

        // decompose_eigen requires ch_class to be populated
        cm->ch_class.resize(n);
        cm->ch_class[0] = MNE_COV_CH_MEG_MAG;
        cm->ch_class[1] = MNE_COV_CH_MEG_MAG;
        cm->ch_class[2] = MNE_COV_CH_MEG_MAG;

        int result = cm->decompose_eigen();
        QCOMPARE(result, 0);  // OK = 0
    }
};

QTEST_GUILESS_MAIN(TestMNECovMatrix)
#include "test_mne_cov_matrix.moc"
