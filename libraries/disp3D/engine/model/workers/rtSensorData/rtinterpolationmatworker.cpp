//=============================================================================================================
/**
* @file     rtsensordataworker.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
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
* @brief    RtSensorDataWorker class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinterpolationmatworker.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//#include <QObject>
//#include <QDebug>
//#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace DISPLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtInterpolationMatWorker::RtInterpolationMatWorker()
: m_bInterpolationInfoIsInit(false) {
    //5cm cancel distance and cubic function as default
    m_lInterpolationData.dCancelDistance = 0.20;
    m_lInterpolationData.interpolationFunction = DISP3DLIB::Interpolation::cubic;
}


//*************************************************************************************************************

void RtInterpolationMatWorker::setInterpolationFunction(const QString &sInterpolationFunction)
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
        m_lInterpolationData.pInterpolationMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                                   m_lInterpolationData.pDistanceMatrix,
                                                                                   m_lInterpolationData.interpolationFunction,
                                                                                   m_lInterpolationData.dCancelDistance,
                                                                                   m_lInterpolationData.fiffInfo,
                                                                                   m_lInterpolationData.iSensorType);

        emit newInterpolationMatrixCalculated(m_lInterpolationData.pInterpolationMatrix);
    }
}


//*************************************************************************************************************

void RtInterpolationMatWorker::setCancelDistance(double dCancelDist)
{
    m_lInterpolationData.dCancelDistance = dCancelDist;

    //recalculate everything because parameters changed
    calculateInterpolationOperator();
}


//*************************************************************************************************************

void RtInterpolationMatWorker::setInterpolationInfo(const MNELIB::MNEBemSurface &bemSurface,
                          const QVector<Vector3f> &vecSensorPos,
                          const FIFFLIB::FiffInfo &fiffInfo,
                          int iSensorType)
{
    if(bemSurface.rr.rows() == 0) {
        qDebug() << "RtSensorDataWorker::calculateSurfaceData - Surface data is empty. Returning ...";
        return;
    }

    //set members
    m_lInterpolationData.bemSurface = bemSurface;
    m_lInterpolationData.fiffInfo = fiffInfo;
    m_lInterpolationData.iSensorType = iSensorType;

    //sensor projecting: One time operation because surface and sensors can not change
    m_lInterpolationData.pVecMappedSubset = GeometryInfo::projectSensors(m_lInterpolationData.bemSurface, vecSensorPos);

    m_bInterpolationInfoIsInit = true;

    calculateInterpolationOperator();
}


//*************************************************************************************************************

void RtInterpolationMatWorker::setBadChannels(const FIFFLIB::FiffInfo& info)
{
    if(!m_bInterpolationInfoIsInit) {
        qDebug() << "RtInterpolationMatWorker::updateBadChannels - Set interpolation info first.";
        return;
    }

    m_lInterpolationData.fiffInfo = info;

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_lInterpolationData.pDistanceMatrix,
                                    m_lInterpolationData.fiffInfo,
                                    m_lInterpolationData.iSensorType);

    //create Interpolation matrix
    m_lInterpolationData.pInterpolationMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                               m_lInterpolationData.pDistanceMatrix,
                                                                               m_lInterpolationData.interpolationFunction,
                                                                               m_lInterpolationData.dCancelDistance,
                                                                               m_lInterpolationData.fiffInfo,
                                                                               m_lInterpolationData.iSensorType);

    emit newInterpolationMatrixCalculated(m_lInterpolationData.pInterpolationMatrix);
}


//*************************************************************************************************************

void RtInterpolationMatWorker::calculateInterpolationOperator()
{
    if(!m_bInterpolationInfoIsInit) {
        qDebug() << "RtInterpolationMatWorker::calculateInterpolationOperator - Set interpolation info first.";
        return;
    }

    //SCDC with cancel distance
    m_lInterpolationData.pDistanceMatrix = GeometryInfo::scdc(m_lInterpolationData.bemSurface,
                                                              m_lInterpolationData.pVecMappedSubset,
                                                              m_lInterpolationData.dCancelDistance);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_lInterpolationData.pDistanceMatrix,
                                    m_lInterpolationData.fiffInfo,
                                    m_lInterpolationData.iSensorType);

    //create Interpolation matrix
    m_lInterpolationData.pInterpolationMatrix = Interpolation::createInterpolationMat(m_lInterpolationData.pVecMappedSubset,
                                                                               m_lInterpolationData.pDistanceMatrix,
                                                                               m_lInterpolationData.interpolationFunction,
                                                                               m_lInterpolationData.dCancelDistance,
                                                                               m_lInterpolationData.fiffInfo,
                                                                               m_lInterpolationData.iSensorType);

    emit newInterpolationMatrixCalculated(m_lInterpolationData.pInterpolationMatrix);
}
