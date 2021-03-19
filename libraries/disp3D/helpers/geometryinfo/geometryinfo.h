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

#ifndef DISP3DLIB_GEOMETRYINFO_H
#define DISP3DLIB_GEOMETRYINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"
#include <fiff/fiff_evoked.h>

//=============================================================================================================
// INCLUDES
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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEmatVertices;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {

#define FLOAT_INFINITY std::numeric_limits<float>::infinity()

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class allows sensor-to-mesh mapping and calculation of surface constrained distances.
 * Given the positions of a row of sensors in 3D space, it finds the best fitting vertex of an underlying mesh.
 * This can be used for later signal interpolation (see class Interpolation for more details).
 * Given a mesh, the class can calculate shortest path on said mesh. It outputs a distance table.
 *
 * @brief This class holds static methods for sensor-to-mesh mapping and surface constrained distance calculation on a mesh
 *
 */

class DISP3DSHARED_EXPORT GeometryInfo
{

public:
    typedef QSharedPointer<GeometryInfo> SPtr;            /**< Shared pointer type for GeometryInfo. */
    typedef QSharedPointer<const GeometryInfo> ConstSPtr; /**< Const shared pointer type for GeometryInfo. */

    //=========================================================================================================
    /**
     * deleted default constructor (static class).
     */
    GeometryInfo() = delete;

    //=========================================================================================================
    /**
     * @brief scdc                           Calculates surface constrained distances on a mesh.
     *
     * @param[in] matVertices                The surface on which distances should be calculated.
     * @param[in] vecNeighborVertices        The neighbor vertex information.
     * @param[in/out] pVecVertSubset         The subset of IDs for which the distances should be calculated.
     * @param[in] dCancelDist                Distances higher than this are ignored, i.e. set to infinity.
     *
     * @return                               A double matrix. One column represents the distances for one vertex inside of the passed subset.
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const Eigen::MatrixX3f &matVertices,
                                                const QVector<QVector<int> > &vecNeighborVertices,
                                                QVector<int> &pVecVertSubset,
                                                double dCancelDist = FLOAT_INFINITY);

    //=========================================================================================================
    /**
     * @brief                            Calculates the nearest neighbor (euclidian distance) vertex to each sensor
     *
     * @param[in] matVertices            Holds all vertex information that is needed.
     * @param[in] vecSensorPositions     Each sensor postion in saved in an Eigen vector with x, y & z coord.
     *
     * @return                           Output vector where the vector index position represents the id of the sensor.
     *                                   and the int in each cell is the vertex it is mapped to
     */
    static QVector<int> projectSensors(const Eigen::MatrixX3f &matVertices,
                                       const QVector<Eigen::Vector3f> &vecSensorPositions);

    //=========================================================================================================
    /**
     * @brief filterBadChannels          Filters bad channels from distance table
     *
     * @param[out] matDistanceTable      Result of SCDC.
     * @param[in] fiffInfo               Container for sensors.
     * @param[in] iSensorType            Sensor type to be filtered out, use fiff constants.
     *
     * @return Vector of bad channel indices.
     */
    static QVector<int> filterBadChannels(QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                          const FIFFLIB::FiffInfo& fiffInfo,
                                          qint32 iSensorType);

protected:
    //=========================================================================================================
    /**
     * @brief squared        Implemented for better readability only
     *
     * @param[in] dBase      Base double value.
     *
     * @return               Base squared.
     */
    static inline  double squared(double dBase);

    //=========================================================================================================
    /**
     * @brief nearestNeighbor        Calculates the nearest vertex of an MNEmatVertices for each position between the two iterators
     *
     * @param[in] matVertices        The MNEmatVertices that holds the vertex information.
     * @param[in] itSensorBegin      The iterator that indicates the start of the wanted section of positions.
     * @param[in] itSensorEnd        The iterator that indicates the end of the wanted section of positions.
     *
     * @return                       A vector of nearest vertex IDs that corresponds to the subvector between the two iterators.
     */
    static QVector<int> nearestNeighbor(const Eigen::MatrixX3f &matVertices,
                                        QVector<Eigen::Vector3f>::const_iterator itSensorBegin,
                                        QVector<Eigen::Vector3f>::const_iterator itSensorEnd);

    //=========================================================================================================
    /**
     * @brief iterativeDijkstra     Calculates shortest distances on the mesh that is held by the MNEmatVertices for each vertex of the passed vector that lies between the two indices
     *
     * @param[out] matOutputDistMatrix  The matrix in which the distances will be stored.
     * @param[in] matVertices           The surface on which distances should be calculated.
     * @param[in] vecNeighborVertices   The neighbor vertex information.
     * @param[in] vecVertSubset         The subset of vertices.
     * @param[in] iBegin                Start index of distance calculation.
     * @param[in] iEnd                  End index of distance calculation, exclusive.
     * @param[in] dCancelDistance       Distance threshold: all vertices that have a higher distance to the respective root vertex are set to infinity.
     */
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

inline double GeometryInfo::squared(double dBase)
{
    return dBase * dBase;
}
} // namespace GEOMETRYINFO

#endif // DISP3DLIB_GEOMETRYINFO_H
