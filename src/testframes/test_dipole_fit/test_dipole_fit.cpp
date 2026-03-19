//=============================================================================================================
/**
 * @file     test_dipole_fit.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Gabriel B Motta. All rights reserved.
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
 * @brief    The dipole fit test implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_dipole_fit.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestDipoleFit
 *
 * @brief The TestDipoleFit class provides dipole fit tests
 *
 */
class TestDipoleFit: public QObject
{
    Q_OBJECT

public:
    TestDipoleFit();

private slots:
    void initTestCase();
    void dipoleFitSimple();
    void dipoleFitAdvanced();
    void cleanupTestCase();

private:
    void compareFit();

    double epsilon;

    InvEcdSet m_ECDSet;
    InvEcdSet m_refECDSet;
};

//=============================================================================================================

TestDipoleFit::TestDipoleFit()
: epsilon(0.000001)
{
}

//=============================================================================================================

void TestDipoleFit::initTestCase()
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> InvDipole Fit Init >>>>>>>>>>>>>>>>>>>>>>>>>\n");
}

//=============================================================================================================

void TestDipoleFit::dipoleFitSimple()
{
    printf("[checkpoint] dipoleFitSimple() entered\n"); fflush(stdout);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> InvDipole FitSimple >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QString refFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_dip_fit.dat");
    QFile testFile;

    //*********************************************************************************************************
    // InvDipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> InvDipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1 --meg
    //--eeg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --dip ../resources/data/mne-cpp-test-data/Result/dip_fit.dat
    InvDipoleFitSettings settings;
    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.measname = testFile.fileName();
    settings.is_raw = false;
    settings.setno = 1;
    settings.include_meg = true;
    settings.include_eeg = true;
    settings.tmin = 32.0f/1000.0f;
    settings.tmax = 148.0f/1000.0f;
    settings.bmin = -100.0f/1000.0f;
    settings.bmax = 0.0f/1000.0f;
    settings.dipname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/dip_fit.dat";

    settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< InvDipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Compute InvDipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute InvDipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);

    printf("[checkpoint] Creating InvDipoleFit (simple)...\n"); fflush(stdout);
    InvDipoleFit dipFit(&settings);
    printf("[checkpoint] InvDipoleFit created, calling calculateFit() (simple)...\n"); fflush(stdout);
    InvEcdSet set = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute InvDipole Fit Finished (simple, set size=%d) <<<<<<<<<<<<<<<<<<<<<<<<<\n", set.size());
    fflush(stdout);

    //*********************************************************************************************************
    // Write Read InvDipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Write Read InvDipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);

    set.save_dipoles_dip(settings.dipname);
    printf("[checkpoint] dipole file written (simple)\n"); fflush(stdout);
    m_ECDSet = InvEcdSet::read_dipoles_dip(settings.dipname);
    printf("[checkpoint] dipole file read back (simple, size=%d)\n", m_ECDSet.size()); fflush(stdout);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Write Read InvDipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Load reference InvDipole Set
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Load InvDipole Fit Reference Set >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);
    m_refECDSet = InvEcdSet::read_dipoles_dip(refFileName);
    printf("[checkpoint] reference loaded (simple, size=%d)\n", m_refECDSet.size()); fflush(stdout);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< InvDipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Compare Fit
    //*********************************************************************************************************

    compareFit();
}

//=============================================================================================================

