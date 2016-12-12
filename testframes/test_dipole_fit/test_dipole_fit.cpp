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

#include <inverse/dipoleFit/dipolefitsettings.h>
#include <inverse/dipoleFit/dipolefit.h>


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
    QString refFileName("./mne-cpp-test-data/Result/ref_dip_fit.dat");
    m_refECDSet = ECDSet::read_dipoles_dip(refFileName);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Reference Set Loaded <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Dipole Fit Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Dipole Fit Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1 --meg --eeg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --dip ./mne-cpp-test-data/Result/dip_fit.dat
    m_settings.measname = "D:/GitHub/mne-cpp/bin/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
    m_settings.is_raw = false;
    m_settings.setno = 1;
    m_settings.include_meg = true;
    m_settings.include_eeg = true;
    m_settings.tmin = 32.0f/1000.0f;
    m_settings.tmax = 148.0f/1000.0f;
    m_settings.bmin = -100.0f/1000.0f;
    m_settings.bmax = 0.0f/1000.0f;
    m_settings.dipname = "D:/GitHub/mne-cpp/bin/mne-cpp-test-data/Result/dip_fit.dat";

    m_settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Dipole Fit Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Compute Dipole Fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Dipole Fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    DipoleFit dipFit(&m_settings);
    m_ECDSet = dipFit.calculateFit();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Dipole Fit Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

}


//*************************************************************************************************************

void TestDipoleFit::compareFit()
{
    qDebug() << "m_refECDSet.size()" << m_refECDSet.size();
    qDebug() << "m_ECDSet.size()" << m_ECDSet.size();
    QVERIFY( m_refECDSet.size() == m_ECDSet.size() );
//    QVERIFY( times_diff.sum() < epsilon );
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
