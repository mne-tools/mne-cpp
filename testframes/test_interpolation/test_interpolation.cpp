//=============================================================================================================
/**
* @file     test_interpolation.cpp
* @author   Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Sugandha Sachdeva and Matti Hamalainen. All rights reserved.
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

#include <geometryInfo/geometryinfo.h>
#include <interpolation/interpolation.h>
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

using namespace INTERPOLATION;
using namespace GEOMETRYINFO;
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

};

TestInterpolation::TestInterpolation() {

}

//*************************************************************************************************************

void TestInterpolation::initTestCase() {

}

//*************************************************************************************************************

void TestInterpolation::testDimensionsForInterpolation() {
    // scdc:
    // generate small test mesh with 100 vertices:
    MNEBemSurface testMesh;
    // generate random vertex positions
    MatrixX3f vertPos(100, 3);
    for(qint8 i = 0; i < 100; i++) {
        float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        vertPos(i, 0) = x;
        vertPos(i, 1) = y;
        vertPos(i, 2) = z;
    }
    testMesh.rr = vertPos;

    // generate random adjacency, assume that every vertex has 4 neighbors
    for (int i = 0; i < 100; ++i) {
        QVector<int> neighborList;
        for (int a = 0; a < 4; ++a) {
            // this allows duplicates, probably is not a problem
            neighborList.push_back(rand() % 100);
        }
        testMesh.neighbor_vert.push_back(neighborList);
    }

    // generate random subset of test mesh of size subsetSize
    int subsetSize = rand() % 100;
    QVector<qint32> testSubset;
    for (int b = 0; b <= subsetSize; b++) {
        // this allows duplicates, probably is not a problem
        testSubset.push_back(rand() % 100);
    }

    // create weight matrix from distance table
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(testMesh, testSubset);
    Interpolation::createInterpolationMat(testSubset, distTable, Interpolation::linear);
    QSharedPointer<SparseMatrix<double>> testWeightMatrix = Interpolation::getResult();

    QVERIFY(testWeightMatrix->rows() == testMesh.rr.rows());
    QVERIFY(testWeightMatrix->cols() == testSubset.size());

    // random data set
    VectorXd testSignal = VectorXd::Random(testSubset.size());

    QVERIFY(testWeightMatrix->cols() == testSignal.rows());

    // interpolate with random data set
    QSharedPointer<VectorXf> testInterpolatedSignal = Interpolation::interpolateSignal(testSignal);

    QVERIFY(testInterpolatedSignal->rows() == testMesh.rr.rows());
    QVERIFY(testInterpolatedSignal->cols() == 1);
}

//*************************************************************************************************************

void TestInterpolation::testSumOfRow() {
    //acquire real data
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    MNEBemSurface testSurface = t_sensorSurfaceVV[0];
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty()) {
        return;
    }
    QVector<Vector3f> megSensors;
    for( const FiffChInfo &info : evoked.info.chs) {
        if(info.kind == FIFFV_MEG_CH) {
            megSensors.push_back(info.chpos.r0);
        }
    }
    // projecting with MEG:
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensors(testSurface, megSensors);
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(testSurface, *mappedSubSet, 0.03);
    // weight matrix creation
    Interpolation::createInterpolationMat(*mappedSubSet, distanceMatrix, Interpolation::linear, 0.03);

    QSharedPointer<SparseMatrix<double> > w = Interpolation::getResult();
    qint32 n = w->rows(), m = w->cols();

    const double LOWER_TRESH = 0.99999999;
    const double UPPER_TRESH = 1.000000001;

    for (int r = 0; r < n; ++r) {
        double rowSum = 0.0;
        for (int c = 0; c < m; ++c) {
            rowSum += w->coeff(r, c);
        }
        // either 1.0 (within range or some sensors) or 0.0 (outside of range for all sensors)
        QVERIFY((rowSum >= LOWER_TRESH && rowSum <= UPPER_TRESH) || rowSum == 0.0);
    }
}

//*************************************************************************************************************

void TestInterpolation::testEmptyInputsForWeightMatrix() {
    //acquire surface data
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

    //acquire sensor positions
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return;
    }
    QVector<Vector3f> eegSensors;
    QVector<Vector3f> megSensors; //currently not used
    for( const FiffChInfo &info : evoked.info.chs) {
        if(info.kind == FIFFV_EEG_CH) {
            eegSensors.push_back(info.chpos.r0);
        }
        if(info.kind == FIFFV_MEG_CH) {
            megSensors.push_back(info.chpos.r0);
        }
    }

    // projecting with EEG:
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensors(t_sensorSurfaceVV[0], megSensors);
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(t_sensorSurfaceVV[0], *mappedSubSet, 0.03);


    // ---------- empty sensor indices ----------
    QVector<qint32> emptySensors;
    Interpolation::createInterpolationMat(emptySensors, distTable, Interpolation::linear, 0.03);
    QVERIFY(Interpolation::getResult()->size() == 0);
    Interpolation::clearInterpolateMatrix();


    // ---------- empty distance table ----------
    QSharedPointer<MatrixXd> emptyDistTable;
    Interpolation::createInterpolationMat(*mappedSubSet, emptyDistTable, Interpolation::linear, 0.03);
    QVERIFY(Interpolation::getResult() == NULL);
    Interpolation::clearInterpolateMatrix();
}

//*************************************************************************************************************

void TestInterpolation::cleanupTestCase() {

}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestInterpolation)
#include "test_interpolation.moc"
