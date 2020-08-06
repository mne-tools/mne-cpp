//=============================================================================================================
/**
 * @file     test_coregistration.cpp
 * @author   Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Doerfel. All rights reserved.
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
 * @brief     Test for the coregistration implementation..
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include "fiff/fiff_dig_point_set.h"
#include "fiff/fiff_dig_point.h"
#include "fiff/fiff_coord_trans.h"

#include <utils/generics/applicationlogger.h>
#include "rtprocessing/icp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTest>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace RTPROCESSINGLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestCoregistration
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestCoregistration: public QObject
{
    Q_OBJECT

public:
    TestCoregistration();

private slots:
    void initTestCase();
    void compareTransformation();
    // add other compareFunctions here
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    double dEpsilon;
    FiffCoordTrans transTestMriHead;
    FiffCoordTrans transMriHead;
};

//=============================================================================================================

TestCoregistration::TestCoregistration()
    : dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestCoregistration::initTestCase()
{
    // Create files
    QFile t_fileTestTrans(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test-sample-mri-head-trans.fif");
    QFile t_fileSrcFid(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample-fiducials.fif");
    QFile t_fileDstDig(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");

    // read test transformatiin
    transTestMriHead = FiffCoordTrans(t_fileTestTrans);
    // read digitizer data
    QList<int> lPickFiducials({FIFFV_POINT_CARDINAL});
    FiffDigPointSet digSetSrc = FiffDigPointSet(t_fileSrcFid).pickTypes(lPickFiducials);
    FiffDigPointSet digSetDst = FiffDigPointSet(t_fileDstDig).pickTypes(lPickFiducials);

    // Declare variables
    Matrix3f matSrc(digSetSrc.size(),3);
    Matrix3f matDst(digSetDst.size(),3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale;

    // get coordinates
    for(int i = 0; i< digSetSrc.size(); ++i) {
        matSrc(i,0) = digSetSrc[i].r[0]; matSrc(i,1) = digSetSrc[i].r[1]; matSrc(i,2) = digSetSrc[i].r[2];
        matDst(i,0) = digSetDst[i].r[0]; matDst(i,1) = digSetDst[i].r[1]; matDst(i,2) = digSetDst[i].r[2];

        // set standart weights
        if(digSetSrc[i].ident == FIFFV_POINT_NASION) {
            vecWeights(i) = 10.0;
        } else {
            vecWeights(i) = 1.0;
        }
    }

     bool bScale = true;

    if(!fitMatched(matSrc,matDst,matTrans,fScale,bScale,vecWeights)) {
        qWarning() << "point cloud registration not succesfull";
    }

    transMriHead = FiffCoordTrans::make(digSetSrc[0].coord_frame, digSetDst[0].coord_frame,matTrans);

}

//=============================================================================================================

void TestCoregistration::compareTransformation()
{
    Matrix4f matDataDiff = transTestMriHead.trans - transMriHead.trans;
    QVERIFY( matDataDiff.sum() < dEpsilon );
}

//=============================================================================================================

void TestCoregistration::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCoregistration);
#include "test_coregistration.moc"

