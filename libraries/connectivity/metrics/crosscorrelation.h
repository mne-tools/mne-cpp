//=============================================================================================================
/**
 * @file     crosscorrelation.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief     CrossCorrelation class declaration.
 *
 */

#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H

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
 * This class computes the cross correlation connectivity metric.
 *
 * @brief This class computes the cross correlation connectivity metric.
 */
class CONNECTIVITYSHARED_EXPORT CrossCorrelation : public AbstractMetric
{    

public:
    typedef QSharedPointer<CrossCorrelation> SPtr;            /**< Shared pointer type for CrossCorrelation. */
    typedef QSharedPointer<const CrossCorrelation> ConstSPtr; /**< Const shared pointer type for CrossCorrelation. */

    //=========================================================================================================
    /**
     * Constructs a CrossCorrelation object.
     */
    explicit CrossCorrelation();

    //=========================================================================================================
    /**
     * Calculates the cross correlation between the rows of the data matrix.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);

protected:
    //=========================================================================================================
    /**
     * Calculates the connectivity matrix for a given input data matrix based on the cross correlation coefficient.
     *
     * @param[in]   inputData           The input data.
     * @param[out]   matDist             The sum of all edge weights.
     * @param[in]   mutex               The mutex used to safely access matDist.
     * @param[in]   iNfft               The FFT length.
     * @param[in]   tapers              The taper information.
     */
    static void compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        Eigen::MatrixXd& matDist,
                        QMutex& mutex,
                        int iNfft,
                        const QPair<Eigen::MatrixXd, Eigen::VectorXd>& tapers);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNECTIVITYLIB

#endif // CROSSCORRELATION_H
