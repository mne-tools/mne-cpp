//=============================================================================================================
/**
* @file     test_dipole_fit.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/dipole_fit.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
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
    void compareFit();
    void cleanupTestCase();

private:
    double epsilon;

    DipoleFitSettings m_settings;
    ECDSet m_ECDSet;
    ECDSet m_refECDSet;
};


//*************************************************************************************************************

TestDipoleFit::TestDipoleFit()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestDipoleFit::initTestCase()
{
    //*********************************************************************************************************
    // Load reference Dipole Set
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Load Dipole Fit Reference Set >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/ref_dip_fit.dat");
    m_refECDSet = ECDSet::read_dipoles_dip(refFileName);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Dipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Dipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1 --meg --eeg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --dip ./mne-cpp-test-data/Result/dip_fit.dat
    m_settings.measname = QDir::currentPath()+"/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
    m_settings.is_raw = false;
    m_settings.setno = 1;
    m_settings.include_meg = true;
    m_settings.include_eeg = true;
    m_settings.tmin = 32.0f/1000.0f;
    m_settings.tmax = 148.0f/1000.0f;
    m_settings.bmin = -100.0f/1000.0f;
    m_settings.bmax = 0.0f/1000.0f;
    m_settings.dipname = QDir::currentPath()+"/mne-cpp-test-data/Result/dip_fit.dat";

    m_settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Compute Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    DipoleFit dipFit(&m_settings);
    ECDSet set = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Write Read Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Write Read Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    set.save_dipoles_dip(m_settings.dipname);
    m_ECDSet = ECDSet::read_dipoles_dip(m_settings.dipname);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Write Read Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

}


//*************************************************************************************************************

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


//*************************************************************************************************************

void TestDipoleFit::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestDipoleFit)
#include "test_dipole_fit.moc"
