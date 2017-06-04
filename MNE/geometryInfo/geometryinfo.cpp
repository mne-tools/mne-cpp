//=============================================================================================================
/**
* @file     geometryinfo.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Mai, 2017
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
#include <mne/mne_bem_surface.h>

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

using namespace GEOMETRYINFO;
using namespace Eigen;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MNEBemSurface &inSurface, const QVector<qint32> &vertSubset, double cancelDist)
{
    // create matrix:
    size_t cols;
    if(!vertSubset.empty()) {
        cols = vertSubset.size();
    }
    else {
        cols = inSurface.rr.rows();
    }
    // convention: first dimension in distance table is "from", second dimension "to"
    QSharedPointer<MatrixXd> ptr = QSharedPointer<MatrixXd>::create(inSurface.rr.rows(), cols);

    // distribute calculation on cores
    int cores = QThread::idealThreadCount();
    if (cores <= 0) {
        // assume that we have at least two available cores
        cores = 2;
    }
    // start threads with their respective parts of the subset
    qint32 subArraySize = ceil(vertSubset.size() / cores);
    QVector<QFuture<void> > threads(cores - 1);
    qint32 begin = 0;
    qint32 end = subArraySize;
    for (int i = 0; i < threads.size(); ++i) {
        threads[i] = QtConcurrent::run(std::bind(iterativeDijkstra, ptr, std::cref(inSurface), std::cref(vertSubset), begin, end, cancelDist));
        begin += subArraySize;
        end += subArraySize;
    }
    // use main thread to calculate last part of the vertSubset
    iterativeDijkstra(ptr, inSurface, vertSubset, begin, vertSubset.size(), cancelDist);

    // wait for all other threads to finish
    bool finished = false;
    while (finished == false) {
        finished = true;
        for (const QFuture<void>& f : threads) {
            if (f.isFinished() == false) {
                finished = false;
            }
        }
        QThread::msleep(2);
    }

    return ptr;
}
//*************************************************************************************************************

QSharedPointer<QVector<qint32> > GeometryInfo::projectSensors(const MNEBemSurface &inSurface, const QVector<Vector3f> &sensorPositions)
{
    QSharedPointer<QVector<qint32>> outputArray = QSharedPointer<QVector<qint32>>::create();

    qint32 cores = QThread::idealThreadCount();
    if (cores <= 0)
    {
        // assume that we have at least two available cores
        cores = 2;
    }

    const qint32 subArraySize = ceil(sensorPositions.size() / cores);

    //small input size no threads needed
    // @todo best method ?? 16 thread prozessor ?
    if(subArraySize <= 1)
    {
        *outputArray = nearestNeighbor(inSurface, sensorPositions.constBegin(),sensorPositions.constEnd());
        return outputArray;
    }
    // split input array + thread start
    QVector<QFuture<QVector<qint32>>> threads(cores - 1);
    qint32 beginOffset = subArraySize;
    qint32 endOffset = beginOffset + subArraySize;
    for(qint32 i = 0; i < threads.size(); ++i)
    {
        //last round
        if(i == threads.size() -1)
        {
            threads[i] = QtConcurrent::run(nearestNeighbor, inSurface, sensorPositions.constBegin() + beginOffset, sensorPositions.constEnd());
            break;
        }
        else
        {
            threads[i] = QtConcurrent::run(nearestNeighbor, inSurface, sensorPositions.constBegin() + beginOffset, sensorPositions.constBegin() + endOffset);
            beginOffset = endOffset;
            endOffset += subArraySize;
        }
    }
    //calc while waiting for other threads
    outputArray->append(nearestNeighbor(inSurface, sensorPositions.constBegin(), sensorPositions.constBegin() + subArraySize));

    //wait for threads to finish
    bool finished = false;
        while (!finished) {
            finished = true;
            for (const auto &f : threads) {
                if (f.isFinished() == false) {
                    finished = false;
                }
            }
            // @todo optimal value for this ?
            QThread::msleep(2);
    }
    //move sub arrays back into output
    for(qint32 i = 0; i < threads.size(); ++i)
    {
        outputArray->append(threads[i].result());
    }

    return outputArray;
}
//*************************************************************************************************************

QVector<qint32> GeometryInfo::nearestNeighbor(const MNEBemSurface &inSurface,  QVector<Vector3f>::const_iterator sensorBegin, QVector<Vector3f>::const_iterator sensorEnd)
{
    ///lin search sensor positions
    QVector<qint32> mappedSensors;
    mappedSensors.reserve(std::distance(sensorBegin, sensorEnd));

    for(auto sensor = sensorBegin; sensor != sensorEnd; ++sensor)
    {
        qint32 championId;
        double champDist = std::numeric_limits<double>::max();
        for(qint32 i = 0; i < inSurface.rr.rows(); ++i)
        {
            double dist = sqrt(squared(inSurface.rr(i, 0) - (*sensor)[0])  // x-cord
                    + squared(inSurface.rr(i, 1) - (*sensor)[1])    // y-cord
                    + squared(inSurface.rr(i, 2) - (*sensor)[2]));  // z-cord
            if(dist < champDist)
            {
                championId = i;
                champDist = dist;
            }
        }
        mappedSensors.push_back(championId);
    }
    return mappedSensors;
}
//*************************************************************************************************************

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> ptr, const MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet, qint32 begin, qint32 end,  double cancelDist) {
    // initialization
    const QVector<QVector<int> > &adjacency = inSurface.neighbor_vert;
    qint32 n = adjacency.size();
    QVector<double> minDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    double INF = DOUBLE_INFINITY;

    // outer loop, iterated for each vertex of 'vertSubset' between 'begin' and 'end'
    for (qint32 i = begin; i < end; ++i) {
        // init phase of dijkstra: set source node for current iteration and reset data fields
        qint32 root = vertSubSet[i];
        vertexQ.clear();
        minDists.fill(INF);
        minDists[root] = 0.0;
        vertexQ.insert(std::make_pair(minDists[root], root));

        // dijkstra main loop
        while (vertexQ.empty() == false) {
            // remove next vertex from queue
            const double dist = vertexQ.begin()->first;
            const qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());
            // check if we are still below cancel distance
            if (dist <= cancelDist) {
                // visit each neighbour of u
                const QVector<int>& neighbours = adjacency[u];
                for (qint32 ne = 0; ne < neighbours.length(); ++ne) {
                    qint32 v = neighbours[ne];
                    // distance from source (i.e. root) to v, using u as its predecessor
                    // calculate inline since designated function was magnitudes slower (even when declared as inline)
                    const double distX = inSurface.rr(u, 0) - inSurface.rr(v, 0);
                    const double distY = inSurface.rr(u, 1) - inSurface.rr(v, 1);
                    const double distZ = inSurface.rr(u, 2) - inSurface.rr(v, 2);
                    const double distWithU = dist + sqrt(distX * distX + distY * distY + distZ * distZ);

                    if (distWithU < minDists[v]) {
                        // this is a combination of insert and decreaseKey
                        vertexQ.erase(std::make_pair(minDists[v], v));
                        minDists[v] = distWithU;
                        vertexQ.insert(std::make_pair(minDists[v], v));
                    }
                }
            }
        }
        // save results for current root in matrix
        for (qint32 m = 0; m < minDists.size(); ++m) {
            (*ptr)(m , i) = minDists[m];
        }
    }
}

//*************************************************************************************************************

void GeometryInfo::matrixDump(QSharedPointer<MatrixXd> ptr, std::string filename) {
    std::cout << "Start writing matrix to file: ";
    std::cout << filename.c_str() << std::endl;
    std::ofstream file;
    file.open(filename.c_str());
    file << *ptr;
    std::cout << "Finished writing !" << std::endl;
}
