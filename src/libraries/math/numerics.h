//=============================================================================================================
/**
 * @file     numerics.h
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
 * @brief    Numerics class declaration.
 *
 */

#ifndef NUMERICS_H
#define NUMERICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVariant>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * General numerical utility functions: GCD, log2, histograms, baseline rescaling, etc.
 *
 * @brief Static numerical utility functions.
 */
class MATHSHARED_EXPORT Numerics
{
public:
    //=========================================================================================================
    /**
     * Destroys the Numerics object.
     */
    ~Numerics() = default;

    //=========================================================================================================
    /**
     * Finds the Greatest Common Divisor (GCD) of two integer values.
     *
     * @param[in] iA    First input integer.
     * @param[in] iB    Second input integer.
     *
     * @return The Greatest Common Divisor (GCD) of iA and iB.
     */
    static int gcd(int iA, int iB);

    //=========================================================================================================
    /**
     * Determines if a given data (stored as vector v) are representing a sparse matrix.
     *
     * @param[in] v      Data to be tested.
     *
     * @return true if sparse, false otherwise.
     */
    static bool issparse(Eigen::VectorXd &v);

    //=========================================================================================================
    /**
     * Calculates the combination of n over 2 (nchoosek(n,2)).
     *
     * @param[in] n  The number of elements which should be combined with each other (n over 2).
     *
     * @return The number of combinations.
     */
    static int nchoose2(int n);

    //=========================================================================================================
    /**
     * Rescale (baseline correct) data.
     *
     * @param[in] data           Data Matrix (m x n_time).
     * @param[in] times          Time instants in seconds.
     * @param[in] baseline       If baseline is (a, b) the interval is between "a (s)" and "b (s)".
     *                           If a and b are equal, use interval between the beginning of the data and time 0.
     * @param[in] mode           Baseline correction mode:
     *                           "logratio" | "ratio" | "zscore" | "mean" | "percent".
     *
     * @return Rescaled data matrix.
     */
    static Eigen::MatrixXd rescale(const Eigen::MatrixXd &data,
                                   const Eigen::RowVectorXf &times,
                                   const QPair<float, float> &baseline,
                                   QString mode);

    //=========================================================================================================
    /**
     * Rescale (baseline correct) data.
     *
     * @param[in] data           Data Matrix (m x n_time).
     * @param[in] times          Time instants in seconds.
     * @param[in] baseline       If baseline is (a, b) the interval is between "a (s)" and "b (s)".
     *                           If a and b are equal, use interval between the beginning of the data and time 0.
     * @param[in] mode           Baseline correction mode:
     *                           "logratio" | "ratio" | "zscore" | "mean" | "percent".
     *
     * @return Rescaled data matrix.
     */
    static Eigen::MatrixXd rescale(const Eigen::MatrixXd& data,
                                   const Eigen::RowVectorXf& times,
                                   const std::pair<float, float>& baseline,
                                   const std::string& mode);

    //=========================================================================================================
    /**
     * Compute log base 2 of a given number.
     *
     * @param[in] d  Input value.
     *
     * @return log2 of d.
     */
    template<typename T>
    static inline double log2(const T d);

    //=========================================================================================================
    /**
     * Creates a class and frequency distribution from data matrix.
     *
     * @param[in] matRawData             Raw data matrix that needs to be analyzed.
     * @param[in] bMakeSymmetrical       User input to turn the x-axis symmetric.
     * @param[in] iClassAmount           User input to determine the amount of classes in the histogram.
     * @param[out] vecResultClassLimits  The upper limit of each individual class.
     * @param[out] vecResultFrequency    The amount of data that fits in the appropriate class ranges.
     * @param[in] dGlobalMin             User input to determine the minimum value allowed in the histogram.
     * @param[in] dGlobalMax             User input to determine the maximum value allowed in the histogram.
     */
    template<typename T>
    static void histcounts(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax = 0.0);

    template<typename T>
    static void histcounts(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax = 0.0);

    template<typename T>
    static void histcounts(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matRawData,
                           bool bMakeSymmetrical,
                           int iClassAmount,
                           Eigen::VectorXd& vecResultClassLimits,
                           Eigen::VectorXi& vecResultFrequency,
                           double dGlobalMin = 0.0,
                           double dGlobalMax = 0.0);
};

