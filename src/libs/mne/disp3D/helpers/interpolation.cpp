//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     interpolation.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     May 2017
 * @brief    Distance-based sparse interpolation kernels (linear / gaussian / square / cubic) and per-frame mat-vec.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "interpolation.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <unordered_set>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::shared_ptr<SparseMatrix<float>> Interpolation::createInterpolationMat(const VectorXi& vecProjectedSensors,
                                                                           const std::shared_ptr<MatrixXd>& matDistanceTable,
                                                                           double (*interpolationFunction)(double),
                                                                           const double dCancelDist,
                                                                           const VectorXi& vecExcludeIndex)
{
    if (matDistanceTable->rows() == 0 && matDistanceTable->cols() == 0) {
        mne::Log::warning("Interpolation::createInterpolationMat - received an empty distance table.");
        return std::make_shared<SparseMatrix<float>>();
    }

    std::shared_ptr<Eigen::SparseMatrix<float>> matInterpolationMatrix = std::make_shared<SparseMatrix<float>>(matDistanceTable->rows(), static_cast<int>(vecProjectedSensors.size()));

    std::vector<Triplet<float>> vecNonZeroEntries;
    const std::int32_t iRows = matInterpolationMatrix->rows();
    const std::int32_t iCols = matInterpolationMatrix->cols();

    // Build exclude set for O(1) lookup
    std::unordered_set<int> excludeSet;
    for (Eigen::Index i = 0; i < vecExcludeIndex.size(); ++i) {
        excludeSet.insert(vecExcludeIndex[i]);
    }

    std::unordered_set<std::int32_t> sensorLookup;
    for (Eigen::Index idx = 0; idx < vecProjectedSensors.size(); ++idx) {
        if (excludeSet.count(static_cast<int>(idx)) == 0) {
            sensorLookup.insert(vecProjectedSensors[idx]);
        }
    }

    for (std::int32_t r = 0; r < iRows; ++r) {
        if (!sensorLookup.contains(r)) {
            std::vector<std::pair<std::int32_t, float>> vecBelowThresh;
            float dWeightsSum = 0.0;
            const RowVectorXd& rowVec = matDistanceTable->row(r);

            for (std::int32_t c = 0; c < iCols; ++c) {
                const float dDist = rowVec[c];

                if (dDist < dCancelDist) {
                    const float dValueWeight = std::fabs(1.0 / interpolationFunction(dDist));
                    dWeightsSum += dValueWeight;
                    vecBelowThresh.emplace_back(c, dValueWeight);
                }
            }

            for (const auto& qp : vecBelowThresh) {
                vecNonZeroEntries.push_back(Eigen::Triplet<float>(r, qp.first, qp.second / dWeightsSum));
            }
        } else {
            // Find index of r in vecProjectedSensors
            int iIndexInSubset = 0;
            for (Eigen::Index k = 0; k < vecProjectedSensors.size(); ++k) {
                if (vecProjectedSensors[k] == r) {
                    iIndexInSubset = static_cast<int>(k);
                    break;
                }
            }
            vecNonZeroEntries.push_back(Eigen::Triplet<float>(r, iIndexInSubset, 1));
        }
    }

    matInterpolationMatrix->setFromTriplets(vecNonZeroEntries.begin(), vecNonZeroEntries.end());

    return matInterpolationMatrix;
}

//=============================================================================================================

VectorXf Interpolation::interpolateSignal(const std::shared_ptr<SparseMatrix<float>>& matInterpolationMatrix,
                                          const std::shared_ptr<VectorXf>& vecMeasurementData)
{
    if (matInterpolationMatrix->cols() != vecMeasurementData->rows()) {
        mne::Log::warning("Interpolation::interpolateSignal - Dimension mismatch. Return null pointer...");
        return VectorXf();
    }

    VectorXf pOutVec = *matInterpolationMatrix * (*vecMeasurementData);
    return pOutVec;
}

//=============================================================================================================

VectorXf Interpolation::interpolateSignal(const SparseMatrix<float>& matInterpolationMatrix,
                                          const VectorXf& vecMeasurementData)
{
    if (matInterpolationMatrix.cols() != vecMeasurementData.rows()) {
        mne::Log::warning("Interpolation::interpolateSignal - Dimension mismatch. Return null pointer...");
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
