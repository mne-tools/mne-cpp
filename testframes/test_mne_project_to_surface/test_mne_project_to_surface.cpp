//=============================================================================================================
/**
 * @file     test_mne_project_to_surface.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     Test the MNEProjectToSurface class..
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <mne/mne_project_to_surface.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

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
 * @brief The TestMNEProjectToSurface class provides read write read fiff verification tests
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
    // add other compareFunctions here
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    double dEpsilon;
    MatrixXf matResult;
    MatrixXf matRef;

};

//=============================================================================================================

TestMNEProjectToSurface::TestMNEProjectToSurface()
    : dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestMNEProjectToSurface::initTestCase()
{
    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    MNEBem bemHead(t_fileBem);
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    VectorXi vecNearest;                        // Triangle of the new point
    VectorXf vecDist;                           // The Distance between matX and matP

    matRef = bemSurface->tri_cent.cast<float>();
    MatrixXf matTriCent = matRef * 1.5;         // Move all points with same amout from surface
    int iNP = matTriCent.rows();
    mneSurfacePoints->mne_find_closest_on_surface(matTriCent, iNP, matResult, vecNearest, vecDist);
}

//=============================================================================================================

void TestMNEProjectToSurface::compareValue()
{
    // check if MNEProjectToSurface was able to get original points on surface
    MatrixXf matDiff = matRef - matResult;
    QVERIFY( matDiff.sum() < dEpsilon );
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

