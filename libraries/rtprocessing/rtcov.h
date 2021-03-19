//=============================================================================================================
/**
 * @file     rtcov.h
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
 * @brief     RtCov class declaration.
 *
 */

#ifndef RTCOV_RTPROCESSING_H
#define RTCOV_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QThread>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// RTPROCESSINGLIB FORWARD DECLARATIONS
//=============================================================================================================

struct RtCovComputeResult {
    Eigen::VectorXd mu;
    Eigen::MatrixXd matData;
};

//=============================================================================================================
/**
 * Real-time covariance worker.
 *
 * @brief Real-time covariance worker.
 */
class RTPROCESINGSHARED_EXPORT RtCov : public QObject
{
    Q_OBJECT

public:
    RtCov(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Perform actual covariance estimation.
     *
     * @param[in] inputData  Data to estimate the covariance from.
     */
    FIFFLIB::FiffCov estimateCovariance(const Eigen::MatrixXd& matData,
                                        int iNewMaxSamples);

protected:
    //=========================================================================================================
    /**
     * Computer multiplication with transposed.
     *
     * @param[in] matData  Data to self multiply with.
     *
     * @return   The multiplication result.
     */
    static RtCovComputeResult compute(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * Computer multiplication with transposed.
     *
     * @param[out]   finalResult     The final covariance estimation.
     * @param[in]   tempResult      The intermediate result from the compute function.
     */
    static void reduce(RtCovComputeResult& finalResult, const RtCovComputeResult &tempResult);

    int                     m_iSamples;                 /**< The number of stored samples. */

    QList<Eigen::MatrixXd>  m_lData;                    /**< The stored data blocks. */

    FIFFLIB::FiffInfo       m_fiffInfo;                 /**< Holds the fiff measurement information. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RTCOV_RTPROCESSING_H
