//=============================================================================================================
/**
 * @file     test_fiff_cov.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Test for I/O of a FiffCov
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff_cov.h>

#include <iostream>
#include <utils/ioutils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCov
 *
 * @brief The TestFiffCov class provides covariance reading verification tests
 *
 */
class TestFiffCov: public QObject
{
    Q_OBJECT

public:
    TestFiffCov();

private slots:
    void initTestCase();
    void compareData();
    void compareKind();
    void compareDiag();
    void compareDim();
    void compareNfree();
    void cleanupTestCase();

private:
    double dEpsilon;

    FiffCov covLoaded;
    FiffCov covResult;
};

//=============================================================================================================

TestFiffCov::TestFiffCov()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestFiffCov::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qDebug() << "Epsilon" << dEpsilon;
    //Read the results produced with MNE-CPP
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-cov.fif");
    covLoaded = FiffCov(t_fileIn);

    //Read the result data produced with mne_matlab
    MatrixXd data;
    IOUtils::read_eigen_matrix(data, QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_data_sample_audvis-cov.dat");
    covResult.data = data;

    covResult.kind = 1;
    covResult.diag = 0;
    covResult.dim = 366;
    covResult.nfree = 15972;
}

//=============================================================================================================

void TestFiffCov::compareData()
{
    //Make the values a little bit bigger
    MatrixXd mDataDiff = covResult.data*1000000 - covLoaded.data*1000000;

//    qDebug()<<"abs(covResult.data.sum()) "<<covResult.data.normalized().sum();
//    qDebug()<<"abs(covLoaded.data.sum()) "<<covLoaded.data.normalized().sum();
//    qDebug()<<"abs(mDataDiff.sum()) "<<abs(mDataDiff.sum());
//    qDebug()<<"epsilon "<<epsilon;

    QVERIFY( std::abs(mDataDiff.sum()) < dEpsilon );
}

//=============================================================================================================

void TestFiffCov::compareKind()
{
    QVERIFY( covResult.kind == covLoaded.kind );
}

//=============================================================================================================

void TestFiffCov::compareDiag()
{
    QVERIFY( covResult.diag == covLoaded.diag );
}

//=============================================================================================================

void TestFiffCov::compareDim()
{
    QVERIFY( covResult.dim == covLoaded.dim );
}

//=============================================================================================================

void TestFiffCov::compareNfree()
{
    QVERIFY( covResult.nfree == covLoaded.nfree );
}

//=============================================================================================================

void TestFiffCov::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCov)
#include "test_fiff_cov.moc"
