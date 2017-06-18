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
class Test_interpolation : public QObject
{
    Q_OBJECT

public:
    Test_interpolation();

private slots:
    void initTestCase();
    void testDimensionsForInterpolation();
    void testSumOfRow();
    void cleanupTestCase();

};

Test_interpolation::Test_interpolation() {

}

//*************************************************************************************************************

void Test_interpolation::initTestCase() {

}

//*************************************************************************************************************

void Test_interpolation::testDimensionsForInterpolation() {
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

void Test_interpolation:: testSumOfRow()
{
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
    //GeometryInfo::matrixDump(testWeightMatrix, "./matrixDump.txt");

    const size_t n = testWeightMatrix->rows();
    const size_t m = testWeightMatrix->cols();

    double sumCols = 0;
    for (int r = 0; r < n; ++r) {
        double sumRow = 0;
        VectorXd row = testWeightMatrix->row(r);
        for (int q = 0; q < m; ++q) {
            //std::cout << "Cell: " << r << " " << q << " " << row[q] << "\n";
            sumRow += row[q];
        }
        sumCols += sumRow;
        //std::cout << "sum of Row: " << sumRow << "\n";
    }
    qDebug() << "Calculated sum of Columns: " << sumCols << " Colums of testWeightMatrix: " << n << "\n";
    QVERIFY(double(sumCols) == double(n));
}


//*************************************************************************************************************
void Test_interpolation::cleanupTestCase() {

}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(Test_interpolation)
#include "test_interpolation.moc"
