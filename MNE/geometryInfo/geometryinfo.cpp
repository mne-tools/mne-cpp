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
#include<mne/mne_bem_surface.h>
#include "FiboHeap.hpp"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cmath>
#include <fstream>
#include <set>
#include <utility>
#include <chrono>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDateTime>
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

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MNEBemSurface &inSurface, const QVector<qint32> &vertSubset)
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

    // distribute on cores
    int cores = QThread::idealThreadCount();
    if (cores <= 0) {
        // assume that we have at least two available cores
        cores = 2;
    }
    // split the subset into equal parts
    // @todo check if original subset size is below number of cores (assign each core a single vertex in this case)
    QVector<QVector<qint32>> parts(cores);
    size_t partSize = vertSubset.size() / cores;
    for (int i = 0; i < cores - 1; ++i) {
        parts[i] = QVector<qint32>(partSize);
        for (int q = 0; q < parts[i].size(); ++q) {
            parts[i][q] = vertSubset[i * partSize + q];
        }
    }
    parts[cores - 1] = QVector<qint32>(vertSubset.size() - (cores - 1) * partSize);
    for (int q = 0; q < parts[cores - 1].size(); ++q) {
        parts[cores - 1][q] = vertSubset[(cores - 1) * partSize + q];
    }

    // start threads with their respective parts of the subset
    QVector<QFuture<void> > threads(cores);
    int offset = 0;
    for (int i = 0; i < threads.size(); ++i) {
        threads[i] = QtConcurrent::run(iterativeDijkstra, ptr, inSurface, parts[i], offset, 0.03);
        offset += parts[i].size();
    }

    // wait for all threads to finish
    bool finished = false;
    while (finished == false) {
        finished = true;
        for (QFuture<void> f : threads) {
            if (f.isFinished() == false) {
                finished = false;
            }
        }
        // @todo optimal value for this ?
        QThread::msleep(20);
    }

    return ptr;
}
//*************************************************************************************************************

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MNEBemSurface  &inSurface, double cancelDistance, const QVector<qint32> &vertSubSet)
{
    QSharedPointer<MatrixXd> outputMat = QSharedPointer<MatrixXd>::create();
    return outputMat;
}
//*************************************************************************************************************

QSharedPointer<QVector<qint32>> GeometryInfo::projectSensor(const MNEBemSurface &inSurface, const QVector<Vector3d> &sensorPositions)
{
    QSharedPointer<QVector<qint32>> outputArray = QSharedPointer<QVector<qint32>>::create();
    return outputArray;
}
//*************************************************************************************************************

// @todo maybe improve this: the algorithm relies on the assumption that the adjacency list is complete (size of adjacency list = biggest id + 1)

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> ptr, const MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet, int offset,  double cancelDist) {
    // initialization
    // @todo if this copies neighbor_vert, a pointer might be the more efficient option
    QVector<QVector<int> > adjacency = inSurface.neighbor_vert;
    qint32 n = adjacency.size();
    // have to use std::vector because QVector.resize takes only one argument
    std::vector<double> minDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    double INF = DOUBLE_INFINITY;

    // outer loop, iterated for each vertex in vertSubSet
    for (qint32 i = 0; i < vertSubSet.size(); ++i) {
        // init phase of dijkstra: set source node for current iteration and reset data fields
        qint32 root = vertSubSet[i];
        minDists.clear();
        vertexQ.clear();
        minDists.resize(n, INF);
        minDists[root] = 0;
        vertexQ.insert(std::make_pair(minDists[root], root));

        // dijkstra main loop
        while (vertexQ.empty() == false) {
            // remove next vertex from queue
            double dist = vertexQ.begin()->first;
            qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());
            // check if we are still below cancel distance
            if (dist <= cancelDist) {
                // visit each neighbour of u
                QVector<int> neighbours = adjacency[u];
                for (qint32 ne = 0; ne < neighbours.length(); ++ne) {
                    qint32 v = neighbours[ne];
                    // distance from source to v, using u as its predecessor
                    // calculate inline since designated function was magnitudes slower (even when declared as inline)
                    double distX = inSurface.rr(u, 0) - inSurface.rr(v, 0);
                    double distY = inSurface.rr(u, 1) - inSurface.rr(v, 1);
                    double distZ = inSurface.rr(u, 2) - inSurface.rr(v, 2);
                    double distWithU = dist + sqrt(distX * distX + distY * distY + distZ * distZ);

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
            (*ptr)(m , i + offset) = minDists[m];
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
