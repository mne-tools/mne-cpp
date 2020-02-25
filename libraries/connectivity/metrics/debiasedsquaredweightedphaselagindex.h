//=============================================================================================================
/**
 * @file     debiasedsquaredweightedphaselagindex.h
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     April, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Daniel Strohmeier, Lorenz Esch. All rights reserved.
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
 * @note Notes:
 * - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
 *
 *
 * @brief    DebiasedSquaredWeightedPhaseLagIndex class declaration.
 *
 */

#ifndef DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
#define DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

#include "abstractmetric.h"
#include "../connectivitysettings.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMutex>


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {


//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;


//=============================================================================================================
/**
 * This class computes the weighted phase lag index connectivity metric.
 *
 * @brief This class computes the weighted phase lag index connectivity metric.
 */
class CONNECTIVITYSHARED_EXPORT DebiasedSquaredWeightedPhaseLagIndex : public AbstractMetric
{    

public:
    typedef QSharedPointer<DebiasedSquaredWeightedPhaseLagIndex> SPtr;            /**< Shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */
    typedef QSharedPointer<const DebiasedSquaredWeightedPhaseLagIndex> ConstSPtr; /**< Const shared pointer type for DebiasedSquaredWeightedPhaseLagIndex. */

    //=========================================================================================================
    /**
     * Constructs a DebiasedSquaredWeightedPhaseLagIndex object.
     */
    explicit DebiasedSquaredWeightedPhaseLagIndex();

    //=========================================================================================================
    /**
     * Calculates the debiased squared weighted phase lag index between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Computes the DSWPLI values. This function gets called in parallel.
     *
     * @param[in] inputData              The input data.
     * @param[out]vecPairCsdSum          The sum of all CSD matrices for each trial.
     * @param[out]vecPairCsdImagAbsSum   The sum of all imag abs CSD matrices for each trial.
     * @param[out]vecPairCsdImagSqrdSum  The sum of all imag aqrd CSD matrices for each trial.
     * @param[in] mutex                  The mutex used to safely access vecPairCsdSum.
     * @param[in] iNRows                 The number of rows.
     * @param[in] iNFreqs                The number of frequenciy bins.
     * @param[in] iNfft                  The FFT length.
     * @param[in] tapers                 The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagAbsSum,
                        QVector<QPair<int,Eigen::MatrixXd> >& vecPairCsdImagSqrdSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);

    //=========================================================================================================
    /**
     * Reduces the DSWPLI computation to a final result.
     *
     * @param[out] connectivitySettings   The input data.
     * @param[in]  finalNetwork           The final network.
     */
    static void computeDSWPLI(ConnectivitySettings &connectivitySettings,
                              Network& finalNetwork);
};


//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // DEBIASEDSQUAREDWEIGHTEDPHASELAGINDEX_H
