//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     test_mne_project_to_surface.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 * @brief     Test the MNEProjectToSurface class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>
#include <mne/mne_project_to_surface.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <utils/ioutils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QtTest>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// Using NAMESPACE
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMNEProjectToSurface
 *
 * @brief The TestMNEProjectToSurface class provides MNEProjectToSurface verification tests
 *
 */
class TestMNEProjectToSurface: public QObject
{
    Q_OBJECT

public:
    TestMNEProjectToSurface();

private slots:
    void initTestCase();
    void compareValue();
    void cleanupTestCase();

private:
    // declare thresholds, and variables
    double dEpsilon;
    MatrixXf matResult;
    MatrixXd matRef;

};

//=============================================================================================================

TestMNEProjectToSurface::TestMNEProjectToSurface()
    : dEpsilon(0.00001)
{
}

//=============================================================================================================

void TestMNEProjectToSurface::initTestCase()
{
    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    QString sRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/mne_project_to_surface.txt");

    MNEBem bemHead(t_fileBem);
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    VectorXi vecNearest;    // Triangle of the new point
    VectorXf vecDist;       // The Distance between matX and matP

    MatrixXf matPointsShifted = bemSurface->rr.cast<float>() * 1.1;     // Move all points with same amout from surface
    int iNP = matPointsShifted.rows();

    mneSurfacePoints->find_closest_on_surface(matPointsShifted, iNP, matResult, vecNearest, vecDist);

    // read reference
    UTILSLIB::IOUtils::read_eigen_matrix(matRef,sRef);
}

//=============================================================================================================

void TestMNEProjectToSurface::compareValue()
{
    // check if MNEProjectToSurface was able to get original points on surface
    MatrixXf matDiff = matRef.cast<float>() - matResult;
    qDebug() << "Summed Difference: " << std::abs(matDiff.sum());
    QVERIFY(std::abs(matDiff.sum()) < dEpsilon);
}

//=============================================================================================================

void TestMNEProjectToSurface::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMNEProjectToSurface)
#include "test_mne_project_to_surface.moc"

