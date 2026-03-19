//=============================================================================================================
/**
 * @file     linalg.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the Linalg class.
 *
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
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd Linalg::combine_xyz(const VectorXd& vec)
{
    if (vec.size() % 3 != 0)
    {
        qWarning("Linalg::combine_xyz: Input must be a row or column vector with 3N components.");
        return VectorXd();
    }

    MatrixXd tmp = MatrixXd(vec.transpose());
    SparseMatrix<double> s = make_block_diag(tmp, 3);

    SparseMatrix<double> sC = s * s.transpose();
    VectorXd comb(sC.rows());

    for (qint32 i = 0; i < sC.rows(); ++i)
        comb[i] = sC.coeff(i, i);

    return comb;
}

//=============================================================================================================

double Linalg::getConditionNumber(const MatrixXd& A,
                                  VectorXd &s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff()/s.minCoeff();

    return c;
}

//=============================================================================================================

double Linalg::getConditionSlope(const MatrixXd& A,
                                 VectorXd &s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff()/s.mean();

    return c;
}

//=============================================================================================================

void Linalg::get_whitener(MatrixXd &A,
                          bool pca,
                          QString ch_type,
                          VectorXd &eig,
                          MatrixXd &eigvec)
{
    SelfAdjointEigenSolver<MatrixXd> t_eigenSolver(A);

    eig = t_eigenSolver.eigenvalues();
    eigvec = t_eigenSolver.eigenvectors().transpose();

    Linalg::sort<double>(eig, eigvec, false);
    qint32 rnk = Linalg::rank(A);

    for(qint32 i = 0; i < eig.size()-rnk; ++i)
        eig(i) = 0;

    printf("Setting small %s eigenvalues to zero.\n", ch_type.toUtf8().constData());
    if (!pca)
        printf("Not doing PCA for %s\n", ch_type.toUtf8().constData());
    else
    {
        printf("Doing PCA for %s.",ch_type.toUtf8().constData());
        eigvec = eigvec.block(eigvec.rows()-rnk, 0, rnk, eigvec.cols());
    }
}

//=============================================================================================================

void Linalg::get_whitener(MatrixXd &A,
                          bool pca,
                          const std::string& ch_type,
                          VectorXd &eig,
                          MatrixXd &eigvec)
{
    SelfAdjointEigenSolver<MatrixXd> t_eigenSolver(A);

    eig = t_eigenSolver.eigenvalues();
    eigvec = t_eigenSolver.eigenvectors().transpose();

    Linalg::sort<double>(eig, eigvec, false);
    qint32 rnk = Linalg::rank(A);

    for(qint32 i = 0; i < eig.size()-rnk; ++i)
        eig(i) = 0;

    printf("Setting small %s eigenvalues to zero.\n", ch_type.c_str());
    if (!pca)
        printf("Not doing PCA for %s\n", ch_type.c_str());
    else
    {
        printf("Doing PCA for %s.",ch_type.c_str());
        eigvec = eigvec.block(eigvec.rows()-rnk, 0, rnk, eigvec.cols());
    }
}

//=============================================================================================================

VectorXi Linalg::intersect(const VectorXi &v1,
                           const VectorXi &v2,
                           VectorXi &idx_sel)
{
    std::vector<int> tmp;

    std::vector< std::pair<int,int> > t_vecIntIdxValue;

    for(qint32 i = 0; i < v1.size(); ++i)
        tmp.push_back(v1[i]);

    std::vector<int>::iterator it;
    for(qint32 i = 0; i < v2.size(); ++i)
    {
        it = std::search(tmp.begin(), tmp.end(), &v2[i], &v2[i]+1);
        if(it != tmp.end())
            t_vecIntIdxValue.push_back(std::pair<int,int>(v2[i], it-tmp.begin()));
    }

    std::sort(t_vecIntIdxValue.begin(), t_vecIntIdxValue.end(), Linalg::compareIdxValuePairSmallerThan<int>);

    VectorXi p_res(t_vecIntIdxValue.size());
    idx_sel = VectorXi(t_vecIntIdxValue.size());

    for(quint32 i = 0; i < t_vecIntIdxValue.size(); ++i)
    {
        p_res[i] = t_vecIntIdxValue[i].first;
        idx_sel[i] = t_vecIntIdxValue[i].second;
    }

    return p_res;
}

//=============================================================================================================

SparseMatrix<double> Linalg::make_block_diag(const MatrixXd &A,
                                             qint32 n)
{
    qint32 ma = A.rows();
    qint32 na = A.cols();
    float bdn = static_cast<float>(na) / n;

    if (bdn - std::floor(bdn))
    {
        qWarning("Linalg::make_block_diag: Width of matrix must be an even multiple of n.");
        return SparseMatrix<double>();
    }

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(static_cast<size_t>(bdn * ma * n));

    for (qint32 i = 0; i < static_cast<qint32>(bdn); ++i)
    {
        qint32 current_col = i * n;
        qint32 current_row = i * ma;

        for (qint32 r = 0; r < ma; ++r)
            for (qint32 c = 0; c < n; ++c)
                tripletList.push_back(T(r + current_row, c + current_col, A(r, c + current_col)));
    }

    SparseMatrix<double> bd(static_cast<int>(std::floor(ma * bdn + 0.5f)), na);
    bd.setFromTriplets(tripletList.begin(), tripletList.end());

    return bd;
}

//=============================================================================================================

qint32 Linalg::rank(const MatrixXd& A,
                    double tol)
{
    JacobiSVD<MatrixXd> t_svdA(A);
    VectorXd s = t_svdA.singularValues();
    double t_dMax = s.maxCoeff();
    t_dMax *= tol;
    qint32 sum = 0;
    for(qint32 i = 0; i < s.size(); ++i)
        sum += s[i] > t_dMax ? 1 : 0;
    return sum;
}
