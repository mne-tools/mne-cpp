//=============================================================================================================
/**
* @file     mnemath.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    MNEMath class declaration.
*
*/

#ifndef MNEMATH_H
#define MNEMATH_H

//ToDo move this to the new MNE math library

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/Eigen>
//#include <Eigen/SVD>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QVariant>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* ToDo make this a template class
* Generalized math methods used by mne methods
*
* @brief Math methods
*/
class UTILSSHARED_EXPORT MNEMath
{
public:
    typedef std::pair<int,int> IdxIntValue;         /**< Typedef of a pair of ints. */

    //=========================================================================================================
    /**
    * Destroys the MNEMath object
    */
    virtual ~MNEMath()
    { }

    //=========================================================================================================
    /**
    * ToDo make this a template function
    *
    * mne_combine_xyz
    *
    * ### MNE toolbox root function ###
    *
    * Compute the three Cartesian components of a vector together
    *
    * @param[in] vec    Input row vector [ x1 y1 z1 ... x_n y_n z_n ]
    *
    * @return Output vector [x1^2+y1^2+z1^2 ... x_n^2+y_n^2+z_n^2 ]
    */
    static VectorXd* combine_xyz(const VectorXd& vec);

//    //=========================================================================================================
//    /**
//    * ### MNE toolbox root function ###: Implementation of the mne_block_diag function - decoding part
//    */
//    static inline MatrixXd extract_block_diag(MatrixXd& A, qint32 n);

    //=========================================================================================================
    /**
    * Returns the whitener of a given matrix.
    *
    * @param[in] A      Matrix to compute the whitener from
    * @param[in] pca    perform a pca
    *
    * @return rank of matrix A
    */
    static void get_whitener(MatrixXd& A, bool pca, QString ch_type, VectorXd& eig, MatrixXd& eigvec);

    //=========================================================================================================
    /**
    * Find the intersection of two vectors
    *
    * @param[in] v1         Input vector 1
    * @param[in] v2         Input vector 2
    * @param[out] idx_sel   Index of intersection based on v1
    *
    * @return the sorted, unique values that are in both of the input arrays.
    */
    static VectorXi intersect(const VectorXi &v1, const VectorXi &v2, VectorXi &idx_sel);

    //=========================================================================================================
    /**
    * Determines if a given data (stored as vector v) are representing a sparse matrix.
    * ToDo: status is experimental -> needs to be increased in speed.
    *
    * @param[in] v      data to be tested
    *
    * @return true if sparse false otherwise;
    */
    static bool issparse(VectorXd &v);

    //=========================================================================================================
    /**
    * ToDo make this a template function
    *
    * ### MNE toolbox root function ###: Implementation of the mne_block_diag function - encoding part
    *
    * Make a sparse block diagonal matrix
    *
    * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
    * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
    * the diagonal of the matrix.
    *
    * @param[in, out] A Matrix which should be diagonlized
    * @param[in, out] n Columns of the submatrices
    *
    * @return A sparse block diagonal, diagonalized from the elements in "A".
    */
    static SparseMatrix<double>* make_block_diag(const MatrixXd &A, qint32 n);

    //=========================================================================================================
    /**
    * ToDo make this a template function
    *
    * Returns the rank of a matrix A.
    *
    * @param[in] A      Matrix to get the rank from
    * @param[in] tol    realtive threshold: biggest singualr value multiplied with tol is smallest singular value considered non-zero
    *
    * @return rank of matrix A
    */
    static qint32 rank(const MatrixXd& A, double tol = 1e-8);

    //=========================================================================================================
    /**
    * ToDo: Maybe new processing class
    *
    * Rescale aka baseline correct data
    *
    * @param[in] data           Data Matrix (m x n_time)
    * @param[in] times          Time instants is seconds.
    * @param[in] baseline       If baseline is (a, b) the interval is between "a (s)" and "b (s)".
    *                           If a is invalid the beginning of the data is used and if b is invalid then b is set to the end of the interval.
    *                           If baseline is equal to (invalid, invalid) all the time interval is used.
    * @param[in] baseline_usage See description of parameter baseline.
    * @param[in] mode           Do baseline correction with ratio (power is divided by mean power during baseline) or zscore (power is divided by standard
    *                           deviatio of power during baseline after substracting the mean, power = [power - mean(power_baseline)] / std(power_baseline)).
    *                           ("logratio" | "ratio" | "zscore" | "mean" | "percent")
    *
    * @return   rescaled data matrix rescaling.
    */
    static MatrixXd rescale(const MatrixXd &data, const RowVectorXf &times, QPair<QVariant,QVariant> baseline, QString mode);

