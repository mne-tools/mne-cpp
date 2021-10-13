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

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/dipole_fit.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;

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

    ECDSet m_ECDSet;
    ECDSet m_refECDSet;
};

//=============================================================================================================

TestDipoleFit::TestDipoleFit()
: epsilon(0.000001)
{
}

//=============================================================================================================

void TestDipoleFit::initTestCase()
{
}

//=============================================================================================================

void TestDipoleFit::dipoleFitSimple()
{
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_dip_fit.dat");
    QFile testFile;

    //*********************************************************************************************************
    // Dipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Dipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1 --meg
    //--eeg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --dip ./mne-cpp-test-data/Result/dip_fit.dat
    DipoleFitSettings settings;
    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.measname = testFile.fileName();
    settings.is_raw = false;
    settings.setno = 1;
    settings.include_meg = true;
    settings.include_eeg = true;
    settings.tmin = 32.0f/1000.0f;
    settings.tmax = 148.0f/1000.0f;
    settings.bmin = -100.0f/1000.0f;
    settings.bmax = 0.0f/1000.0f;
    settings.dipname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/dip_fit.dat";

    settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Compute Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    DipoleFit dipFit(&settings);
    ECDSet set = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Write Read Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Write Read Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    set.save_dipoles_dip(settings.dipname);
    m_ECDSet = ECDSet::read_dipoles_dip(settings.dipname);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Write Read Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Load reference Dipole Set
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Load Dipole Fit Reference Set >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    m_refECDSet = ECDSet::read_dipoles_dip(refFileName);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Compare Fit
    //*********************************************************************************************************

    compareFit();
}

//=============================================================================================================

void TestDipoleFit::dipoleFitAdvanced()
{
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_dip-5120-bem-result.dat");
    QFile testFile;

    //*********************************************************************************************************
    // Dipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Dipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1
    //--noise ./mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif --bem ./mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif
    //--mri ./mne-cpp-test-data/MEG/sample/all-trans.fif --meg --tmin 150 --tmax 250 --tstep 10 --dip ./mne-cpp-test-data/Result/dip-5120-bem_fit.dat
    //--mindist 0 --guessrad 100
    DipoleFitSettings settings;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.measname = testFile.fileName();

    settings.is_raw = false;
    settings.setno = 1;
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.tmin = 0.15f;
    settings.tmax = 0.25f;
    settings.tstep = 0.01f;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif"); QVERIFY( testFile.exists() );
    settings.bemname = testFile.fileName();

    settings.bmin = 1000000.0f;
    settings.bmax = 1000000.0f;

    settings.dipname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/dip-5120-bem_fit.dat";

    settings.guess_mindist = 0.0f;
    settings.guess_rad = 0.1f;

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif"); QVERIFY( testFile.exists() );
    settings.mriname = testFile.fileName();

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif"); QVERIFY( testFile.exists() );
    settings.noisename = testFile.fileName();

    testFile.setFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif"); QVERIFY( testFile.exists() );
    settings.projnames.append(testFile.fileName());

    settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Compute Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    DipoleFit dipFit(&settings);
    ECDSet set = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Write Read Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Write Read Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    set.save_dipoles_dip(settings.dipname);
    m_ECDSet = ECDSet::read_dipoles_dip(settings.dipname);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Write Read Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Load reference Dipole Set
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Load Dipole Fit Reference Set >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    m_refECDSet = ECDSet::read_dipoles_dip(refFileName);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Compare Fit
    //*********************************************************************************************************

    compareFit();
}

//=============================================================================================================

void TestDipoleFit::compareFit()
{
    //*********************************************************************************************************
    // Write Read Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare Dipole Fits >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    QVERIFY( m_refECDSet.size() == m_ECDSet.size() );

    for (int i = 0; i < m_refECDSet.size(); ++i)
    {
        printf("Compare orig Dipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
                1000*m_ECDSet[i].time,1000*m_ECDSet[i].time,
                1000*m_ECDSet[i].rd[0],1000*m_ECDSet[i].rd[1],1000*m_ECDSet[i].rd[2],
                1e9*m_ECDSet[i].Q.norm(),1e9*m_ECDSet[i].Q[0],1e9*m_ECDSet[i].Q[1],1e9*m_ECDSet[i].Q[2],100.0*m_ECDSet[i].good);
        printf("         ref Dipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
                1000*m_refECDSet[i].time,1000*m_refECDSet[i].time,
                1000*m_refECDSet[i].rd[0],1000*m_refECDSet[i].rd[1],1000*m_refECDSet[i].rd[2],
                1e9*m_refECDSet[i].Q.norm(),1e9*m_refECDSet[i].Q[0],1e9*m_refECDSet[i].Q[1],1e9*m_refECDSet[i].Q[2],100.0*m_refECDSet[i].good);

        QVERIFY( m_ECDSet[i].valid == m_refECDSet[i].valid );
        QVERIFY( m_ECDSet[i].time - m_refECDSet[i].time < epsilon );
        QVERIFY( m_ECDSet[i].rd == m_refECDSet[i].rd );
        QVERIFY( m_ECDSet[i].Q == m_refECDSet[i].Q );
        QVERIFY( m_ECDSet[i].good - m_refECDSet[i].good < epsilon );
        QVERIFY( m_ECDSet[i].khi2 - m_refECDSet[i].khi2 < epsilon );
        QVERIFY( m_ECDSet[i].nfree == m_refECDSet[i].nfree );
        QVERIFY( m_ECDSet[i].neval == m_refECDSet[i].neval );
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare Dipole Fits Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestDipoleFit::cleanupTestCase()
{
    QString outFile(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/dip-5120-bem_fit.dat");
    QFile::remove(outFile);

    outFile = QString(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/dip_fit.dat");
    QFile::remove(outFile);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDipoleFit)
#include "test_dipole_fit.moc"
