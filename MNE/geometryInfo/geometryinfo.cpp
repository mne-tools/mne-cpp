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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <utility>
#include <chrono>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDateTime>

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

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet)
{
    //start timer
    qint64 startTimeMsecs = QDateTime::currentMSecsSinceEpoch();

    size_t matColumns;
    if(!vertSubSet.empty()) {
        matColumns = vertSubSet.size();
    }
    else {
        matColumns = inSurface.rr.rows();
    }
    // convention: first dimension in distance table is "from", second dimension "to"
    QSharedPointer<MatrixXd> ptr = QSharedPointer<MatrixXd>::create(inSurface.rr.rows(), matColumns);

    iterativeDijkstra(ptr, inSurface, 0.04, vertSubSet);

    std::cout << "Iterative Dijkstra took ";
    std::cout << QDateTime::currentMSecsSinceEpoch()- startTimeMsecs <<" ms " << std::endl;

    // matrixDump(ptr, "output.txt");

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

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> ptr, const MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet) {
    // Initialization
    // @todo if this copies neighbor_vert, a pointer might be the more efficient option
    QMap<int, QVector<int> > adjacency = inSurface.neighbor_vert;
    qint32 n = adjacency.size();
    // have to use std::vector because QVector.resize takes only one argument
    std::vector<double> minDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    double MAX_WEIGHT = std::numeric_limits<double>::infinity();

    // outer loop, iterated for each vertex in vertSubSet
    qint64 aCounter = 0, b1Counter = 0, b2Counter = 0, nCounter = 0;
    for (qint32 i = 0; i < vertSubSet.size(); ++i) {
        std::cout << "-- " << i << std::endl;
        // set source node for current iteration and reset data fields

        // begin block A
        auto begin = std::chrono::high_resolution_clock::now();
        qint32 root = vertSubSet[i];
        minDists.clear();
        vertexQ.clear();
        // init phase of dijkstra
        minDists.resize(n, MAX_WEIGHT);
        minDists[root] = 0;
        vertexQ.insert(std::make_pair(minDists[root], root));
        auto end = std::chrono::high_resolution_clock::now();
        // std::cout << "A " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
        aCounter += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

        // dijkstra main loop
        while (vertexQ.empty() == false) {

            // begin block B1
            auto begin = std::chrono::high_resolution_clock::now();
            // remove next vertex from queue
            double dist = vertexQ.begin()->first;
            qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());
            auto end = std::chrono::high_resolution_clock::now();
            b1Counter += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

            // begin block B2
            begin = std::chrono::high_resolution_clock::now();
            // visit each neighbour of u
            QVector<int> neighbours = adjacency.find(u).value();
            end = std::chrono::high_resolution_clock::now();
            // std::cout << "\tB " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
            b2Counter += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

            // begin block N
            begin = std::chrono::high_resolution_clock::now();
            for (qint32 ne = 0; ne < neighbours.length(); ++ne) {
                // auto begin = std::chrono::high_resolution_clock::now();
                qint32 v = neighbours[ne];
                // distance from source to v, using u as its predecessor
                // calculate inline since designated function was magnitudes slower (even when declared as inline)
                double distX = inSurface.rr(u, 0) - inSurface.rr(v, 0);
                double distY = inSurface.rr(u, 1) - inSurface.rr(v, 1);
                double distZ = inSurface.rr(u, 2) - inSurface.rr(v, 2);

                double distWithU = dist + sqrt(distX * distX + distY * distY + distZ * distZ);
                if (distWithU < minDists[v]) {
                    // this is a combination of insert and decreaseKey
                    // @todo need an effort estimate for this (complexity ?)
                    vertexQ.erase(std::make_pair(minDists[v], v));
                    minDists[v] = distWithU;
                    vertexQ.insert(std::make_pair(minDists[v], v));
                }
                // auto end = std::chrono::high_resolution_clock::now();
                // std::cout << "\t\tN " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
            }
            end = std::chrono::high_resolution_clock::now();
            nCounter += std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
        }
        // save results in matrix
        for (qint32 m = 0; m < minDists.size(); ++m) {
            (*ptr)(m , i) = minDists[m];
        }
    }
    std::cout << "A     " << aCounter << std::endl;
    std::cout << "B1  " << b1Counter << std::endl;
    std::cout << "B2 " << b2Counter << std::endl;
    std::cout << "N  " << nCounter << std::endl;
}
//*************************************************************************************************************

// @todo maybe improve this: the algorithm relies on the assumption that the adjacency list is complete (size of adjacency list = biggest id + 1)

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> ptr, const MNEBemSurface &inSurface, double cancelDist, const QVector<qint32> &vertSubSet) {
    // initialization
    // @todo if this copies neighbor_vert, a pointer might be the more efficient option
    QMap<int, QVector<int> > adjacency = inSurface.neighbor_vert;

    // sort adjacency information so that we can access it later in constant time (instead of logarithmic)
    QVector<QPair<int, QVector<int> > > versuch;
    while (adjacency.empty() == false) {
        versuch.push_back(qMakePair(adjacency.firstKey(), adjacency.first()));
        // @todo does this empty the original QMap in inSurface ? could be a problem
        adjacency.erase(adjacency.begin());
    }

//    qint64 edges = 0;
//    for (QPair<int, QVector<int> > a : versuch) {
//        edges += a.second.size();
//    }
//    std::cout << edges << std::endl;

    qint32 n = versuch.size();
    // have to use std::vector because QVector.resize takes only one argument
    std::vector<double> minDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    double MAX_WEIGHT = std::numeric_limits<double>::infinity();

    // outer loop, iterated for each vertex in vertSubSet
    for (qint32 i = 0; i < vertSubSet.size(); ++i) {
        // init phase of dijkstra: set source node for current iteration and reset data fields
        qint32 root = vertSubSet[i];
        minDists.clear();
        vertexQ.clear();
        minDists.resize(n, MAX_WEIGHT);
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
                QVector<int> neighbours = versuch[u].second;
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
            (*ptr)(m , i) = minDists[m];
        }
    }
}
//*************************************************************************************************************

// @todo why is this so slow ? maybe dont use pow
inline double GeometryInfo::distanceBetween(MatrixX3f nodes, qint32 u, qint32 v) {
    return sqrt(pow(nodes(u, 0) - nodes(v, 0), 2) +  pow(nodes(u, 1) - nodes(v, 1), 2) +  pow(nodes(u, 2) - nodes(v, 2), 2));
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
