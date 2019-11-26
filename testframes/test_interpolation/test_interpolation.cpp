//=============================================================================================================
/**
* @file     test_interpolation.cpp
* @author   Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Sugandha Sachdeva and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/interpolation/interpolation.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <string>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;


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
    QVector<Vector3f> megSensors;
    FiffEvoked evoked;
    // random data (keep computation times short)
    MNEBemSurface smallSurface;
    QVector<qint32> smallSubset;
};

//*************************************************************************************************************

TestInterpolation::TestInterpolation()
{

}

//*************************************************************************************************************

void TestInterpolation::initTestCase()
{
    //acquire real data
    QFile t_filesensorSurfaceVV("./mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    realSurface = t_sensorSurfaceVV[0];
    QFile t_fileEvoked("./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    evoked = FiffEvoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty()) {
        return;
    }
    for( const FiffChInfo &info : evoked.info.chs) {
        if(info.kind == FIFFV_MEG_CH && info.unit == FIFF_UNIT_T) {
            megSensors.push_back(info.chpos.r0);
        }
    }

    // generate small test mesh with 100 vertices:
    MatrixX3f vertPos(100, 3);
    for(qint8 i = 0; i < 100; i++) {
        float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        vertPos(i, 0) = x;
        vertPos(i, 1) = y;
        vertPos(i, 2) = z;
    }
    smallSurface.rr = vertPos;

    // generate random adjacency, assume that every vertex has 4 neighbors
    for (int i = 0; i < 100; ++i) {
        QVector<int> neighborList;
        for (int a = 0; a < 4; ++a) {
            // this allows duplicates, probably is not a problem
            neighborList.push_back(rand() % 100);
        }
        smallSurface.neighbor_vert.push_back(neighborList);
    }

    // generate random subset of test mesh of size subsetSize
    int subsetSize = rand() % 100;
    for (int b = 0; b <= subsetSize; b++) {
        // this allows duplicates, probably is not a problem
        smallSubset.push_back(rand() % 100);
    }
}


//*************************************************************************************************************

void TestInterpolation::testDimensionsForInterpolation()
{
    // create weight matrix from distance table
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, smallSubset);
    QSharedPointer<SparseMatrix<float> > testWeightMatrix = Interpolation::createInterpolationMat(smallSubset,
                                                                                 distTable,
                                                                                 Interpolation::linear);

    QVERIFY(testWeightMatrix->rows() == distTable->rows());
    QVERIFY(testWeightMatrix->cols() == distTable->cols());

    // random data set
    VectorXd testSignal = VectorXd::Random(smallSubset.size());

    QVERIFY(testWeightMatrix->cols() == testSignal.rows());

    // interpolate with random data set
    VectorXf testInterpolatedSignal = Interpolation::interpolateSignal(*testWeightMatrix, testSignal.cast<float>());

    QVERIFY(testInterpolatedSignal.rows() == smallSurface.rr.rows());
    QVERIFY(testInterpolatedSignal.cols() == 1);
}


//*************************************************************************************************************

void TestInterpolation::testSumOfRow()
{
    // projecting with MEG:
    QVector<qint32> mappedSubSet = GeometryInfo::projectSensors(realSurface.rr,
                                                                megSensors);

    // SCDC with cancel distance 0.20 m:
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(realSurface.rr,
                                                 realSurface.neighbor_vert,
                                                 mappedSubSet,
                                                 0.20);

    // filtering of bad channel
    GeometryInfo::filterBadChannels(distanceMatrix,
                                    evoked.info,
                                    FIFFV_MEG_CH);

    // weight matrix creation
    QSharedPointer<SparseMatrix<float> > w = Interpolation::createInterpolationMat(mappedSubSet,
                                                                  distanceMatrix,
                                                                  Interpolation::linear,
                                                                  0.20);

    qint32 n = w->rows();
    qint32 m = w->cols();

    const float LOWER_TRESH = 0.99999f;
    const float UPPER_TRESH = 1.00001f;

    for (int r = 0; r < n; ++r) {
        float rowSum = 0.0f;
        for (int c = 0; c < m; ++c) {
            rowSum += w->coeff(r, c);
        }
        // either 1.0 (within range of some sensors) or 0.0 (out of range for all sensors)
        QVERIFY((rowSum >= LOWER_TRESH && rowSum <= UPPER_TRESH) || rowSum == 0.0);
    }
}

//*************************************************************************************************************

void TestInterpolation::testEmptyInputsForWeightMatrix()
{
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(smallSurface.rr, smallSurface.neighbor_vert, smallSubset, 0.03);

    // ---------- empty sensor indices ----------
    QVector<qint32> emptySensors;
    QVERIFY(Interpolation::createInterpolationMat(emptySensors,
                                                  distTable,
                                                  Interpolation::linear,
                                                  0.03)->size() == 0);

    // ---------- empty distance table ----------
    QSharedPointer<MatrixXd> emptyDistTable = QSharedPointer<MatrixXd>::create();
    QSharedPointer<SparseMatrix<float> > resultMat = Interpolation::createInterpolationMat(smallSubset,
                                                                          emptyDistTable,
                                                                          Interpolation::linear,
                                                                          0.03);

    QVERIFY((resultMat->rows() == 0) && (resultMat->cols() == 0));
}

//*************************************************************************************************************

void TestInterpolation::cleanupTestCase()
{

}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestInterpolation)
#include "test_interpolation.moc"
