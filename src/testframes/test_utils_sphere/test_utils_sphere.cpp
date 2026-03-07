//=============================================================================================================
/**
 * @file     test_utils_sphere.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Tests for Sphere fitting — unit sphere, offset sphere, noisy data, simplex fit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/sphere.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <random>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for Sphere fitting. Uses known-geometry point clouds (on a sphere of known center/radius)
 *        and validates that the fitting algorithms recover those parameters.
 *        Inspired by mne-python test_surface.py which tests geometric fitting with known ground truth.
 */
class TestSphere : public QObject
{
    Q_OBJECT

public:
    TestSphere();

private slots:
    void initTestCase();

    // Constructor and accessors
    void testConstructor();

    // Analytical fit on perfect sphere (known center + radius)
    void testFitSphereUnitSphere();
    void testFitSphereOffsetSphere();

    // Fit with noisy data
    void testFitSphereNoisyData();

    // Simplex-based fit
    void testFitSphereSimplex();

    // fit_sphere_to_points (Eigen interface)
    void testFitSphereToPointsEigen();

    // fit_sphere_to_points (raw pointer interface)
    void testFitSphereToPointsRawPtr();

    void cleanupTestCase();

private:
    /** Generate N points uniformly on a sphere of given center and radius */
    MatrixX3f generateSpherePoints(const Vector3f& center, float radius, int nPoints, unsigned int seed = 42);

    float m_fEpsilon;
};

//=============================================================================================================

TestSphere::TestSphere()
: m_fEpsilon(1e-3f)
{
}

//=============================================================================================================

void TestSphere::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
}

//=============================================================================================================

MatrixX3f TestSphere::generateSpherePoints(const Vector3f& center, float radius, int nPoints, unsigned int seed)
{
    MatrixX3f points(nPoints, 3);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist_theta(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> dist_phi(-1.0f, 1.0f);

    for (int i = 0; i < nPoints; ++i) {
        float theta = dist_theta(gen);
        float phi = acos(dist_phi(gen));

        points(i, 0) = center(0) + radius * sin(phi) * cos(theta);
        points(i, 1) = center(1) + radius * sin(phi) * sin(theta);
        points(i, 2) = center(2) + radius * cos(phi);
    }

    return points;
}

//=============================================================================================================

void TestSphere::testConstructor()
{
    Vector3f c(1.0f, 2.0f, 3.0f);
    Sphere s(c, 5.0f);

    QVERIFY((s.center() - c).norm() < m_fEpsilon);
    QVERIFY(std::abs(s.radius() - 5.0f) < m_fEpsilon);
}

//=============================================================================================================

void TestSphere::testFitSphereUnitSphere()
{
    // Generate points on unit sphere centered at origin
    Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 1.0f;
    MatrixX3f points = generateSpherePoints(center, radius, 500);

    Sphere fitted = Sphere::fit_sphere(points);

    QVERIFY((fitted.center() - center).norm() < m_fEpsilon);
    QVERIFY(std::abs(fitted.radius() - radius) < m_fEpsilon);
}

//=============================================================================================================

void TestSphere::testFitSphereOffsetSphere()
{
    // Sphere centered at (0.01, -0.02, 0.03), radius 0.08 (typical head coords in meters)
    Vector3f center(0.01f, -0.02f, 0.03f);
    float radius = 0.08f;
    MatrixX3f points = generateSpherePoints(center, radius, 1000);

    Sphere fitted = Sphere::fit_sphere(points);

    QVERIFY((fitted.center() - center).norm() < m_fEpsilon);
    QVERIFY(std::abs(fitted.radius() - radius) < m_fEpsilon);
}

//=============================================================================================================

void TestSphere::testFitSphereNoisyData()
{
    // Add Gaussian noise to sphere surface points
    Vector3f center(0.0f, 0.0f, 0.04f);
    float radius = 0.09f;
    int nPoints = 2000;

    MatrixX3f points = generateSpherePoints(center, radius, nPoints, 123);

    // Add noise (sigma = 0.5mm = 0.0005m, typical for digitization)
    std::mt19937 gen(456);
    std::normal_distribution<float> noise(0.0f, 0.0005f);
    for (int i = 0; i < nPoints; ++i) {
        points(i, 0) += noise(gen);
        points(i, 1) += noise(gen);
        points(i, 2) += noise(gen);
    }

    Sphere fitted = Sphere::fit_sphere(points);

    // With noise, allow 1mm tolerance
    QVERIFY((fitted.center() - center).norm() < 0.001f);
    QVERIFY(std::abs(fitted.radius() - radius) < 0.001f);
}

//=============================================================================================================

void TestSphere::testFitSphereSimplex()
{
    Vector3f center(0.0f, 0.0f, 0.04f);
    float radius = 0.09f;
    MatrixX3f points = generateSpherePoints(center, radius, 500);

    Sphere fitted = Sphere::fit_sphere_simplex(points, 0.02);

    // Simplex fit should recover center and radius within 2mm
    QVERIFY((fitted.center() - center).norm() < 0.002f);
    QVERIFY(std::abs(fitted.radius() - radius) < 0.002f);
}

//=============================================================================================================

void TestSphere::testFitSphereToPointsEigen()
{
    Vector3f center(0.005f, -0.01f, 0.05f);
    float radius = 0.085f;
    MatrixX3f points = generateSpherePoints(center, radius, 800);

    // Use the full Eigen interface (initial guess from centroid)
    VectorXf r0 = points.colwise().mean();
    float R = 0.0f;

    bool success = Sphere::fit_sphere_to_points(points, 0.02f, r0, R);
    QVERIFY(success);
    QVERIFY(std::abs(R - radius) < 0.005f);

    Vector3f fitted_center(r0(0), r0(1), r0(2));
    QVERIFY((fitted_center - center).norm() < 0.005f);
}

//=============================================================================================================

void TestSphere::testFitSphereToPointsRawPtr()
{
    Vector3f center(0.0f, 0.0f, 0.04f);
    float radius = 0.09f;
    int np = 300;
    MatrixX3f points = generateSpherePoints(center, radius, np);

    // Convert to float** format
    float** rr = new float*[np];
    for (int i = 0; i < np; ++i) {
        rr[i] = new float[3];
        rr[i][0] = points(i, 0);
        rr[i][1] = points(i, 1);
        rr[i][2] = points(i, 2);
    }

    float r0[3];
    // Initial guess: centroid
    r0[0] = points.col(0).mean();
    r0[1] = points.col(1).mean();
    r0[2] = points.col(2).mean();
    float R = 0.0f;

    bool success = Sphere::fit_sphere_to_points(rr, np, 0.02f, r0, &R);
    QVERIFY(success);
    QVERIFY(std::abs(R - radius) < 0.005f);

    // Cleanup
    for (int i = 0; i < np; ++i)
        delete[] rr[i];
    delete[] rr;
}

//=============================================================================================================

void TestSphere::cleanupTestCase()
{
    qInfo() << "TestSphere: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestSphere)
#include "test_utils_sphere.moc"