void TestDipoleFit::dipoleFitAdvanced()
{
    QString refFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_dip-5120-bem-result.dat");
    QFile testFile;

    //*********************************************************************************************************
    // InvDipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> InvDipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1
    //--noise ../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif --bem ../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif
    //--mri ../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif --meg --tmin 150 --tmax 250 --tstep 10 --dip ../resources/data/mne-cpp-test-data/Result/dip-5120-bem_fit.dat
    //--mindist 0 --guessrad 100
    InvDipoleFitSettings settings;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.measname = testFile.fileName();

    settings.is_raw = false;
    settings.setno = 1;
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.tmin = 0.15f;
    settings.tmax = 0.25f;
    settings.tstep = 0.01f;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif"); QVERIFY( testFile.exists() );
    settings.bemname = testFile.fileName();

    settings.bmin = 1000000.0f;
    settings.bmax = 1000000.0f;

    settings.dipname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/dip-5120-bem_fit.dat";

    settings.guess_mindist = 0.0f;
    settings.guess_rad = 0.1f;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif"); QVERIFY( testFile.exists() );
    settings.mriname = testFile.fileName();

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif"); QVERIFY( testFile.exists() );
    settings.noisename = testFile.fileName();

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.projnames.append(testFile.fileName());

    settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< InvDipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Compute InvDipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute InvDipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);

    printf("[checkpoint] Creating InvDipoleFit (advanced)...\n"); fflush(stdout);
    InvDipoleFit dipFit(&settings);
    printf("[checkpoint] InvDipoleFit created, calling calculateFit() (advanced)...\n"); fflush(stdout);
    InvEcdSet set = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute InvDipole Fit Finished (advanced, set size=%d) <<<<<<<<<<<<<<<<<<<<<<<<<\n", set.size());
    fflush(stdout);

    //*********************************************************************************************************
    // Write Read InvDipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Write Read InvDipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);

    set.save_dipoles_dip(settings.dipname);
    printf("[checkpoint] dipole file written (advanced)\n"); fflush(stdout);
    m_ECDSet = InvEcdSet::read_dipoles_dip(settings.dipname);
    printf("[checkpoint] dipole file read back (advanced, size=%d)\n", m_ECDSet.size()); fflush(stdout);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Write Read InvDipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Load reference InvDipole Set
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Load InvDipole Fit Reference Set >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    fflush(stdout);
    m_refECDSet = InvEcdSet::read_dipoles_dip(refFileName);
    printf("[checkpoint] reference loaded (advanced, size=%d)\n", m_refECDSet.size()); fflush(stdout);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< InvDipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");
    fflush(stdout);

    //*********************************************************************************************************
    // Compare Fit
    //*********************************************************************************************************

    compareFit();
}

//=============================================================================================================

void TestDipoleFit::compareFit()
{
    //*********************************************************************************************************
    // Write Read InvDipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare InvDipole Fits >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QVERIFY( m_refECDSet.size() == m_ECDSet.size() );

    for (int i = 0; i < m_refECDSet.size(); ++i)
    {
        printf("Compare orig InvDipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
                1000*m_ECDSet[i].time,1000*m_ECDSet[i].time,
                1000*m_ECDSet[i].rd[0],1000*m_ECDSet[i].rd[1],1000*m_ECDSet[i].rd[2],
                1e9*m_ECDSet[i].Q.norm(),1e9*m_ECDSet[i].Q[0],1e9*m_ECDSet[i].Q[1],1e9*m_ECDSet[i].Q[2],100.0*m_ECDSet[i].good);
        printf("         ref InvDipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
                1000*m_refECDSet[i].time,1000*m_refECDSet[i].time,
                1000*m_refECDSet[i].rd[0],1000*m_refECDSet[i].rd[1],1000*m_refECDSet[i].rd[2],
                1e9*m_refECDSet[i].Q.norm(),1e9*m_refECDSet[i].Q[0],1e9*m_refECDSet[i].Q[1],1e9*m_refECDSet[i].Q[2],100.0*m_refECDSet[i].good);

        // Verbose diagnostics for CI debugging
        double dt = std::abs(m_ECDSet[i].time - m_refECDSet[i].time);
        double drd = (m_ECDSet[i].rd - m_refECDSet[i].rd).norm();
        double dQ = (m_ECDSet[i].Q - m_refECDSet[i].Q).norm();
        double dgood = std::abs(m_ECDSet[i].good - m_refECDSet[i].good);
        double dkhi2 = std::abs(m_ECDSet[i].khi2 - m_refECDSet[i].khi2);
        printf("  [diag] dipole %d: dt=%.10e drd=%.10e dQ=%.10e dgood=%.10e dkhi2=%.10e nfree=%d/%d valid=%d/%d\n",
               i, dt, drd, dQ, dgood, dkhi2,
               m_ECDSet[i].nfree, m_refECDSet[i].nfree,
               m_ECDSet[i].valid, m_refECDSet[i].valid);
        fflush(stdout);

        QVERIFY( m_ECDSet[i].valid == m_refECDSet[i].valid );
        QVERIFY( std::abs(m_ECDSet[i].time - m_refECDSet[i].time) < epsilon );
        QVERIFY( (m_ECDSet[i].rd - m_refECDSet[i].rd).norm() < epsilon );
        QVERIFY( (m_ECDSet[i].Q - m_refECDSet[i].Q).norm() < epsilon );
        QVERIFY( std::abs(m_ECDSet[i].good - m_refECDSet[i].good) < epsilon );
        QVERIFY( std::abs(m_ECDSet[i].khi2 - m_refECDSet[i].khi2) < epsilon );
        QVERIFY( m_ECDSet[i].nfree == m_refECDSet[i].nfree );
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare InvDipole Fits Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestDipoleFit::cleanupTestCase()
{
    QString outFile(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/dip-5120-bem_fit.dat");
    QFile::remove(outFile);

    outFile = QString(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/dip_fit.dat");
    QFile::remove(outFile);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDipoleFit)
#include "test_dipole_fit.moc"
