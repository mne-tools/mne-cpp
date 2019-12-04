//=============================================================================================================
/**
* @file     geometryinfo.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    GeometryInfo class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryinfo.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cmath>
#include <fstream>
#include <set>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent/QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MatrixX3f &matVertices,
                                            const QVector<QVector<int> > &vecNeighborVertices,
                                            QVector<qint32> &vecVertSubset,
                                            double dCancelDist)
{
    // create matrix and check for empty subset:
    qint32 iCols = vecVertSubset.size();
    if(vecVertSubset.empty()) {
        // caller passed an empty subset, need to fill in all vertex IDs
        qDebug() << "[WARNING] SCDC received empty subset, calculating full distance table, make sure you have enough memory !";
        vecVertSubset.reserve(matVertices.rows());
        for(qint32 id = 0; id < matVertices.rows(); ++id) {
            vecVertSubset.push_back(id);
        }
        iCols = matVertices.rows();
    }

    // convention: first dimension in distance table is "from", second dimension "to"
    QSharedPointer<MatrixXd> returnMat = QSharedPointer<MatrixXd>::create(matVertices.rows(), iCols);

    // distribute calculation on cores
    int iCores = QThread::idealThreadCount();
    if (iCores <= 0) {
        // assume that we have at least two available cores
        iCores = 2;
    }

    // start threads with their respective parts of the final subset
    qint32 iSubArraySize = int(ceil(double(vecVertSubset.size()) / double(iCores)));
    QVector<QFuture<void> > vecThreads(iCores - 1);
    qint32 iBegin = 0;
    qint32 iEnd = iSubArraySize;

    for (int i = 0; i < vecThreads.size(); ++i) {
        vecThreads[i] = QtConcurrent::run(std::bind(iterativeDijkstra,
                                                    returnMat,
                                                    std::cref(matVertices),
                                                    std::cref(vecNeighborVertices),
                                                    std::cref(vecVertSubset),
                                                    iBegin,
                                                    iEnd,
                                                    dCancelDist));
        iBegin += iSubArraySize;
        iEnd += iSubArraySize;
    }

    // use main thread to calculate last part of the final subset
    iterativeDijkstra(returnMat,
                      matVertices,
                      vecNeighborVertices,
                      vecVertSubset,
                      iBegin,
                      vecVertSubset.size(),
                      dCancelDist);

    // wait for all other threads to finish
    for (QFuture<void>& f : vecThreads) {
        f.waitForFinished();
    }

    return returnMat;
}


//*************************************************************************************************************

QVector<qint32> GeometryInfo::projectSensors(const MatrixX3f &matVertices,
                                             const QVector<Vector3f> &vecSensorPositions)
{
    QVector<qint32> vecOutputArray;

    qint32 iCores = QThread::idealThreadCount();
    if (iCores <= 0)
    {
        // assume that we have at least two available cores
        iCores = 2;
    }

    const qint32 iSubArraySize = ceil(vecSensorPositions.size() / iCores);

    //small input size no threads needed
    if(iSubArraySize <= 1)
    {
        vecOutputArray.append(nearestNeighbor(matVertices,
                                              vecSensorPositions.constBegin(),
                                              vecSensorPositions.constEnd()));
        return vecOutputArray;
    }
    // split input array + thread start
    QVector<QFuture<QVector<qint32> > > vecThreads(iCores - 1);
    qint32 iBeginOffset = iSubArraySize;
    qint32 iEndOffset = iBeginOffset + iSubArraySize;
    for(qint32 i = 0; i < vecThreads.size(); ++i)
    {
        //last round
        if(i == vecThreads.size() -1)
        {
            vecThreads[i] = QtConcurrent::run(nearestNeighbor,
                                              matVertices,
                                              vecSensorPositions.constBegin() + iBeginOffset,
                                              vecSensorPositions.constEnd());
            break;
        }
        else
        {
            vecThreads[i] = QtConcurrent::run(nearestNeighbor,
                                              matVertices,
                                              vecSensorPositions.constBegin() + iBeginOffset,
                                              vecSensorPositions.constBegin() + iEndOffset);
            iBeginOffset = iEndOffset;
            iEndOffset += iSubArraySize;
        }
    }
    //calc while waiting for other threads
    vecOutputArray.append(nearestNeighbor(matVertices,
                                          vecSensorPositions.constBegin(),
                                          vecSensorPositions.constBegin() + iSubArraySize));

    //wait for threads to finish
    for (QFuture<QVector<qint32> >& f : vecThreads) {
        f.waitForFinished();
    }

    //move sub arrays back into output
    for(qint32 i = 0; i < vecThreads.size(); ++i)
    {
        vecOutputArray.append(std::move(vecThreads[i].result()));
    }

    return vecOutputArray;
}


//*************************************************************************************************************

QVector<qint32> GeometryInfo::nearestNeighbor(const MatrixX3f &matVertices,
                                              QVector<Vector3f>::const_iterator itSensorBegin,
                                              QVector<Vector3f>::const_iterator itSensorEnd)
{
    ///lin search sensor positions
    QVector<qint32> vecMappedSensors;
    vecMappedSensors.reserve(std::distance(itSensorBegin, itSensorEnd));

    for(auto sensor = itSensorBegin; sensor != itSensorEnd; ++sensor)
    {
        qint32 iChampionId;
        double iChampDist = std::numeric_limits<double>::max();
        for(qint32 i = 0; i < matVertices.rows(); ++i)
        {
            //calculate 3d euclidian distance
            double dDist = sqrt(squared(matVertices(i, 0) - (*sensor)[0])  // x-cord
                    + squared(matVertices(i, 1) - (*sensor)[1])    // y-cord
                    + squared(matVertices(i, 2) - (*sensor)[2]));  // z-cord
            if(dDist < iChampDist)
            {
                iChampionId = i;
                iChampDist = dDist;
            }
        }
        vecMappedSensors.push_back(iChampionId);
    }
    return vecMappedSensors;
}


//*************************************************************************************************************

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> matOutputDistMatrix,
                                     const MatrixX3f &matVertices,
                                     const QVector<QVector<int> > &vecNeighborVertices,
                                     const QVector<qint32> &vecVertSubset,
                                     qint32 iBegin,
                                     qint32 iEnd,
                                     double dCancelDistance) {
    // initialization
    const QVector<QVector<int> > &vecAdjacency = vecNeighborVertices;
    qint32 n = vecAdjacency.size();
    QVector<double> vecMinDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    const double INF = FLOAT_INFINITY;

    // outer loop, iterated for each vertex of 'vertSubset' between 'begin' and 'end'
    for (qint32 i = iBegin; i < iEnd; ++i) {
        // init phase of dijkstra: set source node for current iteration and reset data fields
        qint32 iRoot = vecVertSubset.at(i);
        vertexQ.clear();
        vecMinDists.fill(INF);
        vecMinDists[iRoot] = 0.0;
        vertexQ.insert(std::make_pair(vecMinDists[iRoot], iRoot));

        // dijkstra main loop
        while (vertexQ.empty() == false) {
            // remove next vertex from queue
            const double dDist = vertexQ.begin()->first;
            const qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());

            // check if we are still below cancel distance
            if (dDist <= dCancelDistance) {
                // visit each neighbour of u
                const QVector<int>& vecNeighbours = vecAdjacency[u];

                for (qint32 ne = 0; ne < vecNeighbours.length(); ++ne) {
                    qint32 v = vecNeighbours[ne];

                    // distance from source (i.e. root) to v, using u as its predecessor
                    // calculate inline since designated function was magnitudes slower (even when declared as inline)
                    const double dDistX = matVertices(u, 0) - matVertices(v, 0);
                    const double dDistY = matVertices(u, 1) - matVertices(v, 1);
                    const double dDistZ = matVertices(u, 2) - matVertices(v, 2);
                    const double dDistWithU = dDist + sqrt(dDistX * dDistX + dDistY * dDistY + dDistZ * dDistZ);

                    if (dDistWithU < vecMinDists[v]) {
                        // this is a combination of insert and decreaseKey
                        vertexQ.erase(std::make_pair(vecMinDists[v], v));
                        vecMinDists[v] = dDistWithU;
                        vertexQ.insert(std::make_pair(vecMinDists[v], v));
                    }
                }
            }
        }

        // save results for current root in matrix
        for (qint32 m = 0; m < vecMinDists.size(); ++m) {
            matOutputDistMatrix->coeffRef(m , i) = vecMinDists[m];
        }
    }
}


//*************************************************************************************************************

QVector<qint32> GeometryInfo::filterBadChannels(QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                                const FIFFLIB::FiffInfo& fiffInfo,
                                                qint32 iSensorType) {
    // use pointer to avoid copying of FiffChInfo objects
    QVector<qint32> vecBadColumns;
    QVector<const FiffChInfo*> vecSensors;
    for(const FiffChInfo& s : fiffInfo.chs){
        //Only take EEG with V as unit or MEG magnetometers with T as unit
        if(s.kind == iSensorType && (s.unit == FIFF_UNIT_T || s.unit == FIFF_UNIT_V)){
           vecSensors.push_back(&s);
        }
    }

    // inefficient: going through all bad sensors, i.e. also the ones which are of different type than the passed one
    for(const QString& b : fiffInfo.bads){
        for(int col = 0; col < vecSensors.size(); ++col){
            if(vecSensors[col]->ch_name == b){
                // found index of our bad channel, set whole column to infinity
                vecBadColumns.push_back(col);
                for(int row = 0; row < matDistanceTable->rows(); ++row){
                    matDistanceTable->coeffRef(row, col) = FLOAT_INFINITY;
                }
                break;
            }
        }
    }
    return vecBadColumns;
}
