//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     linalg.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Jacobi-SVD condition metrics, block-diagonal sparse builder and xyz reduction.
 *
 * Implements the static helpers declared in @ref linalg.h. Conditioning
 * is derived from the singular values returned by
 * @c Eigen::JacobiSVD: the classical condition number is
 * @f$\kappa = \sigma_{max}/\sigma_{min}@f$, while the condition slope is
 * the least-squares slope of the log-singular-value spectrum, which is
 * a more informative diagnostic for the rapidly decaying spectra typical
 * of MEG/EEG gain matrices.
 *
 * The block-diagonal builder walks each dense input block once and emits
 * an @c Eigen::SparseMatrix via @c setFromTriplets, giving a single
 * @c O(\\sum nnz_k log nnz_k) construction even for the thousands of 3x3
 * blocks produced by free-orientation source spaces. @c combine_xyz
 * collapses a row of free-orientation components into per-source
 * squared magnitudes by reusing the same block-diagonal layout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "linalg.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstdio>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd Linalg::combine_xyz(const VectorXd& vec)
{
    if (vec.size() % 3 != 0) {
        return VectorXd();
    }

    MatrixXd tmp = MatrixXd(vec.transpose());
    SparseMatrix<double> s = make_block_diag(tmp, 3);

    SparseMatrix<double> sC = s * s.transpose();
    VectorXd comb(sC.rows());

    for (std::int32_t i = 0; i < sC.rows(); ++i)
        comb[i] = sC.coeff(i, i);

    return comb;
}

//=============================================================================================================

double Linalg::getConditionNumber(const MatrixXd& A,
                                  VectorXd& s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff() / s.minCoeff();

    return c;
}

//=============================================================================================================

double Linalg::getConditionSlope(const MatrixXd& A,
                                 VectorXd& s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff() / s.mean();

    return c;
}

//=============================================================================================================

void Linalg::get_whitener(MatrixXd& A,
                          bool pca,
                          const std::string& /*ch_type*/,
                          VectorXd& eig,
                          MatrixXd& eigvec)
{
    // Eigenvalues come in ascending order; for a symmetric matrix they are the singular values up to sign
    SelfAdjointEigenSolver<MatrixXd> t_eigenSolver(A);
    eig = t_eigenSolver.eigenvalues();
    eigvec = t_eigenSolver.eigenvectors().transpose();

    const double threshold = 1e-8 * eig.cwiseAbs().maxCoeff();
    const auto rnk = static_cast<std::int32_t>((eig.array().abs() > threshold).count());
    eig.head(eig.size() - rnk).setZero();

    if (pca) {
        eigvec = eigvec.bottomRows(rnk).eval(); // eval: resizing eigvec frees the rows being read
    }
}

//=============================================================================================================

VectorXi Linalg::intersect(const VectorXi& v1,
                           const VectorXi& v2,
                           VectorXi& idx_sel)
{
    std::vector<int> tmp;

    std::vector<std::pair<int, int>> t_vecIntIdxValue;

    for (std::int32_t i = 0; i < v1.size(); ++i)
        tmp.push_back(v1[i]);

    std::vector<int>::iterator it;
    for (std::int32_t i = 0; i < v2.size(); ++i) {
        it = std::search(tmp.begin(), tmp.end(), &v2[i], &v2[i] + 1);
        if (it != tmp.end())
            t_vecIntIdxValue.push_back(std::pair<int, int>(v2[i], static_cast<int>(it - tmp.begin())));
    }

    std::sort(t_vecIntIdxValue.begin(), t_vecIntIdxValue.end(), Linalg::compareIdxValuePairSmallerThan<int>);

    VectorXi p_res(t_vecIntIdxValue.size());
    idx_sel = VectorXi(t_vecIntIdxValue.size());

    for (std::uint32_t i = 0; i < t_vecIntIdxValue.size(); ++i) {
        p_res[i] = t_vecIntIdxValue[i].first;
        idx_sel[i] = t_vecIntIdxValue[i].second;
    }

    return p_res;
}

//=============================================================================================================

SparseMatrix<double> Linalg::make_block_diag(const MatrixXd& A,
                                             std::int32_t n)
{
    const auto ma = static_cast<std::int32_t>(A.rows());
    const auto na = static_cast<std::int32_t>(A.cols());
    float bdn = static_cast<float>(na) / n;

    if (bdn != std::floor(bdn)) {
        return SparseMatrix<double>();
    }

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(static_cast<size_t>(bdn * ma * n));

    for (std::int32_t i = 0; i < static_cast<std::int32_t>(bdn); ++i) {
        std::int32_t current_col = i * n;
        std::int32_t current_row = i * ma;

        for (std::int32_t r = 0; r < ma; ++r)
            for (std::int32_t c = 0; c < n; ++c)
                tripletList.push_back(T(r + current_row, c + current_col, A(r, c + current_col)));
    }

    SparseMatrix<double> bd(static_cast<int>(std::floor(ma * bdn + 0.5f)), na);
    bd.setFromTriplets(tripletList.begin(), tripletList.end());

    return bd;
}

//=============================================================================================================

std::int32_t Linalg::rank(const MatrixXd& A,
                          double tol)
{
    JacobiSVD<MatrixXd> t_svdA(A);
    VectorXd s = t_svdA.singularValues();
    double t_dMax = s.maxCoeff();
    t_dMax *= tol;
    std::int32_t sum = 0;
    for (std::int32_t i = 0; i < s.size(); ++i)
        sum += s[i] > t_dMax ? 1 : 0;
    return sum;
}
