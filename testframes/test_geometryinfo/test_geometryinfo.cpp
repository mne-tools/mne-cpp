//=============================================================================================================
/**
* @file     test_geometryinfo.cpp
* @author   Felix Griesau <felix.griesau@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Felix Griesau and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <geometryInfo/geometryinfo.h>
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

using namespace GEOMETRYINFO;
using namespace MNELIB;

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
    QSharedPointer<QVector<qint32>> smallSubset;
};

//*************************************************************************************************************

TestGeometryInfo::TestGeometryInfo() {

}

//*************************************************************************************************************
void TestGeometryInfo::initTestCase() {
    //acquire real surface data
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    realSurface = t_sensorSurfaceVV[0];

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

    //generate random subset of test mesh of size subsetSize
    smallSubset = QSharedPointer<QVector<qint32>>::create();
    int subsetSize = rand() % 100;
    for (int b = 0; b <= subsetSize; b++) {
        // this allows duplicates, probably is not a problem
        smallSubset->push_back(rand() % 100);
    }
}

//*************************************************************************************************************

void TestGeometryInfo::testBadChannelFiltering() {
    //acquire real sensor positions
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return;
    }
    QVector<Vector3f> megSensors;
    for( const FiffChInfo &info : evoked.info.chs) {
        if(info.kind == FIFFV_MEG_CH) {
            megSensors.push_back(info.chpos.r0);
        }
    }

    // projecting with MEG:
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensors(realSurface, megSensors);
    // SCDC with cancel distance 0.03:
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(realSurface, mappedSubSet, 0.03);
    // filter for bad MEG channels:
    QVector<qint32> erasedColums = GeometryInfo::filterBadChannels(distanceMatrix, evoked, FIFFV_MEG_CH);

    for (qint32 col : erasedColums) {
        qint64 notInfCount = 0;
        for (qint32 row = 0; row < distanceMatrix->rows(); ++row) {
            if ((*distanceMatrix)(row, col) != DOUBLE_INFINITY) {
                notInfCount++;
            }
        }
        QVERIFY(notInfCount == 0);
    }
}

//*************************************************************************************************************

void TestGeometryInfo::testEmptyInputsForProjecting() {
    // sensor projecting:
    QVector<Vector3f> emptySensors;
    QVector<qint32> emptyMapping = *GeometryInfo::projectSensors(realSurface, emptySensors);
    QVERIFY(emptyMapping.size() == 0);
}

//*************************************************************************************************************

void TestGeometryInfo::testEmptyInputsForSCDC() {
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(smallSurface);
    QVERIFY(distTable->rows() == distTable->cols());
}

//*************************************************************************************************************

void TestGeometryInfo::testDimensionsForSCDC() {
    QSharedPointer<MatrixXd> distTable = GeometryInfo::scdc(smallSurface, smallSubset);
    QVERIFY(distTable->rows() == smallSurface.rr.rows());
    QVERIFY(distTable->cols() == smallSubset->size());
}

//*************************************************************************************************************

void TestGeometryInfo::cleanupTestCase() {

}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestGeometryInfo)
#include "test_geometryinfo.moc"
