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


#include"geometryinfo_global.h"



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

//*************************************************************************************************************
//=============================================================================================================
// SWP FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
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
     * @brief scdc
     * @param inSurface
     * @param vertSubSet
     * @return
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const MNELIB::MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet = QVector<qint32>());

    //=========================================================================================================
    /**
     * @brief scdc
     * @param inSurface
     * @param cancelDistance
     * @param vertSubSet
     * @return
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const MNELIB::MNEBemSurface &inSurface, double cancelDistance, const QVector<qint32> &vertSubSet = QVector<qint32>());

    //=========================================================================================================
    /**
     * @brief projectSensor
     * @param inSurface
     * @param sensorPositions
     * @return
     */
    static QSharedPointer<QVector<qint32>> projectSensor(const MNELIB::MNEBemSurface &inSurface, const QVector<Eigen::Vector3d> &sensorPositions);

    //=========================================================================================================
    /**
     * @brief iterativeDijkstra Calculates shortest distances for each vertex of vertSubSet
     * @param ptr The matrix in which the distances will be stored
     * @param inSurface
     * @param vertSubSet
     * @return
     */
    static void iterativeDijkstra(QSharedPointer<Eigen::MatrixXd> ptr, const MNELIB::MNEBemSurface &inSurface, const QVector<qint32> &vertSubSet = QVector<qint32>());

    //=========================================================================================================
    /**
     * @brief iterativeDijkstra Calculates shortest distances for each vertex of vertSubSet
     * @param ptr The matrix in which the distances will be stored
     * @param inSurface
     * @param cancelDist Vertices that have a distance to root that is higher than canceldist are ignored, i.e. set to MAX_WEIGHT
     * @param vertSubSet
     */
    static void iterativeDijkstra(QSharedPointer<Eigen::MatrixXd> ptr, const MNELIB::MNEBemSurface &inSurface, double cancelDist, const QVector<qint32> &vertSubSet = QVector<qint32>());

    //=========================================================================================================
    /**
     * @brief distanceBetween Returns the euclidian distance between nodes u and v
     * @param nodes Absolute positions in 3D space
     * @param u
     * @param v
     * @return
     */
    static double distanceBetween(Eigen::MatrixX3f nodes, qint32 u, qint32 v);

    //=========================================================================================================
    /**
     * @brief matrixDump Creates a file named 'filename' and writes the contents of ptr into it
     * @param ptr The matrix to be written
     * @param filename The file to be written to
     */
    static void matrixDump(QSharedPointer<Eigen::MatrixXd> ptr, std::string filename);

protected:

private:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace GEOMETRYINFO

#endif // GEOMETRYINFO_GEOMETRYINFO_H
