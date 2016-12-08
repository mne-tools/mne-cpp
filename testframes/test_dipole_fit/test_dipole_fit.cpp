//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

#include <fiff/fiff.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

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
    void compareData();
    void compareTimes();
    void compareInfo();
    void cleanupTestCase();

private:
    double epsilon;
};


//*************************************************************************************************************

TestDipoleFit::TestDipoleFit()
: epsilon(0.000001)
{
}



//*************************************************************************************************************

void TestDipoleFit::initTestCase()
{
    qDebug() << "Epsilon" << epsilon;

    QFile t_fileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QFile t_fileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_test_rwr_out.fif");

    //
    //   Make sure test folder exists
    //
    QFileInfo t_fileOutInfo(t_fileOut);
    QDir().mkdir(t_fileOutInfo.path());


    //*********************************************************************************************************
    // First Read & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Second Read
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read Again >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read Again Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestDipoleFit::compareData()
{

//    QVERIFY( data_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestDipoleFit::compareTimes()
{

//    QVERIFY( times_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestDipoleFit::compareInfo()
{
//    //Sampling frequency
//    std::cout << "[1] Sampling Frequency Check\n";
//    QVERIFY( first_in_raw.info.sfreq == second_in_raw.info.sfreq );

//    //Projection
//    std::cout << "[2] Projection Check\n";
//    QVERIFY( first_in_raw.info.projs.size() == second_in_raw.info.projs.size() );

//    for( qint32 i = 0; i < first_in_raw.info.projs.size(); ++i )
//    {
//        std::cout << "Projector " << i << std::endl;
//        MatrixXd tmp = first_in_raw.info.projs[i].data->data - second_in_raw.info.projs[i].data->data;
//        QVERIFY( tmp.sum() < epsilon );
//    }

//    //Compensators
//    std::cout << "[3] Compensator Check\n";
//    QVERIFY( first_in_raw.info.comps.size() == second_in_raw.info.comps.size() );

//    for( qint32 i = 0; i < first_in_raw.info.comps.size(); ++i )
//    {
//        std::cout << "Compensator " << i << std::endl;
//        MatrixXd tmp = first_in_raw.info.comps[i].data->data - second_in_raw.info.comps[i].data->data;
//        QVERIFY( tmp.sum() < epsilon );
//    }
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