    //=========================================================================================================
    /**
    * Sorts a vector (ascending order) in place and returns the track of the original indeces
    *
    * @param[in, out] v     vector to sort; it's sorted in place
    * @param[in] desc       if true its sorted in a descending order, otherwise ascending (optional, default = true)
    *
    * @return Vector of the original indeces in the new order
    */
    template<typename T>
    static VectorXi sort(Matrix<T, Dynamic, 1> &v, bool desc = true);

    //=========================================================================================================
    /**
    * Sorts a vector (ascending order) and a corresponding matrix in place and returns the track of the original indeces
    * The matrix is sorted along the columns using the vector values for comparison.
    *
    * @param[in, out] v_prime   vector to sort (sorted in place)
    * @param[in, out] mat       matrix to sort (sorted in place)
    * @param[in] desc           if true its sorted in a descending order, otherwise ascending (optional, default = true)
    *
    * @return Vector of the original indeces in the new order
    */
    template<typename T>
    static VectorXi sort(Matrix<T, Dynamic, 1> &v_prime, Matrix<T, Dynamic, Dynamic> &mat, bool desc = true);

    //=========================================================================================================
    /**
    * Compares two index-value-pairs.
    *
    * @param[in] lhs    left hand side of the comparison
    * @param[in] rhs    right hand side of the comparison
    *
    * @return true if value of lhs is bigger than value of rhs
    */
    template<typename T>
    static inline bool compareIdxValuePairBiggerThan( const std::pair<int,T>& lhs, const std::pair<int,T>& rhs);

    //=========================================================================================================
    /**
    * Compares two index-value-pairs.
    *
    * @param[in] lhs    left hand side of the comparison
    * @param[in] rhs    right hand side of the comparison
    *
    * @return true if value of lhs is smaller than value of rhs
    */
    template<typename T>
    static inline bool compareIdxValuePairSmallerThan( const std::pair<int,T>& lhs, const std::pair<int,T>& rhs);

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE & TEMPLATE DEFINITIONS
//=============================================================================================================

template< typename T>
VectorXi MNEMath::sort(Matrix<T, Dynamic, 1> &v, bool desc)
{
    std::vector< std::pair<int,T> > t_vecIdxValue;
    VectorXi idx(v.size());

    if(v.size() > 0)
    {
        //fill temporal vector
        for(qint32 i = 0; i < v.size(); ++i)
            t_vecIdxValue.push_back(std::pair<int,T>(i, v[i]));

        //sort temporal vector
        if(desc)
            std::sort(t_vecIdxValue.begin(), t_vecIdxValue.end(), MNEMath::compareIdxValuePairBiggerThan<T>);
        else
            std::sort(t_vecIdxValue.begin(), t_vecIdxValue.end(), MNEMath::compareIdxValuePairSmallerThan<T>);

        //store results
        for(qint32 i = 0; i < v.size(); ++i)
        {
            idx[i] = t_vecIdxValue[i].first;
            v[i] = t_vecIdxValue[i].second;
        }
    }

    return idx;
}


//*************************************************************************************************************

template<typename T>
static VectorXi MNEMath::sort(Matrix<T, Dynamic, 1> &v_prime, Matrix<T, Dynamic, Dynamic> &mat, bool desc)
{
    VectorXi idx = MNEMath::sort<T>(v_prime, desc);

    if(v_prime.size() > 0)
    {
        //sort Matrix
        MatrixXd newMat(mat.rows(), mat.cols());
        for(qint32 i = 0; i < idx.size(); ++i)
            newMat.col(i) = mat.col(idx[i]);
        mat = newMat;
    }

    return idx;
}


//*************************************************************************************************************

template<typename T>
inline bool MNEMath::compareIdxValuePairBiggerThan( const std::pair<int,T>& lhs, const std::pair<int,T>& rhs)
{
    return lhs.second > rhs.second;
}


//*************************************************************************************************************

template<typename T>
inline bool MNEMath::compareIdxValuePairSmallerThan( const std::pair<int,T>& lhs, const std::pair<int,T>& rhs)
{
    return lhs.second < rhs.second;
}

} // NAMESPACE

#endif // MNEMATH_H