//=============================================================================================================
// INLINE & TEMPLATE DEFINITIONS
//=============================================================================================================

template<typename T>
inline double Numerics::log2(const T d)
{
    return log(d)/log(2);
}

//=============================================================================================================

template<typename T>
void Numerics::histcounts(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matRawData,
                          bool bMakeSymmetrical,
                          int iClassAmount,
                          Eigen::VectorXd& vecResultClassLimits,
                          Eigen::VectorXi& vecResultFrequency,
                          double dGlobalMin,
                          double dGlobalMax)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matRawData.rows(), 1);
    matrixName.col(0) = matRawData;
    Numerics::histcounts(matrixName, bMakeSymmetrical, iClassAmount, vecResultClassLimits, vecResultFrequency, dGlobalMin, dGlobalMax);
}

//=============================================================================================================

template<typename T>
void Numerics::histcounts(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matRawData,
                          bool bMakeSymmetrical,
                          int iClassAmount,
                          Eigen::VectorXd& vecResultClassLimits,
                          Eigen::VectorXi& vecResultFrequency,
                          double dGlobalMin,
                          double dGlobalMax)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1, matRawData.cols());
    matrixName.row(0) = matRawData;
    Numerics::histcounts(matrixName, bMakeSymmetrical, iClassAmount, vecResultClassLimits, vecResultFrequency, dGlobalMin, dGlobalMax);
}

//=============================================================================================================

template<typename T>
void Numerics::histcounts(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matRawData,
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

    for (int count = 0; count < iClassAmount; ++count)
    {
        vecResultFrequency(count) = 0;
    }

    double desiredMin,
           desiredMax;
    double rawMin(0.0),
           rawMax(0.0),
           localMin(0.0),
           localMax(0.0);

    rawMin = matRawData.minCoeff();
    rawMax = matRawData.maxCoeff();

    if (bMakeSymmetrical == true)
    {
        if (std::fabs(rawMin) > rawMax)
        {
            localMax = std::fabs(rawMin);
            localMin = rawMin;
        }
        else if (rawMax > std::fabs(rawMin))
        {
            localMin = -(rawMax);
            localMax = rawMax;
        }
        else
        {
            localMin = rawMin;
            localMax = rawMax;
        }
    }
    else
    {
        localMin = rawMin;
        localMax = rawMax;
    }

    if (dGlobalMin == 0.0 && dGlobalMax == 0.0)
    {
        desiredMin = localMin;
        desiredMax = localMax;
        vecResultClassLimits[0] = desiredMin;
        vecResultClassLimits[iClassAmount] = desiredMax;
    }
    else
    {
        desiredMin = dGlobalMin;
        desiredMax = dGlobalMax;
        vecResultClassLimits(0) = desiredMin;
        vecResultClassLimits(iClassAmount) = desiredMax;
    }

    double range = (vecResultClassLimits(iClassAmount) - vecResultClassLimits(0)),
           dynamicUpperClassLimit;

    for (int kr = 0; kr < iClassAmount; ++kr)
    {
        dynamicUpperClassLimit = (vecResultClassLimits(0) + (kr * (range / iClassAmount)));
        vecResultClassLimits(kr) = dynamicUpperClassLimit;
    }

    for (int ir = 0; ir < matRawData.rows(); ++ir)
    {
        for (int jr = 0; jr < matRawData.cols(); ++jr)
        {
            if(matRawData(ir,jr) != 0.0) {
                for (int kr = 0; kr < iClassAmount; ++kr)
                {
                    if (kr == iClassAmount - 1)
                    {
                        if (matRawData(ir,jr) >= vecResultClassLimits(kr) && matRawData(ir,jr) <= vecResultClassLimits(kr + 1))
                        {
                            vecResultFrequency(kr) = vecResultFrequency(kr) + 1;
                        }
                    }
                    else
                    {
                        if (matRawData(ir,jr) >= vecResultClassLimits(kr) && matRawData(ir,jr) < vecResultClassLimits(kr + 1))
                        {
                            vecResultFrequency(kr) = vecResultFrequency(kr) + 1;
                        }
                    }
                }
            }
        }
    }
}

//=============================================================================================================
} // NAMESPACE

#endif // NUMERICS_H
