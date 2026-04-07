//=============================================================================================================
/**
 * @file     test_surface_checks.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 *
 * @brief    Unit tests for SurfaceChecks (BEM surface topology verification).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../tools/forward/mne_surf2bem/surfacechecks.h"

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Creates a synthetic icosahedron BEM surface for testing.
 *
 * The icosahedron is a closed convex surface ideal for testing solid angle
 * completeness and surface size checks.
 */
static MNEBemSurface createIcosahedron(float radius, int surfId)
{
    MNEBemSurface surf;
    surf.id = surfId;

    // Golden ratio
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    const float scale = radius / std::sqrt(1.0f + phi * phi);

    // 12 vertices of an icosahedron
    surf.np = 12;
    surf.rr.resize(12, 3);

    surf.rr.row(0)  = Vector3f(-1,  phi, 0).normalized() * radius;
    surf.rr.row(1)  = Vector3f( 1,  phi, 0).normalized() * radius;
    surf.rr.row(2)  = Vector3f(-1, -phi, 0).normalized() * radius;
    surf.rr.row(3)  = Vector3f( 1, -phi, 0).normalized() * radius;
    surf.rr.row(4)  = Vector3f(0, -1,  phi).normalized() * radius;
    surf.rr.row(5)  = Vector3f(0,  1,  phi).normalized() * radius;
    surf.rr.row(6)  = Vector3f(0, -1, -phi).normalized() * radius;
    surf.rr.row(7)  = Vector3f(0,  1, -phi).normalized() * radius;
    surf.rr.row(8)  = Vector3f( phi, 0, -1).normalized() * radius;
    surf.rr.row(9)  = Vector3f( phi, 0,  1).normalized() * radius;
    surf.rr.row(10) = Vector3f(-phi, 0, -1).normalized() * radius;
    surf.rr.row(11) = Vector3f(-phi, 0,  1).normalized() * radius;

    // 20 triangular faces
    surf.ntri = 20;
    surf.itris.resize(20, 3);

    // Top cap
    surf.itris.row(0)  = Vector3i(0, 11, 5);
    surf.itris.row(1)  = Vector3i(0, 5, 1);
    surf.itris.row(2)  = Vector3i(0, 1, 7);
    surf.itris.row(3)  = Vector3i(0, 7, 10);
    surf.itris.row(4)  = Vector3i(0, 10, 11);

    // Adjacent faces
    surf.itris.row(5)  = Vector3i(1, 5, 9);
    surf.itris.row(6)  = Vector3i(5, 11, 4);
    surf.itris.row(7)  = Vector3i(11, 10, 2);
    surf.itris.row(8)  = Vector3i(10, 7, 6);
    surf.itris.row(9)  = Vector3i(7, 1, 8);

    // Bottom cap adjacent
    surf.itris.row(10) = Vector3i(3, 9, 4);
    surf.itris.row(11) = Vector3i(3, 4, 2);
    surf.itris.row(12) = Vector3i(3, 2, 6);
    surf.itris.row(13) = Vector3i(3, 6, 8);
    surf.itris.row(14) = Vector3i(3, 8, 9);

    // Bottom cap
    surf.itris.row(15) = Vector3i(4, 9, 5);
    surf.itris.row(16) = Vector3i(2, 4, 11);
    surf.itris.row(17) = Vector3i(6, 2, 10);
    surf.itris.row(18) = Vector3i(8, 6, 7);
    surf.itris.row(19) = Vector3i(9, 8, 1);

    return surf;
}

//=============================================================================================================
/**
 * @brief Test class for SurfaceChecks.
 */
class TestSurfaceChecks : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // getNameOf tests
    void testGetNameOfHead();
    void testGetNameOfSkull();
    void testGetNameOfBrain();
    void testGetNameOfUnknown();

    // Solid angle / completeness tests
    void testIsCompleteSurface();
    void testIsCompleteWithRealBemData();

    // Surface nesting tests
    void testCheckSurfaces();
    void testCheckSurfacesWrongOrder();

    // Surface size tests
    void testCheckSurfaceSizeValid();
    void testCheckSurfaceSizeTooSmall();

    // Thickness tests
    void testCheckThicknesses();

    // Triangle area reporting
    void testReportTriangleAreas();
    void testReportTriangleAreasNoData();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
};

//=============================================================================================================

void TestSurfaceChecks::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================

void TestSurfaceChecks::testGetNameOfHead()
{
    QString name = SurfaceChecks::getNameOf(FIFFV_BEM_SURF_ID_HEAD);
    QVERIFY(name.contains("skin"));
}

void TestSurfaceChecks::testGetNameOfSkull()
{
    QString name = SurfaceChecks::getNameOf(FIFFV_BEM_SURF_ID_SKULL);
    QVERIFY(name.contains("skull"));
}

void TestSurfaceChecks::testGetNameOfBrain()
{
    QString name = SurfaceChecks::getNameOf(FIFFV_BEM_SURF_ID_BRAIN);
    QVERIFY(name.contains("skull"));
}

void TestSurfaceChecks::testGetNameOfUnknown()
{
    QString name = SurfaceChecks::getNameOf(9999);
    QVERIFY(name.contains("unknown"));
}

