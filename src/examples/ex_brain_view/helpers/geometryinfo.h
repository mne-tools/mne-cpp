//=============================================================================================================
/**
 * @file     geometryinfo.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief     GeometryInfo class declaration.
 *
 */

#ifndef GEOMETRYINFO_H
#define GEOMETRYINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <limits>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace BRAINVIEWLIB {

#define FLOAT_INFINITY std::numeric_limits<float>::infinity()

//=============================================================================================================
/**
 * This class allows sensor-to-mesh mapping and calculation of surface constrained distances.
 *
 * @brief This class holds static methods for sensor-to-mesh mapping and surface constrained distance calculation on a mesh
 */

class GeometryInfo
{

public:
    typedef QSharedPointer<GeometryInfo> SPtr;
    typedef QSharedPointer<const GeometryInfo> ConstSPtr;

    GeometryInfo() = delete;

    //=========================================================================================================
    /**
     * @brief scdc   Calculates surface constrained distances on a mesh.
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const Eigen::MatrixX3f &matVertices,
                                                const QVector<QVector<int> > &vecNeighborVertices,
                                                QVector<int> &pVecVertSubset,
                                                double dCancelDist = FLOAT_INFINITY);

    //=========================================================================================================
    /**
     * @brief projectSensors   Calculates the nearest neighbor vertex to each sensor.
     */
    static QVector<int> projectSensors(const Eigen::MatrixX3f &matVertices,
                                       const QVector<Eigen::Vector3f> &vecSensorPositions);

    //=========================================================================================================
    /**
     * @brief filterBadChannels   Filters bad channels from distance table.
     */
    static QVector<int> filterBadChannels(QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                          const FIFFLIB::FiffInfo& fiffInfo,
                                          qint32 iSensorType);

protected:
    static inline double squared(double dBase);

    static QVector<int> nearestNeighbor(const Eigen::MatrixX3f &matVertices,
                                        QVector<Eigen::Vector3f>::const_iterator itSensorBegin,
                                        QVector<Eigen::Vector3f>::const_iterator itSensorEnd);

    static void iterativeDijkstra(QSharedPointer<Eigen::MatrixXd> matOutputDistMatrix,
                                  const Eigen::MatrixX3f &matVertices,
                                  const QVector<QVector<int> > &vecNeighborVertices,
                                  const QVector<int> &vecVertSubset,
                                  qint32 iBegin,
                                  qint32 iEnd,
                                  double dCancelDistance);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

double GeometryInfo::squared(double dBase)
{
    return dBase * dBase;
}

} // namespace BRAINVIEWLIB

#endif // GEOMETRYINFO_H
