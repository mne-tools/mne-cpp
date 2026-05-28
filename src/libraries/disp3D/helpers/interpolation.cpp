//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     interpolation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Distance-based sparse interpolation kernels (linear / gaussian / square / cubic) and per-frame mat-vec.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "interpolation.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSet>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <unordered_set>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<SparseMatrix<float> > Interpolation::createInterpolationMat(const VectorXi &vecProjectedSensors,
                                                                             const QSharedPointer<MatrixXd> matDistanceTable,
                                                                             double (*interpolationFunction) (double),
                                                                             const double dCancelDist,
                                                                             const VectorXi &vecExcludeIndex)
{
    if(matDistanceTable->rows() == 0 && matDistanceTable->cols() == 0) {
        qDebug() << "[WARNING] Interpolation::createInterpolationMat - received an empty distance table.";
        return QSharedPointer<SparseMatrix<float> >::create();
    }

    QSharedPointer<Eigen::SparseMatrix<float> > matInterpolationMatrix = QSharedPointer<SparseMatrix<float> >::create(matDistanceTable->rows(), static_cast<int>(vecProjectedSensors.size()));

    QVector<Triplet<float> > vecNonZeroEntries;
    const qint32 iRows = matInterpolationMatrix->rows();
    const qint32 iCols = matInterpolationMatrix->cols();

    // Build exclude set for O(1) lookup
    std::unordered_set<int> excludeSet;
    for(Eigen::Index i = 0; i < vecExcludeIndex.size(); ++i) {
        excludeSet.insert(vecExcludeIndex[i]);
    }

    QSet<qint32> sensorLookup;
    for(Eigen::Index idx = 0; idx < vecProjectedSensors.size(); ++idx){
        if(excludeSet.count(static_cast<int>(idx)) == 0){
            sensorLookup.insert(vecProjectedSensors[idx]);
        }
    }

    for (qint32 r = 0; r < iRows; ++r) {
        if (sensorLookup.contains(r) == false) {
            QVector<QPair<qint32, float> > vecBelowThresh;
            float dWeightsSum = 0.0;
            const RowVectorXd& rowVec = matDistanceTable->row(r);

            for (qint32 c = 0; c < iCols; ++c) {
                const float dDist = rowVec[c];

                if (dDist < dCancelDist) {
                    const float dValueWeight = std::fabs(1.0 / interpolationFunction(dDist));
                    dWeightsSum += dValueWeight;
                    vecBelowThresh.push_back(qMakePair(c, dValueWeight));
                }
            }

            for (const QPair<qint32, float> &qp : vecBelowThresh) {
                vecNonZeroEntries.push_back(Eigen::Triplet<float> (r, qp.first, qp.second / dWeightsSum));
            }
        } else {
            // Find index of r in vecProjectedSensors
            int iIndexInSubset = 0;
            for(Eigen::Index k = 0; k < vecProjectedSensors.size(); ++k) {
                if(vecProjectedSensors[k] == r) {
                    iIndexInSubset = static_cast<int>(k);
                    break;
                }
            }
            vecNonZeroEntries.push_back(Eigen::Triplet<float> (r, iIndexInSubset, 1));
        }
    }

    matInterpolationMatrix->setFromTriplets(vecNonZeroEntries.begin(), vecNonZeroEntries.end());

    return matInterpolationMatrix;
}

//=============================================================================================================

VectorXf Interpolation::interpolateSignal(const QSharedPointer<SparseMatrix<float> > matInterpolationMatrix,
                                          const QSharedPointer<VectorXf> &vecMeasurementData)
{
    if (matInterpolationMatrix->cols() != vecMeasurementData->rows()) {
        qDebug() << "[WARNING] Interpolation::interpolateSignal - Dimension mismatch. Return null pointer...";
        return VectorXf();
    }

    VectorXf pOutVec = *matInterpolationMatrix * (*vecMeasurementData);
    return pOutVec;
}

//=============================================================================================================

VectorXf Interpolation::interpolateSignal(const SparseMatrix<float> &matInterpolationMatrix,
                                          const VectorXf &vecMeasurementData)
{
    if (matInterpolationMatrix.cols() != vecMeasurementData.rows()) {
        qDebug() << "[WARNING] Interpolation::interpolateSignal - Dimension mismatch. Return null pointer...";
        return VectorXf();
    }

    VectorXf pOutVec = matInterpolationMatrix * vecMeasurementData;
    return pOutVec;
}

//=============================================================================================================

double Interpolation::linear(const double dIn)
{
    return dIn;
}

//=============================================================================================================

double Interpolation::gaussian(const double dIn)
{
    return exp(-((dIn * dIn) / 2.0));
}

//=============================================================================================================

double Interpolation::square(const double dIn)
{
    return std::max((-(1.0f / 9.0f) * (dIn * dIn) + 1), 0.0);
}

//=============================================================================================================

double Interpolation::cubic(const double dIn)
{
    return dIn * dIn * dIn;
}
