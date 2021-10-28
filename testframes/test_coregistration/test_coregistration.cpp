//=============================================================================================================
/**
 * @file     test_coregistration.cpp
 * @author   Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.5
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

#include "mne/mne_bem.h"
#include "mne/mne_bem_surface.h"
#include "mne/mne_project_to_surface.h"

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
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestCoregistration
 *
 * @brief The TestCoregistration class provides a coregistration verification tests
 *
 */
class TestCoregistration: public QObject
{
    Q_OBJECT

public:
    TestCoregistration();

private slots:
    void initTestCase();
    void compareFitMatchedPoints();
    void comparePerformIcp();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    FiffCoordTrans transFitMatched;
    FiffCoordTrans transPerformICP;
    FiffCoordTrans transFitMatchedRef;
    FiffCoordTrans transPerformICPRef;
};

//=============================================================================================================

TestCoregistration::TestCoregistration()
{
}

//=============================================================================================================

void TestCoregistration::initTestCase()
{
    // Create files
    QFile t_fileDig(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    QFile t_fileTransRefFit(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif");
    QFile t_fileTransRefIcp(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/icp-trans.fif");

    float fTol = 0.01f/1000.0f;
    float fMaxDist = 0.02f;

    // read reference Transformation
    transFitMatchedRef = FiffCoordTrans(t_fileTransRefFit);
    transFitMatchedRef.invert_transform();

    // transPerformICPRef = FiffCoordTrans(t_fileTransRefIcp);

    // read Bem
    MNEBem bemHead(t_fileBem);
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    // read digitizer data
    QList<int> lPickFiducials({FIFFV_POINT_CARDINAL});
    QList<int> lPickHSP({FIFFV_POINT_CARDINAL,FIFFV_POINT_HPI,FIFFV_POINT_EXTRA});
    FiffDigPointSet digSetSrc = FiffDigPointSet(t_fileDig).pickTypes(lPickFiducials);   // Fiducials Head-Space
    FiffDigPointSet digSetDst = FiffDigPointSet(t_fileDig).pickTypes(lPickFiducials);
    digSetDst.applyTransform(transFitMatchedRef, false);                                   // Fiducials MRI-Space
    FiffDigPointSet digSetHsp = FiffDigPointSet(t_fileDig).pickTypes(lPickHSP);         // Head shape points Head-Space

    // Initial Fiducial Alignment
    // Declare variables
    Matrix3f matSrc(digSetSrc.size(),3);
    Matrix3f matDst(digSetDst.size(),3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale = 1.0f;
    bool bScale = true;

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

    // align fiducials
    if(!RTPROCESSINGLIB::fitMatchedPoints(matSrc,matDst,matTrans,fScale,bScale,vecWeights)) {
        qWarning() << "Point cloud registration not succesfull.";
    }

    fiff_int_t iFrom = digSetSrc[0].coord_frame;
    fiff_int_t iTo = bemSurface.data()->coord_frame;
    transFitMatched = FiffCoordTrans::make(iFrom, iTo, matTrans);
    transPerformICP = FiffCoordTrans(transFitMatched);

    // Prepare Icp:
    VectorXf vecWeightsICP(digSetHsp.size()); // Weigths vector
    int iMaxIter = 20;
    MatrixXf matHsp(digSetHsp.size(),3);

    for(int i = 0; i < digSetHsp.size(); ++i) {
        matHsp(i,0) = digSetHsp[i].r[0]; matHsp(i,1) = digSetHsp[i].r[1]; matHsp(i,2) = digSetHsp[i].r[2];
        // set standart weights
        if((digSetHsp[i].kind == FIFFV_POINT_CARDINAL) && (digSetHsp[i].ident == FIFFV_POINT_NASION)) {
            vecWeightsICP(i) = 10.0;
        } else {
            vecWeightsICP(i) = 1.0;
        }
    }

    MatrixXf matHspClean;
    VectorXi vecTake;

    // discard outliers
    if(!RTPROCESSINGLIB::discard3DPointOutliers(mneSurfacePoints, matHsp, transPerformICP, vecTake, matHspClean, fMaxDist)) {
        qWarning() << "Discard outliers was not succesfull.";
    }
    VectorXf vecWeightsICPClean(vecTake.size());
    for(int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    float fRMSE = 0.0;
    if(!RTPROCESSINGLIB::performIcp(mneSurfacePoints, matHspClean, transPerformICP, fRMSE, bScale, iMaxIter, fTol, vecWeightsICPClean)) {
        qWarning() << "ICP was not succesfull.";
    }
    transPerformICPRef = FiffCoordTrans(t_fileTransRefIcp);
}

//=============================================================================================================

void TestCoregistration::compareFitMatchedPoints()
{
    QVERIFY(transFitMatchedRef == transFitMatched);
}

//=============================================================================================================

void TestCoregistration::comparePerformIcp()
{
    QVERIFY(transPerformICPRef == transPerformICP);
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