//=============================================================================================================

void TestSurfaceChecks::testIsCompleteSurface()
{
    // Create a closed icosahedron — solid angle from center must be 4*pi
    MNEBemSurface surf = createIcosahedron(0.1f, FIFFV_BEM_SURF_ID_HEAD);
    QVERIFY(SurfaceChecks::isCompleteSurface(surf));
}

void TestSurfaceChecks::testIsCompleteWithRealBemData()
{
    // Load a real BEM surface from test data
    QString bemPath = m_sResourcePath + "subjects/sample/bem/sample-5120-bem.fif";
    if (!QFile::exists(bemPath))
        QSKIP("BEM test data not available");

    QFile file(bemPath);

    FIFFLIB::FiffStream::SPtr stream(new FIFFLIB::FiffStream(&file));
    if (!stream->open())
        QSKIP("Cannot open FiffStream");

    MNELIB::MNEBem bem;
    if (!MNELIB::MNEBem::readFromStream(stream, true, bem))
        QSKIP("Cannot read BEM surfaces");

    QVERIFY(bem.size() > 0);
    for (int i = 0; i < bem.size(); ++i) {
        QVERIFY(SurfaceChecks::isCompleteSurface(bem[i]));
    }
}

//=============================================================================================================

void TestSurfaceChecks::testCheckSurfaces()
{
    // Create nested surfaces: brain inside skull inside skin
    MNEBemSurface skin  = createIcosahedron(0.10f, FIFFV_BEM_SURF_ID_HEAD);
    MNEBemSurface skull = createIcosahedron(0.08f, FIFFV_BEM_SURF_ID_SKULL);
    MNEBemSurface brain = createIcosahedron(0.06f, FIFFV_BEM_SURF_ID_BRAIN);

    QVector<MNEBemSurface> surfs;
    surfs << skin << skull << brain;

    QVERIFY(SurfaceChecks::checkSurfaces(surfs));
}

void TestSurfaceChecks::testCheckSurfacesWrongOrder()
{
    // Create surfaces in wrong order: brain outside skull
    MNEBemSurface skin  = createIcosahedron(0.10f, FIFFV_BEM_SURF_ID_HEAD);
    MNEBemSurface skull = createIcosahedron(0.08f, FIFFV_BEM_SURF_ID_SKULL);
    MNEBemSurface brain = createIcosahedron(0.12f, FIFFV_BEM_SURF_ID_BRAIN); // bigger than skin!

    QVector<MNEBemSurface> surfs;
    surfs << skin << skull << brain;

    // This should fail because brain is not inside skull
    QVERIFY(!SurfaceChecks::checkSurfaces(surfs));
}

//=============================================================================================================

void TestSurfaceChecks::testCheckSurfaceSizeValid()
{
    // Surface in meters (reasonable size, > 0.05m)
    MNEBemSurface surf = createIcosahedron(0.1f, FIFFV_BEM_SURF_ID_HEAD);
    QVERIFY(SurfaceChecks::checkSurfaceSize(surf));
}

void TestSurfaceChecks::testCheckSurfaceSizeTooSmall()
{
    // Surface too small (< 0.05m in some dimension, as if given in meters for a very small head)
    MNEBemSurface surf = createIcosahedron(0.01f, FIFFV_BEM_SURF_ID_HEAD);
    QVERIFY(!SurfaceChecks::checkSurfaceSize(surf));
}

//=============================================================================================================

void TestSurfaceChecks::testCheckThicknesses()
{
    MNEBemSurface skin  = createIcosahedron(0.10f, FIFFV_BEM_SURF_ID_HEAD);
    MNEBemSurface skull = createIcosahedron(0.08f, FIFFV_BEM_SURF_ID_SKULL);

    QVector<MNEBemSurface> surfs;
    surfs << skin << skull;

    // checkThicknesses always returns true (just reports distances)
    QVERIFY(SurfaceChecks::checkThicknesses(surfs));
}

//=============================================================================================================

void TestSurfaceChecks::testReportTriangleAreas()
{
    MNEBemSurface surf = createIcosahedron(0.1f, FIFFV_BEM_SURF_ID_HEAD);

    // Compute triangle areas
    surf.tri_area.resize(surf.ntri);
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.itris(k, 0));
        Vector3f v1 = surf.rr.row(surf.itris(k, 1));
        Vector3f v2 = surf.rr.row(surf.itris(k, 2));
        Eigen::Vector3f a = v1 - v0;
        Eigen::Vector3f b = v2 - v0;
        surf.tri_area(k) = 0.5f * a.cross(b).norm();
    }

    // Should not crash, just reports
    SurfaceChecks::reportTriangleAreas(surf, "test_icosahedron");
    QVERIFY(true);
}

void TestSurfaceChecks::testReportTriangleAreasNoData()
{
    MNEBemSurface surf = createIcosahedron(0.1f, FIFFV_BEM_SURF_ID_HEAD);
    // tri_area is empty by default — should handle gracefully
    SurfaceChecks::reportTriangleAreas(surf, "test_no_area_data");
    QVERIFY(true);
}

//=============================================================================================================

void TestSurfaceChecks::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestSurfaceChecks)
#include "test_surface_checks.moc"
