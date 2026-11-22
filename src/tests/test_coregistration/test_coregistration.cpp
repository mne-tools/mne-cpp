//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     test_coregistration.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.5
 * @date     August, 2020
 * @brief     Test for the coregistration implementation..
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne/fiff/fiff_dig_point_set.h"
#include "mne/fiff/fiff_dig_point.h"
#include "mne/fiff/fiff_coord_trans.h"
#include "mne/mne/mne_bem.h"
#include "mne/mne/mne_bem_surface.h"
#include "mne/mne/mne_project_to_surface.h"
#include "mne/mne/mne_icp.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <memory>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestCoregistration
 *
 * @brief The TestCoregistration class provides a coregistration verification tests
 *
 */
class TestCoregistration : public QObject
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
    double dEpsilon;
    FiffCoordTrans transFitMatched;
    FiffCoordTrans transPerformICP;
    FiffCoordTrans transFitMatchedRef;
    FiffCoordTrans transPerformICPRef;
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
    QFile t_fileTransRefFit(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif");
    QFile t_fileTransRefIcp(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/icp-trans.fif");

    float fTol = 0.01f / 1000.0f;
    float fMaxDist = 0.02f;

    // read reference Transformation
    transFitMatchedRef = FiffCoordTrans(t_fileTransRefFit.fileName().toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);


    // read Bem
    MNEBem bemHead((QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif").toStdU16String());
    std::shared_ptr<MNEBemSurface> bemSurface = std::make_shared<MNEBemSurface>(bemHead[0]);
    std::shared_ptr<MNEProjectToSurface> mneSurfacePoints = std::make_shared<MNEProjectToSurface>(*bemSurface);

    // read digitizer data
    const FiffDigPointSet dig((QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif").toStdString());
    FiffDigPointSet digSetSrc = dig.pick_types({FIFFV_POINT_CARDINAL}); // Fiducials Head-Space
    FiffDigPointSet digSetDst = digSetSrc;                              // Fiducials MRI-Space
    digSetDst.apply_transform(transFitMatchedRef);
    FiffDigPointSet digSetHsp = dig.pick_types({FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA}); // Head shape points Head-Space

    // Initial Fiducial Alignment
    // Declare variables
    Matrix3f matSrc(digSetSrc.size(), 3);
    Matrix3f matDst(digSetDst.size(), 3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale = 1.0f;
    bool bScale = true;

    // get coordinates
    for (int i = 0; i < std::ssize(digSetSrc); ++i) {
        matSrc(i, 0) = digSetSrc[i].r[0];
        matSrc(i, 1) = digSetSrc[i].r[1];
        matSrc(i, 2) = digSetSrc[i].r[2];
        matDst(i, 0) = digSetDst[i].r[0];
        matDst(i, 1) = digSetDst[i].r[1];
        matDst(i, 2) = digSetDst[i].r[2];

        // set standart weights
        if (digSetSrc[i].ident == FIFFV_POINT_NASION) {
            vecWeights(i) = 10.0;
        } else {
            vecWeights(i) = 1.0;
        }
    }

    // align fiducials
    if (!mne::fitMatchedPoints(matSrc, matDst, matTrans, fScale, bScale, vecWeights)) {
        Log::warning("Point cloud registration not succesfull.");
    }

    fiff_int_t iFrom = digSetSrc[0].coord_frame;
    fiff_int_t iTo = bemSurface.get()->coord_frame;
    transFitMatched = FiffCoordTrans(iFrom, iTo, matTrans);
    transPerformICP = FiffCoordTrans(transFitMatched);

    // Prepare Icp:
    VectorXf vecWeightsICP(digSetHsp.size()); // Weigths vector
    int iMaxIter = 20;
    MatrixXf matHsp(digSetHsp.size(), 3);

    for (int i = 0; i < std::ssize(digSetHsp); ++i) {
        matHsp(i, 0) = digSetHsp[i].r[0];
        matHsp(i, 1) = digSetHsp[i].r[1];
        matHsp(i, 2) = digSetHsp[i].r[2];
        // set standart weights
        if ((digSetHsp[i].kind == FIFFV_POINT_CARDINAL) && (digSetHsp[i].ident == FIFFV_POINT_NASION)) {
            vecWeightsICP(i) = 10.0;
        } else {
            vecWeightsICP(i) = 1.0;
        }
    }

    MatrixXf matHspClean;
    VectorXi vecTake;

    // discard outliers
    if (!mne::discard3DPointOutliers(mneSurfacePoints, matHsp, transPerformICP, vecTake, matHspClean, fMaxDist)) {
        Log::warning("Discard outliers was not succesfull.");
    }
    VectorXf vecWeightsICPClean(vecTake.size());
    for (int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    float fRMSE = 0.0;
    if (!mne::performIcp(mneSurfacePoints, matHspClean, transPerformICP, fRMSE, bScale, iMaxIter, fTol, vecWeightsICPClean)) {
        Log::warning("ICP was not succesfull.");
    }
    transPerformICPRef = FiffCoordTrans(t_fileTransRefIcp.fileName().toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
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
