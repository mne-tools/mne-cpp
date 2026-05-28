//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file surface_laplacian.cpp
 * @since May 2026
 * @brief Implementation of SurfaceLaplacian.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surface_laplacian.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

namespace
{
constexpr double CSD_PI = 3.14159265358979323846;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd SurfaceLaplacian::evaluateLegendre(const MatrixXd& matX, int iMaxOrder)
{
    // Returns matrix of shape (iMaxOrder+1, matX.size()) where row n = P_n(x)
    // using flattened matX
    const Index nElements = matX.size();
    MatrixXd result(iMaxOrder + 1, nElements);

    // P_0(x) = 1
    result.row(0).setOnes();

    if (iMaxOrder >= 1) {
        // P_1(x) = x
        for (Index i = 0; i < nElements; ++i)
            result(1, i) = matX.data()[i];
    }

    // Bonnet's recurrence: (n+1) P_{n+1}(x) = (2n+1) x P_n(x) - n P_{n-1}(x)
    for (int n = 1; n < iMaxOrder; ++n) {
        const double a = static_cast<double>(2 * n + 1) / static_cast<double>(n + 1);
        const double b = static_cast<double>(n) / static_cast<double>(n + 1);
        for (Index i = 0; i < nElements; ++i)
            result(n + 1, i) = a * matX.data()[i] * result(n, i) - b * result(n - 1, i);
    }

    return result;
}

//=============================================================================================================

MatrixXd SurfaceLaplacian::computeG(const MatrixXd& matCosAng,
                                    int iStiffness,
                                    int iNLegendreTerms)
{
    // G(cos θ) = Σ_{n=1}^{N} (2n+1) / (n^m (n+1)^m 4π) · P_n(cos θ)
    const Index nRows = matCosAng.rows();
    const Index nCols = matCosAng.cols();

    // Evaluate Legendre polynomials
    MatrixXd legP = evaluateLegendre(matCosAng, iNLegendreTerms);

    // Accumulate weighted sum
    MatrixXd G = MatrixXd::Zero(nRows, nCols);

    for (int n = 1; n <= iNLegendreTerms; ++n) {
        const double factor = static_cast<double>(2 * n + 1)
                            / (std::pow(static_cast<double>(n), iStiffness)
                             * std::pow(static_cast<double>(n + 1), iStiffness)
                             * 4.0 * CSD_PI);

        for (Index i = 0; i < nRows * nCols; ++i)
            G.data()[i] += factor * legP(n, i);
    }

    return G;
}

//=============================================================================================================

MatrixXd SurfaceLaplacian::computeH(const MatrixXd& matCosAng,
                                    int iStiffness,
                                    int iNLegendreTerms)
{
    // H(cos θ) = Σ_{n=1}^{N} (2n+1) / (n^(m-1) (n+1)^(m-1) 4π) · P_n(cos θ)
    const Index nRows = matCosAng.rows();
    const Index nCols = matCosAng.cols();

    MatrixXd legP = evaluateLegendre(matCosAng, iNLegendreTerms);

    MatrixXd H = MatrixXd::Zero(nRows, nCols);

    for (int n = 1; n <= iNLegendreTerms; ++n) {
        const double factor = static_cast<double>(2 * n + 1)
                            / (std::pow(static_cast<double>(n), iStiffness - 1)
                             * std::pow(static_cast<double>(n + 1), iStiffness - 1)
                             * 4.0 * CSD_PI);

        for (Index i = 0; i < nRows * nCols; ++i)
            H.data()[i] += factor * legP(n, i);
    }

    return H;
}

//=============================================================================================================

MatrixXd SurfaceLaplacian::computeTransform(const MatrixX3d& matPositions,
                                            double dLambda2,
                                            int iStiffness,
                                            int iNLegendreTerms,
                                            double dSphereRadius)
{
    const int nCh = static_cast<int>(matPositions.rows());
    if (nCh < 2) {
        qWarning() << "[SurfaceLaplacian::computeTransform] Need at least 2 channels.";
        return MatrixXd();
    }

    // 1. Centre positions and normalise to unit sphere
    Vector3d centre = matPositions.colwise().mean();
    MatrixX3d posCentered = matPositions.rowwise() - centre.transpose();

    // Fit sphere radius if not provided
    if (dSphereRadius <= 0.0) {
        dSphereRadius = 0.0;
        for (int i = 0; i < nCh; ++i)
            dSphereRadius += posCentered.row(i).norm();
        dSphereRadius /= static_cast<double>(nCh);
    }

    // Normalise to unit sphere
    MatrixX3d posNorm(nCh, 3);
    for (int i = 0; i < nCh; ++i) {
        double norm = posCentered.row(i).norm();
        if (norm > 0.0)
            posNorm.row(i) = posCentered.row(i) / norm;
        else
            posNorm.row(i).setZero();
    }

    // 2. Cosine angle matrix: cos(θ_ij) = pos_i · pos_j (on unit sphere)
    MatrixXd cosAng = posNorm * posNorm.transpose();

    // Clamp to [-1, 1]
    cosAng = cosAng.cwiseMax(-1.0).cwiseMin(1.0);

    // 3. Compute G and H matrices
    MatrixXd G = computeG(cosAng, iStiffness, iNLegendreTerms);
    MatrixXd H = computeH(cosAng, iStiffness, iNLegendreTerms);

    // 4. Regularise G: G(i,i) += lambda²
    for (int i = 0; i < nCh; ++i)
        G(i, i) += dLambda2;

    // 5. Invert G
    MatrixXd Gi = G.inverse();

    // Column sums of Gi
    VectorXd TC = Gi.colwise().sum();
    double sgi = TC.sum();

    // 6. Build CSD transform
    // Z = I - (1/n) * ones: average-reference the identity
    MatrixXd Z = MatrixXd::Identity(nCh, nCh);
    Z.array() -= 1.0 / static_cast<double>(nCh);

    // Cp2 = Gi * Z
    MatrixXd Cp2 = Gi * Z;

    // c02 = (colwise sum of Cp2) / sgi
    RowVectorXd c02 = Cp2.colwise().sum() / sgi;

    // C2 = Cp2 - TC * c02
    MatrixXd C2 = Cp2 - TC * c02;

    // Transform = (C2^T * H)^T / R^2 = H^T * C2 / R^2
    MatrixXd X = (H.transpose() * C2) / (dSphereRadius * dSphereRadius);

    return X;
}

//=============================================================================================================

SurfaceLaplacianResult SurfaceLaplacian::compute(const MatrixXd& matData,
                                                  const MatrixX3d& matPositions,
                                                  double dLambda2,
                                                  int iStiffness,
                                                  int iNLegendreTerms,
                                                  double dSphereRadius)
{
    SurfaceLaplacianResult result;

    if (matData.rows() != matPositions.rows()) {
        qWarning() << "[SurfaceLaplacian::compute] Data rows" << matData.rows()
                    << "!= position rows" << matPositions.rows();
        return result;
    }

    result.matTransform = computeTransform(matPositions, dLambda2, iStiffness,
                                           iNLegendreTerms, dSphereRadius);

    if (result.matTransform.size() == 0)
        return result;

    // Apply transform: CSD_data = X * data
    result.matData = result.matTransform * matData;

    return result;
}
