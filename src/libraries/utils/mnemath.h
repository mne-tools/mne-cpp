//=============================================================================================================
/**
 * @file     mnemath.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    MNEMath class declaration.
 *
 */

#ifndef MNEMATH_H
#define MNEMATH_H

//ToDo move this to the new MNE math library

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <string>
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

#include <QVariant>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// UTILSLIB FORWARD DECLARATIONS
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
     * Finds the Greatest Common Divisor (GCD) of two integer values
     *
     * @param[in] a    First input integer.
     * @param[in] b    Second input integer.
     *
     * @return The Greatest Common Divisor (GCD) of a and b.
     */
    static int gcd(int iA, int iB);

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
     * @param[in] vec    Input row vector [ x1 y1 z1 ... x_n y_n z_n ].
     *
     * @return Output vector [x1^2+y1^2+z1^2 ... x_n^2+y_n^2+z_n^2 ].
     */
    static Eigen::VectorXd* combine_xyz(const Eigen::VectorXd& vec);

//    //=========================================================================================================
//    /**
//    * ### MNE toolbox root function ###: Definition of the mne_block_diag function - decoding part
//    */
//    static inline Eigen::MatrixXd extract_block_diag(MatrixXd& A, qint32 n);

    //=========================================================================================================
    /**
     * Returns the condition number of a given matrix.
     *
     * @param[in] A      Matrix to compute the condition number from.
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
     *
     * @return the condition slope.
     */
    static double getConditionSlope(const Eigen::MatrixXd& A,
                                    Eigen::VectorXd &s);

    //=========================================================================================================
    /**
     * Returns the whitener of a given matrix.
     *
     * @param[in] A      Matrix to compute the whitener from.
     * @param[in] pca    perform a pca.
     *
     * @return rank of matrix A.
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
     * @param[in] A      Matrix to compute the whitener from.
     * @param[in] pca    perform a pca.
     *
     * @return rank of matrix A.
     */
    static void get_whitener(Eigen::MatrixXd& A,
                             bool pca,
                             const std::string& ch_type,
                             Eigen::VectorXd& eig,
                             Eigen::MatrixXd& eigvec);

    //=========================================================================================================
    /**
     * Find the intersection of two vectors
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
     * Determines if a given data (stored as vector v) are representing a sparse matrix.
     * ToDo: status is experimental -> needs to be increased in speed.
     *
     * @param[in] v      data to be tested.
     *
     * @return true if sparse false otherwise;.
     */
    static bool issparse(Eigen::VectorXd &v);

    //=========================================================================================================
    /**
     * LEGENDRE Associated Legendre function.
     *
     *   P = LEGENDRE(N,X) computes the associated Legendre functions
     *   of degree N and order M = 0, 1, ..., N, evaluated for each element
     *   of X.  N must be a scalar integer and X must contain real values
     *   between -1 <= X <= 1.
     *
     * @return associated Legendre functions.
     */
    static Eigen::MatrixXd legendre(qint32 n,
                                    const Eigen::VectorXd &X,
                                    QString normalize = QString("unnorm"));

    //=========================================================================================================
    /**
     * LEGENDRE Associated Legendre function.
     *
     *   P = LEGENDRE(N,X) computes the associated Legendre functions
     *   of degree N and order M = 0, 1, ..., N, evaluated for each element
     *   of X.  N must be a scalar integer and X must contain real values
     *   between -1 <= X <= 1.
     *
     * @return associated Legendre functions.
     */
    static Eigen::MatrixXd legendre(qint32 n,
                                    const Eigen::VectorXd &X,
                                    std::string normalize = "unnorm");

    //=========================================================================================================
    /**
     * ToDo make this a template function
     *
     * ### MNE toolbox root function ###: Definition of the mne_block_diag function - encoding part
     *
     * Make a sparse block diagonal matrix
     *
     * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
     * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
     * the diagonal of the matrix.
     *
     * @param[in, out] A Matrix which should be diagonlized.
     * @param[in, out] n Columns of the submatrices.
     *
     * @return A sparse block diagonal, diagonalized from the elements in "A".
     */
    static Eigen::SparseMatrix<double>* make_block_diag(const Eigen::MatrixXd &A,
                                                        qint32 n);

    //=========================================================================================================
    /**
     * Calculates the combination of n over 2 (nchoosek(n,2))
     *
     * @param[in] n  The number of elements which should be combined with each other (n over 2).
     * @return   The number of combinations.
     */
    static int nchoose2(int n);

    //=========================================================================================================
    /**
     * ToDo make this a template function
     *
     * Returns the rank of a matrix A.
     *
     * @param[in] A      Matrix to get the rank from.
     * @param[in] tol    realtive threshold: biggest singualr value multiplied with tol is smallest singular value considered non-zero.
     *
     * @return rank of matrix A.
     */
    static qint32 rank(const Eigen::MatrixXd& A,
                       double tol = 1e-8);

    //=========================================================================================================
    /**
     * ToDo: Maybe new processing class
     *
     * Rescale aka baseline correct data
     *
     * @param[in] data           Data Matrix (m x n_time).
     * @param[in] times          Time instants is seconds.
     * @param[in] baseline       If baseline is (a, b) the interval is between "a (s)" and "b (s)".
     *                           If a and b are equal use interval between the beginning of the data and the time point 0 (stimulus onset).
     * @param[in] mode           Do baseline correction with ratio (power is divided by mean power during baseline) or zscore (power is divided by standard.
     *                           deviatio of power during baseline after substracting the mean, power = [power - mean(power_baseline)] / std(power_baseline)).
     *                           ("logratio" | "ratio" | "zscore" | "mean" | "percent")
     *
     * @return   rescaled data matrix rescaling.
     */
    static Eigen::MatrixXd rescale(const Eigen::MatrixXd &data,
                                   const Eigen::RowVectorXf &times,
                                   const QPair<float, float> &baseline,
                                   QString mode);

    //=========================================================================================================
    /**
     * ToDo: Maybe new processing class
     *
     * Rescale aka baseline correct data
     *
     * @param[in] data           Data Matrix (m x n_time).
     * @param[in] times          Time instants is seconds.
     * @param[in] baseline       If baseline is (a, b) the interval is between "a (s)" and "b (s)".
     *                           If a and b are equal use interval between the beginning of the data and the time point 0 (stimulus onset).
     * @param[in] mode           Do baseline correction with ratio (power is divided by mean power during baseline) or zscore (power is divided by standard.
     *                           deviatio of power during baseline after substracting the mean, power = [power - mean(power_baseline)] / std(power_baseline)).
     *                           ("logratio" | "ratio" | "zscore" | "mean" | "percent")
     *
     * @return   rescaled data matrix rescaling.
     */
    static Eigen::MatrixXd rescale(const Eigen::MatrixXd& data,
                                   const Eigen::RowVectorXf& times,
                                   const std::pair<float, float>& baseline,
                                   const std::string& mode);

    //=========================================================================================================
    /**
     * Sorts a vector (ascending order) in place and returns the track of the original indeces
     *
     * @param[in, out] v     vector to sort; it's sorted in place.
     * @param[in] desc       if true its sorted in a descending order, otherwise ascending (optional, default = true).
     *
     * @return Vector of the original indeces in the new order.
     */
    template<typename T>
    static Eigen::VectorXi sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v,
                                bool desc = true);

    //=========================================================================================================
    /**
     * Sorts a vector (ascending order) and a corresponding matrix in place and returns the track of the original indeces
     * The matrix is sorted along the columns using the vector values for comparison.
     *
     * @param[in, out] v_prime   vector to sort (sorted in place).
     * @param[in, out] mat       matrix to sort (sorted in place).
     * @param[in] desc           if true its sorted in a descending order, otherwise ascending (optional, default = true).
     *
     * @return Vector of the original indeces in the new order.
     */
    template<typename T>
    static Eigen::VectorXi sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v_prime,
                                Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &mat,
                                bool desc = true);

    //=========================================================================================================
    /**
     * Sort rows in ascending order
     *
     * @param[in] A          triplet vector to sort (sorted in place).
     * @param[in] column     sorts the triplet vector based on the column specified.
     *
     * @return Vector of the original indeces in the new order.
     */
    template<typename T>
    static std::vector<Eigen::Triplet<T> > sortrows(const std::vector<Eigen::Triplet<T> > &A,
                                                    qint32 column = 0);

    //=========================================================================================================
    /**
     * Compares two index-value-pairs.
     *
     * @param[in] lhs    left hand side of the comparison.
     * @param[in] rhs    right hand side of the comparison.
     *
     * @return true if value of lhs is bigger than value of rhs.
     */
    template<typename T>
    static inline bool compareIdxValuePairBiggerThan(const std::pair<int,T>& lhs,
                                                     const std::pair<int,T>& rhs);

    //=========================================================================================================
    /**
     * Compares two index-value-pairs.
     *
     * @param[in] lhs    left hand side of the comparison.
     * @param[in] rhs    right hand side of the comparison.
     *
     * @return true if value of lhs is smaller than value of rhs.
     */
    template<typename T>
    static inline bool compareIdxValuePairSmallerThan(const std::pair<int,T>& lhs,
                                                      const std::pair<int,T>& rhs);

    //=========================================================================================================
    /**
     * Compares triplet first entry
     *
     * @param[in] lhs    left hand side of the comparison.
     * @param[in] rhs    right hand side of the comparison.
     *
     * @return true if value of lhs is smaller than value of rhs.
     */
    template<typename T>
    static inline bool compareTripletFirstEntry(const Eigen::Triplet<T>& lhs,
                                                const Eigen::Triplet<T> & rhs);

    //=========================================================================================================
    /**
     * Compares triplet second entry
     *
     * @param[in] lhs    left hand side of the comparison.
     * @param[in] rhs    right hand side of the comparison.
     *
     * @return true if value of lhs is smaller than value of rhs.
     */
    template<typename T>
    static inline bool compareTripletSecondEntry(const Eigen::Triplet<T>& lhs,
                                                 const Eigen::Triplet<T> & rhs);

    //=========================================================================================================
    /**
     * Compute log2 of given number
     *
     * @param[in] d  input value.
     *
     * @return double result of log2 operation.
     */
    template<typename T>
    static inline double log2(const T d);

    //=========================================================================================================
    /**
     * creates a class and frequency distribution from data matrix
     *
     * @param[in] matRawData             raw data matrix that needs to be analyzed.
     * @param[in] bMakeSymmetrical       user input to turn the x-axis symmetric.
     * @param[in] iClassCount            user input to determine the amount of classes in the histogram.
     * @param[out] vecResultClassLimits   the upper limit of each individual class.
     * @param[out] vecResultFrequency     the amount of data that fits in the appropriate class ranges.
     * @param[in] dGlobalMin             user input to determine the maximum value allowed in the histogram.
     * @param[in] dGlobalMax             user input to determine the minimum value allowed in the histogram.
     */
    template<typename T>
    static void histcounts(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax= 0.0);
    template<typename T>
    static void histcounts(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax= 0.0);
    template<typename T>
    static void histcounts(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax= 0.0);

    //=========================================================================================================
    /**
     * Creates the pseudo inverse of a matrix.
     *
     * @param[in] a        raw data matrix that needs to be analyzed.
     */
    template<typename T>
    static Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> pinv(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& a);

    //=========================================================================================================
    /**
     * Compare new head position with current head position and update dev_head_t if big displacement occured
     *
     * @param[in] mDevHeadTrans      The device to head transformation matrix to compare to.
     * @param[in] mDevHeadTransNew   The device to head transformation matrix to be compared.
     * @param[in] fTreshRot          The threshold for big head rotation in degree.
     * @param[in] fThreshTrans       The threshold for big head movement in m.
     *
     * @return bState                The status that shows if devHead is updated or not.
     *
     */
    static bool compareTransformation(const Eigen::MatrixX4f& mDevHeadT,
                                      const Eigen::MatrixX4f& mDevHeadTNew,
                                      const float& fThreshRot,
                                      const float& fThreshTrans);

};

