//=============================================================================================================
/**
* @file     geometryinfo.h
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
* @brief     GeometryInfo class declaration.
*
*/

#ifndef GEOMETRYINFO_GEOMETRYINFO_H
#define GEOMETRYINFO_GEOMETRYINFO_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryinfo_global.h"
#include <fiff/fiff_evoked.h>

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <limits>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>



//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEBemSurface;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SWP
//=============================================================================================================

namespace GEOMETRYINFO {

#define DOUBLE_INFINITY std::numeric_limits<double>::infinity()

//*************************************************************************************************************
//=============================================================================================================
// SWP FORWARD DECLARATIONS
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

class GEOMETRYINFOSHARED_EXPORT GeometryInfo
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
     * @brief scdc Calculates surface constrained distances on the mesh that is held by the passed MNEBemSurface
     * @param inSurface The surface on which distances should be calculated
     * @param vertSubSet The subset of IDs for which the distances should be calculated
     * @param cancelDistance Distances higher than this are ignored, i.e. set to infinity
     * @return A shared pointer to a double matrix. One column represents the distances for one vertex inside of the passed subset
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const MNELIB::MNEBemSurface &inSurface, const QSharedPointer<QVector<qint32>> vertSubset = QSharedPointer<QVector<qint32>>::create(), double cancelDist = DOUBLE_INFINITY);

    //=========================================================================================================
    /**
     * @brief Calculates the nearest neighbor (euclidian distance) vertex to each sensor
     * @param inSurface: Holds all vertex information that is needed (public member rr)
     * @param sensorPositions: Each sensor postion in saved in an Eigen vector with x, y & z coord.
     * @return  Pointer to output vector where the vector index position represents the id of the sensor and the int in each cell is the vertex it is mapped to
     */
    static QSharedPointer<QVector<qint32>> projectSensors(const MNELIB::MNEBemSurface &inSurface, const QVector<Eigen::Vector3f> &sensorPositions);

    //=========================================================================================================
    /**
     * @brief matrixDump Creates a file named 'filename' and writes the contents of ptr into it
     * @param ptr The matrix to be written
     * @param filename The file to be written to
     */
    static void matrixDump(QSharedPointer<Eigen::MatrixXd> ptr, std::string filename);

    /**
     * @brief filterBadChannels Filters bad channels from distance table
     * @param distanceTable Result of SCDC
     * @param evoked Container for sensors
     * @param sensorType Sensor type to be filtered out, use fiff constants
     */
    static QVector<qint32> filterBadChannels(QSharedPointer<Eigen::MatrixXd> distanceTable, const FIFFLIB::FiffEvoked& evoked, qint32 sensorType);

protected:

private:

    //=========================================================================================================
    /**
     * @brief squared Implemented for better readability only
     * @param base Base double value
     * @return Base squared
     */
    static inline  double squared(double base);

    //=========================================================================================================
    /**
     * @brief nearestNeighbor Calculates the nearest vertex of an MNEBemSurface for each position between the two iterators
     * @param inSurface The MNEBemSurface that holds the vertex information
     * @param sensorBegin The iterator that indicates the start of the wanted section of positions
     * @param sensorEnd The iterator that indicates the end of the wanted section of positions
     * @return A vector of nearest vertex IDs that corresponds to the subvector between the two iterators
     */
    static QVector<qint32> nearestNeighbor(const MNELIB::MNEBemSurface &inSurface,  QVector<Eigen::Vector3f>::const_iterator sensorBegin, QVector<Eigen::Vector3f>::const_iterator sensorEnd);

    //=========================================================================================================
    /**
     * @brief iterativeDijkstra Calculates shortest distances on the mesh that is held by the MNEBemsurface for each vertex of the passed vector that lies between the two indices
     * @param ptr The matrix in which the distances will be stored
     * @param inSurface The surface on which distances should be calculated
     * @param vertSubSet The subset of vertices
     * @param begin Start index of distance calculation
     * @param end   End index of distance calculation, exclusive
     * @param cancelDist Distance threshold: all vertices that have a higher distance to the respective root vertex are set to infinity
     */
    static void iterativeDijkstra(QSharedPointer<Eigen::MatrixXd> ptr, const MNELIB::MNEBemSurface &inSurface, const QSharedPointer<QVector<qint32> > vertSubSet, qint32 begin, qint32 end, double cancelDist);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline double GeometryInfo::squared(double base)
{
    return base * base;
}

} // namespace GEOMETRYINFO

#endif // GEOMETRYINFO_GEOMETRYINFO_H
