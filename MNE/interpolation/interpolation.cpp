//=============================================================================================================
/**
* @file     interpolation.cpp
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
* @brief    Interpolation class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "interpolation.h"
#include <mne/mne_bem_surface.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INTERPOLATION;
using namespace Eigen;



//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// INITIALIZE STATIC MEMBER
//=============================================================================================================
QSharedPointer<MatrixXd> Interpolation::m_interpolationMatrix = nullptr;


void Interpolation::createInterpolationMat(const QVector<qint32> &projectedSensors, const QSharedPointer<MatrixXd> distanceTable, qint32 interpolationType)
{
    m_interpolationMatrix = QSharedPointer<MatrixXd>::create(distanceTable->rows(), projectedSensors.size());
    m_interpolationMatrix->setZero();
    switch (interpolationType) {
    case LINEAR:
        calculateLinear(projectedSensors, distanceTable);
        break;
    default:
        std::cout << "[WARNING] Unknown interpolation type" << std::endl;
    }
}
//*************************************************************************************************************

QSharedPointer<VectorXd> Interpolation::interpolateSignal(const VectorXd &measurementData)
{
    QSharedPointer<VectorXd> interpolatedVec = QSharedPointer<VectorXd>::create();
    (*interpolatedVec) = (*m_interpolationMatrix) * measurementData;
    return interpolatedVec;

}
//*************************************************************************************************************

void Interpolation::clearInterpolateMatrix()
{
    // @todo why does the compiler say the there is no member clear ?
    // m_interpolationMatrix->clear();
}
//*************************************************************************************************************

void Interpolation::calculateLinear(const QVector<qint32> &projectedSensors, const QSharedPointer<MatrixXd> distanceTable) {
    size_t n = m_interpolationMatrix->rows();
    size_t m = projectedSensors.length();
    double INF = DOUBLE_INFINITY;
    for (int i = 0; i < n; ++i) {
        // @todo improve this operation (is linear right now, should be logarithmic)
        int indexInSubset = projectedSensors.indexOf(i);
        if (indexInSubset == -1) {
            // "normal" node, i.e. one which was not assigned a sensor
            // notInf: stores the indizes that are not infinity (i.e. the ones we have to consider when interpolating)
            QVector<QPair<qint32, double> > notInf;
            notInf.reserve(m);
            double invDistSum = 0;
            const Eigen::RowVectorXd row = distanceTable->row(i);
            for (int q = 0; q < m; ++q) {
                const double d = row[q];
                if (d != INF) {
                    invDistSum += (1 / d);
                    notInf.push_back(qMakePair<qint32, double> (q, d));
                }
            }
            for (QPair<qint32, double> qp : notInf) {
                (*m_interpolationMatrix)(i, qp.first) = 1 / (qp.second * invDistSum);
            }
        } else {
            // a sensor has been assigned to this node, we do not need to interpolate anything
            (*m_interpolationMatrix)(i, indexInSubset) = 1;
        }
    }
}

QSharedPointer<MatrixXd> Interpolation::getResult() {
    return m_interpolationMatrix;
}

