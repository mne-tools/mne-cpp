//=============================================================================================================
/**
 * @file     test_source_estimate_types.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for InvVectorSourceEstimate and InvVolumeSourceEstimate.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_vector_source_estimate.h>
#include <inv/inv_volume_source_estimate.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================

class TestSourceEstimateTypes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    // Vector source estimate tests
    void testVectorConstruct();
    void testVectorNVertices();
    void testVectorMagnitude();
    void testVectorVertexData();
    void testVectorProjectNormals();
    void testVectorOrientation();
    // Volume source estimate tests
    void testVolumeConstruct();
    void testVolumeShape();
    void testVolumeToVolume();
    void testVolumeCentreOfMass();
    void testVolumeSourceSpaceType();
    void testVolumeNoShape();
    void cleanupTestCase();
};

//=============================================================================================================

void TestSourceEstimateTypes::initTestCase()
{
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorConstruct()
{
    int nVert = 4;
    int nTimes = 10;
    MatrixXd data = MatrixXd::Random(nVert * 3, nTimes);
    VectorXi verts = VectorXi::LinSpaced(nVert, 0, nVert - 1);

    InvVectorSourceEstimate vstc(data, verts, 0.0f, 0.001f);
    QVERIFY(!vstc.isEmpty());
    QCOMPARE(static_cast<int>(vstc.data.rows()), nVert * 3);
    QCOMPARE(static_cast<int>(vstc.data.cols()), nTimes);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorNVertices()
{
    int nVert = 5;
    MatrixXd data = MatrixXd::Random(nVert * 3, 8);
    VectorXi verts = VectorXi::LinSpaced(nVert, 0, nVert - 1);

    InvVectorSourceEstimate vstc(data, verts, 0.0f, 0.001f);
    QCOMPARE(vstc.nVertices(), nVert);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorMagnitude()
{
    int nVert = 3;
    int nTimes = 2;
    MatrixXd data = MatrixXd::Zero(nVert * 3, nTimes);

    // Vertex 0: (3, 4, 0) → magnitude 5
    data(0, 0) = 3.0;
    data(1, 0) = 4.0;
    data(2, 0) = 0.0;

    // Vertex 1: (0, 0, 1) → magnitude 1
    data(3, 0) = 0.0;
    data(4, 0) = 0.0;
    data(5, 0) = 1.0;

    VectorXi verts(nVert);
    verts << 0, 1, 2;

    InvVectorSourceEstimate vstc(data, verts, 0.0f, 0.001f);
    InvSourceEstimate mag = vstc.magnitude();

    QCOMPARE(static_cast<int>(mag.data.rows()), nVert);
    QVERIFY(std::abs(mag.data(0, 0) - 5.0) < 1e-10);
    QVERIFY(std::abs(mag.data(1, 0) - 1.0) < 1e-10);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorVertexData()
{
    int nVert = 2;
    MatrixXd data = MatrixXd::Random(nVert * 3, 5);
    VectorXi verts(nVert);
    verts << 10, 20;

    InvVectorSourceEstimate vstc(data, verts, 0.0f, 0.001f);
    MatrixXd vd = vstc.vertexData(0);
    QCOMPARE(static_cast<int>(vd.rows()), 3);
    QCOMPARE(static_cast<int>(vd.cols()), 5);

    // Should match the first 3 rows of data
    QVERIFY((vd - data.topRows(3)).norm() < 1e-15);

    // Invalid index returns empty
    MatrixXd vdBad = vstc.vertexData(5);
    QCOMPARE(static_cast<int>(vdBad.size()), 0);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorProjectNormals()
{
    int nVert = 2;
    int nTimes = 1;
    MatrixXd data = MatrixXd::Zero(nVert * 3, nTimes);
    // Vertex 0: vector (1, 0, 0)
    data(0, 0) = 1.0;
    // Vertex 1: vector (0, 1, 0)
    data(4, 0) = 1.0;

    VectorXi verts(nVert);
    verts << 0, 1;

    MatrixX3f normals(nVert, 3);
    // Normal 0: (1, 0, 0) → projection = 1.0
    normals << 1.0f, 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f;  // Normal 1: (0, 0, 1) → projection = 0.0

    InvVectorSourceEstimate vstc(data, verts, 0.0f, 0.001f);
    InvSourceEstimate proj = vstc.projectToNormals(normals);

    QCOMPARE(static_cast<int>(proj.data.rows()), nVert);
    QVERIFY(std::abs(proj.data(0, 0) - 1.0) < 1e-10);
    QVERIFY(std::abs(proj.data(1, 0)) < 1e-10);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVectorOrientation()
{
    InvVectorSourceEstimate vstc;
    QCOMPARE(vstc.orientationType, InvOrientationType::Free);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeConstruct()
{
    int nVox = 8;
    int nTimes = 5;
    MatrixXd data = MatrixXd::Random(nVox, nTimes);
    VectorXi verts = VectorXi::LinSpaced(nVox, 0, nVox - 1);

    InvVolumeSourceEstimate vol(data, verts, 0.0f, 0.001f);
    QVERIFY(!vol.isEmpty());
    QCOMPARE(static_cast<int>(vol.data.rows()), nVox);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeShape()
{
    InvVolumeSourceEstimate vol;
    QVERIFY(!vol.hasShape());

    vol.setShape({4, 5, 6});
    QVERIFY(vol.hasShape());
    QCOMPARE(vol.shape()[0], 4);
    QCOMPARE(vol.shape()[1], 5);
    QCOMPARE(vol.shape()[2], 6);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeToVolume()
{
    int nx = 3, ny = 3, nz = 3;
    int nTotal = nx * ny * nz;  // 27
    int nTimes = 2;

    // Active voxels: 0, 5, 13
    VectorXi verts(3);
    verts << 0, 5, 13;
    MatrixXd data(3, nTimes);
    data << 1.0, 2.0,
            3.0, 4.0,
            5.0, 6.0;

    InvVolumeSourceEstimate vol(data, verts, 0.0f, 0.001f);
    vol.setShape({nx, ny, nz});

    VectorXd flat = vol.toVolume(0);
    QCOMPARE(static_cast<int>(flat.size()), nTotal);
    QVERIFY(std::abs(flat[0] - 1.0) < 1e-10);
    QVERIFY(std::abs(flat[5] - 3.0) < 1e-10);
    QVERIFY(std::abs(flat[13] - 5.0) < 1e-10);
    QVERIFY(std::abs(flat[1]) < 1e-10);  // Inactive voxel
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeCentreOfMass()
{
    int nx = 4, ny = 4, nz = 4;

    // Single active voxel at (2, 1, 3) → index = 2*4*4 + 1*4 + 3 = 39
    VectorXi verts(1);
    verts << 39;
    MatrixXd data(1, 1);
    data(0, 0) = 1.0;

    InvVolumeSourceEstimate vol(data, verts, 0.0f, 0.001f);
    vol.setShape({nx, ny, nz});

    Vector3d com = vol.centreOfMass(0);
    QVERIFY(std::abs(com[0] - 2.0) < 1e-10);
    QVERIFY(std::abs(com[1] - 1.0) < 1e-10);
    QVERIFY(std::abs(com[2] - 3.0) < 1e-10);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeSourceSpaceType()
{
    InvVolumeSourceEstimate vol;
    QCOMPARE(vol.sourceSpaceType, InvSourceSpaceType::Volume);
}

//=============================================================================================================

void TestSourceEstimateTypes::testVolumeNoShape()
{
    VectorXi verts(2);
    verts << 0, 1;
    MatrixXd data = MatrixXd::Ones(2, 3);
    InvVolumeSourceEstimate vol(data, verts, 0.0f, 0.001f);

    VectorXd flat = vol.toVolume(0);
    QCOMPARE(static_cast<int>(flat.size()), 0);
}

//=============================================================================================================

void TestSourceEstimateTypes::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestSourceEstimateTypes)
#include "test_source_estimate_types.moc"
