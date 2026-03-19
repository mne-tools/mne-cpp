//=============================================================================================================
/**
 * @file     linalg.h
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
 * @brief    Linalg class declaration.
 *
 */

#ifndef LINALG_H
#define LINALG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

#include <utility>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/SVD>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * Linear algebra utility functions: decompositions, sorting, block-diagonal construction, etc.
 *
 * @brief Static linear algebra utility functions.
 */
class MATHSHARED_EXPORT Linalg
{
public:
    typedef std::pair<int,int> IdxIntValue;         /**< Typedef of a pair of ints. */

    //=========================================================================================================
    /**
     * Destroys the Linalg object.
     */
    ~Linalg() = default;

    //=========================================================================================================
    /**
     * Compute the three Cartesian components of a vector together.
     *
     * @param[in] vec    Input row vector [ x1 y1 z1 ... x_n y_n z_n ].
     *
     * @return Output vector [x1^2+y1^2+z1^2 ... x_n^2+y_n^2+z_n^2 ].
     */
    static Eigen::VectorXd combine_xyz(const Eigen::VectorXd& vec);

    //=========================================================================================================
    /**
     * Returns the condition number of a given matrix.
     *
     * @param[in] A      Matrix to compute the condition number from.
     * @param[out] s     Singular values of A.
     *
     * @return the condition number.
     */
    static double getConditionNumber(const Eigen::MatrixXd& A,
                                     Eigen::VectorXd &s);

    //=========================================================================================================
    /**
     * Returns the condition slope of a given matrix.
     *
     * @param[in] A      Matrix to compute the condition number from.
     * @param[out] s     Singular values of A.
     *
     * @return the condition slope.
     */
    static double getConditionSlope(const Eigen::MatrixXd& A,
                                    Eigen::VectorXd &s);

    //=========================================================================================================
    /**
     * Returns the whitener of a given matrix.
     *
     * @param[in] A         Matrix to compute the whitener from.
     * @param[in] pca       Perform a PCA.
     * @param[in] ch_type   Channel type string.
     * @param[out] eig      Eigenvalues.
     * @param[out] eigvec   Eigenvectors (transposed).
     */
    static void get_whitener(Eigen::MatrixXd& A,
                             bool pca,
                             QString ch_type,
                             Eigen::VectorXd& eig,
                             Eigen::MatrixXd& eigvec);

    //=========================================================================================================
    /**
     * Returns the whitener of a given matrix.
     *
     * @param[in] A         Matrix to compute the whitener from.
     * @param[in] pca       Perform a PCA.
     * @param[in] ch_type   Channel type string.
     * @param[out] eig      Eigenvalues.
     * @param[out] eigvec   Eigenvectors (transposed).
     */
    static void get_whitener(Eigen::MatrixXd& A,
                             bool pca,
                             const std::string& ch_type,
                             Eigen::VectorXd& eig,
                             Eigen::MatrixXd& eigvec);

    //=========================================================================================================
    /**
     * Find the intersection of two vectors.
     *
     * @param[in] v1         Input vector 1.
     * @param[in] v2         Input vector 2.
     * @param[out] idx_sel   Index of intersection based on v1.
     *
     * @return the sorted, unique values that are in both of the input arrays.
     */
    static Eigen::VectorXi intersect(const Eigen::VectorXi &v1,
                                     const Eigen::VectorXi &v2,
                                     Eigen::VectorXi &idx_sel);

    //=========================================================================================================
    /**
     * Make a sparse block diagonal matrix.
     *
     * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
     * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
     * the diagonal of the matrix.
     *
     * @param[in] A Matrix which should be diagonalized.
     * @param[in] n Columns of the submatrices.
     *
     * @return A sparse block diagonal, diagonalized from the elements in "A".
     */
    static Eigen::SparseMatrix<double> make_block_diag(const Eigen::MatrixXd &A,
                                                       qint32 n);

    //=========================================================================================================
    /**
     * Creates the pseudo inverse of a matrix.
     *
     * @param[in] a    Matrix to compute the pseudo inverse of.
     *
     * @return the pseudo inverse of a.
     */
    template<typename T>
    static Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> pinv(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& a);

    //=========================================================================================================
    /**
     * Returns the rank of a matrix A.
     *
     * @param[in] A      Matrix to get the rank from.
     * @param[in] tol    Relative threshold: biggest singular value multiplied with tol is smallest singular value considered non-zero.
     *
     * @return rank of matrix A.
     */
    static qint32 rank(const Eigen::MatrixXd& A,
                       double tol = 1e-8);

