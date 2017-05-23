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
     * @brief calcualtes the nearest neighbor (euclidian distance) vertex to each sensor
     * @param inSurface: holds all vertex information that is needed for the claculation in its public member rr
     * @param sensorPositions: each sensor postion in saved in an Eigen vector with x, y & z cord.
     * @return  pointer to output vector where the vecotr index position represents the id of the sensor and the int in each cell is the vertex it is mapped to
     */
    static QSharedPointer<QVector<qint32>> linProjectSensor(const MNELIB::MNEBemSurface &inSurface, const QVector<Eigen::Vector3d> &sensorPositions);


protected:

private:

    static inline  double squared(double base);
    static QVector<qint32> nearestNeighbor(const MNELIB::MNEBemSurface &inSurface,  QVector<Eigen::Vector3d>::const_iterator sensorBegin, QVector<Eigen::Vector3d>::const_iterator sensorEnd);
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
