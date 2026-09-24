//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     test_geometryinfo.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Blerta Hamzallari <blerta.hamzallari@tu-ilmenau.de>;
 *           Felix Griesau <Felix.Griesau@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Marco Klamke <marco.klamke@tu-ilmenau.de>;
 *           Petros Simidyan <petros.simidyan@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2017
 * Copyright (C) 2017, Lars Debor, Blerta Hamzallari, Felix Griesau, Gabriel B Motta, Lorenz Esch, 
 *                     Marco Klamke, Petros Simidyan, Simon Heinke, Sugandha Sachdeva. All rights reserved.
 * @brief    test_geometryinfo class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <disp3D/helpers/geometryinfo.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestGeometryInfo
 *
 * @brief The TestGeometryInfo class provides basic verification tests
 *
 */
class TestGeometryInfo: public QObject
{
    Q_OBJECT

public:
    TestGeometryInfo();

private slots:
    void initTestCase();
    void testBadChannelFiltering();
    void testEmptyInputsForProjecting();
    void testEmptyInputsForSCDC();
    void testDimensionsForSCDC();
    void cleanupTestCase();

private:
    // real data
    MNEBemSurface realSurface;
    // random data (keep computation times short)
    MNEBemSurface smallSurface;
    Eigen::VectorXi vSmallSubset;
};

//=============================================================================================================

TestGeometryInfo::TestGeometryInfo() {
}

//=============================================================================================================

void TestGeometryInfo::initTestCase() {
    //acquire real surface data
    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    realSurface = t_sensorSurfaceVV[0];

    // generate small test mesh with 100 vertices:
    MatrixX3f mVertPos(100, 3);
    for(qint8 i = 0; i < 100; i++) {
        float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        mVertPos(i, 0) = x;
        mVertPos(i, 1) = y;
        mVertPos(i, 2) = z;
    }
    smallSurface.rr = mVertPos;

    // generate random adjacency, assume that every vertex has 4 neighbors
    for (int i = 0; i < 100; ++i) {
        Eigen::VectorXi vNeighborList(4);
        for (int a = 0; a < 4; ++a) {
            // this allows duplicates, probably is not a problem
            vNeighborList[a] = rand() % 100;
        }
        smallSurface.neighbor_vert.push_back(vNeighborList);
    }

    //generate random subset of test mesh of size SubsetSize
    int iSubsetSize = rand() % 100;
    vSmallSubset.resize(iSubsetSize + 1);
    for (int b = 0; b <= iSubsetSize; b++) {
        // this allows duplicates, probably is not a problem
        vSmallSubset[b] = rand() % 100;
    }
}

//=============================================================================================================

void TestGeometryInfo::testBadChannelFiltering() {
    //acquire real sensor positions
    QFile t_fileEvoked(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return;
    }
    // Build sensor position matrix
    int nMegSensors = 0;
    for (const FiffChInfo &info : evoked.info.chs) {
        if (info.kind == FIFFV_MEG_CH) ++nMegSensors;
    }
    MatrixX3f matMegSensors(nMegSensors, 3);
    int sIdx = 0;
    for (const FiffChInfo &info : evoked.info.chs) {
        if (info.kind == FIFFV_MEG_CH) {
            matMegSensors.row(sIdx++) = info.chpos.r0.transpose();
        }
    }

    // projecting with MEG:
    VectorXi mappedSubSet = GeometryInfo::projectSensors(realSurface.rr, matMegSensors);
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> pDistanceMatrix = GeometryInfo::scdc(realSurface.rr, realSurface.neighbor_vert, mappedSubSet, 0.03);
    // filter for bad MEG channels:
    VectorXi vErasedColums = GeometryInfo::filterBadChannels(pDistanceMatrix, evoked.info, FIFFV_MEG_CH);

    for (Eigen::Index c = 0; c < vErasedColums.size(); ++c) {
        qint32 col = vErasedColums[c];
        qint64 iNotInfCount = 0;
        for (qint32 row = 0; row < pDistanceMatrix->rows(); ++row) {
            if (pDistanceMatrix->coeff(row, col) != FLOAT_INFINITY) {
                iNotInfCount++;
            }
        }
        QVERIFY(iNotInfCount == 0);
    }
}

//=============================================================================================================

void TestGeometryInfo::testEmptyInputsForProjecting() {
    // sensor projecting:
    MatrixX3f matEmptySensors(0, 3);
    VectorXi vEmptyMapping = GeometryInfo::projectSensors(realSurface.rr, matEmptySensors);
    QVERIFY(vEmptyMapping.size() == 0);
}

//=============================================================================================================

void TestGeometryInfo::testEmptyInputsForSCDC() {
    VectorXi vVertSubset;
    QSharedPointer<MatrixXd> pDistTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, vVertSubset);
    QVERIFY(pDistTable->rows() == pDistTable->cols());
}

//=============================================================================================================

void TestGeometryInfo::testDimensionsForSCDC() {
    QSharedPointer<MatrixXd> pDistTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, vSmallSubset);
    QVERIFY(pDistTable->rows() == smallSurface.rr.rows());
    QVERIFY(pDistTable->cols() == vSmallSubset.size());
}

//=============================================================================================================

void TestGeometryInfo::cleanupTestCase() {
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestGeometryInfo)
#include "test_geometryinfo.moc"
