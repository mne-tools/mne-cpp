//=============================================================================================================
/**
 * @file     interpolation.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     May, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief     Interpolation class declaration.
 *
 */

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <limits>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DRHILIB {

#ifndef FLOAT_INFINITY
#define FLOAT_INFINITY std::numeric_limits<float>::infinity()
#endif

//=============================================================================================================
/**
 * This class holds methods for creating distance-based weight matrices and for interpolating signals.
 *
 * @brief This class holds methods for creating distance-based weight matrices and for interpolating signals
 */

class DISP3DRHISHARED_EXPORT Interpolation
{

public:
    typedef QSharedPointer<Interpolation> SPtr;
    typedef QSharedPointer<const Interpolation> ConstSPtr;

    Interpolation() = delete;

    //=========================================================================================================
    /**
     * @brief createInterpolationMat   Calculates the weight matrix for interpolation.
     */
    static QSharedPointer<Eigen::SparseMatrix<float> > createInterpolationMat(const QVector<int> &vecProjectedSensors,
                                                                              const QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                                                              double (*interpolationFunction) (double),
                                                                              const double dCancelDist = FLOAT_INFINITY,
                                                                              const QVector<int> &vecExcludeIndex = QVector<int>());

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (shared pointer version).
     */
    static Eigen::VectorXf interpolateSignal(const QSharedPointer<Eigen::SparseMatrix<float> > matInterpolationMatrix,
                                             const QSharedPointer<Eigen::VectorXf> &vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (reference version).
     */
    static Eigen::VectorXf interpolateSignal(const Eigen::SparseMatrix<float> &matInterpolationMatrix,
                                             const Eigen::VectorXf &vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief linear   Identity interpolation function.
     */
    static double linear(const double dIn);

    //=========================================================================================================
    /**
     * @brief gaussian   Gaussian interpolation function (sigma=1).
     */
    static double gaussian(const double dIn);

    //=========================================================================================================
    /**
     * @brief square   Negative parabola interpolation function with y-offset of 1.
     */
    static double square(const double dIn);

    //=========================================================================================================
    /**
     * @brief cubic   Cubic hyperbola interpolation function.
     */
    static double cubic(const double dIn);
};

} // namespace DISP3DRHILIB

#endif // INTERPOLATION_H
