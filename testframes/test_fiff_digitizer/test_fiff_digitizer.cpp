//=============================================================================================================
/**
* @file     test_fiff_digitizer.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Test for I/O of a FiffDigitizerData
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_dig_point.h>
#include <fiff/c/fiff_digitizer_data.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//=============================================================================================================
/**
* DECLARE CLASS TestFiffDigitizer
*
* @brief The TestFiffDigitizer class provides digitizer data reading verification tests
*
*/
class TestFiffDigitizer: public QObject
{
    Q_OBJECT

public:
    TestFiffDigitizer();

private slots:
    void initTestCase();
    void comparePoints();
    void compareCoordFrame();
    void compareNPoint();
    void cleanupTestCase();

private:
    double      m_dEpsilon;
    double      m_dSumPointsDigDataResult;
    int         m_iCoordFrameDigDataResult;
    int         m_iNPointDigDataResult;

    FiffDigitizerData digDataLoaded;
};


//*************************************************************************************************************

TestFiffDigitizer::TestFiffDigitizer()
: m_dEpsilon(1.0e-04)
{
}



//*************************************************************************************************************

void TestFiffDigitizer::initTestCase()
{
    //Read the results produced with MNE-CPP
    QFile t_fileIn(QCoreApplication::applicationDirPath()+"/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    digDataLoaded = FiffDigitizerData(t_fileIn);

    //Prepare reference result
    m_iCoordFrameDigDataResult = FIFFV_COORD_HEAD;
    m_iNPointDigDataResult = 146;
    m_dSumPointsDigDataResult = 13.6212;
}


//*************************************************************************************************************

void TestFiffDigitizer::comparePoints()
{
    double sum = 0.0;
    for(int i = 0; i < digDataLoaded.points.size(); ++i) {
        sum += digDataLoaded.points[i].r[0];
        sum += digDataLoaded.points[i].r[1];
        sum += digDataLoaded.points[i].r[2];
    }

    double diff = sum - m_dSumPointsDigDataResult;

    QVERIFY( diff < m_dEpsilon );
}


//*************************************************************************************************************

void TestFiffDigitizer::compareCoordFrame()
{
    QVERIFY( m_iCoordFrameDigDataResult == digDataLoaded.coord_frame );
}


//*************************************************************************************************************

void TestFiffDigitizer::compareNPoint()
{
    QVERIFY( m_iNPointDigDataResult == digDataLoaded.npoint );
}


//*************************************************************************************************************

void TestFiffDigitizer::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestFiffDigitizer)
#include "test_fiff_digitizer.moc"
