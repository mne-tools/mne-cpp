//=============================================================================================================
/**
 * @file     rtsensorinterpolationmatworker.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2017
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
 * @brief    RtSensorInterpolationMatWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorinterpolationmatworker.h"
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorInterpolationMatWorker::RtSensorInterpolationMatWorker()
: m_bInterpolationInfoIsInit(false)
{
    m_lInterpolationData.dCancelDistance = 0.05;
    m_lInterpolationData.interpolationFunction = DISP3DLIB::Interpolation::cubic;
    m_lInterpolationData.matDistanceMatrix = QSharedPointer<MatrixXd>(new MatrixXd());
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setInterpolationFunction(const QString &sInterpolationFunction)
{
    if(sInterpolationFunction == "Linear") {
        m_lInterpolationData.interpolationFunction = Interpolation::linear;
    }
    else if(sInterpolationFunction == "Square") {
        m_lInterpolationData.interpolationFunction = Interpolation::square;
    }
    else if(sInterpolationFunction == "Cubic") {
        m_lInterpolationData.interpolationFunction = Interpolation::cubic;
    }
    else if(sInterpolationFunction == "Gaussian") {
        m_lInterpolationData.interpolationFunction = Interpolation::gaussian;
    }

    if(m_bInterpolationInfoIsInit == true){
        //recalculate Interpolation matrix parameters changed
        emitMatrix();
    }
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setCancelDistance(double dCancelDist)
{
    m_lInterpolationData.dCancelDistance = dCancelDist;

    //recalculate everything because parameters changed
    calculateInterpolationOperator();
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setInterpolationInfo(const Eigen::MatrixX3f &matVertices,
                                                          const QVector<QVector<int> > &vecNeighborVertices,
                                                          const QVector<Vector3f> &vecSensorPos,
                                                          const FiffInfo &fiffInfo,
                                                          int iSensorType)
{
    if(matVertices.rows() == 0) {
        qDebug() << "RtSensorInterpolationMatWorker::calculateSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    //set members
    m_lInterpolationData.matVertices = matVertices;
    m_lInterpolationData.fiffInfo = fiffInfo;
    m_lInterpolationData.iSensorType = iSensorType;
    m_lInterpolationData.vecNeighborVertices = vecNeighborVertices;

    //set vecExcludeIndex
    m_lInterpolationData.vecExcludeIndex.clear();
    int iCounter = 0;
    for(const FiffChInfo &info : m_lInterpolationData.fiffInfo.chs) {
        if(info.kind == m_lInterpolationData.iSensorType &&
                (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
            if(m_lInterpolationData.fiffInfo.bads.contains(info.ch_name)) {
                m_lInterpolationData.vecExcludeIndex.push_back(iCounter);
            }
            iCounter++;
        }
    }

    //sensor projecting: One time operation because surface and sensors can not change
    m_lInterpolationData.vecMappedSubset = GeometryInfo::projectSensors(m_lInterpolationData.matVertices,
                                                                        vecSensorPos);

    m_bInterpolationInfoIsInit = true;

    calculateInterpolationOperator();
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setBadChannels(const FiffInfo& info)
{
    if(!m_bInterpolationInfoIsInit) {
        qDebug() << "RtSensorInterpolationMatWorker::updateBadChannels - Set interpolation info first.";
        return;
    }

    m_lInterpolationData.fiffInfo = info;

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_lInterpolationData.matDistanceMatrix,
                                    m_lInterpolationData.fiffInfo,
                                    m_lInterpolationData.iSensorType);    

    //set vecExcludeIndex
    m_lInterpolationData.vecExcludeIndex.clear();
    int iCounter = 0;
    for(const FiffChInfo &info : m_lInterpolationData.fiffInfo.chs) {
        if(info.kind == m_lInterpolationData.iSensorType &&
                (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
            if(m_lInterpolationData.fiffInfo.bads.contains(info.ch_name)) {
                m_lInterpolationData.vecExcludeIndex.push_back(iCounter);
            }
            iCounter++;
        }
    }

    emitMatrix();
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::calculateInterpolationOperator()
{
    if(!m_bInterpolationInfoIsInit) {
        qDebug() << "RtSensorInterpolationMatWorker::calculateInterpolationOperator - Set interpolation info first.";
        return;
    }

    //SCDC with cancel distance
    m_lInterpolationData.matDistanceMatrix = GeometryInfo::scdc(m_lInterpolationData.matVertices,
                                                                m_lInterpolationData.vecNeighborVertices,
                                                                m_lInterpolationData.vecMappedSubset,
                                                                m_lInterpolationData.dCancelDistance);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_lInterpolationData.matDistanceMatrix,
                                    m_lInterpolationData.fiffInfo,
                                    m_lInterpolationData.iSensorType);

    emitMatrix();
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::emitMatrix()
{
    //create Interpolation matrix
    emit newInterpolationMatrixCalculated(Interpolation::createInterpolationMat(m_lInterpolationData.vecMappedSubset,
                                                                                m_lInterpolationData.matDistanceMatrix,
                                                                                m_lInterpolationData.interpolationFunction,
                                                                                m_lInterpolationData.dCancelDistance,
                                                                                m_lInterpolationData.vecExcludeIndex));
}