//=============================================================================================================
// INLINE & TEMPLATE DEFINITIONS
//=============================================================================================================

template< typename T>
Eigen::VectorXi MNEMath::sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v,
                              bool desc)
{
    std::vector< std::pair<int,T> > t_vecIdxValue;
    Eigen::VectorXi idx(v.size());

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

//=============================================================================================================

template<typename T>
Eigen::VectorXi MNEMath::sort(Eigen::Matrix<T, Eigen::Dynamic, 1> &v_prime,
                              Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &mat,
                              bool desc)
{
    Eigen::VectorXi idx = MNEMath::sort<T>(v_prime, desc);

    if(v_prime.size() > 0)
    {
        //sort Matrix
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> newMat(mat.rows(), mat.cols());
        for(qint32 i = 0; i < idx.size(); ++i)
            newMat.col(i) = mat.col(idx[i]);
        mat = newMat;
    }

    return idx;
}

//=============================================================================================================

template<typename T>
std::vector<Eigen::Triplet<T> > MNEMath::sortrows(const std::vector<Eigen::Triplet<T> > &A,
                                                  qint32 column)
{
    std::vector<Eigen::Triplet<T> > p_ASorted;

    for(quint32 i = 0; i < A.size(); ++i)
        p_ASorted.push_back(A[i]);

    if(column == 0)
        std::sort(p_ASorted.begin(), p_ASorted.end(), MNEMath::compareTripletFirstEntry<T>);
    if(column == 1)
        std::sort(p_ASorted.begin(), p_ASorted.end(), MNEMath::compareTripletSecondEntry<T>);

    return p_ASorted;
}

//=============================================================================================================

template<typename T>
inline bool MNEMath::compareIdxValuePairBiggerThan(const std::pair<int,T>& lhs,
                                                   const std::pair<int,T>& rhs)
{
    return lhs.second > rhs.second;
}

//=============================================================================================================

template<typename T>
inline bool MNEMath::compareIdxValuePairSmallerThan( const std::pair<int,T>& lhs,
                                                     const std::pair<int,T>& rhs)
{
    return lhs.second < rhs.second;
}

//=============================================================================================================

template<typename T>
inline bool MNEMath::compareTripletFirstEntry( const Eigen::Triplet<T>& lhs,
                                               const Eigen::Triplet<T> & rhs)
{
    return lhs.row() < rhs.row();
}

//=============================================================================================================

template<typename T>
inline bool MNEMath::compareTripletSecondEntry( const Eigen::Triplet<T>& lhs,
                                                const Eigen::Triplet<T> & rhs)
{
    return lhs.col() < rhs.col();
}

//=============================================================================================================

template<typename T>
inline double MNEMath::log2( const T d)
{
    return log(d)/log(2);
}

//=============================================================================================================

template<typename T>
void MNEMath::histcounts(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matRawData,
                         bool bMakeSymmetrical,
                         int iClassAmount,
                         Eigen::VectorXd& vecResultClassLimits,
                         Eigen::VectorXi& vecResultFrequency,
                         double dGlobalMin,
                         double dGlobalMax)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matRawData.rows(),1);
    matrixName.col(0)= matRawData;
    MNEMath::histcounts(matrixName, bMakeSymmetrical, iClassAmount, vecResultClassLimits, vecResultFrequency, dGlobalMin, dGlobalMax);
}