    //=========================================================================================================
    /**
     * Sorts a vector (ascending order) in place and returns the track of the original indices.
     *
     * @param[in, out] v     Vector to sort; it's sorted in place.
     * @param[in] desc       If true its sorted in a descending order, otherwise ascending (default = true).
     *
     * @return Vector of the original indices in the new order.
     */
    template<typename T>
    static Eigen::VectorXi sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v,
                                bool desc = true);

    //=========================================================================================================
    /**
     * Sorts a vector and a corresponding matrix in place and returns the track of the original indices.
     * The matrix is sorted along the columns using the vector values for comparison.
     *
     * @param[in, out] v_prime   Vector to sort (sorted in place).
     * @param[in, out] mat       Matrix to sort (sorted in place).
     * @param[in] desc           If true its sorted in a descending order, otherwise ascending (default = true).
     *
     * @return Vector of the original indices in the new order.
     */
    template<typename T>
    static Eigen::VectorXi sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v_prime,
                                Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &mat,
                                bool desc = true);

    //=========================================================================================================
    /**
     * Sort rows in ascending order.
     *
     * @param[in] A          Triplet vector to sort.
     * @param[in] column     Sorts the triplet vector based on the column specified.
     *
     * @return Sorted copy of the triplet vector.
     */
    template<typename T>
    static std::vector<Eigen::Triplet<T> > sortrows(const std::vector<Eigen::Triplet<T> > &A,
                                                    qint32 column = 0);

    //=========================================================================================================
    /**
     * Compares two index-value-pairs (greater-than).
     */
    template<typename T>
    static inline bool compareIdxValuePairBiggerThan(const std::pair<int,T>& lhs,
                                                     const std::pair<int,T>& rhs);

    //=========================================================================================================
    /**
     * Compares two index-value-pairs (less-than).
     */
    template<typename T>
    static inline bool compareIdxValuePairSmallerThan(const std::pair<int,T>& lhs,
                                                      const std::pair<int,T>& rhs);

    //=========================================================================================================
    /**
     * Compares triplet first entry (row).
     */
    template<typename T>
    static inline bool compareTripletFirstEntry(const Eigen::Triplet<T>& lhs,
                                                const Eigen::Triplet<T> & rhs);

    //=========================================================================================================
    /**
     * Compares triplet second entry (column).
     */
    template<typename T>
    static inline bool compareTripletSecondEntry(const Eigen::Triplet<T>& lhs,
                                                 const Eigen::Triplet<T> & rhs);
};

//=============================================================================================================
// INLINE & TEMPLATE DEFINITIONS
//=============================================================================================================

template<typename T>
Eigen::VectorXi Linalg::sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v,
                             bool desc)
{
    std::vector< std::pair<int,T> > t_vecIdxValue;
    Eigen::VectorXi idx(v.size());

    if(v.size() > 0)
    {
        for(qint32 i = 0; i < v.size(); ++i)
            t_vecIdxValue.push_back(std::pair<int,T>(i, v[i]));

        if(desc)
            std::sort(t_vecIdxValue.begin(), t_vecIdxValue.end(), Linalg::compareIdxValuePairBiggerThan<T>);
        else
            std::sort(t_vecIdxValue.begin(), t_vecIdxValue.end(), Linalg::compareIdxValuePairSmallerThan<T>);

        for(qint32 i = 0; i < v.size(); ++i)
        {
            idx[i] = t_vecIdxValue[i].first;
            v[i] = t_vecIdxValue[i].second;
        }
    }

    return idx;
}

//=============================================================================================================

template<typename T>
Eigen::VectorXi Linalg::sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v_prime,
                             Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &mat,
                             bool desc)
{
    Eigen::VectorXi idx = Linalg::sort<T>(v_prime, desc);

    if(v_prime.size() > 0)
    {
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> newMat(mat.rows(), mat.cols());
        for(qint32 i = 0; i < idx.size(); ++i)
            newMat.col(i) = mat.col(idx[i]);
        mat = newMat;
    }

    return idx;
}

//=============================================================================================================

template<typename T>
std::vector<Eigen::Triplet<T> > Linalg::sortrows(const std::vector<Eigen::Triplet<T> > &A,
                                                  qint32 column)
{
    std::vector<Eigen::Triplet<T> > p_ASorted;

    for(quint32 i = 0; i < A.size(); ++i)
        p_ASorted.push_back(A[i]);

    if(column == 0)
        std::sort(p_ASorted.begin(), p_ASorted.end(), Linalg::compareTripletFirstEntry<T>);
    if(column == 1)
        std::sort(p_ASorted.begin(), p_ASorted.end(), Linalg::compareTripletSecondEntry<T>);

    return p_ASorted;
}

//=============================================================================================================

template<typename T>
inline bool Linalg::compareIdxValuePairBiggerThan(const std::pair<int,T>& lhs,
                                                  const std::pair<int,T>& rhs)
{
    return lhs.second > rhs.second;
}

//=============================================================================================================

template<typename T>
inline bool Linalg::compareIdxValuePairSmallerThan(const std::pair<int,T>& lhs,
                                                   const std::pair<int,T>& rhs)
{
    return lhs.second < rhs.second;
}

//=============================================================================================================

template<typename T>
inline bool Linalg::compareTripletFirstEntry(const Eigen::Triplet<T>& lhs,
                                             const Eigen::Triplet<T> & rhs)
{
    return lhs.row() < rhs.row();
}

//=============================================================================================================

template<typename T>
inline bool Linalg::compareTripletSecondEntry(const Eigen::Triplet<T>& lhs,
                                              const Eigen::Triplet<T> & rhs)
{
    return lhs.col() < rhs.col();
}

//=============================================================================================================

template<typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> Linalg::pinv(const Eigen::Matrix<T,
                                                               Eigen::Dynamic,
                                                               Eigen::Dynamic>& a)
{
    double epsilon = std::numeric_limits<double>::epsilon();
    Eigen::JacobiSVD< Eigen::MatrixXd > svd(a, Eigen::ComputeThinU | Eigen::ComputeThinV);
    double tolerance = epsilon * std::max(a.cols(), a.rows()) * svd.singularValues().array().abs()(0);
    return svd.matrixV() * (svd.singularValues().array().abs() > tolerance).select(svd.singularValues().array().inverse(), 0).matrix().asDiagonal() * svd.matrixU().adjoint();
}

//=============================================================================================================
} // NAMESPACE

#endif // LINALG_H
