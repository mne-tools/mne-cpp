//=============================================================================================================
/**
 * @file     test_geometryinfo.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Blerta Hamzallari <blerta.hamzallari@tu-ilmenau.de>;
 *           Felix Griesau <Felix.Griesau@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Marco Klamke <marco.klamke@tu-ilmenau.de>;
 *           Petros Simidyan <petros.simidyan@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Blerta Hamzallari, Felix Griesau, Gabriel B Motta, Lorenz Esch, 
 *                     Marco Klamke, Petros Simidyan, Simon Heinke, Sugandha Sachdeva. All rights reserved.
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
 * @brief    test_geometryinfo class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <disp3D/helpers/geometryinfo/geometryinfo.h>
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
    QVector<int> vSmallSubset;
};

//=============================================================================================================

TestGeometryInfo::TestGeometryInfo() {
}

//=============================================================================================================

void TestGeometryInfo::initTestCase() {
    //acquire real surface data
    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif");
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
        QVector<int> vNeighborList;
        for (int a = 0; a < 4; ++a) {
            // this allows duplicates, probably is not a problem
            vNeighborList.push_back(rand() % 100);
        }
        smallSurface.neighbor_vert.push_back(vNeighborList);
    }

    //generate random subset of test mesh of size SubsetSize
    int iSubsetSize = rand() % 100;
    for (int b = 0; b <= iSubsetSize; b++) {
        // this allows duplicates, probably is not a problem
        vSmallSubset.push_back(rand() % 100);
    }
}

//=============================================================================================================

void TestGeometryInfo::testBadChannelFiltering() {
    //acquire real sensor positions
    QFile t_fileEvoked(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return;
    }
    QVector<Vector3f> vMegSensors;
    for( const FiffChInfo &info : qAsConst(evoked.info.chs)) {
        if(info.kind == FIFFV_MEG_CH) {
            vMegSensors.push_back(info.chpos.r0);
        }
    }

    // projecting with MEG:
    QVector<int> mappedSubSet = GeometryInfo::projectSensors(realSurface.rr, vMegSensors);
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> pDistanceMatrix = GeometryInfo::scdc(realSurface.rr, realSurface.neighbor_vert, mappedSubSet, 0.03);
    // filter for bad MEG channels:
    QVector<int> vErasedColums = GeometryInfo::filterBadChannels(pDistanceMatrix, evoked.info, FIFFV_MEG_CH);

    for (qint32 col : qAsConst(vErasedColums)) {
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
    QVector<Vector3f> vEmptySensors;
    QVector<int> vEmptyMapping = GeometryInfo::projectSensors(realSurface.rr, vEmptySensors);
    QVERIFY(vEmptyMapping.size() == 0);
}

//=============================================================================================================

void TestGeometryInfo::testEmptyInputsForSCDC() {
    QVector<int> vVertSubset;
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