//=============================================================================================================

template<typename T>
void MNEMath::histcounts(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matRawData,
                         bool bMakeSymmetrical,
                         int iClassAmount,
                         Eigen::VectorXd& vecResultClassLimits,
                         Eigen::VectorXi& vecResultFrequency,
                         double dGlobalMin,
                         double dGlobalMax)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1,matRawData.cols());
    matrixName.row(0)= matRawData;
    MNEMath::histcounts(matrixName, bMakeSymmetrical, iClassAmount, vecResultClassLimits, vecResultFrequency, dGlobalMin, dGlobalMax);
}

//=============================================================================================================

template<typename T>
void MNEMath::histcounts(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matRawData,
                         bool bMakeSymmetrical,
                         int iClassAmount,
                         Eigen::VectorXd& vecResultClassLimits,
                         Eigen::VectorXi& vecResultFrequency,
                         double dGlobalMin,
                         double dGlobalMax)
{
   if(matRawData.rows() == 0 || matRawData.cols() == 0) {
       return;
   }

   vecResultClassLimits.resize(iClassAmount + 1);
   vecResultFrequency.resize(iClassAmount);

    for (int count = 0; count < iClassAmount; ++count) //initialize the vector with zero values
    {
        vecResultFrequency(count) = 0;
    }

    double desiredMin,
           desiredMax;
    double rawMin(0.0),
           rawMax(0.0),
           localMin(0.0),
           localMax(0.0);

    rawMin = matRawData.minCoeff();        //finds the raw matrix minimum value
    rawMax = matRawData.maxCoeff();        //finds the raw matrix maximum value

    if (bMakeSymmetrical == true)          //in case the user wants the histogram to have symmetrical class ranges
    {
        if (std::fabs(rawMin) > rawMax)          //in case the negative side is larger than the positive side
        {
            localMax = std::fabs(rawMin);        //positive side is "stretched" to the exact length as negative side
            localMin = rawMin;
        }
        else if (rawMax > std::fabs(rawMin))     //in case the positive side is larger than the negative side
        {
            localMin = -(rawMax);          //negative side is "stretched" to the exact length as positive side
            localMax = rawMax;
        }
        else                               //in case both sides are exactly the same
        {
            localMin = rawMin;
            localMax = rawMax;
        }
    }
    else                                   //in case bMakeSymmetrical == false
    {
        localMin = rawMin;
        localMax = rawMax;
    }
    //selects either local or global range (according to user preference and input)
    if (dGlobalMin == 0.0 && dGlobalMax == 0.0)               //if global range is NOT given by the user, use local ranges
    {
        desiredMin = localMin;
        desiredMax = localMax;
        vecResultClassLimits[0] = desiredMin;                 //replace default value with local minimum at position 0
        vecResultClassLimits[iClassAmount] = desiredMax;      //replace default value with local maximum at position n
    }
    else
    {
        desiredMin = dGlobalMin;
        desiredMax = dGlobalMax;
        vecResultClassLimits(0)= desiredMin;                 //replace default value with global minimum at position 0
        vecResultClassLimits(iClassAmount)= desiredMax;      //replace default value with global maximum at position n
    }

    double	range = (vecResultClassLimits(iClassAmount) - vecResultClassLimits(0)),      //calculates the length from maximum positive value to zero
            dynamicUpperClassLimit;

    for (int kr = 0; kr < iClassAmount; ++kr)                                            //dynamically initialize the upper class limit values
    {
        dynamicUpperClassLimit = (vecResultClassLimits(0) + (kr*(range/iClassAmount)));  //generic formula to determine the upper class limit with respect to range and number of class
        vecResultClassLimits(kr) = dynamicUpperClassLimit;                               //places the appropriate upper class limit value to the right position in the QVector
    }

    for (int ir = 0; ir < matRawData.rows(); ++ir)       //iterates through all columns of the data matrix
    {
        for (int jr = 0; jr<matRawData.cols(); ++jr)     //iterates through all rows of the data matrix
        {
            if(matRawData(ir,jr) != 0.0) {
                for (int kr = 0; kr < iClassAmount; ++kr)    //starts iteration from 1 to iClassAmount
                {
                    if (kr == iClassAmount-1)                //used for the final iteration; if the data value is exactly the same as the final upper class limit, it will be included in the histogram
                    {
                        if (matRawData(ir,jr) >= vecResultClassLimits(kr) && matRawData(ir,jr) <= vecResultClassLimits(kr + 1))    //compares value in the matrix with lower and upper limit of each class
                        {
                             vecResultFrequency(kr) = vecResultFrequency(kr) + 1 ;           //if the value fits both arguments, the appropriate class frequency is increased by 1
                        }
                    }
                    else
                    {
                        if (matRawData(ir,jr) >= vecResultClassLimits(kr) && matRawData(ir,jr) < vecResultClassLimits(kr + 1))    //compares value in the matrix with lower and upper limit of each class
                        {
                            vecResultFrequency(kr) = vecResultFrequency(kr) + 1 ;           //if the value fits both arguments, the appropriate class frequency is increased by 1
                        }
                    }
                }
            }
        }
    }
}

//=============================================================================================================

template<typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> MNEMath::pinv(const Eigen::Matrix<T,
                                                               Eigen::Dynamic,
                                                               Eigen::Dynamic>& a)
{
    double epsilon = std::numeric_limits<double>::epsilon();
    Eigen::JacobiSVD< Eigen::MatrixXd > svd(a ,Eigen::ComputeThinU | Eigen::ComputeThinV);
    double tolerance = epsilon * std::max(a.cols(), a.rows()) * svd.singularValues().array().abs()(0);
    return svd.matrixV() * (svd.singularValues().array().abs() > tolerance).select(svd.singularValues().array().inverse(),0).matrix().asDiagonal() * svd.matrixU().adjoint();
}

//=============================================================================================================
} // NAMESPACE

#endif // MNEMATH_H
