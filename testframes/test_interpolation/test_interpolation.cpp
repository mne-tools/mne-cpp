//=============================================================================================================
/**
 * @file     test_interpolation.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
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
 * Copyright (C) 2017, Lars Debor, Gabriel B Motta, Lorenz Esch, Marco Klamke, Petros Simidyan, 
 *                     Simon Heinke, Sugandha Sachdeva. All rights reserved.
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
 * @brief    The interpolation unit test
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/interpolation/interpolation.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <string>

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
 * DECLARE CLASS interpolation
 *
 * @brief The test_interpolation class provides basic verification tests
 *
 */
class TestInterpolation : public QObject
{
    Q_OBJECT

public:
    TestInterpolation();

private slots:
    void initTestCase();
    void testDimensionsForInterpolation();
    void testSumOfRow();
    void testEmptyInputsForWeightMatrix();
    void cleanupTestCase();

private:
    // real data
    MNEBemSurface realSurface;
    QVector<Vector3f> vMegSensors;
    FiffEvoked evoked;
    // random data (keep computation times short)
    MNEBemSurface smallSurface;
    QVector<int> vSmallSubset;
};

//=============================================================================================================

TestInterpolation::TestInterpolation()
{
}

//=============================================================================================================

void TestInterpolation::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    //acquire real data
    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    realSurface = t_sensorSurfaceVV[0];
    QFile t_fileEvoked(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    evoked = FiffEvoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty()) {
        return;
    }
    for( const FiffChInfo &info : evoked.info.chs) {
        if(info.kind == FIFFV_MEG_CH && info.unit == FIFF_UNIT_T) {
            vMegSensors.push_back(info.chpos.r0);
        }
    }

    // generate small test mesh with 100 vertices:
    MatrixX3f vVertPos(100, 3);
    for(qint8 i = 0; i < 100; i++) {
        float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        vVertPos(i, 0) = x;
        vVertPos(i, 1) = y;
        vVertPos(i, 2) = z;
    }
    smallSurface.rr = vVertPos;

    // generate random adjacency, assume that every vertex has 4 neighbors
    for (int i = 0; i < 100; ++i) {
        QVector<int> vNeighborList;
        for (int a = 0; a < 4; ++a) {
            // this allows duplicates, probably is not a problem
            vNeighborList.push_back(rand() % 100);
        }
        smallSurface.neighbor_vert.push_back(vNeighborList);
    }

    // generate random subset of test mesh of size iSubsetSize
    int iSubsetSize = rand() % 100;
    for (int b = 0; b <= iSubsetSize; b++) {
        // this allows duplicates, probably is not a problem
        vSmallSubset.push_back(rand() % 100);
    }
}

//=============================================================================================================

void TestInterpolation::testDimensionsForInterpolation()
{
    // create weight matrix from distance table
    QSharedPointer<MatrixXd> pDistTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, vSmallSubset);
    QSharedPointer<SparseMatrix<float> > pTestWeightMatrix = Interpolation::createInterpolationMat(vSmallSubset,
                                                                                 pDistTable,
                                                                                 Interpolation::linear);

    QVERIFY(pTestWeightMatrix->rows() == pDistTable->rows());
    QVERIFY(pTestWeightMatrix->cols() == pDistTable->cols());

    // random data set
    VectorXd testSignal = VectorXd::Random(vSmallSubset.size());

    QVERIFY(pTestWeightMatrix->cols() == testSignal.rows());

    // interpolate with random data set
    VectorXf vTestInterpolatedSignal = Interpolation::interpolateSignal(*pTestWeightMatrix, testSignal.cast<float>());

    QVERIFY(vTestInterpolatedSignal.rows() == smallSurface.rr.rows());
    QVERIFY(vTestInterpolatedSignal.cols() == 1);
}

//=============================================================================================================

void TestInterpolation::testSumOfRow()
{
    // projecting with MEG:
    QVector<int> vMappedSubSet = GeometryInfo::projectSensors(realSurface.rr,
                                                                vMegSensors);

    // SCDC with cancel distance 0.20 m:
    QSharedPointer<MatrixXd> pDistanceMatrix = GeometryInfo::scdc(realSurface.rr,
                                                 realSurface.neighbor_vert,
                                                 vMappedSubSet,
                                                 0.20);

    // filtering of bad channel
    GeometryInfo::filterBadChannels(pDistanceMatrix,
                                    evoked.info,
                                    FIFFV_MEG_CH);

    // weight matrix creation
    QSharedPointer<SparseMatrix<float>> pW = Interpolation::createInterpolationMat(vMappedSubSet,
                                                                  pDistanceMatrix,
                                                                  Interpolation::linear,
                                                                  0.20);

    qint32 n = pW->rows();
    qint32 m = pW->cols();

    const float LOWER_TRESH = 0.99999f;
    const float UPPER_TRESH = 1.00001f;

    for (int r = 0; r < n; ++r) {
        float fRowSum = 0.0f;
        for (int c = 0; c < m; ++c) {
            fRowSum += pW->coeff(r, c);
        }
        // either 1.0 (within range of some sensors) or 0.0 (out of range for all sensors)
        QVERIFY((fRowSum >= LOWER_TRESH && fRowSum <= UPPER_TRESH) || fRowSum == 0.0);
    }
}

//=============================================================================================================

void TestInterpolation::testEmptyInputsForWeightMatrix()
{
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> pDistTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, vSmallSubset, 0.03);

    // ---------- empty sensor indices ----------
    QVector<int> vEmptySensors;
    QVERIFY(Interpolation::createInterpolationMat(vEmptySensors,
                                                  pDistTable,
                                                  Interpolation::linear,
                                                  0.03)->size() == 0);

    // ---------- empty distance table ----------
    QSharedPointer<MatrixXd> pEmptypDistTable = QSharedPointer<MatrixXd>::create();
    QSharedPointer<SparseMatrix<float> > pResultMat = Interpolation::createInterpolationMat(vSmallSubset,
                                                                          pEmptypDistTable,
                                                                          Interpolation::linear,
                                                                          0.03);

    QVERIFY((pResultMat->rows() == 0) && (pResultMat->cols() == 0));
}

//=============================================================================================================

void TestInterpolation::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInterpolation)
#include "test_interpolation.moc"
