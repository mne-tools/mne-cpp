//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_cov.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Real-time noise covariance estimation from streaming MEG / EEG data blocks.
 *
 * RtCov maintains a running unbiased estimate of the channel–channel
 * covariance matrix used by linear inverse operators (MNE, dSPM, sLORETA,
 * beamformers). Every incoming block contributes its centred outer product
 * @c X⋅Xᵀ to the accumulator together with the per-block sample count;
 * the final covariance is the weighted sum divided by the total number of
 * samples minus one. Computation is offloaded to a worker @c QThread so
 * the acquisition pipeline never blocks on the dense matrix multiply.
 *
 * The @ref RtCovComputeResult bundle carries both the matrix and the
 * sample count, which lets downstream consumers combine partial estimates,
 * apply rank-corrections, or convert to a @ref FIFFLIB::FiffCov for
 * persistence and inverse-operator construction.
 */

#ifndef RT_COV_RTPROCESSING_H
#define RT_COV_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

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

/**
 * @brief Bundled output of a real-time covariance computation step containing the covariance matrix and sample count.
 */
struct RtCovComputeResult {
    Eigen::VectorXd mu;
    Eigen::MatrixXd matData;
};

//=============================================================================================================
/**
 * Real-time covariance worker.
 *
 * @brief Controller that manages background covariance matrix estimation from streaming data.
 */
class DSPSHARED_EXPORT RtCov : public QObject
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

    QVector<int>            m_picks;                    /**< Indices of MEG/EEG channels to include. */
    bool                    m_bPicksReady;              /**< Whether picks have been computed. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RT_COV_RTPROCESSING_H
