//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_label_time_course.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the label-aggregation modes and sign-flip computation declared in @c inv_label_time_course.h.
 *
 * Implements per-label slicing of the vertex index list, the four
 * aggregation kernels (simple mean, sign-flipped mean, PCA-flip via
 * Eigen's SVD, and absolute-max), and the @c computeSignFlip helper
 * that derives the per-vertex ±1 alignment from the first singular
 * vector of the label's data block — the same recipe used by
 * mne-python so that downstream ROI averages stay numerically
 * comparable across the two stacks.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_label_time_course.h"
#include <base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd InvLabelTimeCourse::computeSignFlip(const MatrixXd& stcData)
{
    const int nVerts = static_cast<int>(stcData.rows());
    if (nVerts == 0)
        return VectorXd();

    // Use SVD to find the dominant temporal pattern
    // Then flip each vertex to align with it
    JacobiSVD<MatrixXd> svd(stcData, ComputeThinU);
    VectorXd u1 = svd.matrixU().col(0);

    VectorXd signs(nVerts);
    for (int i = 0; i < nVerts; ++i)
        signs[i] = (u1[i] >= 0.0) ? 1.0 : -1.0;

    return signs;
}

//=============================================================================================================

MatrixXd InvLabelTimeCourse::extract(const InvSourceEstimate& stc,
                                     const std::vector<FsLabel>& labels,
                                     std::string_view sMode,
                                     bool bAllowEmpty)
{
    if (stc.empty() || labels.empty()) {
        mne::Log::warning("[InvLabelTimeCourse::extract] Empty stc or labels.");
        return MatrixXd();
    }

    const int nTimes = static_cast<int>(stc.data.cols());
    const int nLabels = static_cast<int>(labels.size());

    // Resolve "auto" mode
    const std::string_view mode = sMode == "auto" ? std::string_view("mean_flip") : sMode;

    // Build a map from vertex index → row in stc.data
    std::unordered_map<int, int> vertexToRow;
    vertexToRow.reserve(static_cast<std::size_t>(stc.vertices.size()));
    for (int i = 0; i < stc.vertices.size(); ++i)
        vertexToRow.emplace(stc.vertices[i], i);

    // Count output labels (skip empty if !bAllowEmpty)
    std::vector<int> outputLabelIndices;
    for (int li = 0; li < nLabels; ++li) {
        int count = 0;
        for (int vi = 0; vi < labels[li].vertices.size(); ++vi) {
            if (vertexToRow.contains(labels[li].vertices[vi]))
                ++count;
        }
        if (count > 0 || bAllowEmpty)
            outputLabelIndices.push_back(li);
    }

    MatrixXd result = MatrixXd::Zero(std::ssize(outputLabelIndices), nTimes);

    for (int oi = 0; oi < std::ssize(outputLabelIndices); ++oi) {
        const FsLabel& label = labels[outputLabelIndices[oi]];

        // Find matching vertex rows
        std::vector<int> rows;
        for (int vi = 0; vi < label.vertices.size(); ++vi) {
            auto it = vertexToRow.find(label.vertices[vi]);
            if (it != vertexToRow.end())
                rows.push_back(it->second);
        }

        if (rows.empty())
            continue;

        const int nVerts = static_cast<int>(rows.size());

        // Extract sub-matrix for this label
        MatrixXd labelData(nVerts, nTimes);
        for (int i = 0; i < nVerts; ++i)
            labelData.row(i) = stc.data.row(rows[i]);

        if (mode == "mean") {
            result.row(oi) = labelData.colwise().mean();
        } else if (mode == "mean_flip") {
            VectorXd signs = computeSignFlip(labelData);
            MatrixXd flipped = labelData;
            for (int i = 0; i < nVerts; ++i)
                flipped.row(i) *= signs[i];
            result.row(oi) = flipped.colwise().mean();
        } else if (mode == "pca_flip") {
            // Demean across time
            RowVectorXd meanTime = labelData.colwise().mean();
            MatrixXd centered = labelData.rowwise() - meanTime;

            JacobiSVD<MatrixXd> svd(centered, ComputeThinV);
            // First PC time course
            RowVectorXd pc1 = svd.matrixV().col(0).transpose();

            // Scale by singular value and sign
            double sv1 = svd.singularValues()[0];
            pc1 *= sv1 / std::sqrt(static_cast<double>(nVerts));

            // Flip sign to match the dominant sign of the data
            // Check correlation with mean
            double corr = (meanTime.array() * pc1.array()).sum();
            if (corr < 0.0)
                pc1 = -pc1;

            result.row(oi) = pc1;
        } else if (mode == "max") {
            // Maximum absolute value at each time point
            for (int t = 0; t < nTimes; ++t) {
                double maxVal = 0.0;
                double maxAbsVal = 0.0;
                for (int i = 0; i < nVerts; ++i) {
                    double absVal = std::abs(labelData(i, t));
                    if (absVal > maxAbsVal) {
                        maxAbsVal = absVal;
                        maxVal = labelData(i, t);
                    }
                }
                result(oi, t) = maxVal;
            }
        } else {
            mne::Log::warning("[InvLabelTimeCourse::extract] Unknown mode: {} — using mean.", mode);
            result.row(oi) = labelData.colwise().mean();
        }
    }

    return result;
}
