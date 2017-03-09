//=============================================================================================================
/**
* @file     test_forward_solution.cpp
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
* @brief    The forward solution test implementation
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/computeFwd/compute_fwd_settings.h>
#include <fwd/computeFwd/compute_fwd.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;


//=============================================================================================================
/**
* DECLARE CLASS TestForwardSolution
*
* @brief The TestForwardSolution class provides dipole fit tests
*
*/
class TestForwardSolution : public QObject
{
    Q_OBJECT

public:
    TestForwardSolution();

private slots:
    void initTestCase();
    void computeForward();
    void cleanupTestCase();

private:
    void compareForward();

    double epsilon;

};


//*************************************************************************************************************

TestForwardSolution::TestForwardSolution()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestForwardSolution::initTestCase()
{

}


//*************************************************************************************************************

void TestForwardSolution::computeForward()
{
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/ref_dip_fit.dat");
    QFile testFile;

    //*********************************************************************************************************
    // Forward Solution Settings
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Forward Solution Settings >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //Following is equivalent to: --meas ./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif --set 1 --meg --eeg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --dip ./mne-cpp-test-data/Result/dip_fit.dat
    ComputeFwdSettings settings;


    settings.checkIntegrity();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Forward Solution Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Compute Forward Solution
    //*********************************************************************************************************


    //*********************************************************************************************************
    // Write Read Dipole Fit
    //*********************************************************************************************************


    //*********************************************************************************************************
    // Load reference Dipole Set
    //*********************************************************************************************************


    //*********************************************************************************************************
    // Compare Fit
    //*********************************************************************************************************

    compareForward();
}


//*************************************************************************************************************

void TestForwardSolution::compareForward()
{
    //*********************************************************************************************************
    // Write Read Forward Solution
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>\n");

//    QVERIFY( m_refECDSet.size() == m_ECDSet.size() );

//    for (int i = 0; i < m_refECDSet.size(); ++i)
//    {
//        printf("Compare orig Dipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
//                1000*m_ECDSet[i].time,1000*m_ECDSet[i].time,
//                1000*m_ECDSet[i].rd[0],1000*m_ECDSet[i].rd[1],1000*m_ECDSet[i].rd[2],
//                1e9*m_ECDSet[i].Q.norm(),1e9*m_ECDSet[i].Q[0],1e9*m_ECDSet[i].Q[1],1e9*m_ECDSet[i].Q[2],100.0*m_ECDSet[i].good);
//        printf("         ref Dipole %d: %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n", i,
//                1000*m_refECDSet[i].time,1000*m_refECDSet[i].time,
//                1000*m_refECDSet[i].rd[0],1000*m_refECDSet[i].rd[1],1000*m_refECDSet[i].rd[2],
//                1e9*m_refECDSet[i].Q.norm(),1e9*m_refECDSet[i].Q[0],1e9*m_refECDSet[i].Q[1],1e9*m_refECDSet[i].Q[2],100.0*m_refECDSet[i].good);

//        QVERIFY( m_ECDSet[i].valid == m_refECDSet[i].valid );
//        QVERIFY( m_ECDSet[i].time - m_refECDSet[i].time < epsilon );
//        QVERIFY( m_ECDSet[i].rd == m_refECDSet[i].rd );
//        QVERIFY( m_ECDSet[i].Q == m_refECDSet[i].Q );
//        QVERIFY( m_ECDSet[i].good - m_refECDSet[i].good < epsilon );
//        QVERIFY( m_ECDSet[i].khi2 - m_refECDSet[i].khi2 < epsilon );
//        QVERIFY( m_ECDSet[i].nfree == m_refECDSet[i].nfree );
//        QVERIFY( m_ECDSet[i].neval == m_refECDSet[i].neval );
//    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestForwardSolution::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestForwardSolution)
#include "test_forward_solution.moc"
